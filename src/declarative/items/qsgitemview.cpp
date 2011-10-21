/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgitemview_p_p.h"

QT_BEGIN_NAMESPACE


FxViewItem::FxViewItem(QSGItem *i, bool own)
    : item(i), ownItem(own), index(-1)
{
}

FxViewItem::~FxViewItem()
{
    if (ownItem && item) {
        item->setParentItem(0);
        item->deleteLater();
        item = 0;
    }
}


QSGItemViewChangeSet::QSGItemViewChangeSet()
    : active(false)
{
    reset();
}

bool QSGItemViewChangeSet::hasPendingChanges() const
{
    return !pendingChanges.isEmpty();
}

void QSGItemViewChangeSet::applyChanges(const QDeclarativeChangeSet &changeSet)
{
    pendingChanges.apply(changeSet);

    int moveId = -1;
    int moveOffset;

    foreach (const QDeclarativeChangeSet::Remove &r, changeSet.removes()) {
        itemCount -= r.count;
        if (moveId == -1 && newCurrentIndex >= r.index + r.count) {
            newCurrentIndex -= r.count;
            currentChanged = true;
        } else if (moveId == -1 && newCurrentIndex >= r.index && newCurrentIndex < r.index + r.count) {
            // current item has been removed.
            if (r.isMove()) {
                moveId = r.moveId;
                moveOffset = newCurrentIndex - r.index;
            } else {
                currentRemoved = true;
                newCurrentIndex = -1;
                if (itemCount)
                    newCurrentIndex = qMin(r.index, itemCount - 1);
            }
            currentChanged = true;
        }
    }
    foreach (const QDeclarativeChangeSet::Insert &i, changeSet.inserts()) {
        if (moveId == -1) {
            if (itemCount && newCurrentIndex >= i.index) {
                newCurrentIndex += i.count;
                currentChanged = true;
            } else if (newCurrentIndex < 0) {
                newCurrentIndex = 0;
                currentChanged = true;
            } else if (newCurrentIndex == 0 && !itemCount) {
                // this is the first item, set the initial current index
                currentChanged = true;
            }
        } else if (moveId == i.moveId) {
            newCurrentIndex = i.index + moveOffset;
        }
        itemCount += i.count;
    }
}

void QSGItemViewChangeSet::prepare(int currentIndex, int count)
{
    if (active)
        return;
    reset();
    active = true;
    itemCount = count;
    newCurrentIndex = currentIndex;
}

void QSGItemViewChangeSet::reset()
{
    itemCount = 0;
    newCurrentIndex = -1;
    pendingChanges.clear();
    removedItems.clear();
    active = false;
    currentChanged = false;
    currentRemoved = false;
}


QSGItemView::QSGItemView(QSGFlickablePrivate &dd, QSGItem *parent)
    : QSGFlickable(dd, parent)
{
    Q_D(QSGItemView);
    d->init();
}

QSGItemView::~QSGItemView()
{
    Q_D(QSGItemView);
    d->clear();
    if (d->ownModel)
        delete d->model;
    delete d->header;
    delete d->footer;
}


QSGItem *QSGItemView::currentItem() const
{
    Q_D(const QSGItemView);
    if (!d->currentItem)
        return 0;
    const_cast<QSGItemViewPrivate*>(d)->applyPendingChanges();
    return d->currentItem->item;
}

QVariant QSGItemView::model() const
{
    Q_D(const QSGItemView);
    return d->modelVariant;
}

void QSGItemView::setModel(const QVariant &model)
{
    Q_D(QSGItemView);
    if (d->modelVariant == model)
        return;
    if (d->model) {
        disconnect(d->model, SIGNAL(modelUpdated(QDeclarativeChangeSet,bool)),
                this, SLOT(modelUpdated(QDeclarativeChangeSet,bool)));
        disconnect(d->model, SIGNAL(createdItem(int,QSGItem*)), this, SLOT(createdItem(int,QSGItem*)));
        disconnect(d->model, SIGNAL(destroyingItem(QSGItem*)), this, SLOT(destroyingItem(QSGItem*)));
    }

    QSGVisualModel *oldModel = d->model;

    d->clear();
    d->setPosition(d->contentStartPosition());
    d->model = 0;
    d->modelVariant = model;

    QObject *object = qvariant_cast<QObject*>(model);
    QSGVisualModel *vim = 0;
    if (object && (vim = qobject_cast<QSGVisualModel *>(object))) {
        if (d->ownModel) {
            delete oldModel;
            d->ownModel = false;
        }
        d->model = vim;
    } else {
        if (!d->ownModel) {
            d->model = new QSGVisualDataModel(qmlContext(this), this);
            d->ownModel = true;
            if (isComponentComplete())
                static_cast<QSGVisualDataModel *>(d->model.data())->componentComplete();
        } else {
            d->model = oldModel;
        }
        if (QSGVisualDataModel *dataModel = qobject_cast<QSGVisualDataModel*>(d->model))
            dataModel->setModel(model);
    }

    if (d->model) {
        d->bufferMode = QSGItemViewPrivate::BufferBefore | QSGItemViewPrivate::BufferAfter;
        if (isComponentComplete()) {
            updateSections();
            d->refill();
            if ((d->currentIndex >= d->model->count() || d->currentIndex < 0) && !d->currentIndexCleared) {
                setCurrentIndex(0);
            } else {
                d->moveReason = QSGItemViewPrivate::SetIndex;
                d->updateCurrent(d->currentIndex);
                if (d->highlight && d->currentItem) {
                    if (d->autoHighlight)
                        d->resetHighlightPosition();
                    d->updateTrackedItem();
                }
                d->moveReason = QSGItemViewPrivate::Other;
            }
            d->updateViewport();
        }
        connect(d->model, SIGNAL(modelUpdated(QDeclarativeChangeSet,bool)),
                this, SLOT(modelUpdated(QDeclarativeChangeSet,bool)));
        connect(d->model, SIGNAL(createdItem(int,QSGItem*)), this, SLOT(createdItem(int,QSGItem*)));
        connect(d->model, SIGNAL(destroyingItem(QSGItem*)), this, SLOT(destroyingItem(QSGItem*)));
        emit countChanged();
    }
    emit modelChanged();
}

QDeclarativeComponent *QSGItemView::delegate() const
{
    Q_D(const QSGItemView);
    if (d->model) {
        if (QSGVisualDataModel *dataModel = qobject_cast<QSGVisualDataModel*>(d->model))
            return dataModel->delegate();
    }

    return 0;
}

void QSGItemView::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QSGItemView);
    if (delegate == this->delegate())
        return;
    if (!d->ownModel) {
        d->model = new QSGVisualDataModel(qmlContext(this));
        d->ownModel = true;
    }
    if (QSGVisualDataModel *dataModel = qobject_cast<QSGVisualDataModel*>(d->model)) {
        int oldCount = dataModel->count();
        dataModel->setDelegate(delegate);
        if (isComponentComplete()) {
            for (int i = 0; i < d->visibleItems.count(); ++i)
                d->releaseItem(d->visibleItems.at(i));
            d->visibleItems.clear();
            d->releaseItem(d->currentItem);
            d->currentItem = 0;
            updateSections();
            d->refill();
            d->moveReason = QSGItemViewPrivate::SetIndex;
            d->updateCurrent(d->currentIndex);
            if (d->highlight && d->currentItem) {
                if (d->autoHighlight)
                    d->resetHighlightPosition();
                d->updateTrackedItem();
            }
            d->moveReason = QSGItemViewPrivate::Other;
            d->updateViewport();
        }
        if (oldCount != dataModel->count())
            emit countChanged();
    }
    emit delegateChanged();
}


int QSGItemView::count() const
{
    Q_D(const QSGItemView);
    if (!d->model)
        return 0;
    const_cast<QSGItemViewPrivate*>(d)->applyPendingChanges();
    return d->model->count();
}

int QSGItemView::currentIndex() const
{
    Q_D(const QSGItemView);
    const_cast<QSGItemViewPrivate*>(d)->applyPendingChanges();
    return d->currentIndex;
}

void QSGItemView::setCurrentIndex(int index)
{
    Q_D(QSGItemView);
    if (d->requestedIndex >= 0)  // currently creating item
        return;
    d->currentIndexCleared = (index == -1);

    d->applyPendingChanges();
    if (index == d->currentIndex)
        return;
    if (isComponentComplete() && d->isValid()) {
        d->moveReason = QSGItemViewPrivate::SetIndex;
        d->updateCurrent(index);
    } else if (d->currentIndex != index) {
        d->currentIndex = index;
        emit currentIndexChanged();
    }
}


bool QSGItemView::isWrapEnabled() const
{
    Q_D(const QSGItemView);
    return d->wrap;
}

void QSGItemView::setWrapEnabled(bool wrap)
{
    Q_D(QSGItemView);
    if (d->wrap == wrap)
        return;
    d->wrap = wrap;
    emit keyNavigationWrapsChanged();
}

int QSGItemView::cacheBuffer() const
{
    Q_D(const QSGItemView);
    return d->buffer;
}

void QSGItemView::setCacheBuffer(int b)
{
    Q_D(QSGItemView);
    if (d->buffer != b) {
        d->buffer = b;
        if (isComponentComplete()) {
            d->bufferMode = QSGItemViewPrivate::BufferBefore | QSGItemViewPrivate::BufferAfter;
            d->refill();
        }
        emit cacheBufferChanged();
    }
}


Qt::LayoutDirection QSGItemView::layoutDirection() const
{
    Q_D(const QSGItemView);
    return d->layoutDirection;
}

void QSGItemView::setLayoutDirection(Qt::LayoutDirection layoutDirection)
{
    Q_D(QSGItemView);
    if (d->layoutDirection != layoutDirection) {
        d->layoutDirection = layoutDirection;
        d->regenerate();
        emit layoutDirectionChanged();
        emit effectiveLayoutDirectionChanged();
    }
}

Qt::LayoutDirection QSGItemView::effectiveLayoutDirection() const
{
    Q_D(const QSGItemView);
    if (d->effectiveLayoutMirror)
        return d->layoutDirection == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft;
    else
        return d->layoutDirection;
}


QDeclarativeComponent *QSGItemView::header() const
{
    Q_D(const QSGItemView);
    return d->headerComponent;
}

QSGItem *QSGItemView::headerItem() const
{
    Q_D(const QSGItemView);
    const_cast<QSGItemViewPrivate*>(d)->applyPendingChanges();
    return d->header ? d->header->item : 0;
}

void QSGItemView::setHeader(QDeclarativeComponent *headerComponent)
{
    Q_D(QSGItemView);
    if (d->headerComponent != headerComponent) {
        d->applyPendingChanges();
        delete d->header;
        d->header = 0;
        d->headerComponent = headerComponent;

        d->markExtentsDirty();

        if (isComponentComplete()) {
            d->updateHeader();
            d->updateFooter();
            d->updateViewport();
            d->fixupPosition();
        } else {
            emit headerItemChanged();
        }
        emit headerChanged();
    }
}

QDeclarativeComponent *QSGItemView::footer() const
{
    Q_D(const QSGItemView);
    return d->footerComponent;
}

QSGItem *QSGItemView::footerItem() const
{
    Q_D(const QSGItemView);
    const_cast<QSGItemViewPrivate*>(d)->applyPendingChanges();
    return d->footer ? d->footer->item : 0;
}

void QSGItemView::setFooter(QDeclarativeComponent *footerComponent)
{
    Q_D(QSGItemView);
    if (d->footerComponent != footerComponent) {
        d->applyPendingChanges();
        delete d->footer;
        d->footer = 0;
        d->footerComponent = footerComponent;

        if (isComponentComplete()) {
            d->updateFooter();
            d->updateViewport();
            d->fixupPosition();
        } else {
            emit footerItemChanged();
        }
        emit footerChanged();
    }
}

QDeclarativeComponent *QSGItemView::highlight() const
{
    Q_D(const QSGItemView);
    const_cast<QSGItemViewPrivate*>(d)->applyPendingChanges();
    return d->highlightComponent;
}

void QSGItemView::setHighlight(QDeclarativeComponent *highlightComponent)
{
    Q_D(QSGItemView);
    if (highlightComponent != d->highlightComponent) {
        d->applyPendingChanges();
        d->highlightComponent = highlightComponent;
        d->createHighlight();
        if (d->currentItem)
            d->updateHighlight();
        emit highlightChanged();
    }
}

QSGItem *QSGItemView::highlightItem() const
{
    Q_D(const QSGItemView);
    const_cast<QSGItemViewPrivate*>(d)->applyPendingChanges();
    return d->highlight ? d->highlight->item : 0;
}

bool QSGItemView::highlightFollowsCurrentItem() const
{
    Q_D(const QSGItemView);
    return d->autoHighlight;
}

void QSGItemView::setHighlightFollowsCurrentItem(bool autoHighlight)
{
    Q_D(QSGItemView);
    if (d->autoHighlight != autoHighlight) {
        d->autoHighlight = autoHighlight;
        if (autoHighlight)
            d->updateHighlight();
        emit highlightFollowsCurrentItemChanged();
    }
}

QSGItemView::HighlightRangeMode QSGItemView::highlightRangeMode() const
{
    Q_D(const QSGItemView);
    return static_cast<QSGItemView::HighlightRangeMode>(d->highlightRange);
}

void QSGItemView::setHighlightRangeMode(HighlightRangeMode mode)
{
    Q_D(QSGItemView);
    if (d->highlightRange == mode)
        return;
    d->highlightRange = mode;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit highlightRangeModeChanged();
}

//###Possibly rename these properties, since they are very useful even without a highlight?
qreal QSGItemView::preferredHighlightBegin() const
{
    Q_D(const QSGItemView);
    return d->highlightRangeStart;
}

void QSGItemView::setPreferredHighlightBegin(qreal start)
{
    Q_D(QSGItemView);
    d->highlightRangeStartValid = true;
    if (d->highlightRangeStart == start)
        return;
    d->highlightRangeStart = start;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit preferredHighlightBeginChanged();
}

void QSGItemView::resetPreferredHighlightBegin()
{
    Q_D(QSGItemView);
    d->highlightRangeStartValid = false;
    if (d->highlightRangeStart == 0)
        return;
    d->highlightRangeStart = 0;
    emit preferredHighlightBeginChanged();
}

qreal QSGItemView::preferredHighlightEnd() const
{
    Q_D(const QSGItemView);
    return d->highlightRangeEnd;
}

void QSGItemView::setPreferredHighlightEnd(qreal end)
{
    Q_D(QSGItemView);
    d->highlightRangeEndValid = true;
    if (d->highlightRangeEnd == end)
        return;
    d->highlightRangeEnd = end;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit preferredHighlightEndChanged();
}

void QSGItemView::resetPreferredHighlightEnd()
{
    Q_D(QSGItemView);
    d->highlightRangeEndValid = false;
    if (d->highlightRangeEnd == 0)
        return;
    d->highlightRangeEnd = 0;
    emit preferredHighlightEndChanged();
}

int QSGItemView::highlightMoveDuration() const
{
    Q_D(const QSGItemView);
    return d->highlightMoveDuration;
}

void QSGItemView::setHighlightMoveDuration(int duration)
{
    Q_D(QSGItemView);
    if (d->highlightMoveDuration != duration) {
        d->highlightMoveDuration = duration;
        emit highlightMoveDurationChanged();
    }
}

void QSGItemViewPrivate::positionViewAtIndex(int index, int mode)
{
    Q_Q(QSGItemView);
    if (!isValid())
        return;
    if (mode < QSGItemView::Beginning || mode > QSGItemView::Contain)
        return;

    applyPendingChanges();
    int idx = qMax(qMin(index, model->count()-1), 0);

    qreal pos = isContentFlowReversed() ? -position() - size() : position();
    FxViewItem *item = visibleItem(idx);
    qreal maxExtent;
    if (layoutOrientation() == Qt::Vertical)
        maxExtent = -q->maxYExtent();
    else
        maxExtent = isContentFlowReversed() ? q->minXExtent()-size(): -q->maxXExtent();
    if (!item) {
        int itemPos = positionAt(idx);
        changedVisibleIndex(idx);
        // save the currently visible items in case any of them end up visible again
        QList<FxViewItem *> oldVisible = visibleItems;
        visibleItems.clear();
        setPosition(qMin(qreal(itemPos), maxExtent));
        // now release the reference to all the old visible items.
        for (int i = 0; i < oldVisible.count(); ++i)
            releaseItem(oldVisible.at(i));
        item = visibleItem(idx);
    }
    if (item) {
        const qreal itemPos = item->position();
        switch (mode) {
        case QSGItemView::Beginning:
            pos = itemPos;
            if (index < 0 && header)
                pos -= headerSize();
            break;
        case QSGItemView::Center:
            pos = itemPos - (size() - item->size())/2;
            break;
        case QSGItemView::End:
            pos = itemPos - size() + item->size();
            if (index >= model->count() && footer)
                pos += footerSize();
            break;
        case QSGItemView::Visible:
            if (itemPos > pos + size())
                pos = itemPos - size() + item->size();
            else if (item->endPosition() <= pos)
                pos = itemPos;
            break;
        case QSGItemView::Contain:
            if (item->endPosition() >= pos + size())
                pos = itemPos - size() + item->size();
            if (itemPos < pos)
                pos = itemPos;
        }
        pos = qMin(pos, maxExtent);
        qreal minExtent;
        if (layoutOrientation() == Qt::Vertical)
            minExtent = -q->minYExtent();
        else
            minExtent = isContentFlowReversed() ? q->maxXExtent()-size(): -q->minXExtent();
        pos = qMax(pos, minExtent);
        moveReason = QSGItemViewPrivate::Other;
        q->cancelFlick();
        setPosition(pos);

        if (highlight) {
            if (autoHighlight)
                resetHighlightPosition();
            updateHighlight();
        }
    }
    fixupPosition();
}

void QSGItemView::positionViewAtIndex(int index, int mode)
{
    Q_D(QSGItemView);
    if (!d->isValid() || index < 0 || index >= d->model->count())
        return;
    d->positionViewAtIndex(index, mode);
}


void QSGItemView::positionViewAtBeginning()
{
    Q_D(QSGItemView);
    if (!d->isValid())
        return;
    d->positionViewAtIndex(-1, Beginning);
}

void QSGItemView::positionViewAtEnd()
{
    Q_D(QSGItemView);
    if (!d->isValid())
        return;
    d->positionViewAtIndex(d->model->count(), End);
}

int QSGItemView::indexAt(qreal x, qreal y) const
{
    Q_D(const QSGItemView);
    for (int i = 0; i < d->visibleItems.count(); ++i) {
        const FxViewItem *item = d->visibleItems.at(i);
        if (item->contains(x, y))
            return item->index;
    }

    return -1;
}

void QSGItemViewPrivate::applyPendingChanges()
{
    Q_Q(QSGItemView);
    if (q->isComponentComplete() && currentChanges.hasPendingChanges())
        layout();
}

// for debugging only
void QSGItemViewPrivate::checkVisible() const
{
    int skip = 0;
    for (int i = 0; i < visibleItems.count(); ++i) {
        FxViewItem *item = visibleItems.at(i);
        if (item->index == -1) {
            ++skip;
        } else if (item->index != visibleIndex + i - skip) {
            qFatal("index %d %d %d", visibleIndex, i, item->index);
        }
    }
}

void QSGItemViewPrivate::itemGeometryChanged(QSGItem *item, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_Q(QSGItemView);
    QSGFlickablePrivate::itemGeometryChanged(item, newGeometry, oldGeometry);
    if (!q->isComponentComplete())
        return;

    if (header && header->item == item) {
        updateHeader();
        markExtentsDirty();
        if (!q->isMoving() && !q->isFlicking())
            fixupPosition();
    } else if (footer && footer->item == item) {
        updateFooter();
        markExtentsDirty();
        if (!q->isMoving() && !q->isFlicking())
            fixupPosition();
    }

    if (currentItem && currentItem->item == item)
        updateHighlight();
    if (trackedItem && trackedItem->item == item)
        q->trackedPositionChanged();
}

void QSGItemView::destroyRemoved()
{
    Q_D(QSGItemView);
    for (QList<FxViewItem*>::Iterator it = d->visibleItems.begin();
            it != d->visibleItems.end();) {
        FxViewItem *item = *it;
        if (item->index == -1 && item->attached->delayRemove() == false) {
            d->releaseItem(item);
            it = d->visibleItems.erase(it);
        } else {
            ++it;
        }
    }

    // Correct the positioning of the items
    d->updateSections();
    d->forceLayout = true;
    d->layout();
}

void QSGItemView::modelUpdated(const QDeclarativeChangeSet &changeSet, bool reset)
{
    Q_D(QSGItemView);
    if (reset) {
        d->moveReason = QSGItemViewPrivate::SetIndex;
        d->regenerate();
        if (d->highlight && d->currentItem) {
            if (d->autoHighlight)
                d->resetHighlightPosition();
            d->updateTrackedItem();
        }
        d->moveReason = QSGItemViewPrivate::Other;

        emit countChanged();
    } else {
        d->currentChanges.prepare(d->currentIndex, d->itemCount);
        d->currentChanges.applyChanges(changeSet);
        polish();
    }
}

void QSGItemView::createdItem(int index, QSGItem *item)
{
    Q_D(QSGItemView);
    if (d->requestedIndex != index) {
        item->setParentItem(contentItem());
        d->unrequestedItems.insert(item, index);
        d->repositionPackageItemAt(item, index);
    }
}

void QSGItemView::destroyingItem(QSGItem *item)
{
    Q_D(QSGItemView);
    d->unrequestedItems.remove(item);
}

void QSGItemView::animStopped()
{
    Q_D(QSGItemView);
    d->bufferMode = QSGItemViewPrivate::NoBuffer;
    if (d->haveHighlightRange && d->highlightRange == QSGItemView::StrictlyEnforceRange)
        d->updateHighlight();
}


void QSGItemView::trackedPositionChanged()
{
    Q_D(QSGItemView);
    if (!d->trackedItem || !d->currentItem)
        return;
    if (d->moveReason == QSGItemViewPrivate::SetIndex) {
        qreal trackedPos = d->trackedItem->position();
        qreal trackedSize = d->trackedItem->size();
        if (d->trackedItem != d->currentItem) {
            trackedSize += d->currentItem->sectionSize();
        }
        qreal viewPos = d->isContentFlowReversed() ? -d->position()-d->size() : d->position();
        qreal pos = viewPos;
        if (d->haveHighlightRange) {
            if (trackedPos > pos + d->highlightRangeEnd - trackedSize)
                pos = trackedPos - d->highlightRangeEnd + trackedSize;
            if (trackedPos < pos + d->highlightRangeStart)
                pos = trackedPos - d->highlightRangeStart;
            if (d->highlightRange != StrictlyEnforceRange) {
                if (pos > d->endPosition() - d->size())
                    pos = d->endPosition() - d->size();
                if (pos < d->startPosition())
                    pos = d->startPosition();
            }
        } else {
            qreal trackedEndPos = d->trackedItem->endPosition();
            qreal toItemPos = d->currentItem->position();
            qreal toItemEndPos = d->currentItem->endPosition();

            if (d->header && d->showHeaderForIndex(d->currentIndex)) {
                trackedPos -= d->headerSize();
                trackedEndPos -= d->headerSize();
                toItemPos -= d->headerSize();
                toItemEndPos -= d->headerSize();
            } else if (d->footer && d->showFooterForIndex(d->currentIndex)) {
                trackedPos += d->footerSize();
                trackedEndPos += d->footerSize();
                toItemPos += d->footerSize();
                toItemEndPos += d->footerSize();
            }

            if (trackedPos < viewPos && toItemPos < viewPos) {
                pos = qMax(trackedPos, toItemPos);
            } else if (trackedEndPos >= viewPos + d->size()
                && toItemEndPos >= viewPos + d->size()) {
                if (trackedEndPos <= toItemEndPos) {
                    pos = trackedEndPos - d->size();
                    if (trackedSize > d->size())
                        pos = trackedPos;
                } else {
                    pos = toItemEndPos - d->size();
                    if (d->currentItem->size() > d->size())
                        pos = d->currentItem->position();
                }
            }
        }
        if (viewPos != pos) {
            cancelFlick();
            d->calcVelocity = true;
            d->setPosition(pos);
            d->calcVelocity = false;
        }
    }
}

void QSGItemView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QSGItemView);
    d->markExtentsDirty();
    QSGFlickable::geometryChanged(newGeometry, oldGeometry);
}


qreal QSGItemView::minYExtent() const
{
    Q_D(const QSGItemView);
    if (d->layoutOrientation() == Qt::Horizontal)
        return QSGFlickable::minYExtent();

    if (d->vData.minExtentDirty) {
        d->minExtent = d->vData.startMargin-d->startPosition();
        if (d->header)
            d->minExtent += d->headerSize();
        if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
            d->minExtent += d->highlightRangeStart;
            if (d->visibleItem(0))
                d->minExtent -= d->visibleItem(0)->sectionSize();
            d->minExtent = qMax(d->minExtent, -(d->endPositionAt(0) - d->highlightRangeEnd));
        }
        d->vData.minExtentDirty = false;
    }

    return d->minExtent;
}

qreal QSGItemView::maxYExtent() const
{
    Q_D(const QSGItemView);
    if (d->layoutOrientation() == Qt::Horizontal)
        return height();

    if (d->vData.maxExtentDirty) {
        if (!d->model || !d->model->count()) {
            d->maxExtent = d->header ? -d->headerSize() : 0;
            d->maxExtent += height();
        } else if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
            d->maxExtent = -(d->positionAt(d->model->count()-1) - d->highlightRangeStart);
            if (d->highlightRangeEnd != d->highlightRangeStart)
                d->maxExtent = qMin(d->maxExtent, -(d->endPosition() - d->highlightRangeEnd));
        } else {
            d->maxExtent = -(d->endPosition() - height());
        }

        if (d->footer)
            d->maxExtent -= d->footerSize();
        d->maxExtent -= d->vData.endMargin;
        qreal minY = minYExtent();
        if (d->maxExtent > minY)
            d->maxExtent = minY;
        d->vData.maxExtentDirty = false;
    }
    return d->maxExtent;
}

qreal QSGItemView::minXExtent() const
{
    Q_D(const QSGItemView);
    if (d->layoutOrientation() == Qt::Vertical)
        return QSGFlickable::minXExtent();

    if (d->hData.minExtentDirty) {
        d->minExtent = -d->startPosition();
        qreal highlightStart;
        qreal highlightEnd;
        qreal endPositionFirstItem = 0;
        if (d->isContentFlowReversed()) {
            d->minExtent += d->hData.endMargin;
            if (d->model && d->model->count())
                endPositionFirstItem = d->positionAt(d->model->count()-1);
            else if (d->header)
                d->minExtent += d->headerSize();
            highlightStart = d->highlightRangeEndValid ? d->size() - d->highlightRangeEnd : d->size();
            highlightEnd = d->highlightRangeStartValid ? d->size() - d->highlightRangeStart : d->size();
            if (d->footer)
                d->minExtent += d->footerSize();
            qreal maxX = maxXExtent();
            if (d->minExtent < maxX)
                d->minExtent = maxX;
        } else {
            d->minExtent += d->hData.startMargin;
            endPositionFirstItem = d->endPositionAt(0);
            highlightStart = d->highlightRangeStart;
            highlightEnd = d->highlightRangeEnd;
            if (d->header)
                d->minExtent += d->headerSize();
        }
        if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
            d->minExtent += highlightStart;
            d->minExtent = d->isContentFlowReversed()
                                ? qMin(d->minExtent, endPositionFirstItem + highlightEnd)
                                : qMax(d->minExtent, -(endPositionFirstItem - highlightEnd));
        }
        d->hData.minExtentDirty = false;
    }

    return d->minExtent;
}

qreal QSGItemView::maxXExtent() const
{
    Q_D(const QSGItemView);
    if (d->layoutOrientation() == Qt::Vertical)
        return width();

    if (d->hData.maxExtentDirty) {
        qreal highlightStart;
        qreal highlightEnd;
        qreal lastItemPosition = 0;
        d->maxExtent = 0;
        if (d->isContentFlowReversed()) {
            highlightStart = d->highlightRangeEndValid ? d->size() - d->highlightRangeEnd : d->size();
            highlightEnd = d->highlightRangeStartValid ? d->size() - d->highlightRangeStart : d->size();
            lastItemPosition = d->endPosition();
        } else {
            highlightStart = d->highlightRangeStart;
            highlightEnd = d->highlightRangeEnd;
            if (d->model && d->model->count())
                lastItemPosition = d->positionAt(d->model->count()-1);
        }
        if (!d->model || !d->model->count()) {
            if (!d->isContentFlowReversed())
                d->maxExtent = d->header ? -d->headerSize() : 0;
            d->maxExtent += width();
        } else if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
            d->maxExtent = -(lastItemPosition - highlightStart);
            if (highlightEnd != highlightStart) {
                d->maxExtent = d->isContentFlowReversed()
                        ? qMax(d->maxExtent, -(d->endPosition() - highlightEnd))
                        : qMin(d->maxExtent, -(d->endPosition() - highlightEnd));
            }
        } else {
            d->maxExtent = -(d->endPosition() - width());
        }
        if (d->isContentFlowReversed()) {
            if (d->header)
                d->maxExtent -= d->headerSize();
            d->maxExtent -= d->hData.startMargin;
        } else {
            if (d->footer)
                d->maxExtent -= d->footerSize();
            d->maxExtent -= d->hData.endMargin;
            qreal minX = minXExtent();
            if (d->maxExtent > minX)
                d->maxExtent = minX;
        }
        d->hData.maxExtentDirty = false;
    }

    return d->maxExtent;
}

void QSGItemView::setContentX(qreal pos)
{
    Q_D(QSGItemView);
    // Positioning the view manually should override any current movement state
    d->moveReason = QSGItemViewPrivate::Other;
    QSGFlickable::setContentX(pos);
}

void QSGItemView::setContentY(qreal pos)
{
    Q_D(QSGItemView);
    // Positioning the view manually should override any current movement state
    d->moveReason = QSGItemViewPrivate::Other;
    QSGFlickable::setContentY(pos);
}

qreal QSGItemView::xOrigin() const
{
    Q_D(const QSGItemView);
    if (d->isContentFlowReversed())
        return -maxXExtent() + d->size() - d->hData.startMargin;
    else
        return -minXExtent() + d->hData.startMargin;
}

void QSGItemView::updatePolish()
{
    Q_D(QSGItemView);
    QSGFlickable::updatePolish();
    d->layout();
}

void QSGItemView::componentComplete()
{
    Q_D(QSGItemView);
    if (d->model && d->ownModel)
        static_cast<QSGVisualDataModel *>(d->model.data())->componentComplete();

    QSGFlickable::componentComplete();

    updateSections();
    d->updateHeader();
    d->updateFooter();
    d->updateViewport();
    d->setPosition(d->contentStartPosition());
    if (d->isValid()) {
        d->refill();
        d->moveReason = QSGItemViewPrivate::SetIndex;
        if (d->currentIndex < 0 && !d->currentIndexCleared)
            d->updateCurrent(0);
        else
            d->updateCurrent(d->currentIndex);
        if (d->highlight && d->currentItem) {
            if (d->autoHighlight)
                d->resetHighlightPosition();
            d->updateTrackedItem();
        }
        d->moveReason = QSGItemViewPrivate::Other;
        d->fixupPosition();
    }
    if (d->model && d->model->count())
        emit countChanged();
}



QSGItemViewPrivate::QSGItemViewPrivate()
    : itemCount(0)
    , buffer(0), bufferMode(BufferBefore | BufferAfter)
    , layoutDirection(Qt::LeftToRight)
    , moveReason(Other)
    , visibleIndex(0)
    , currentIndex(-1), currentItem(0)
    , trackedItem(0), requestedIndex(-1)
    , highlightComponent(0), highlight(0)
    , highlightRange(QSGItemView::NoHighlightRange)
    , highlightRangeStart(0), highlightRangeEnd(0)
    , highlightMoveDuration(150)
    , headerComponent(0), header(0), footerComponent(0), footer(0)
    , minExtent(0), maxExtent(0)
    , ownModel(false), wrap(false), lazyRelease(false), deferredRelease(false)
    , inApplyModelChanges(false), inViewportMoved(false), forceLayout(false), currentIndexCleared(false)
    , haveHighlightRange(false), autoHighlight(true), highlightRangeStartValid(false), highlightRangeEndValid(false)
{
}

bool QSGItemViewPrivate::isValid() const
{
    return model && model->count() && model->isValid();
}

qreal QSGItemViewPrivate::position() const
{
    Q_Q(const QSGItemView);
    return layoutOrientation() == Qt::Vertical ? q->contentY() : q->contentX();
}

qreal QSGItemViewPrivate::size() const
{
    Q_Q(const QSGItemView);
    return layoutOrientation() == Qt::Vertical ? q->height() : q->width();
}

qreal QSGItemViewPrivate::startPosition() const
{
    return isContentFlowReversed() ? -lastPosition() : originPosition();
}

qreal QSGItemViewPrivate::endPosition() const
{
    return isContentFlowReversed() ? -originPosition() : lastPosition();
}

qreal QSGItemViewPrivate::contentStartPosition() const
{
    Q_Q(const QSGItemView);
    qreal pos = -headerSize();
    if (layoutOrientation() == Qt::Vertical)
        pos -= vData.startMargin;
    else if (isContentFlowReversed())
        pos -= hData.endMargin;
    else
        pos -= hData.startMargin;

    return pos;
}

int QSGItemViewPrivate::findLastVisibleIndex(int defaultValue) const
{
    if (visibleItems.count()) {
        int i = visibleItems.count() - 1;
        while (i > 0 && visibleItems.at(i)->index == -1)
            --i;
        if (visibleItems.at(i)->index != -1)
            return visibleItems.at(i)->index;
    }
    return defaultValue;
}

FxViewItem *QSGItemViewPrivate::visibleItem(int modelIndex) const {
    if (modelIndex >= visibleIndex && modelIndex < visibleIndex + visibleItems.count()) {
        for (int i = modelIndex - visibleIndex; i < visibleItems.count(); ++i) {
            FxViewItem *item = visibleItems.at(i);
            if (item->index == modelIndex)
                return item;
        }
    }
    return 0;
}

FxViewItem *QSGItemViewPrivate::firstVisibleItem() const {
    const qreal pos = isContentFlowReversed() ? -position()-size() : position();
    for (int i = 0; i < visibleItems.count(); ++i) {
        FxViewItem *item = visibleItems.at(i);
        if (item->index != -1 && item->endPosition() > pos)
            return item;
    }
    return visibleItems.count() ? visibleItems.first() : 0;
}

// Map a model index to visibleItems list index.
// These may differ if removed items are still present in the visible list,
// e.g. doing a removal animation
int QSGItemViewPrivate::mapFromModel(int modelIndex) const
{
    if (modelIndex < visibleIndex || modelIndex >= visibleIndex + visibleItems.count())
        return -1;
    for (int i = 0; i < visibleItems.count(); ++i) {
        FxViewItem *item = visibleItems.at(i);
        if (item->index == modelIndex)
            return i;
        if (item->index > modelIndex)
            return -1;
    }
    return -1; // Not in visibleList
}

void QSGItemViewPrivate::init()
{
    Q_Q(QSGItemView);
    QSGItemPrivate::get(contentItem)->childrenDoNotOverlap = true;
    q->setFlag(QSGItem::ItemIsFocusScope);
    addItemChangeListener(this, Geometry);
    QObject::connect(q, SIGNAL(movementEnded()), q, SLOT(animStopped()));
    q->setFlickableDirection(QSGFlickable::VerticalFlick);
}

void QSGItemViewPrivate::updateCurrent(int modelIndex)
{
    Q_Q(QSGItemView);
    applyPendingChanges();

    if (!q->isComponentComplete() || !isValid() || modelIndex < 0 || modelIndex >= model->count()) {
        if (currentItem) {
            currentItem->attached->setIsCurrentItem(false);
            releaseItem(currentItem);
            currentItem = 0;
            currentIndex = modelIndex;
            emit q->currentIndexChanged();
            updateHighlight();
        } else if (currentIndex != modelIndex) {
            currentIndex = modelIndex;
            emit q->currentIndexChanged();
        }
        return;
    }

    if (currentItem && currentIndex == modelIndex) {
        updateHighlight();
        return;
    }

    FxViewItem *oldCurrentItem = currentItem;
    currentIndex = modelIndex;
    currentItem = createItem(modelIndex);
    if (oldCurrentItem && (!currentItem || oldCurrentItem->item != currentItem->item))
        oldCurrentItem->attached->setIsCurrentItem(false);
    if (currentItem) {
        currentItem->item->setFocus(true);
        currentItem->attached->setIsCurrentItem(true);
        initializeCurrentItem();
    }

    updateHighlight();
    emit q->currentIndexChanged();
    releaseItem(oldCurrentItem);
}

void QSGItemViewPrivate::clear()
{
    currentChanges.reset();
    timeline.clear();

    for (int i = 0; i < visibleItems.count(); ++i)
        releaseItem(visibleItems.at(i));
    visibleItems.clear();
    visibleIndex = 0;

    releaseItem(currentItem);
    currentItem = 0;
    createHighlight();
    trackedItem = 0;

    markExtentsDirty();
    itemCount = 0;
}


void QSGItemViewPrivate::mirrorChange()
{
    Q_Q(QSGItemView);
    regenerate();
    emit q->effectiveLayoutDirectionChanged();
}

void QSGItemViewPrivate::refill()
{
    if (isContentFlowReversed())
        refill(-position()-size(), -position());
    else
        refill(position(), position()+size());
}

void QSGItemViewPrivate::refill(qreal from, qreal to, bool doBuffer)
{
    Q_Q(QSGItemView);
    if (!isValid() || !q->isComponentComplete())
        return;

    currentChanges.reset();

    int prevCount = itemCount;
    itemCount = model->count();
    qreal bufferFrom = from - buffer;
    qreal bufferTo = to + buffer;
    qreal fillFrom = from;
    qreal fillTo = to;
    if (doBuffer && (bufferMode & BufferAfter))
        fillTo = bufferTo;
    if (doBuffer && (bufferMode & BufferBefore))
        fillFrom = bufferFrom;

    // Item creation and release is staggered in order to avoid
    // creating/releasing multiple items in one frame
    // while flicking (as much as possible).

    bool changed = addVisibleItems(fillFrom, fillTo, doBuffer);

    if (!lazyRelease || !changed || deferredRelease) { // avoid destroying items in the same frame that we create
        if (removeNonVisibleItems(bufferFrom, bufferTo))
            changed = true;
        deferredRelease = false;
    } else {
        deferredRelease = true;
    }

    if (changed) {
        markExtentsDirty();
        visibleItemsChanged();
    } else if (!doBuffer && buffer && bufferMode != NoBuffer) {
        refill(from, to, true);
    }

    lazyRelease = false;
    if (prevCount != itemCount)
        emit q->countChanged();
}

void QSGItemViewPrivate::regenerate()
{
    Q_Q(QSGItemView);
    if (q->isComponentComplete()) {
        currentChanges.reset();
        delete header;
        header = 0;
        delete footer;
        footer = 0;
        updateHeader();
        updateFooter();
        clear();
        updateViewport();
        setPosition(contentStartPosition());
        refill();
        updateCurrent(currentIndex);
    }
}

void QSGItemViewPrivate::updateViewport()
{
    Q_Q(QSGItemView);
    if (isValid()) {
        if (layoutOrientation() == Qt::Vertical)
            q->setContentHeight(endPosition() - startPosition());
        else
            q->setContentWidth(endPosition() - startPosition());
    }
}

void QSGItemViewPrivate::layout()
{
    Q_Q(QSGItemView);
    if (inApplyModelChanges)
        return;

    if (!isValid() && !visibleItems.count()) {
        clear();
        setPosition(contentStartPosition());
        return;
    }

    if (!applyModelChanges() && !forceLayout)
        return;
    forceLayout = false;

    layoutVisibleItems();
    refill();

    markExtentsDirty();

    updateHighlight();

    if (!q->isMoving() && !q->isFlicking()) {
        fixupPosition();
        refill();
    }

    updateHeader();
    updateFooter();
    updateViewport();
    updateUnrequestedPositions();
}

bool QSGItemViewPrivate::applyModelChanges()
{
    Q_Q(QSGItemView);
    if (!q->isComponentComplete() || !currentChanges.hasPendingChanges() || inApplyModelChanges)
        return false;
    inApplyModelChanges = true;

    updateUnrequestedIndexes();
    moveReason = QSGItemViewPrivate::Other;

    int prevCount = itemCount;
    bool removedVisible = false;
    bool viewportChanged = !currentChanges.pendingChanges.removes().isEmpty()
            || !currentChanges.pendingChanges.inserts().isEmpty();

    FxViewItem *firstVisible = firstVisibleItem();
    FxViewItem *origVisibleItemsFirst = visibleItems.count() ? visibleItems.first() : 0;
    int firstItemIndex = firstVisible ? firstVisible->index : -1;
    qreal removedBeforeFirstVisibleBy = 0;

    const QVector<QDeclarativeChangeSet::Remove> &removals = currentChanges.pendingChanges.removes();
    for (int i=0; i<removals.count(); i++) {
        itemCount -= removals[i].count;

        // Remove the items from the visible list, skipping anything already marked for removal
        QList<FxViewItem*>::Iterator it = visibleItems.begin();
        while (it != visibleItems.end()) {
            FxViewItem *item = *it;
            if (item->index == -1 || item->index < removals[i].index) {
                // already removed, or before removed items
                if (item->index < removals[i].index && !removedVisible)
                    removedVisible = true;
                ++it;
            } else if (item->index >= removals[i].index + removals[i].count) {
                // after removed items
                item->index -= removals[i].count;
                ++it;
            } else {
                // removed item
                removedVisible = true;
                if (!removals[i].isMove())
                    item->attached->emitRemove();

                if (item->attached->delayRemove() && !removals[i].isMove()) {
                    item->index = -1;
                    QObject::connect(item->attached, SIGNAL(delayRemoveChanged()), q, SLOT(destroyRemoved()), Qt::QueuedConnection);
                    ++it;
                } else {
                    if (firstVisible && item->position() < firstVisible->position() && item != visibleItems.first())
                        removedBeforeFirstVisibleBy += item->size();
                    if (removals[i].isMove()) {
                        currentChanges.removedItems.insert(removals[i].moveKey(item->index), item);
                    } else {
                        if (item == firstVisible)
                            firstVisible = 0;
                        currentChanges.removedItems.insertMulti(QDeclarativeChangeSet::MoveKey(), item);
                    }
                    it = visibleItems.erase(it);
                }
            }
        }

    }
    if (!removals.isEmpty())
        updateVisibleIndex();

    const QVector<QDeclarativeChangeSet::Insert> &insertions = currentChanges.pendingChanges.inserts();
    bool addedVisible = false;
    InsertionsResult insertResult;
    bool allInsertionsBeforeVisible = true;

    for (int i=0; i<insertions.count(); i++) {
        bool wasEmpty = visibleItems.isEmpty();
        if (applyInsertionChange(insertions[i], firstVisible, &insertResult))
            addedVisible = true;
        if (insertions[i].index >= visibleIndex)
            allInsertionsBeforeVisible = false;
        if (wasEmpty && !visibleItems.isEmpty())
            resetFirstItemPosition();
        itemCount += insertions[i].count;
    }
    for (int i=0; i<insertResult.addedItems.count(); ++i)
        insertResult.addedItems.at(i)->attached->emitAdd();

    // if the first visible item has moved, ensure another one takes its place
    // so that we avoid shifting all content forwards
    // (if an item is removed from before the first visible, the first visible should not move upwards)
    if (firstVisible && firstItemIndex >= 0) {
        bool found = false;
        for (int i=0; i<insertResult.movedBackwards.count(); i++) {
            if (insertResult.movedBackwards[i]->index == firstItemIndex) {
                // an item has moved backwards up to the first visible's position
                resetItemPosition(insertResult.movedBackwards[i], firstVisible);
                insertResult.movedBackwards.removeAt(i);
                found = true;
                break;
            }
        }
        if (!found && !allInsertionsBeforeVisible) {
            // first visible item has moved forward, another visible item takes its place
            FxViewItem *item = visibleItem(firstItemIndex);
            if (item)
                resetItemPosition(item, firstVisible);
        }
    }

    // Ensure we don't cause an ugly list scroll
    if (firstVisible && visibleItems.count() && visibleItems.first() != firstVisible) {
        // ensure first item is placed at correct postion if moving backward
        // since it will be used to position all subsequent items
        if (insertResult.movedBackwards.count() && origVisibleItemsFirst)
            resetItemPosition(visibleItems.first(), origVisibleItemsFirst);
        qreal moveBackwardsBy = insertResult.sizeAddedBeforeVisible;
        for (int i=0; i<insertResult.movedBackwards.count(); i++)
            moveBackwardsBy += insertResult.movedBackwards[i]->size();
        moveItemBy(visibleItems.first(), removedBeforeFirstVisibleBy, moveBackwardsBy);
    }

    // Whatever removed/moved items remain are no longer visible items.
    for (QHash<QDeclarativeChangeSet::MoveKey, FxViewItem *>::Iterator it = currentChanges.removedItems.begin();
         it != currentChanges.removedItems.end(); ++it) {
        releaseItem(it.value());
    }
    currentChanges.removedItems.clear();

    if (currentChanges.currentChanged) {
        if (currentChanges.currentRemoved && currentItem) {
            currentItem->attached->setIsCurrentItem(false);
            releaseItem(currentItem);
            currentItem = 0;
        }
        if (!currentIndexCleared)
            updateCurrent(currentChanges.newCurrentIndex);
    }
    currentChanges.reset();

    updateSections();
    if (prevCount != itemCount)
        emit q->countChanged();

    bool visibleAffected = removedVisible || addedVisible || !currentChanges.pendingChanges.changes().isEmpty();
    if (!visibleAffected && viewportChanged)
        updateViewport();

    inApplyModelChanges = false;
    return visibleAffected;
}

FxViewItem *QSGItemViewPrivate::createItem(int modelIndex)
{
    Q_Q(QSGItemView);

    requestedIndex = modelIndex;
    FxViewItem *viewItem = 0;

    if (QSGItem *item = model->item(modelIndex, false)) {
        viewItem = newViewItem(modelIndex, item);
        if (viewItem) {
            viewItem->index = modelIndex;
            if (model->completePending()) {
                // complete
                viewItem->item->setZ(1);
                QDeclarative_setParent_noEvent(viewItem->item, q->contentItem());
                viewItem->item->setParentItem(q->contentItem());
                model->completeItem();
            } else {
                QDeclarative_setParent_noEvent(viewItem->item, q->contentItem());
                viewItem->item->setParentItem(q->contentItem());
            }
            // do other set up for the new item that should not happen
            // until after bindings are evaluated
            initializeViewItem(viewItem);

            unrequestedItems.remove(viewItem->item);
        }
    }
    requestedIndex = -1;
    return viewItem;
}


void QSGItemViewPrivate::releaseItem(FxViewItem *item)
{
    Q_Q(QSGItemView);
    if (!item || !model)
        return;
    if (trackedItem == item)
        trackedItem = 0;
    QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item->item);
    itemPrivate->removeItemChangeListener(this, QSGItemPrivate::Geometry);
    if (model->release(item->item) == 0) {
        // item was not destroyed, and we no longer reference it.
        unrequestedItems.insert(item->item, model->indexOf(item->item, q));
    }
    delete item;
}

QSGItem *QSGItemViewPrivate::createHighlightItem()
{
    return createComponentItem(highlightComponent, true, true);
}

QSGItem *QSGItemViewPrivate::createComponentItem(QDeclarativeComponent *component, bool receiveItemGeometryChanges, bool createDefault)
{
    Q_Q(QSGItemView);

    QSGItem *item = 0;
    if (component) {
        QDeclarativeContext *creationContext = component->creationContext();
        QDeclarativeContext *context = new QDeclarativeContext(
                creationContext ? creationContext : qmlContext(q));
        QObject *nobj = component->create(context);
        if (nobj) {
            QDeclarative_setParent_noEvent(context, nobj);
            item = qobject_cast<QSGItem *>(nobj);
            if (!item)
                delete nobj;
        } else {
            delete context;
        }
    } else if (createDefault) {
        item = new QSGItem;
    }
    if (item) {
        QDeclarative_setParent_noEvent(item, q->contentItem());
        item->setParentItem(q->contentItem());
        if (receiveItemGeometryChanges) {
            QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item);
            itemPrivate->addItemChangeListener(this, QSGItemPrivate::Geometry);
        }
    }
    return item;
}

void QSGItemViewPrivate::updateTrackedItem()
{
    Q_Q(QSGItemView);
    FxViewItem *item = currentItem;
    if (highlight)
        item = highlight;
    trackedItem = item;

    if (trackedItem)
        q->trackedPositionChanged();
}

void QSGItemViewPrivate::updateUnrequestedIndexes()
{
    Q_Q(QSGItemView);
    for (QHash<QSGItem*,int>::iterator it = unrequestedItems.begin(); it != unrequestedItems.end(); ++it)
        *it = model->indexOf(it.key(), q);
}

void QSGItemViewPrivate::updateUnrequestedPositions()
{
    for (QHash<QSGItem*,int>::const_iterator it = unrequestedItems.begin(); it != unrequestedItems.end(); ++it)
        repositionPackageItemAt(it.key(), it.value());
}

void QSGItemViewPrivate::updateVisibleIndex()
{
    visibleIndex = 0;
    for (QList<FxViewItem*>::Iterator it = visibleItems.begin(); it != visibleItems.end(); ++it) {
        if ((*it)->index != -1) {
            visibleIndex = (*it)->index;
            break;
        }
    }
}

QT_END_NAMESPACE
