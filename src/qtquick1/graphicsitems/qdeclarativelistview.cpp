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

#include "QtQuick1/private/qdeclarativelistview_p.h"

#include "QtQuick1/private/qdeclarativeflickable_p_p.h"
#include "QtQuick1/private/qdeclarativevisualitemmodel_p.h"

#include "QtQuick1/private/qdeclarativesmoothedanimation_p_p.h"
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/private/qdeclarativeguard_p.h>
#include <QtDeclarative/qdeclarativeinfo.h>

#include <qmath.h>
#include <QKeyEvent>
#include "qplatformdefs.h"

QT_BEGIN_NAMESPACE

#ifndef QML_FLICK_SNAPONETHRESHOLD
#define QML_FLICK_SNAPONETHRESHOLD 30
#endif

void QDeclarative1ViewSection::setProperty(const QString &property)
{
    if (property != m_property) {
        m_property = property;
        emit propertyChanged();
    }
}

void QDeclarative1ViewSection::setCriteria(QDeclarative1ViewSection::SectionCriteria criteria)
{
    if (criteria != m_criteria) {
        m_criteria = criteria;
        emit criteriaChanged();
    }
}

void QDeclarative1ViewSection::setDelegate(QDeclarativeComponent *delegate)
{
    if (delegate != m_delegate) {
        m_delegate = delegate;
        emit delegateChanged();
    }
}

QString QDeclarative1ViewSection::sectionString(const QString &value)
{
    if (m_criteria == FirstCharacter)
        return value.isEmpty() ? QString() : value.at(0);
    else
        return value;
}

//----------------------------------------------------------------------------

class FxListItem1
{
public:
    FxListItem1(QDeclarativeItem *i, QDeclarative1ListView *v) : item(i), section(0), view(v) {
        attached = static_cast<QDeclarative1ListViewAttached*>(qmlAttachedPropertiesObject<QDeclarative1ListView>(item));
        if (attached)
            attached->setView(view);
    }
    ~FxListItem1() {}
    qreal position() const {
        if (section) {
            if (view->orientation() == QDeclarative1ListView::Vertical)
                return section->y();
            else
                return (view->effectiveLayoutDirection() == Qt::RightToLeft ? -section->width()-section->x() : section->x());
        } else {
            return itemPosition();
        }
    }

    qreal itemPosition() const {
        if (view->orientation() == QDeclarative1ListView::Vertical)
            return item->y();
        else
            return (view->effectiveLayoutDirection() == Qt::RightToLeft ? -item->width()-item->x() : item->x());
    }
    qreal size() const {
        if (section)
            return (view->orientation() == QDeclarative1ListView::Vertical ? item->height()+section->height() : item->width()+section->width());
        else
            return (view->orientation() == QDeclarative1ListView::Vertical ? item->height() : item->width());
    }
    qreal itemSize() const {
        return (view->orientation() == QDeclarative1ListView::Vertical ? item->height() : item->width());
    }
    qreal sectionSize() const {
        if (section)
            return (view->orientation() == QDeclarative1ListView::Vertical ? section->height() : section->width());
        return 0.0;
    }
    qreal endPosition() const {
        if (view->orientation() == QDeclarative1ListView::Vertical) {
            return item->y() + (item->height() >= 1.0 ? item->height() : 1) - 1;
        } else {
            return (view->effectiveLayoutDirection() == Qt::RightToLeft
                    ? -item->width()-item->x() + (item->width() >= 1.0 ? item->width() : 1)
                    : item->x() + (item->width() >= 1.0 ? item->width() : 1)) - 1;
        }
    }
    void setPosition(qreal pos) {
        if (view->orientation() == QDeclarative1ListView::Vertical) {
            if (section) {
                section->setY(pos);
                pos += section->height();
            }
            item->setY(pos);
        } else {
            if (view->effectiveLayoutDirection() == Qt::RightToLeft) {
                if (section) {
                    section->setX(-section->width()-pos);
                    pos += section->width();
                }
                item->setX(-item->width()-pos);
            } else {
                if (section) {
                    section->setX(pos);
                    pos += section->width();
                }
                item->setX(pos);
            }
        }
    }
    void setSize(qreal size) {
        if (view->orientation() == QDeclarative1ListView::Vertical)
            item->setHeight(size);
        else
            item->setWidth(size);
    }
    bool contains(qreal x, qreal y) const {
        return (x >= item->x() && x < item->x() + item->width() &&
                y >= item->y() && y < item->y() + item->height());
    }

    QDeclarativeItem *item;
    QDeclarativeItem *section;
    QDeclarative1ListView *view;
    QDeclarative1ListViewAttached *attached;
    int index;
};

//----------------------------------------------------------------------------

class QDeclarative1ListViewPrivate : public QDeclarative1FlickablePrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1ListView)

public:
    QDeclarative1ListViewPrivate()
        : currentItem(0), orient(QDeclarative1ListView::Vertical), layoutDirection(Qt::LeftToRight)
        , visiblePos(0), visibleIndex(0)
        , averageSize(100.0), currentIndex(-1), requestedIndex(-1)
        , itemCount(0), highlightRangeStart(0), highlightRangeEnd(0)
        , highlightRangeStartValid(false), highlightRangeEndValid(false)
        , highlightComponent(0), highlight(0), trackedItem(0)
        , moveReason(Other), buffer(0), highlightPosAnimator(0), highlightSizeAnimator(0)
        , sectionCriteria(0), spacing(0.0)
        , highlightMoveSpeed(400), highlightMoveDuration(-1)
        , highlightResizeSpeed(400), highlightResizeDuration(-1), highlightRange(QDeclarative1ListView::NoHighlightRange)
        , snapMode(QDeclarative1ListView::NoSnap), overshootDist(0.0)
        , footerComponent(0), footer(0), headerComponent(0), header(0)
        , bufferMode(BufferBefore | BufferAfter)
        , ownModel(false), wrap(false), autoHighlight(true), haveHighlightRange(false)
        , correctFlick(false), inFlickCorrection(false), lazyRelease(false)
        , deferredRelease(false), layoutScheduled(false), currentIndexCleared(false)
        , inViewportMoved(false)
        , minExtentDirty(true), maxExtentDirty(true)
    {}

    void init();
    void clear();
    FxListItem1 *createItem(int modelIndex);
    void releaseItem(FxListItem1 *item);

    FxListItem1 *visibleItem(int modelIndex) const {
        if (modelIndex >= visibleIndex && modelIndex < visibleIndex + visibleItems.count()) {
            for (int i = modelIndex - visibleIndex; i < visibleItems.count(); ++i) {
                FxListItem1 *item = visibleItems.at(i);
                if (item->index == modelIndex)
                    return item;
            }
        }
        return 0;
    }

    FxListItem1 *firstVisibleItem() const {
        const qreal pos = isRightToLeft() ? -position()-size() : position();
        for (int i = 0; i < visibleItems.count(); ++i) {
            FxListItem1 *item = visibleItems.at(i);
            if (item->index != -1 && item->endPosition() > pos)
                return item;
        }
        return visibleItems.count() ? visibleItems.first() : 0;
    }

    // Returns the item before modelIndex, if created.
    // May return an item marked for removal.
    FxListItem1 *itemBefore(int modelIndex) const {
        if (modelIndex < visibleIndex)
            return 0;
        int idx = 1;
        int lastIndex = -1;
        while (idx < visibleItems.count()) {
            FxListItem1 *item = visibleItems.at(idx);
            if (item->index != -1)
                lastIndex = item->index;
            if (item->index == modelIndex)
                return visibleItems.at(idx-1);
            ++idx;
        }
        if (lastIndex == modelIndex-1)
            return visibleItems.last();
        return 0;
    }

    void regenerate() {
        Q_Q(QDeclarative1ListView);
        if (q->isComponentComplete()) {
            if (header) {
                if (q->scene())
                    q->scene()->removeItem(header->item);
                header->item->deleteLater();
                delete header;
                header = 0;
            }
            if (footer) {
                if (q->scene())
                    q->scene()->removeItem(footer->item);
                footer->item->deleteLater();
                delete footer;
                footer = 0;
            }
            updateHeader();
            updateFooter();
            clear();
            setPosition(0);
            q->refill();
            updateCurrent(currentIndex);
        }
    }

    void mirrorChange() {
        Q_Q(QDeclarative1ListView);
        regenerate();
        emit q->effectiveLayoutDirectionChanged();
    }

    bool isRightToLeft() const {
        Q_Q(const QDeclarative1ListView);
        return orient == QDeclarative1ListView::Horizontal && q->effectiveLayoutDirection() == Qt::RightToLeft;
    }

    qreal position() const {
        Q_Q(const QDeclarative1ListView);
        return orient == QDeclarative1ListView::Vertical ? q->contentY() : q->contentX();
    }

    void setPosition(qreal pos) {
        Q_Q(QDeclarative1ListView);
        if (orient == QDeclarative1ListView::Vertical) {
            q->QDeclarative1Flickable::setContentY(pos);
        } else {
            if (isRightToLeft())
                q->QDeclarative1Flickable::setContentX(-pos-size());
            else
                q->QDeclarative1Flickable::setContentX(pos);
        }
    }
    qreal size() const {
        Q_Q(const QDeclarative1ListView);
        return orient == QDeclarative1ListView::Vertical ? q->height() : q->width();
    }

    qreal originPosition() const {
        qreal pos = 0;
        if (!visibleItems.isEmpty()) {
            pos = (*visibleItems.constBegin())->position();
            if (visibleIndex > 0)
                pos -= visibleIndex * (averageSize + spacing);
        }
        return pos;
    }

    qreal lastPosition() const {
        qreal pos = 0;
        if (!visibleItems.isEmpty()) {
            int invisibleCount = visibleItems.count() - visibleIndex;
            for (int i = visibleItems.count()-1; i >= 0; --i) {
                if (visibleItems.at(i)->index != -1) {
                    invisibleCount = model->count() - visibleItems.at(i)->index - 1;
                    break;
                }
            }
            pos = (*(--visibleItems.constEnd()))->endPosition() + invisibleCount * (averageSize + spacing);
        } else if (model && model->count()) {
            pos = model->count() * averageSize + (model->count()-1) * spacing;
        }
        return pos;
    }

    qreal startPosition() const {
        return isRightToLeft() ? -lastPosition()-1 : originPosition();
    }

    qreal endPosition() const {
        return isRightToLeft() ? -originPosition()-1 : lastPosition();
    }

    qreal positionAt(int modelIndex) const {
        if (FxListItem1 *item = visibleItem(modelIndex))
            return item->position();
        if (!visibleItems.isEmpty()) {
            if (modelIndex < visibleIndex) {
                int count = visibleIndex - modelIndex;
                qreal cs = 0;
                if (modelIndex == currentIndex && currentItem) {
                    cs = currentItem->size() + spacing;
                    --count;
                }
                return (*visibleItems.constBegin())->position() - count * (averageSize + spacing) - cs;
            } else {
                int idx = visibleItems.count() - 1;
                while (idx >= 0 && visibleItems.at(idx)->index == -1)
                    --idx;
                if (idx < 0)
                    idx = visibleIndex;
                else
                    idx = visibleItems.at(idx)->index;
                int count = modelIndex - idx - 1;

                return (*(--visibleItems.constEnd()))->endPosition() + spacing + count * (averageSize + spacing) + 1;
            }
        }
        return 0;
    }

    qreal endPositionAt(int modelIndex) const {
        if (FxListItem1 *item = visibleItem(modelIndex))
            return item->endPosition();
        if (!visibleItems.isEmpty()) {
            if (modelIndex < visibleIndex) {
                int count = visibleIndex - modelIndex;
                return (*visibleItems.constBegin())->position() - (count - 1) * (averageSize + spacing) - spacing - 1;
            } else {
                int idx = visibleItems.count() - 1;
                while (idx >= 0 && visibleItems.at(idx)->index == -1)
                    --idx;
                if (idx < 0)
                    idx = visibleIndex;
                else
                    idx = visibleItems.at(idx)->index;
                int count = modelIndex - idx - 1;
                return (*(--visibleItems.constEnd()))->endPosition() + count * (averageSize + spacing);
            }
        }
        return 0;
    }

    QString sectionAt(int modelIndex) {
        if (FxListItem1 *item = visibleItem(modelIndex))
            return item->attached->section();

        QString section;
        if (sectionCriteria) {
            QString propValue = model->stringValue(modelIndex, sectionCriteria->property());
            section = sectionCriteria->sectionString(propValue);
        }

        return section;
    }

    bool isValid() const {
        return model && model->count() && model->isValid();
    }

    qreal snapPosAt(qreal pos) {
        if (FxListItem1 *snapItem = snapItemAt(pos))
            return snapItem->position();
        if (visibleItems.count()) {
            qreal firstPos = visibleItems.first()->position();
            qreal endPos = visibleItems.last()->position();
            if (pos < firstPos) {
                return firstPos - qRound((firstPos - pos) / averageSize) * averageSize;
            } else if (pos > endPos)
                return endPos + qRound((pos - endPos) / averageSize) * averageSize;
        }
        return qRound((pos - originPosition()) / averageSize) * averageSize + originPosition();
    }

    FxListItem1 *snapItemAt(qreal pos) {
        FxListItem1 *snapItem = 0;
        qreal prevItemSize = 0;
        for (int i = 0; i < visibleItems.count(); ++i) {
            FxListItem1 *item = visibleItems[i];
            if (item->index == -1)
                continue;
            qreal itemTop = item->position();
            if (highlight && itemTop >= pos && item->endPosition() <= pos + highlight->size() - 1)
                return item;
            if (itemTop+item->size()/2 >= pos && itemTop-prevItemSize/2 < pos)
                snapItem = item;
            prevItemSize = item->size();
        }
        return snapItem;
    }

    int lastVisibleIndex() const {
        int lastIndex = -1;
        for (int i = visibleItems.count()-1; i >= 0; --i) {
            FxListItem1 *listItem = visibleItems.at(i);
            if (listItem->index != -1) {
                lastIndex = listItem->index;
                break;
            }
        }
        return lastIndex;
    }

    // map a model index to visibleItems index.
    int mapFromModel(int modelIndex) const {
        if (modelIndex < visibleIndex || modelIndex >= visibleIndex + visibleItems.count())
            return -1;
        for (int i = 0; i < visibleItems.count(); ++i) {
            FxListItem1 *listItem = visibleItems.at(i);
            if (listItem->index == modelIndex)
                return i;
            if (listItem->index > modelIndex)
                return -1;
        }
        return -1; // Not in visibleList
    }

    void updateViewport() {
        Q_Q(QDeclarative1ListView);
        if (orient == QDeclarative1ListView::Vertical) {
            q->setContentHeight(endPosition() - startPosition() + 1);
        } else {
            q->setContentWidth(endPosition() - startPosition() + 1);
        }
    }

    void itemGeometryChanged(QDeclarativeItem *item, const QRectF &newGeometry, const QRectF &oldGeometry) {
        Q_Q(QDeclarative1ListView);
        QDeclarative1FlickablePrivate::itemGeometryChanged(item, newGeometry, oldGeometry);
        if (!q->isComponentComplete())
            return;
        if (item != contentItem && (!highlight || item != highlight->item)) {
            if ((orient == QDeclarative1ListView::Vertical && newGeometry.height() != oldGeometry.height())
                || (orient == QDeclarative1ListView::Horizontal && newGeometry.width() != oldGeometry.width())) {
                scheduleLayout();
            }
        }
        if ((header && header->item == item) || (footer && footer->item == item)) {
            if (header)
                updateHeader();
            if (footer)
                updateFooter();
        }
        if (currentItem && currentItem->item == item)
            updateHighlight();
        if (trackedItem && trackedItem->item == item)
            q->trackedPositionChanged();
    }

    // for debugging only
    void checkVisible() const {
        int skip = 0;
        for (int i = 0; i < visibleItems.count(); ++i) {
            FxListItem1 *listItem = visibleItems.at(i);
            if (listItem->index == -1) {
                ++skip;
            } else if (listItem->index != visibleIndex + i - skip) {
                qFatal("index %d %d %d", visibleIndex, i, listItem->index);
            }
        }
    }

    void refill(qreal from, qreal to, bool doBuffer = false);
    void scheduleLayout();
    void layout();
    void updateUnrequestedIndexes();
    void updateUnrequestedPositions();
    void updateTrackedItem();
    void createHighlight();
    void updateHighlight();
    void createSection(FxListItem1 *);
    void updateSections();
    void updateCurrentSection();
    void updateCurrent(int);
    void updateAverage();
    void updateHeader();
    void updateFooter();
    void fixupPosition();
    void positionViewAtIndex(int index, int mode);
    virtual void fixup(AxisData &data, qreal minExtent, qreal maxExtent);
    virtual void flick(QDeclarative1FlickablePrivate::AxisData &data, qreal minExtent, qreal maxExtent, qreal vSize,
                        QDeclarative1TimeLineCallback::Callback fixupCallback, qreal velocity);

    QDeclarativeGuard<QDeclarative1VisualModel> model;
    QVariant modelVariant;
    QList<FxListItem1*> visibleItems;
    QHash<QDeclarativeItem*,int> unrequestedItems;
    FxListItem1 *currentItem;
    QDeclarative1ListView::Orientation orient;
    Qt::LayoutDirection layoutDirection;
    qreal visiblePos;
    int visibleIndex;
    qreal averageSize;
    int currentIndex;
    int requestedIndex;
    int itemCount;
    qreal highlightRangeStart;
    qreal highlightRangeEnd;
    bool highlightRangeStartValid;
    bool highlightRangeEndValid;
    QDeclarativeComponent *highlightComponent;
    FxListItem1 *highlight;
    FxListItem1 *trackedItem;
    enum MovementReason { Other, SetIndex, Mouse };
    MovementReason moveReason;
    int buffer;
    QSmoothedAnimation_1 *highlightPosAnimator;
    QSmoothedAnimation_1 *highlightSizeAnimator;
    QDeclarative1ViewSection *sectionCriteria;
    QString currentSection;
    static const int sectionCacheSize = 4;
    QDeclarativeItem *sectionCache[sectionCacheSize];
    qreal spacing;
    qreal highlightMoveSpeed;
    int highlightMoveDuration;
    qreal highlightResizeSpeed;
    int highlightResizeDuration;
    QDeclarative1ListView::HighlightRangeMode highlightRange;
    QDeclarative1ListView::SnapMode snapMode;
    qreal overshootDist;
    QDeclarativeComponent *footerComponent;
    FxListItem1 *footer;
    QDeclarativeComponent *headerComponent;
    FxListItem1 *header;
    enum BufferMode { NoBuffer = 0x00, BufferBefore = 0x01, BufferAfter = 0x02 };
    int bufferMode;
    mutable qreal minExtent;
    mutable qreal maxExtent;

    bool ownModel : 1;
    bool wrap : 1;
    bool autoHighlight : 1;
    bool haveHighlightRange : 1;
    bool correctFlick : 1;
    bool inFlickCorrection : 1;
    bool lazyRelease : 1;
    bool deferredRelease : 1;
    bool layoutScheduled : 1;
    bool currentIndexCleared : 1;
    bool inViewportMoved : 1;
    mutable bool minExtentDirty : 1;
    mutable bool maxExtentDirty : 1;
};

void QDeclarative1ListViewPrivate::init()
{
    Q_Q(QDeclarative1ListView);
    q->setFlag(QGraphicsItem::ItemIsFocusScope);
    addItemChangeListener(this, Geometry);
    QObject::connect(q, SIGNAL(movementEnded()), q, SLOT(animStopped()));
    q->setFlickableDirection(QDeclarative1Flickable::VerticalFlick);
    ::memset(sectionCache, 0, sizeof(QDeclarativeItem*) * sectionCacheSize);
}

void QDeclarative1ListViewPrivate::clear()
{
    timeline.clear();
    for (int i = 0; i < visibleItems.count(); ++i)
        releaseItem(visibleItems.at(i));
    visibleItems.clear();
    for (int i = 0; i < sectionCacheSize; ++i) {
        delete sectionCache[i];
        sectionCache[i] = 0;
    }
    visiblePos = header ? header->size() : 0;
    visibleIndex = 0;
    releaseItem(currentItem);
    currentItem = 0;
    createHighlight();
    trackedItem = 0;
    minExtentDirty = true;
    maxExtentDirty = true;
    itemCount = 0;
}

FxListItem1 *QDeclarative1ListViewPrivate::createItem(int modelIndex)
{
    Q_Q(QDeclarative1ListView);
    // create object
    requestedIndex = modelIndex;
    FxListItem1 *listItem = 0;
    if (QDeclarativeItem *item = model->item(modelIndex, false)) {
        listItem = new FxListItem1(item, q);
        listItem->index = modelIndex;
        // initialise attached properties
        if (sectionCriteria) {
            QString propValue = model->stringValue(modelIndex, sectionCriteria->property());
            listItem->attached->m_section = sectionCriteria->sectionString(propValue);
            if (modelIndex > 0) {
                if (FxListItem1 *item = itemBefore(modelIndex))
                    listItem->attached->m_prevSection = item->attached->section();
                else
                    listItem->attached->m_prevSection = sectionAt(modelIndex-1);
            }
            if (modelIndex < model->count()-1) {
                if (FxListItem1 *item = visibleItem(modelIndex+1))
                    listItem->attached->m_nextSection = item->attached->section();
                else
                    listItem->attached->m_nextSection = sectionAt(modelIndex+1);
            }
        }
        if (model->completePending()) {
            // complete
            listItem->item->setZValue(1);
            listItem->item->setParentItem(q->contentItem());
            model->completeItem();
        } else {
            listItem->item->setParentItem(q->contentItem());
        }
        QDeclarativeItemPrivate *itemPrivate = static_cast<QDeclarativeItemPrivate*>(QGraphicsItemPrivate::get(item));
        itemPrivate->addItemChangeListener(this, QDeclarativeItemPrivate::Geometry);
        if (sectionCriteria && sectionCriteria->delegate()) {
            if (listItem->attached->m_prevSection != listItem->attached->m_section)
                createSection(listItem);
        }
        unrequestedItems.remove(listItem->item);
    }
    requestedIndex = -1;

    return listItem;
}

void QDeclarative1ListViewPrivate::releaseItem(FxListItem1 *item)
{
    Q_Q(QDeclarative1ListView);
    if (!item || !model)
        return;
    if (trackedItem == item)
        trackedItem = 0;
    QDeclarativeItemPrivate *itemPrivate = static_cast<QDeclarativeItemPrivate*>(QGraphicsItemPrivate::get(item->item));
    itemPrivate->removeItemChangeListener(this, QDeclarativeItemPrivate::Geometry);
    if (model->release(item->item) == 0) {
        // item was not destroyed, and we no longer reference it.
        unrequestedItems.insert(item->item, model->indexOf(item->item, q));
    }
    if (item->section) {
        int i = 0;
        do {
            if (!sectionCache[i]) {
                sectionCache[i] = item->section;
                sectionCache[i]->setVisible(false);
                item->section = 0;
                break;
            }
            ++i;
        } while (i < sectionCacheSize);
        delete item->section;
    }
    delete item;
}

void QDeclarative1ListViewPrivate::refill(qreal from, qreal to, bool doBuffer)
{
    Q_Q(QDeclarative1ListView);
    if (!isValid() || !q->isComponentComplete())
        return;
    itemCount = model->count();
    qreal bufferFrom = from - buffer;
    qreal bufferTo = to + buffer;
    qreal fillFrom = from;
    qreal fillTo = to;
    if (doBuffer && (bufferMode & BufferAfter))
        fillTo = bufferTo;
    if (doBuffer && (bufferMode & BufferBefore))
        fillFrom = bufferFrom;

    bool haveValidItems = false;
    int modelIndex = visibleIndex;
    qreal itemEnd = visiblePos-1;
    if (!visibleItems.isEmpty()) {
        visiblePos = (*visibleItems.constBegin())->position();
        itemEnd = (*(--visibleItems.constEnd()))->endPosition() + spacing;
        int i = visibleItems.count() - 1;
        while (i > 0 && visibleItems.at(i)->index == -1)
            --i;
        if (visibleItems.at(i)->index != -1) {
            haveValidItems = true;
            modelIndex = visibleItems.at(i)->index + 1;
        }
    }

    if (haveValidItems && (fillFrom > itemEnd+averageSize+spacing
        || fillTo < visiblePos - averageSize - spacing)) {
        // We've jumped more than a page.  Estimate which items are now
        // visible and fill from there.
        int count = (fillFrom - itemEnd) / (averageSize + spacing);
        for (int i = 0; i < visibleItems.count(); ++i)
            releaseItem(visibleItems.at(i));
        visibleItems.clear();
        modelIndex += count;
        if (modelIndex >= model->count()) {
            count -= modelIndex - model->count() + 1;
            modelIndex = model->count() - 1;
        } else if (modelIndex < 0) {
            count -= modelIndex;
            modelIndex = 0;
        }
        visibleIndex = modelIndex;
        visiblePos = itemEnd + count * (averageSize + spacing) + 1;
        itemEnd = visiblePos-1;
    }

    bool changed = false;
    FxListItem1 *item = 0;
    qreal pos = itemEnd + 1;
    while (modelIndex < model->count() && pos <= fillTo) {
//        qDebug() << "refill: append item" << modelIndex << "pos" << pos;
        if (!(item = createItem(modelIndex)))
            break;
        item->setPosition(pos);
        pos += item->size() + spacing;
        visibleItems.append(item);
        ++modelIndex;
        changed = true;
        if (doBuffer) // never buffer more than one item per frame
            break;
    }
    while (visibleIndex > 0 && visibleIndex <= model->count() && visiblePos-1 >= fillFrom) {
//        qDebug() << "refill: prepend item" << visibleIndex-1 << "current top pos" << visiblePos;
        if (!(item = createItem(visibleIndex-1)))
            break;
        --visibleIndex;
        visiblePos -= item->size() + spacing;
        item->setPosition(visiblePos);
        visibleItems.prepend(item);
        changed = true;
        if (doBuffer) // never buffer more than one item per frame
            break;
    }

    if (!lazyRelease || !changed || deferredRelease) { // avoid destroying items in the same frame that we create
        while (visibleItems.count() > 1 && (item = visibleItems.first()) && item->endPosition() < bufferFrom) {
            if (item->attached->delayRemove())
                break;
//            qDebug() << "refill: remove first" << visibleIndex << "top end pos" << item->endPosition();
            if (item->index != -1)
                visibleIndex++;
            visibleItems.removeFirst();
            releaseItem(item);
            changed = true;
        }
        while (visibleItems.count() > 1 && (item = visibleItems.last()) && item->position() > bufferTo) {
            if (item->attached->delayRemove())
                break;
//            qDebug() << "refill: remove last" << visibleIndex+visibleItems.count()-1 << item->position();
            visibleItems.removeLast();
            releaseItem(item);
            changed = true;
        }
        deferredRelease = false;
    } else {
        deferredRelease = true;
    }
    if (changed) {
        minExtentDirty = true;
        maxExtentDirty = true;
        if (visibleItems.count())
            visiblePos = (*visibleItems.constBegin())->position();
        updateAverage();
        if (currentIndex >= 0 && currentItem && !visibleItem(currentIndex)) {
            currentItem->setPosition(positionAt(currentIndex));
            updateHighlight();
        }

        if (sectionCriteria)
            updateCurrentSection();
        if (header)
            updateHeader();
        if (footer)
            updateFooter();
        updateViewport();
        updateUnrequestedPositions();
    } else if (!doBuffer && buffer && bufferMode != NoBuffer) {
        refill(from, to, true);
    }
    lazyRelease = false;
}

void QDeclarative1ListViewPrivate::scheduleLayout()
{
    Q_Q(QDeclarative1ListView);
    if (!layoutScheduled) {
        layoutScheduled = true;
        QCoreApplication::postEvent(q, new QEvent(QEvent::User), Qt::HighEventPriority);
    }
}

void QDeclarative1ListViewPrivate::layout()
{
    Q_Q(QDeclarative1ListView);
    layoutScheduled = false;
    if (!isValid() && !visibleItems.count()) {
        clear();
        setPosition(0);
        return;
    }
    if (!visibleItems.isEmpty()) {
        bool fixedCurrent = currentItem && visibleItems.first()->item == currentItem->item;
        qreal sum = visibleItems.first()->size();
        qreal pos = visibleItems.first()->position() + visibleItems.first()->size() + spacing;
        for (int i=1; i < visibleItems.count(); ++i) {
            FxListItem1 *item = visibleItems.at(i);
            item->setPosition(pos);
            pos += item->size() + spacing;
            sum += item->size();
            fixedCurrent = fixedCurrent || (currentItem && item->item == currentItem->item);
        }
        averageSize = qRound(sum / visibleItems.count());
        // move current item if it is not a visible item.
        if (currentIndex >= 0 && currentItem && !fixedCurrent)
            currentItem->setPosition(positionAt(currentIndex));
    }
    q->refill();
    minExtentDirty = true;
    maxExtentDirty = true;
    updateHighlight();
    if (!q->isMoving() && !q->isFlicking()) {
        fixupPosition();
        q->refill();
    }
    if (header)
        updateHeader();
    if (footer)
        updateFooter();
    updateViewport();
}

void QDeclarative1ListViewPrivate::updateUnrequestedIndexes()
{
    Q_Q(QDeclarative1ListView);
    QHash<QDeclarativeItem*,int>::iterator it;
    for (it = unrequestedItems.begin(); it != unrequestedItems.end(); ++it)
        *it = model->indexOf(it.key(), q);
}

void QDeclarative1ListViewPrivate::updateUnrequestedPositions()
{
    Q_Q(QDeclarative1ListView);
    if (unrequestedItems.count()) {
        qreal pos = position();
        QHash<QDeclarativeItem*,int>::const_iterator it;
        for (it = unrequestedItems.begin(); it != unrequestedItems.end(); ++it) {
            QDeclarativeItem *item = it.key();
            if (orient == QDeclarative1ListView::Vertical) {
                if (item->y() + item->height() > pos && item->y() < pos + q->height())
                    item->setY(positionAt(*it));
            } else {
                if (item->x() + item->width() > pos && item->x() < pos + q->width()) {
                    if (isRightToLeft())
                        item->setX(-positionAt(*it)-item->width());
                    else
                        item->setX(positionAt(*it));
                }
            }
        }
    }
}

void QDeclarative1ListViewPrivate::updateTrackedItem()
{
    Q_Q(QDeclarative1ListView);
    FxListItem1 *item = currentItem;
    if (highlight)
        item = highlight;
    trackedItem = item;
    if (trackedItem)
        q->trackedPositionChanged();
}

void QDeclarative1ListViewPrivate::createHighlight()
{
    Q_Q(QDeclarative1ListView);
    bool changed = false;
    if (highlight) {
        if (trackedItem == highlight)
            trackedItem = 0;
        if (highlight->item->scene())
            highlight->item->scene()->removeItem(highlight->item);
        highlight->item->deleteLater();
        delete highlight;
        highlight = 0;
        delete highlightPosAnimator;
        delete highlightSizeAnimator;
        highlightPosAnimator = 0;
        highlightSizeAnimator = 0;
        changed = true;
    }

    if (currentItem) {
        QDeclarativeItem *item = 0;
        if (highlightComponent) {
            QDeclarativeContext *highlightContext = new QDeclarativeContext(qmlContext(q));
            QObject *nobj = highlightComponent->create(highlightContext);
            if (nobj) {
                QDeclarative_setParent_noEvent(highlightContext, nobj);
                item = qobject_cast<QDeclarativeItem *>(nobj);
                if (!item)
                    delete nobj;
            } else {
                delete highlightContext;
            }
        } else {
            item = new QDeclarativeItem;
        }
        if (item) {
            QDeclarative_setParent_noEvent(item, q->contentItem());
            item->setParentItem(q->contentItem());
            highlight = new FxListItem1(item, q);
            if (currentItem && autoHighlight) {
                if (orient == QDeclarative1ListView::Vertical) {
                    highlight->item->setHeight(currentItem->item->height());
                } else {
                    highlight->item->setWidth(currentItem->item->width());
                }
                highlight->setPosition(currentItem->itemPosition());
            }
            QDeclarativeItemPrivate *itemPrivate = static_cast<QDeclarativeItemPrivate*>(QGraphicsItemPrivate::get(item));
            itemPrivate->addItemChangeListener(this, QDeclarativeItemPrivate::Geometry);
            const QLatin1String posProp(orient == QDeclarative1ListView::Vertical ? "y" : "x");
            highlightPosAnimator = new QSmoothedAnimation_1(q);
            highlightPosAnimator->target = QDeclarativeProperty(highlight->item, posProp);
            highlightPosAnimator->velocity = highlightMoveSpeed;
            highlightPosAnimator->userDuration = highlightMoveDuration;
            const QLatin1String sizeProp(orient == QDeclarative1ListView::Vertical ? "height" : "width");
            highlightSizeAnimator = new QSmoothedAnimation_1(q);
            highlightSizeAnimator->velocity = highlightResizeSpeed;
            highlightSizeAnimator->userDuration = highlightResizeDuration;
            highlightSizeAnimator->target = QDeclarativeProperty(highlight->item, sizeProp);
            if (autoHighlight) {
                highlightPosAnimator->restart();
                highlightSizeAnimator->restart();
            }
            changed = true;
        }
    }
    if (changed)
        emit q->highlightItemChanged();
}

void QDeclarative1ListViewPrivate::updateHighlight()
{
    if ((!currentItem && highlight) || (currentItem && !highlight))
        createHighlight();
    if (currentItem && autoHighlight && highlight && !hData.moving && !vData.moving) {
        // auto-update highlight
        highlightPosAnimator->to = isRightToLeft()
                ? -currentItem->itemPosition()-currentItem->itemSize()
                : currentItem->itemPosition();
        highlightSizeAnimator->to = currentItem->itemSize();
        if (orient == QDeclarative1ListView::Vertical) {
            if (highlight->item->width() == 0)
                highlight->item->setWidth(currentItem->item->width());
        } else {
            if (highlight->item->height() == 0)
                highlight->item->setHeight(currentItem->item->height());
        }
        highlightPosAnimator->restart();
        highlightSizeAnimator->restart();
    }
    updateTrackedItem();
}

void QDeclarative1ListViewPrivate::createSection(FxListItem1 *listItem)
{
    Q_Q(QDeclarative1ListView);
    if (!sectionCriteria || !sectionCriteria->delegate())
        return;
    if (listItem->attached->m_prevSection != listItem->attached->m_section) {
        if (!listItem->section) {
            qreal pos = listItem->position();
            int i = sectionCacheSize-1;
            while (i >= 0 && !sectionCache[i])
                --i;
            if (i >= 0) {
                listItem->section = sectionCache[i];
                sectionCache[i] = 0;
                listItem->section->setVisible(true);
                QDeclarativeContext *context = QDeclarativeEngine::contextForObject(listItem->section)->parentContext();
                context->setContextProperty(QLatin1String("section"), listItem->attached->m_section);
            } else {
                QDeclarativeContext *context = new QDeclarativeContext(qmlContext(q));
                context->setContextProperty(QLatin1String("section"), listItem->attached->m_section);
                QObject *nobj = sectionCriteria->delegate()->beginCreate(context);
                if (nobj) {
                    QDeclarative_setParent_noEvent(context, nobj);
                    listItem->section = qobject_cast<QDeclarativeItem *>(nobj);
                    if (!listItem->section) {
                        delete nobj;
                    } else {
                        listItem->section->setZValue(1);
                        QDeclarative_setParent_noEvent(listItem->section, q->contentItem());
                        listItem->section->setParentItem(q->contentItem());
                    }
                } else {
                    delete context;
                }
                sectionCriteria->delegate()->completeCreate();
            }
            listItem->setPosition(pos);
        } else {
            QDeclarativeContext *context = QDeclarativeEngine::contextForObject(listItem->section)->parentContext();
            context->setContextProperty(QLatin1String("section"), listItem->attached->m_section);
        }
    } else if (listItem->section) {
        qreal pos = listItem->position();
        int i = 0;
        do {
            if (!sectionCache[i]) {
                sectionCache[i] = listItem->section;
                sectionCache[i]->setVisible(false);
                listItem->section = 0;
                return;
            }
            ++i;
        } while (i < sectionCacheSize);
        delete listItem->section;
        listItem->section = 0;
        listItem->setPosition(pos);
    }
}

void QDeclarative1ListViewPrivate::updateSections()
{
    if (sectionCriteria && !visibleItems.isEmpty()) {
        QString prevSection;
        if (visibleIndex > 0)
            prevSection = sectionAt(visibleIndex-1);
        QDeclarative1ListViewAttached *prevAtt = 0;
        int idx = -1;
        for (int i = 0; i < visibleItems.count(); ++i) {
            QDeclarative1ListViewAttached *attached = visibleItems.at(i)->attached;
            attached->setPrevSection(prevSection);
            if (visibleItems.at(i)->index != -1) {
                QString propValue = model->stringValue(visibleItems.at(i)->index, sectionCriteria->property());
                attached->setSection(sectionCriteria->sectionString(propValue));
                idx = visibleItems.at(i)->index;
            }
            createSection(visibleItems.at(i));
            if (prevAtt)
                prevAtt->setNextSection(attached->section());
            prevSection = attached->section();
            prevAtt = attached;
        }
        if (prevAtt) {
            if (idx > 0 && idx < model->count()-1)
                prevAtt->setNextSection(sectionAt(idx+1));
            else
                prevAtt->setNextSection(QString());
        }
    }
}

void QDeclarative1ListViewPrivate::updateCurrentSection()
{
    Q_Q(QDeclarative1ListView);
    if (!sectionCriteria || visibleItems.isEmpty()) {
        if (!currentSection.isEmpty()) {
            currentSection.clear();
            emit q->currentSectionChanged();
        }
        return;
    }
    int index = 0;
    while (index < visibleItems.count() && visibleItems.at(index)->endPosition() < position())
        ++index;

    QString newSection = currentSection;
    if (index < visibleItems.count())
        newSection = visibleItems.at(index)->attached->section();
    else
        newSection = visibleItems.first()->attached->section();
    if (newSection != currentSection) {
        currentSection = newSection;
        emit q->currentSectionChanged();
    }
}

void QDeclarative1ListViewPrivate::updateCurrent(int modelIndex)
{
    Q_Q(QDeclarative1ListView);
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
    FxListItem1 *oldCurrentItem = currentItem;
    currentIndex = modelIndex;
    currentItem = createItem(modelIndex);
    if (oldCurrentItem && (!currentItem || oldCurrentItem->item != currentItem->item))
        oldCurrentItem->attached->setIsCurrentItem(false);
    if (currentItem) {
        if (modelIndex == visibleIndex - 1 && visibleItems.count()) {
            // We can calculate exact postion in this case
            currentItem->setPosition(visibleItems.first()->position() - currentItem->size() - spacing);
        } else {
            // Create current item now and position as best we can.
            // Its position will be corrected when it becomes visible.
            currentItem->setPosition(positionAt(modelIndex));
        }
        currentItem->item->setFocus(true);
        currentItem->attached->setIsCurrentItem(true);
        // Avoid showing section delegate twice.  We still need the section heading so that
        // currentItem positioning works correctly.
        // This is slightly sub-optimal, but section heading caching minimizes the impact.
        if (currentItem->section)
            currentItem->section->setVisible(false);
        if (visibleItems.isEmpty())
            averageSize = currentItem->size();
    }
    updateHighlight();
    emit q->currentIndexChanged();
    // Release the old current item
    releaseItem(oldCurrentItem);
}

void QDeclarative1ListViewPrivate::updateAverage()
{
    if (!visibleItems.count())
        return;
    qreal sum = 0.0;
    for (int i = 0; i < visibleItems.count(); ++i)
        sum += visibleItems.at(i)->size();
    averageSize = qRound(sum / visibleItems.count());
}

void QDeclarative1ListViewPrivate::updateFooter()
{
    Q_Q(QDeclarative1ListView);
    if (!footer && footerComponent) {
        QDeclarativeItem *item = 0;
        QDeclarativeContext *context = new QDeclarativeContext(qmlContext(q));
        QObject *nobj = footerComponent->create(context);
        if (nobj) {
            QDeclarative_setParent_noEvent(context, nobj);
            item = qobject_cast<QDeclarativeItem *>(nobj);
            if (!item)
                delete nobj;
        } else {
            delete context;
        }
        if (item) {
            QDeclarative_setParent_noEvent(item, q->contentItem());
            item->setParentItem(q->contentItem());
            item->setZValue(1);
            QDeclarativeItemPrivate *itemPrivate = static_cast<QDeclarativeItemPrivate*>(QGraphicsItemPrivate::get(item));
            itemPrivate->addItemChangeListener(this, QDeclarativeItemPrivate::Geometry);
            footer = new FxListItem1(item, q);
        }
    }
    if (footer) {
        if (visibleItems.count()) {
            qreal endPos = lastPosition() + 1;
            if (lastVisibleIndex() == model->count()-1) {
                footer->setPosition(endPos);
            } else {
                qreal visiblePos = position() + q->height();
                if (endPos <= visiblePos || footer->position() < endPos)
                    footer->setPosition(endPos);
            }
        } else {
            footer->setPosition(visiblePos);
        }
    }
}

void QDeclarative1ListViewPrivate::updateHeader()
{
    Q_Q(QDeclarative1ListView);
    if (!header && headerComponent) {
        QDeclarativeItem *item = 0;
        QDeclarativeContext *context = new QDeclarativeContext(qmlContext(q));
        QObject *nobj = headerComponent->create(context);
        if (nobj) {
            QDeclarative_setParent_noEvent(context, nobj);
            item = qobject_cast<QDeclarativeItem *>(nobj);
            if (!item)
                delete nobj;
        } else {
            delete context;
        }
        if (item) {
            QDeclarative_setParent_noEvent(item, q->contentItem());
            item->setParentItem(q->contentItem());
            item->setZValue(1);
            QDeclarativeItemPrivate *itemPrivate = static_cast<QDeclarativeItemPrivate*>(QGraphicsItemPrivate::get(item));
            itemPrivate->addItemChangeListener(this, QDeclarativeItemPrivate::Geometry);
            header = new FxListItem1(item, q);
        }
    }
    if (header) {
        if (visibleItems.count()) {
            qreal startPos = originPosition();
            if (visibleIndex == 0) {
                header->setPosition(startPos - header->size());
            } else {
                if (position() <= startPos || header->position() > startPos - header->size())
                    header->setPosition(startPos - header->size());
            }
        } else {
            visiblePos = header->size();
            header->setPosition(0);
        }
    }
}

void QDeclarative1ListViewPrivate::fixupPosition()
{
    if ((haveHighlightRange && highlightRange == QDeclarative1ListView::StrictlyEnforceRange)
        || snapMode != QDeclarative1ListView::NoSnap)
        moveReason = Other;
    if (orient == QDeclarative1ListView::Vertical)
        fixupY();
    else
        fixupX();
}

void QDeclarative1ListViewPrivate::fixup(AxisData &data, qreal minExtent, qreal maxExtent)
{
    if ((orient == QDeclarative1ListView::Horizontal && &data == &vData)
        || (orient == QDeclarative1ListView::Vertical && &data == &hData))
        return;

    correctFlick = false;
    fixupMode = moveReason == Mouse ? fixupMode : Immediate;
    bool strictHighlightRange = haveHighlightRange && highlightRange == QDeclarative1ListView::StrictlyEnforceRange;

    qreal highlightStart;
    qreal highlightEnd;
    qreal viewPos;
    if (isRightToLeft()) {
        // Handle Right-To-Left exceptions
        viewPos = -position()-size();
        highlightStart = highlightRangeStartValid ? size() - highlightRangeEnd : highlightRangeStart;
        highlightEnd = highlightRangeEndValid ? size() - highlightRangeStart : highlightRangeEnd;
    } else {
        viewPos = position();
        highlightStart = highlightRangeStart;
        highlightEnd = highlightRangeEnd;
    }

    if (snapMode != QDeclarative1ListView::NoSnap && moveReason != QDeclarative1ListViewPrivate::SetIndex) {
        qreal tempPosition = isRightToLeft() ? -position()-size() : position();
        if (snapMode == QDeclarative1ListView::SnapOneItem && moveReason == Mouse) {
            // if we've been dragged < averageSize/2 then bias towards the next item
            qreal dist = data.move.value() - (data.pressPos - data.dragStartOffset);
            qreal bias = 0;
            if (data.velocity > 0 && dist > QML_FLICK_SNAPONETHRESHOLD && dist < averageSize/2)
                bias = averageSize/2;
            else if (data.velocity < 0 && dist < -QML_FLICK_SNAPONETHRESHOLD && dist > -averageSize/2)
                bias = -averageSize/2;
            if (isRightToLeft())
                bias = -bias;
            tempPosition -= bias;
        }
        FxListItem1 *topItem = snapItemAt(tempPosition+highlightStart);
        if (!topItem && strictHighlightRange && currentItem) {
            // StrictlyEnforceRange always keeps an item in range
            updateHighlight();
            topItem = currentItem;
        }
        FxListItem1 *bottomItem = snapItemAt(tempPosition+highlightEnd);
        if (!bottomItem && strictHighlightRange && currentItem) {
            // StrictlyEnforceRange always keeps an item in range
            updateHighlight();
            bottomItem = currentItem;
        }
        qreal pos;
        bool isInBounds = -position() > maxExtent && -position() <= minExtent;
        if (topItem && (isInBounds || strictHighlightRange)) {
            if (topItem->index == 0 && header && tempPosition+highlightStart < header->position()+header->size()/2 && !strictHighlightRange) {
                pos = isRightToLeft() ? - header->position() + highlightStart - size() : header->position() - highlightStart;
            } else {
                if (isRightToLeft())
                    pos = qMax(qMin(-topItem->position() + highlightStart - size(), -maxExtent), -minExtent);
                else
                    pos = qMax(qMin(topItem->position() - highlightStart, -maxExtent), -minExtent);
            }
        } else if (bottomItem && isInBounds) {
            if (isRightToLeft())
                pos = qMax(qMin(-bottomItem->position() + highlightEnd - size(), -maxExtent), -minExtent);
            else
                pos = qMax(qMin(bottomItem->position() - highlightEnd, -maxExtent), -minExtent);
        } else {
            QDeclarative1FlickablePrivate::fixup(data, minExtent, maxExtent);
            return;
        }

        qreal dist = qAbs(data.move + pos);
        if (dist > 0) {
            timeline.reset(data.move);
            if (fixupMode != Immediate) {
                timeline.move(data.move, -pos, QEasingCurve(QEasingCurve::InOutQuad), fixupDuration/2);
                data.fixingUp = true;
            } else {
                timeline.set(data.move, -pos);
            }
            vTime = timeline.time();
        }
    } else if (currentItem && strictHighlightRange
                && moveReason != QDeclarative1ListViewPrivate::SetIndex) {
        updateHighlight();
        qreal pos = currentItem->itemPosition();
        if (viewPos < pos + currentItem->itemSize() - highlightEnd)
            viewPos = pos + currentItem->itemSize() - highlightEnd;
        if (viewPos > pos - highlightStart)
            viewPos = pos - highlightStart;
        if (isRightToLeft())
            viewPos = -viewPos-size();

        timeline.reset(data.move);
        if (viewPos != position()) {
            if (fixupMode != Immediate) {
                timeline.move(data.move, -viewPos, QEasingCurve(QEasingCurve::InOutQuad), fixupDuration/2);
                data.fixingUp = true;
            } else {
                timeline.set(data.move, -viewPos);
            }
        }
        vTime = timeline.time();
    } else {
        QDeclarative1FlickablePrivate::fixup(data, minExtent, maxExtent);
    }
    data.inOvershoot = false;
    fixupMode = Normal;
}

void QDeclarative1ListViewPrivate::flick(AxisData &data, qreal minExtent, qreal maxExtent, qreal vSize,
                                        QDeclarative1TimeLineCallback::Callback fixupCallback, qreal velocity)
{
    Q_Q(QDeclarative1ListView);

    data.fixingUp = false;
    moveReason = Mouse;
    if ((!haveHighlightRange || highlightRange != QDeclarative1ListView::StrictlyEnforceRange) && snapMode == QDeclarative1ListView::NoSnap) {
        correctFlick = true;
        QDeclarative1FlickablePrivate::flick(data, minExtent, maxExtent, vSize, fixupCallback, velocity);
        return;
    }
    qreal maxDistance = 0;
    qreal dataValue = isRightToLeft() ? -data.move.value()+size() : data.move.value();
    qreal highlightStart = isRightToLeft() && highlightRangeStartValid ? size()-highlightRangeEnd : highlightRangeStart;
    // -ve velocity means list is moving up/left
    if (velocity > 0) {
        if (data.move.value() < minExtent) {
            if (snapMode == QDeclarative1ListView::SnapOneItem && !hData.flicking && !vData.flicking) {
                // if we've been dragged < averageSize/2 then bias towards the next item
                qreal dist = data.move.value() - (data.pressPos - data.dragStartOffset);
                qreal bias = dist < averageSize/2 ? averageSize/2 : 0;
                if (isRightToLeft())
                    bias = -bias;
                data.flickTarget = -snapPosAt(-(dataValue - highlightStart) - bias) + highlightStart;
                maxDistance = qAbs(data.flickTarget - data.move.value());
                velocity = maxVelocity;
            } else {
                maxDistance = qAbs(minExtent - data.move.value());
            }
        }
        if (snapMode == QDeclarative1ListView::NoSnap && highlightRange != QDeclarative1ListView::StrictlyEnforceRange)
            data.flickTarget = minExtent;
    } else {
        if (data.move.value() > maxExtent) {
            if (snapMode == QDeclarative1ListView::SnapOneItem && !hData.flicking && !vData.flicking) {
                // if we've been dragged < averageSize/2 then bias towards the next item
                qreal dist = data.move.value() - (data.pressPos - data.dragStartOffset);
                qreal bias = -dist < averageSize/2 ? averageSize/2 : 0;
                if (isRightToLeft())
                    bias = -bias;
                data.flickTarget = -snapPosAt(-(dataValue - highlightStart) + bias) + highlightStart;
                maxDistance = qAbs(data.flickTarget - data.move.value());
                velocity = -maxVelocity;
            } else {
                maxDistance = qAbs(maxExtent - data.move.value());
            }
        }
        if (snapMode == QDeclarative1ListView::NoSnap && highlightRange != QDeclarative1ListView::StrictlyEnforceRange)
            data.flickTarget = maxExtent;
    }

    bool overShoot = boundsBehavior == QDeclarative1Flickable::DragAndOvershootBounds;

    if (maxDistance > 0 || overShoot) {
        // These modes require the list to stop exactly on an item boundary.
        // The initial flick will estimate the boundary to stop on.
        // Since list items can have variable sizes, the boundary will be
        // reevaluated and adjusted as we approach the boundary.
        qreal v = velocity;
        if (maxVelocity != -1 && maxVelocity < qAbs(v)) {
            if (v < 0)
                v = -maxVelocity;
            else
                v = maxVelocity;
        }
        if (!hData.flicking && !vData.flicking) {
            // the initial flick - estimate boundary
            qreal accel = deceleration;
            qreal v2 = v * v;
            overshootDist = 0.0;
            // + averageSize/4 to encourage moving at least one item in the flick direction
            qreal dist = v2 / (accel * 2.0) + averageSize/4;
            if (maxDistance > 0)
                dist = qMin(dist, maxDistance);
            if (v > 0)
                dist = -dist;
            if ((maxDistance > 0.0 && v2 / (2.0f * maxDistance) < accel) || snapMode == QDeclarative1ListView::SnapOneItem) {
                if (snapMode != QDeclarative1ListView::SnapOneItem) {
                    qreal distTemp = isRightToLeft() ? -dist : dist;
                    data.flickTarget = -snapPosAt(-(dataValue - highlightStart) + distTemp) + highlightStart;
                }
                data.flickTarget = isRightToLeft() ? -data.flickTarget+size() : data.flickTarget;
                if (overShoot) {
                    if (data.flickTarget >= minExtent) {
                        overshootDist = overShootDistance(vSize);
                        data.flickTarget += overshootDist;
                    } else if (data.flickTarget <= maxExtent) {
                        overshootDist = overShootDistance(vSize);
                        data.flickTarget -= overshootDist;
                    }
                }
                qreal adjDist = -data.flickTarget + data.move.value();
                if (qAbs(adjDist) > qAbs(dist)) {
                    // Prevent painfully slow flicking - adjust velocity to suit flickDeceleration
                    qreal adjv2 = accel * 2.0f * qAbs(adjDist);
                    if (adjv2 > v2) {
                        v2 = adjv2;
                        v = qSqrt(v2);
                        if (dist > 0)
                            v = -v;
                    }
                }
                dist = adjDist;
                accel = v2 / (2.0f * qAbs(dist));
            } else if (overShoot) {
                data.flickTarget = data.move.value() - dist;
                if (data.flickTarget >= minExtent) {
                    overshootDist = overShootDistance(vSize);
                    data.flickTarget += overshootDist;
                } else if (data.flickTarget <= maxExtent) {
                    overshootDist = overShootDistance(vSize);
                    data.flickTarget -= overshootDist;
                }
            }

            timeline.reset(data.move);
            timeline.accel(data.move, v, accel, maxDistance + overshootDist);
            timeline.callback(QDeclarative1TimeLineCallback(&data.move, fixupCallback, this));
            if (!hData.flicking && q->xflick()) {
                hData.flicking = true;
                emit q->flickingChanged();
                emit q->flickingHorizontallyChanged();
                emit q->flickStarted();
            }
            if (!vData.flicking && q->yflick()) {
                vData.flicking = true;
                emit q->flickingChanged();
                emit q->flickingVerticallyChanged();
                emit q->flickStarted();
            }
            correctFlick = true;
        } else {
            // reevaluate the target boundary.
            qreal newtarget = data.flickTarget;
            if (snapMode != QDeclarative1ListView::NoSnap || highlightRange == QDeclarative1ListView::StrictlyEnforceRange) {
                qreal tempFlickTarget = isRightToLeft() ? -data.flickTarget+size() : data.flickTarget;
                newtarget = -snapPosAt(-(tempFlickTarget - highlightStart)) + highlightStart;
                newtarget = isRightToLeft() ? -newtarget+size() : newtarget;
            }
            if (velocity < 0 && newtarget <= maxExtent)
                newtarget = maxExtent - overshootDist;
            else if (velocity > 0 && newtarget >= minExtent)
                newtarget = minExtent + overshootDist;
            if (newtarget == data.flickTarget) { // boundary unchanged - nothing to do
                if (qAbs(velocity) < MinimumFlickVelocity)
                    correctFlick = false;
                return;
            }
            data.flickTarget = newtarget;
            qreal dist = -newtarget + data.move.value();
            if ((v < 0 && dist < 0) || (v > 0 && dist > 0)) {
                correctFlick = false;
                timeline.reset(data.move);
                fixup(data, minExtent, maxExtent);
                return;
            }

            timeline.reset(data.move);
            timeline.accelDistance(data.move, v, -dist);
            timeline.callback(QDeclarative1TimeLineCallback(&data.move, fixupCallback, this));
        }
    } else {
        correctFlick = false;
        timeline.reset(data.move);
        fixup(data, minExtent, maxExtent);
    }
}

//----------------------------------------------------------------------------

/*!
    \qmlclass ListView QDeclarative1ListView
    \inqmlmodule QtQuick 1
    \ingroup qml-view-elements
    \since QtQuick 1.0
    \inherits Flickable
    \brief The ListView item provides a list view of items provided by a model.

    A ListView displays data from models created from built-in QML elements like ListModel
    and XmlListModel, or custom model classes defined in C++ that inherit from
    QAbstractListModel.

    A ListView has a \l model, which defines the data to be displayed, and
    a \l delegate, which defines how the data should be displayed. Items in a
    ListView are laid out horizontally or vertically. List views are inherently
    flickable because ListView inherits from \l Flickable.

    \section1 Example Usage

    The following example shows the definition of a simple list model defined
    in a file called \c ContactModel.qml:

    \snippet doc/src/snippets/qtquick1/listview/ContactModel.qml 0

    Another component can display this model data in a ListView, like this:

    \snippet doc/src/snippets/qtquick1/listview/listview.qml import
    \codeline
    \snippet doc/src/snippets/qtquick1/listview/listview.qml classdocs simple

    \image listview-simple.png

    Here, the ListView creates a \c ContactModel component for its model, and a \l Text element
    for its delegate. The view will create a new \l Text component for each item in the model. Notice
    the delegate is able to access the model's \c name and \c number data directly.

    An improved list view is shown below. The delegate is visually improved and is moved 
    into a separate \c contactDelegate component.

    \snippet doc/src/snippets/qtquick1/listview/listview.qml classdocs advanced
    \image listview-highlight.png

    The currently selected item is highlighted with a blue \l Rectangle using the \l highlight property,
    and \c focus is set to \c true to enable keyboard navigation for the list view.
    The list view itself is a focus scope (see \l{qmlfocus#Acquiring Focus and Focus Scopes}{the focus documentation page} for more details).

    Delegates are instantiated as needed and may be destroyed at any time.
    State should \e never be stored in a delegate.

    ListView attaches a number of properties to the root item of the delegate, for example
    \c {ListView.isCurrentItem}.  In the following example, the root delegate item can access
    this attached property directly as \c ListView.isCurrentItem, while the child
    \c contactInfo object must refer to this property as \c wrapper.ListView.isCurrentItem.

    \snippet doc/src/snippets/qtquick1/listview/listview.qml isCurrentItem

    \note Views do not enable \e clip automatically.  If the view
    is not clipped by another item or the screen, it will be necessary
    to set \e {clip: true} in order to have the out of view items clipped
    nicely.

    \sa {QML Data Models}, GridView, {declarative/modelviews/listview}{ListView examples}
*/

QDeclarative1ListView::QDeclarative1ListView(QDeclarativeItem *parent)
    : QDeclarative1Flickable(*(new QDeclarative1ListViewPrivate), parent)
{
    Q_D(QDeclarative1ListView);
    d->init();
}

QDeclarative1ListView::~QDeclarative1ListView()
{
    Q_D(QDeclarative1ListView);
    d->clear();
    if (d->ownModel)
        delete d->model;
    delete d->header;
    delete d->footer;
}

/*!
    \qmlattachedproperty bool ListView::isCurrentItem
    This attached property is true if this delegate is the current item; otherwise false.

    It is attached to each instance of the delegate.

    This property may be used to adjust the appearance of the current item, for example:

    \snippet doc/src/snippets/qtquick1/listview/listview.qml isCurrentItem
*/

/*!
    \qmlattachedproperty ListView ListView::view
    This attached property holds the view that manages this delegate instance.

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty string ListView::previousSection
    This attached property holds the section of the previous element.

    It is attached to each instance of the delegate.

    The section is evaluated using the \l {ListView::section.property}{section} properties.
*/

/*!
    \qmlattachedproperty string ListView::nextSection
    This attached property holds the section of the next element.

    It is attached to each instance of the delegate.

    The section is evaluated using the \l {ListView::section.property}{section} properties.
*/

/*!
    \qmlattachedproperty string ListView::section
    This attached property holds the section of this element.

    It is attached to each instance of the delegate.

    The section is evaluated using the \l {ListView::section.property}{section} properties.
*/

/*!
    \qmlattachedproperty bool ListView::delayRemove
    This attached property holds whether the delegate may be destroyed.

    It is attached to each instance of the delegate.

    It is sometimes necessary to delay the destruction of an item
    until an animation completes.

    The example delegate below ensures that the animation completes before
    the item is removed from the list.

    \snippet doc/src/snippets/qtquick1/listview/listview.qml delayRemove
*/

/*!
    \qmlattachedsignal QtQuick1::ListView::onAdd()
    This attached handler is called immediately after an item is added to the view.
*/

/*!
    \qmlattachedsignal QtQuick1::ListView::onRemove()
    This attached handler is called immediately before an item is removed from the view.
*/

/*!
    \qmlproperty model QtQuick1::ListView::model
    This property holds the model providing data for the list.

    The model provides the set of data that is used to create the items
    in the view. Models can be created directly in QML using \l ListModel, \l XmlListModel
    or \l VisualItemModel, or provided by C++ model classes. If a C++ model class is
    used, it must be a subclass of \l QAbstractItemModel or a simple list.

    \sa {qmlmodels}{Data Models}
*/
QVariant QDeclarative1ListView::model() const
{
    Q_D(const QDeclarative1ListView);
    return d->modelVariant;
}

void QDeclarative1ListView::setModel(const QVariant &model)
{
    Q_D(QDeclarative1ListView);
    if (d->modelVariant == model)
        return;
    if (d->model) {
        disconnect(d->model, SIGNAL(itemsInserted(int,int)), this, SLOT(itemsInserted(int,int)));
        disconnect(d->model, SIGNAL(itemsRemoved(int,int)), this, SLOT(itemsRemoved(int,int)));
        disconnect(d->model, SIGNAL(itemsMoved(int,int,int)), this, SLOT(itemsMoved(int,int,int)));
        disconnect(d->model, SIGNAL(itemsChanged(int,int)), this, SLOT(itemsChanged(int,int)));
        disconnect(d->model, SIGNAL(modelReset()), this, SLOT(modelReset()));
        disconnect(d->model, SIGNAL(createdItem(int,QDeclarativeItem*)), this, SLOT(createdItem(int,QDeclarativeItem*)));
        disconnect(d->model, SIGNAL(destroyingItem(QDeclarativeItem*)), this, SLOT(destroyingItem(QDeclarativeItem*)));
    }
    d->clear();
    QDeclarative1VisualModel *oldModel = d->model;
    d->model = 0;
    d->setPosition(0);
    d->modelVariant = model;
    QObject *object = qvariant_cast<QObject*>(model);
    QDeclarative1VisualModel *vim = 0;
    if (object && (vim = qobject_cast<QDeclarative1VisualModel *>(object))) {
        if (d->ownModel) {
            delete oldModel;
            d->ownModel = false;
        }
        d->model = vim;
    } else {
        if (!d->ownModel) {
            d->model = new QDeclarative1VisualDataModel(qmlContext(this), this);
            d->ownModel = true;
        } else {
            d->model = oldModel;
        }
        if (QDeclarative1VisualDataModel *dataModel = qobject_cast<QDeclarative1VisualDataModel*>(d->model))
            dataModel->setModel(model);
    }
    if (d->model) {
        d->bufferMode = QDeclarative1ListViewPrivate::BufferBefore | QDeclarative1ListViewPrivate::BufferAfter;
        if (isComponentComplete()) {
            updateSections();
            refill();
            if ((d->currentIndex >= d->model->count() || d->currentIndex < 0) && !d->currentIndexCleared) {
                setCurrentIndex(0);
            } else {
                d->moveReason = QDeclarative1ListViewPrivate::SetIndex;
                d->updateCurrent(d->currentIndex);
                if (d->highlight && d->currentItem) {
                    if (d->autoHighlight)
                        d->highlight->setPosition(d->currentItem->position());
                    d->updateTrackedItem();
                }
            }
            d->updateViewport();
        }
        connect(d->model, SIGNAL(itemsInserted(int,int)), this, SLOT(itemsInserted(int,int)));
        connect(d->model, SIGNAL(itemsRemoved(int,int)), this, SLOT(itemsRemoved(int,int)));
        connect(d->model, SIGNAL(itemsMoved(int,int,int)), this, SLOT(itemsMoved(int,int,int)));
        connect(d->model, SIGNAL(itemsChanged(int,int)), this, SLOT(itemsChanged(int,int)));
        connect(d->model, SIGNAL(modelReset()), this, SLOT(modelReset()));
        connect(d->model, SIGNAL(createdItem(int,QDeclarativeItem*)), this, SLOT(createdItem(int,QDeclarativeItem*)));
        connect(d->model, SIGNAL(destroyingItem(QDeclarativeItem*)), this, SLOT(destroyingItem(QDeclarativeItem*)));
        emit countChanged();
    }
    emit modelChanged();
}

/*!
    \qmlproperty Component QtQuick1::ListView::delegate

    The delegate provides a template defining each item instantiated by the view.
    The index is exposed as an accessible \c index property.  Properties of the
    model are also available depending upon the type of \l {qmlmodels}{Data Model}.

    The number of elements in the delegate has a direct effect on the
    flicking performance of the view.  If at all possible, place functionality
    that is not needed for the normal display of the delegate in a \l Loader which
    can load additional elements when needed.

    The ListView will lay out the items based on the size of the root item
    in the delegate.

    It is recommended that the delagate's size be a whole number to avoid sub-pixel
    alignment of items.

    \note Delegates are instantiated as needed and may be destroyed at any time.
    State should \e never be stored in a delegate.
*/
QDeclarativeComponent *QDeclarative1ListView::delegate() const
{
    Q_D(const QDeclarative1ListView);
    if (d->model) {
        if (QDeclarative1VisualDataModel *dataModel = qobject_cast<QDeclarative1VisualDataModel*>(d->model))
            return dataModel->delegate();
    }

    return 0;
}

void QDeclarative1ListView::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QDeclarative1ListView);
    if (delegate == this->delegate())
        return;
    if (!d->ownModel) {
        d->model = new QDeclarative1VisualDataModel(qmlContext(this));
        d->ownModel = true;
    }
    if (QDeclarative1VisualDataModel *dataModel = qobject_cast<QDeclarative1VisualDataModel*>(d->model)) {
        int oldCount = dataModel->count();
        dataModel->setDelegate(delegate);
        if (isComponentComplete()) {
            for (int i = 0; i < d->visibleItems.count(); ++i)
                d->releaseItem(d->visibleItems.at(i));
            d->visibleItems.clear();
            d->releaseItem(d->currentItem);
            d->currentItem = 0;
            updateSections();
            refill();
            d->moveReason = QDeclarative1ListViewPrivate::SetIndex;
            d->updateCurrent(d->currentIndex);
            if (d->highlight && d->currentItem) {
                if (d->autoHighlight)
                    d->highlight->setPosition(d->currentItem->position());
                d->updateTrackedItem();
            }
            d->updateViewport();
        }
        if (oldCount != dataModel->count())
            emit countChanged();
    }
    emit delegateChanged();
}

/*!
    \qmlproperty int QtQuick1::ListView::currentIndex
    \qmlproperty Item QtQuick1::ListView::currentItem

    The \c currentIndex property holds the index of the current item, and
    \c currentItem holds the current item.   Setting the currentIndex to -1
    will clear the highlight and set currentItem to null.

    If highlightFollowsCurrentItem is \c true, setting either of these 
    properties will smoothly scroll the ListView so that the current 
    item becomes visible.
    
    Note that the position of the current item
    may only be approximate until it becomes visible in the view.
*/
int QDeclarative1ListView::currentIndex() const
{
    Q_D(const QDeclarative1ListView);
    return d->currentIndex;
}

void QDeclarative1ListView::setCurrentIndex(int index)
{
    Q_D(QDeclarative1ListView);
    if (d->requestedIndex >= 0)  // currently creating item
        return;
    d->currentIndexCleared = (index == -1);
    if (index == d->currentIndex)
        return;
    if (isComponentComplete() && d->isValid()) {
        if (d->layoutScheduled)
            d->layout();
        d->moveReason = QDeclarative1ListViewPrivate::SetIndex;
        d->updateCurrent(index);
    } else if (d->currentIndex != index) {
        d->currentIndex = index;
        emit currentIndexChanged();
    }
}

QDeclarativeItem *QDeclarative1ListView::currentItem()
{
    Q_D(QDeclarative1ListView);
    if (!d->currentItem)
        return 0;
    return d->currentItem->item;
}

/*!
  \qmlproperty Item QtQuick1::ListView::highlightItem

    This holds the highlight item created from the \l highlight component.

  The \c highlightItem is managed by the view unless
  \l highlightFollowsCurrentItem is set to false.

  \sa highlight, highlightFollowsCurrentItem
*/
QDeclarativeItem *QDeclarative1ListView::highlightItem()
{
    Q_D(QDeclarative1ListView);
    if (!d->highlight)
        return 0;
    return d->highlight->item;
}

/*!
  \qmlproperty int QtQuick1::ListView::count
  This property holds the number of items in the view.
*/
int QDeclarative1ListView::count() const
{
    Q_D(const QDeclarative1ListView);
    if (d->model)
        return d->model->count();
    return 0;
}

/*!
    \qmlproperty Component QtQuick1::ListView::highlight
    This property holds the component to use as the highlight.

    An instance of the highlight component is created for each list.
    The geometry of the resulting component instance is managed by the list
    so as to stay with the current item, unless the highlightFollowsCurrentItem
    property is false.

    \sa highlightItem, highlightFollowsCurrentItem, {declarative/modelviews/listview}{ListView examples}
*/
QDeclarativeComponent *QDeclarative1ListView::highlight() const
{
    Q_D(const QDeclarative1ListView);
    return d->highlightComponent;
}

void QDeclarative1ListView::setHighlight(QDeclarativeComponent *highlight)
{
    Q_D(QDeclarative1ListView);
    if (highlight != d->highlightComponent) {
        d->highlightComponent = highlight;
        d->createHighlight();
        if (d->currentItem)
            d->updateHighlight();
        emit highlightChanged();
    }
}

/*!
    \qmlproperty bool QtQuick1::ListView::highlightFollowsCurrentItem
    This property holds whether the highlight is managed by the view.

    If this property is true (the default value), the highlight is moved smoothly
    to follow the current item.  Otherwise, the
    highlight is not moved by the view, and any movement must be implemented
    by the highlight.  
    
    Here is a highlight with its motion defined by a \l {SpringAnimation} item:

    \snippet doc/src/snippets/qtquick1/listview/listview.qml highlightFollowsCurrentItem

    Note that the highlight animation also affects the way that the view
    is scrolled.  This is because the view moves to maintain the
    highlight within the preferred highlight range (or visible viewport).

    \sa highlight, highlightMoveSpeed
*/
bool QDeclarative1ListView::highlightFollowsCurrentItem() const
{
    Q_D(const QDeclarative1ListView);
    return d->autoHighlight;
}

void QDeclarative1ListView::setHighlightFollowsCurrentItem(bool autoHighlight)
{
    Q_D(QDeclarative1ListView);
    if (d->autoHighlight != autoHighlight) {
        d->autoHighlight = autoHighlight;
        if (autoHighlight) {
            d->updateHighlight();
        } else {
            if (d->highlightPosAnimator)
                d->highlightPosAnimator->stop();
            if (d->highlightSizeAnimator)
                d->highlightSizeAnimator->stop();
        }
        emit highlightFollowsCurrentItemChanged();
    }
}

//###Possibly rename these properties, since they are very useful even without a highlight?
/*!
    \qmlproperty real QtQuick1::ListView::preferredHighlightBegin
    \qmlproperty real QtQuick1::ListView::preferredHighlightEnd
    \qmlproperty enumeration QtQuick1::ListView::highlightRangeMode

    These properties define the preferred range of the highlight (for the current item)
    within the view. The \c preferredHighlightBegin value must be less than the
    \c preferredHighlightEnd value. 

    These properties affect the position of the current item when the list is scrolled.
    For example, if the currently selected item should stay in the middle of the
    list when the view is scrolled, set the \c preferredHighlightBegin and 
    \c preferredHighlightEnd values to the top and bottom coordinates of where the middle 
    item would be. If the \c currentItem is changed programmatically, the list will
    automatically scroll so that the current item is in the middle of the view.
    Furthermore, the behavior of the current item index will occur whether or not a
    highlight exists.

    Valid values for \c highlightRangeMode are:

    \list
    \o ListView.ApplyRange - the view attempts to maintain the highlight within the range.
       However, the highlight can move outside of the range at the ends of the list or due
       to mouse interaction.
    \o ListView.StrictlyEnforceRange - the highlight never moves outside of the range.
       The current item changes if a keyboard or mouse action would cause the highlight to move
       outside of the range.
    \o ListView.NoHighlightRange - this is the default value.
    \endlist
*/
qreal QDeclarative1ListView::preferredHighlightBegin() const
{
    Q_D(const QDeclarative1ListView);
    return d->highlightRangeStart;
}

void QDeclarative1ListView::setPreferredHighlightBegin(qreal start)
{
    Q_D(QDeclarative1ListView);
    d->highlightRangeStartValid = true;
    if (d->highlightRangeStart == start)
        return;
    d->highlightRangeStart = start;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit preferredHighlightBeginChanged();
}

void QDeclarative1ListView::resetPreferredHighlightBegin()
{
    Q_D(QDeclarative1ListView);
    d->highlightRangeStartValid = false;
    if (d->highlightRangeStart == 0)
        return;
    d->highlightRangeStart = 0;
    emit preferredHighlightBeginChanged();
}

qreal QDeclarative1ListView::preferredHighlightEnd() const
{
    Q_D(const QDeclarative1ListView);
    return d->highlightRangeEnd;
}

void QDeclarative1ListView::setPreferredHighlightEnd(qreal end)
{
    Q_D(QDeclarative1ListView);
    d->highlightRangeEndValid = true;
    if (d->highlightRangeEnd == end)
        return;
    d->highlightRangeEnd = end;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit preferredHighlightEndChanged();
}

void QDeclarative1ListView::resetPreferredHighlightEnd()
{
    Q_D(QDeclarative1ListView);
    d->highlightRangeEndValid = false;
    if (d->highlightRangeEnd == 0)
        return;
    d->highlightRangeEnd = 0;
    emit preferredHighlightEndChanged();
}

QDeclarative1ListView::HighlightRangeMode QDeclarative1ListView::highlightRangeMode() const
{
    Q_D(const QDeclarative1ListView);
    return d->highlightRange;
}

void QDeclarative1ListView::setHighlightRangeMode(HighlightRangeMode mode)
{
    Q_D(QDeclarative1ListView);
    if (d->highlightRange == mode)
        return;
    d->highlightRange = mode;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    emit highlightRangeModeChanged();
}

/*!
    \qmlproperty real QtQuick1::ListView::spacing

    This property holds the spacing between items.

    The default value is 0.
*/
qreal QDeclarative1ListView::spacing() const
{
    Q_D(const QDeclarative1ListView);
    return d->spacing;
}

void QDeclarative1ListView::setSpacing(qreal spacing)
{
    Q_D(QDeclarative1ListView);
    if (spacing != d->spacing) {
        d->spacing = spacing;
        d->layout();
        emit spacingChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick1::ListView::orientation
    This property holds the orientation of the list.

    Possible values:

    \list
    \o ListView.Horizontal - Items are laid out horizontally
    \o ListView.Vertical (default) - Items are laid out vertically
    \endlist

    \table
    \row
    \o Horizontal orientation:
    \image ListViewHorizontal.png

    \row
    \o Vertical orientation:
    \image listview-highlight.png
    \endtable
*/
QDeclarative1ListView::Orientation QDeclarative1ListView::orientation() const
{
    Q_D(const QDeclarative1ListView);
    return d->orient;
}

void QDeclarative1ListView::setOrientation(QDeclarative1ListView::Orientation orientation)
{
    Q_D(QDeclarative1ListView);
    if (d->orient != orientation) {
        d->orient = orientation;
        if (d->orient == QDeclarative1ListView::Vertical) {
            setContentWidth(-1);
            setFlickableDirection(VerticalFlick);
            setContentX(0);
        } else {
            setContentHeight(-1);
            setFlickableDirection(HorizontalFlick);
            setContentY(0);
        }
        d->regenerate();
        emit orientationChanged();
    }
}

/*!
  \qmlproperty enumeration QtQuick1::ListView::layoutDirection
  This property holds the layout direction of the horizontal list.

  Possible values:

  \list
  \o Qt.LeftToRight (default) - Items will be laid out from left to right.
  \o Qt.RightToLeft - Items will be laid out from right to let.
  \endlist

  \sa ListView::effectiveLayoutDirection
*/

Qt::LayoutDirection QDeclarative1ListView::layoutDirection() const
{
    Q_D(const QDeclarative1ListView);
    return d->layoutDirection;
}

void QDeclarative1ListView::setLayoutDirection(Qt::LayoutDirection layoutDirection)
{
    Q_D(QDeclarative1ListView);
    if (d->layoutDirection != layoutDirection) {
        d->layoutDirection = layoutDirection;
        d->regenerate();
        emit layoutDirectionChanged();
        emit effectiveLayoutDirectionChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick1::ListView::effectiveLayoutDirection
    This property holds the effective layout direction of the horizontal list.

    When using the attached property \l {LayoutMirroring::enabled}{LayoutMirroring::enabled} for locale layouts,
    the visual layout direction of the horizontal list will be mirrored. However, the
    property \l {ListView::layoutDirection}{layoutDirection} will remain unchanged.

    \sa ListView::layoutDirection, {LayoutMirroring}{LayoutMirroring}
*/

Qt::LayoutDirection QDeclarative1ListView::effectiveLayoutDirection() const
{
    Q_D(const QDeclarative1ListView);
    if (d->effectiveLayoutMirror)
        return d->layoutDirection == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft;
    else
        return d->layoutDirection;
}

/*!
    \qmlproperty bool QtQuick1::ListView::keyNavigationWraps
    This property holds whether the list wraps key navigation. 

    If this is true, key navigation that would move the current item selection
    past the end of the list instead wraps around and moves the selection to
    the start of the list, and vice-versa.

    By default, key navigation is not wrapped.
*/
bool QDeclarative1ListView::isWrapEnabled() const
{
    Q_D(const QDeclarative1ListView);
    return d->wrap;
}

void QDeclarative1ListView::setWrapEnabled(bool wrap)
{
    Q_D(QDeclarative1ListView);
    if (d->wrap == wrap)
        return;
    d->wrap = wrap;
    emit keyNavigationWrapsChanged();
}

/*!
    \qmlproperty int QtQuick1::ListView::cacheBuffer
    This property determines whether delegates are retained outside the
    visible area of the view.

    If this value is non-zero, the view keeps as many delegates
    instantiated as it can fit within the buffer specified.  For example,
    if in a vertical view the delegate is 20 pixels high and \c cacheBuffer is
    set to 40, then up to 2 delegates above and 2 delegates below the visible
    area may be retained.

    Note that cacheBuffer is not a pixel buffer - it only maintains additional
    instantiated delegates.

    Setting this value can improve the smoothness of scrolling behavior at the expense
    of additional memory usage.  It is not a substitute for creating efficient
    delegates; the fewer elements in a delegate, the faster a view can be
    scrolled.
*/
int QDeclarative1ListView::cacheBuffer() const
{
    Q_D(const QDeclarative1ListView);
    return d->buffer;
}

void QDeclarative1ListView::setCacheBuffer(int b)
{
    Q_D(QDeclarative1ListView);
    if (d->buffer != b) {
        d->buffer = b;
        if (isComponentComplete()) {
            d->bufferMode = QDeclarative1ListViewPrivate::BufferBefore | QDeclarative1ListViewPrivate::BufferAfter;
            refill();
        }
        emit cacheBufferChanged();
    }
}

/*!
    \qmlproperty string QtQuick1::ListView::section.property
    \qmlproperty enumeration QtQuick1::ListView::section.criteria
    \qmlproperty Component QtQuick1::ListView::section.delegate

    These properties hold the expression to be evaluated for the \l section attached property.

    The \l section attached property enables a ListView to be visually
    separated into different parts. These properties determine how sections
    are created.
    
    \c section.property holds the name of the property that is the basis
    of each section.

    \c section.criteria holds the criteria for forming each section based on
    \c section.property. This value can be one of:

    \list
    \o ViewSection.FullString (default) - sections are created based on the 
    \c section.property value.
    \o ViewSection.FirstCharacter - sections are created based on the first
    character of the \c section.property value (for example, 'A', 'B', 'C' 
    sections, etc. for an address book)
    \endlist

    \c section.delegate holds the delegate component for each section.

    Each item in the list has attached properties named \c ListView.section,
    \c ListView.previousSection and \c ListView.nextSection.  These may be
    used to place a section header for related items.

    For example, here is a ListView that displays a list of animals, separated 
    into sections. Each item in the ListView is placed in a different section 
    depending on the "size" property of the model item. The \c sectionHeading
    delegate component provides the light blue bar that marks the beginning of
    each section.

       
    \snippet examples/declarative/modelviews/listview/sections.qml 0

    \image qml-listview-sections-example.png

    \note Adding sections to a ListView does not automatically re-order the
    list items by the section criteria.
    If the model is not ordered by section, then it is possible that
    the sections created will not be unique; each boundary between
    differing sections will result in a section header being created
    even if that section exists elsewhere.

    \sa {declarative/modelviews/listview}{ListView examples}
*/
QDeclarative1ViewSection *QDeclarative1ListView::sectionCriteria()
{
    Q_D(QDeclarative1ListView);
    if (!d->sectionCriteria) {
        d->sectionCriteria = new QDeclarative1ViewSection(this);
        connect(d->sectionCriteria, SIGNAL(propertyChanged()), this, SLOT(updateSections()));
    }
    return d->sectionCriteria;
}

/*!
    \qmlproperty string QtQuick1::ListView::currentSection
    This property holds the section that is currently at the beginning of the view.
*/
QString QDeclarative1ListView::currentSection() const
{
    Q_D(const QDeclarative1ListView);
    return d->currentSection;
}

/*!
    \qmlproperty real QtQuick1::ListView::highlightMoveSpeed
    \qmlproperty int QtQuick1::ListView::highlightMoveDuration
    \qmlproperty real QtQuick1::ListView::highlightResizeSpeed
    \qmlproperty int QtQuick1::ListView::highlightResizeDuration

    These properties hold the move and resize animation speed of the highlight delegate.

    \l highlightFollowsCurrentItem must be true for these properties
    to have effect.

    The default value for the speed properties is 400 pixels/second.
    The default value for the duration properties is -1, i.e. the
    highlight will take as much time as necessary to move at the set speed.

    These properties have the same characteristics as a SmoothedAnimation.

    \sa highlightFollowsCurrentItem
*/
qreal QDeclarative1ListView::highlightMoveSpeed() const
{
    Q_D(const QDeclarative1ListView);\
    return d->highlightMoveSpeed;
}

void QDeclarative1ListView::setHighlightMoveSpeed(qreal speed)
{
    Q_D(QDeclarative1ListView);\
    if (d->highlightMoveSpeed != speed) {
        d->highlightMoveSpeed = speed;
        if (d->highlightPosAnimator)
            d->highlightPosAnimator->velocity = d->highlightMoveSpeed;
        emit highlightMoveSpeedChanged();
    }
}

int QDeclarative1ListView::highlightMoveDuration() const
{
    Q_D(const QDeclarative1ListView);
    return d->highlightMoveDuration;
}

void QDeclarative1ListView::setHighlightMoveDuration(int duration)
{
    Q_D(QDeclarative1ListView);\
    if (d->highlightMoveDuration != duration) {
        d->highlightMoveDuration = duration;
        if (d->highlightPosAnimator)
            d->highlightPosAnimator->userDuration = d->highlightMoveDuration;
        emit highlightMoveDurationChanged();
    }
}

qreal QDeclarative1ListView::highlightResizeSpeed() const
{
    Q_D(const QDeclarative1ListView);\
    return d->highlightResizeSpeed;
}

void QDeclarative1ListView::setHighlightResizeSpeed(qreal speed)
{
    Q_D(QDeclarative1ListView);\
    if (d->highlightResizeSpeed != speed) {
        d->highlightResizeSpeed = speed;
        if (d->highlightSizeAnimator)
            d->highlightSizeAnimator->velocity = d->highlightResizeSpeed;
        emit highlightResizeSpeedChanged();
    }
}

int QDeclarative1ListView::highlightResizeDuration() const
{
    Q_D(const QDeclarative1ListView);
    return d->highlightResizeDuration;
}

void QDeclarative1ListView::setHighlightResizeDuration(int duration)
{
    Q_D(QDeclarative1ListView);\
    if (d->highlightResizeDuration != duration) {
        d->highlightResizeDuration = duration;
        if (d->highlightSizeAnimator)
            d->highlightSizeAnimator->userDuration = d->highlightResizeDuration;
        emit highlightResizeDurationChanged();
    }
}

/*!
    \qmlproperty enumeration QtQuick1::ListView::snapMode

    This property determines how the view scrolling will settle following a drag or flick.
    The possible values are:

    \list
    \o ListView.NoSnap (default) - the view stops anywhere within the visible area.
    \o ListView.SnapToItem - the view settles with an item aligned with the start of
    the view.
    \o ListView.SnapOneItem - the view settles no more than one item away from the first
    visible item at the time the mouse button is released.  This mode is particularly
    useful for moving one page at a time.
    \endlist

    \c snapMode does not affect the \l currentIndex.  To update the
    \l currentIndex as the list is moved, set \l highlightRangeMode
    to \c ListView.StrictlyEnforceRange.

    \sa highlightRangeMode
*/
QDeclarative1ListView::SnapMode QDeclarative1ListView::snapMode() const
{
    Q_D(const QDeclarative1ListView);
    return d->snapMode;
}

void QDeclarative1ListView::setSnapMode(SnapMode mode)
{
    Q_D(QDeclarative1ListView);
    if (d->snapMode != mode) {
        d->snapMode = mode;
        emit snapModeChanged();
    }
}

/*!
    \qmlproperty Component QtQuick1::ListView::footer
    This property holds the component to use as the footer.

    An instance of the footer component is created for each view.  The
    footer is positioned at the end of the view, after any items.

    \sa header
*/
QDeclarativeComponent *QDeclarative1ListView::footer() const
{
    Q_D(const QDeclarative1ListView);
    return d->footerComponent;
}

void QDeclarative1ListView::setFooter(QDeclarativeComponent *footer)
{
    Q_D(QDeclarative1ListView);
    if (d->footerComponent != footer) {
        if (d->footer) {
            if (scene())
                scene()->removeItem(d->footer->item);
            d->footer->item->deleteLater();
            delete d->footer;
            d->footer = 0;
        }
        d->footerComponent = footer;
        d->minExtentDirty = true;
        d->maxExtentDirty = true;
        if (isComponentComplete()) {
            d->updateFooter();
            d->updateViewport();
            d->fixupPosition();
        }
        emit footerChanged();
    }
}

/*!
    \qmlproperty Component QtQuick1::ListView::header
    This property holds the component to use as the header.

    An instance of the header component is created for each view.  The
    header is positioned at the beginning of the view, before any items.

    \sa footer
*/
QDeclarativeComponent *QDeclarative1ListView::header() const
{
    Q_D(const QDeclarative1ListView);
    return d->headerComponent;
}

void QDeclarative1ListView::setHeader(QDeclarativeComponent *header)
{
    Q_D(QDeclarative1ListView);
    if (d->headerComponent != header) {
        if (d->header) {
            if (scene())
                scene()->removeItem(d->header->item);
            d->header->item->deleteLater();
            delete d->header;
            d->header = 0;
        }
        d->headerComponent = header;
        d->minExtentDirty = true;
        d->maxExtentDirty = true;
        if (isComponentComplete()) {
            d->updateHeader();
            d->updateFooter();
            d->updateViewport();
            d->fixupPosition();
        }
        emit headerChanged();
    }
}

void QDeclarative1ListView::setContentX(qreal pos)
{
    Q_D(QDeclarative1ListView);
    // Positioning the view manually should override any current movement state
    d->moveReason = QDeclarative1ListViewPrivate::Other;
    QDeclarative1Flickable::setContentX(pos);
}

void QDeclarative1ListView::setContentY(qreal pos)
{
    Q_D(QDeclarative1ListView);
    // Positioning the view manually should override any current movement state
    d->moveReason = QDeclarative1ListViewPrivate::Other;
    QDeclarative1Flickable::setContentY(pos);
}

bool QDeclarative1ListView::event(QEvent *event)
{
    Q_D(QDeclarative1ListView);
    if (event->type() == QEvent::User) {
        if (d->layoutScheduled)
            d->layout();
        return true;
    }

    return QDeclarative1Flickable::event(event);
}

void QDeclarative1ListView::viewportMoved()
{
    Q_D(QDeclarative1ListView);
    QDeclarative1Flickable::viewportMoved();
    if (!d->itemCount)
        return;
    // Recursion can occur due to refill changing the content size.
    if (d->inViewportMoved)
        return;
    d->inViewportMoved = true;
    d->lazyRelease = true;
    refill();
    if (d->hData.flicking || d->vData.flicking || d->hData.moving || d->vData.moving)
        d->moveReason = QDeclarative1ListViewPrivate::Mouse;
    if (d->moveReason != QDeclarative1ListViewPrivate::SetIndex) {
        if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange && d->highlight) {
            // reposition highlight
            qreal pos = d->highlight->position();
            qreal viewPos;
            qreal highlightStart;
            qreal highlightEnd;
            if (d->isRightToLeft()) {
                // Handle Right-To-Left exceptions
                viewPos = -d->position()-d->size();
                highlightStart = d->highlightRangeStartValid ? d->size()-d->highlightRangeEnd : d->highlightRangeStart;
                highlightEnd = d->highlightRangeEndValid ? d->size()-d->highlightRangeStart : d->highlightRangeEnd;
            } else {
                viewPos = d->position();
                highlightStart = d->highlightRangeStart;
                highlightEnd = d->highlightRangeEnd;
            }
            if (pos > viewPos + highlightEnd - d->highlight->size())
                pos = viewPos + highlightEnd - d->highlight->size();
            if (pos < viewPos + highlightStart)
                pos = viewPos + highlightStart;
            d->highlightPosAnimator->stop();
            d->highlight->setPosition(qRound(pos));

            // update current index
            if (FxListItem1 *snapItem = d->snapItemAt(d->highlight->position())) {
                if (snapItem->index >= 0 && snapItem->index != d->currentIndex)
                    d->updateCurrent(snapItem->index);
            }
        }
    }

    if ((d->hData.flicking || d->vData.flicking) && d->correctFlick && !d->inFlickCorrection) {
        d->inFlickCorrection = true;
        // Near an end and it seems that the extent has changed?
        // Recalculate the flick so that we don't end up in an odd position.
        if (yflick() && !d->vData.inOvershoot) {
            if (d->vData.velocity > 0) {
                const qreal minY = minYExtent();
                if ((minY - d->vData.move.value() < height()/2 || d->vData.flickTarget - d->vData.move.value() < height()/2)
                    && minY != d->vData.flickTarget)
                    d->flickY(-d->vData.smoothVelocity.value());
                d->bufferMode = QDeclarative1ListViewPrivate::BufferBefore;
            } else if (d->vData.velocity < 0) {
                const qreal maxY = maxYExtent();
                if ((d->vData.move.value() - maxY < height()/2 || d->vData.move.value() - d->vData.flickTarget < height()/2)
                    && maxY != d->vData.flickTarget)
                    d->flickY(-d->vData.smoothVelocity.value());
                d->bufferMode = QDeclarative1ListViewPrivate::BufferAfter;
            }
        }

        if (xflick() && !d->hData.inOvershoot) {
            if (d->hData.velocity > 0) {
                const qreal minX = minXExtent();
                if ((minX - d->hData.move.value() < width()/2 || d->hData.flickTarget - d->hData.move.value() < width()/2)
                    && minX != d->hData.flickTarget)
                    d->flickX(-d->hData.smoothVelocity.value());
                d->bufferMode = d->isRightToLeft()
                        ? QDeclarative1ListViewPrivate::BufferAfter : QDeclarative1ListViewPrivate::BufferBefore;
            } else if (d->hData.velocity < 0) {
                const qreal maxX = maxXExtent();
                if ((d->hData.move.value() - maxX < width()/2 || d->hData.move.value() - d->hData.flickTarget < width()/2)
                    && maxX != d->hData.flickTarget)
                    d->flickX(-d->hData.smoothVelocity.value());
                d->bufferMode = d->isRightToLeft()
                        ? QDeclarative1ListViewPrivate::BufferBefore : QDeclarative1ListViewPrivate::BufferAfter;
            }
        }
        d->inFlickCorrection = false;
    }
    d->inViewportMoved = false;
}

qreal QDeclarative1ListView::minYExtent() const
{
    Q_D(const QDeclarative1ListView);
    if (d->orient == QDeclarative1ListView::Horizontal)
        return QDeclarative1Flickable::minYExtent();
    if (d->minExtentDirty) {
        d->minExtent = -d->startPosition();
        if (d->header && d->visibleItems.count())
            d->minExtent += d->header->size();
        if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
            d->minExtent += d->highlightRangeStart;
            if (d->sectionCriteria) {
                if (d->visibleItem(0))
                    d->minExtent -= d->visibleItem(0)->sectionSize();
            }
            d->minExtent = qMax(d->minExtent, -(d->endPositionAt(0) - d->highlightRangeEnd + 1));
        }
        d->minExtentDirty = false;
    }

    return d->minExtent;
}

qreal QDeclarative1ListView::maxYExtent() const
{
    Q_D(const QDeclarative1ListView);
    if (d->orient == QDeclarative1ListView::Horizontal)
        return height();
    if (d->maxExtentDirty) {
        if (!d->model || !d->model->count()) {
            d->maxExtent = d->header ? -d->header->size() : 0;
            d->maxExtent += height();
        } else if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
            d->maxExtent = -(d->positionAt(d->model->count()-1) - d->highlightRangeStart);
            if (d->highlightRangeEnd != d->highlightRangeStart)
                d->maxExtent = qMin(d->maxExtent, -(d->endPosition() - d->highlightRangeEnd + 1));
        } else {
            d->maxExtent = -(d->endPosition() - height() + 1);
        }
        if (d->footer)
            d->maxExtent -= d->footer->size();
        qreal minY = minYExtent();
        if (d->maxExtent > minY)
            d->maxExtent = minY;
        d->maxExtentDirty = false;
    }
    return d->maxExtent;
}

qreal QDeclarative1ListView::minXExtent() const
{
    Q_D(const QDeclarative1ListView);
    if (d->orient == QDeclarative1ListView::Vertical)
        return QDeclarative1Flickable::minXExtent();
    if (d->minExtentDirty) {
        d->minExtent = -d->startPosition();

        qreal highlightStart;
        qreal highlightEnd;
        qreal endPositionFirstItem = 0;
        if (d->isRightToLeft()) {
            if (d->model && d->model->count())
                endPositionFirstItem = d->positionAt(d->model->count()-1);
            else if (d->header)
                d->minExtent += d->header->size();
            highlightStart = d->highlightRangeStartValid
                    ? d->highlightRangeStart - (d->lastPosition()-endPositionFirstItem)
                    : d->size() - (d->lastPosition()-endPositionFirstItem);
            highlightEnd = d->highlightRangeEndValid ? d->highlightRangeEnd : d->size();
            if (d->footer)
                d->minExtent += d->footer->size();
            qreal maxX = maxXExtent();
            if (d->minExtent < maxX)
                d->minExtent = maxX;
        } else {
            endPositionFirstItem = d->endPositionAt(0);
            highlightStart = d->highlightRangeStart;
            highlightEnd = d->highlightRangeEnd;
            if (d->header && d->visibleItems.count())
                d->minExtent += d->header->size();
        }
        if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
            d->minExtent += d->isRightToLeft() ? -highlightStart : highlightStart;
            d->minExtent = qMax(d->minExtent, -(endPositionFirstItem - highlightEnd + 1));
        }
        d->minExtentDirty = false;
    }

    return d->minExtent;
}

qreal QDeclarative1ListView::maxXExtent() const
{
    Q_D(const QDeclarative1ListView);
    if (d->orient == QDeclarative1ListView::Vertical)
        return width();
    if (d->maxExtentDirty) {
        qreal highlightStart;
        qreal highlightEnd;
        qreal lastItemPosition = 0;
        d->maxExtent = 0;
        if (d->isRightToLeft()) {
            highlightStart = d->highlightRangeStartValid ? d->highlightRangeEnd : d->size();
            highlightEnd = d->highlightRangeEndValid ? d->highlightRangeStart : d->size();
            lastItemPosition = d->endPosition();
        } else {
            highlightStart = d->highlightRangeStart;
            highlightEnd = d->highlightRangeEnd;
            if (d->model && d->model->count())
                lastItemPosition = d->positionAt(d->model->count()-1);
        }
        if (!d->model || !d->model->count()) {
            if (!d->isRightToLeft())
                d->maxExtent = d->header ? -d->header->size() : 0;
            d->maxExtent += width();
        } else if (d->haveHighlightRange && d->highlightRange == StrictlyEnforceRange) {
            d->maxExtent = -(lastItemPosition - highlightStart);
            if (highlightEnd != highlightStart) {
                d->maxExtent = d->isRightToLeft()
                        ? qMax(d->maxExtent, -(d->endPosition() - highlightEnd + 1))
                        : qMin(d->maxExtent, -(d->endPosition() - highlightEnd + 1));
            }
        } else {
            d->maxExtent = -(d->endPosition() - width() + 1);
        }
        if (d->isRightToLeft()) {
            if (d->header && d->visibleItems.count())
                d->maxExtent -= d->header->size();
        } else {
            if (d->footer)
                d->maxExtent -= d->footer->size();
            qreal minX = minXExtent();
            if (d->maxExtent > minX)
                d->maxExtent = minX;
        }
        d->maxExtentDirty = false;
    }
    return d->maxExtent;
}

void QDeclarative1ListView::keyPressEvent(QKeyEvent *event)
{
    Q_D(QDeclarative1ListView);
    keyPressPreHandler(event);
    if (event->isAccepted())
        return;

    if (d->model && d->model->count() && d->interactive) {
        if ((d->orient == QDeclarative1ListView::Horizontal && !d->isRightToLeft() && event->key() == Qt::Key_Left)
                    || (d->orient == QDeclarative1ListView::Horizontal && d->isRightToLeft() && event->key() == Qt::Key_Right)
                    || (d->orient == QDeclarative1ListView::Vertical && event->key() == Qt::Key_Up)) {
            if (currentIndex() > 0 || (d->wrap && !event->isAutoRepeat())) {
                decrementCurrentIndex();
                event->accept();
                return;
            } else if (d->wrap) {
                event->accept();
                return;
            }
        } else if ((d->orient == QDeclarative1ListView::Horizontal && !d->isRightToLeft() && event->key() == Qt::Key_Right)
                    || (d->orient == QDeclarative1ListView::Horizontal && d->isRightToLeft() && event->key() == Qt::Key_Left)
                    || (d->orient == QDeclarative1ListView::Vertical && event->key() == Qt::Key_Down)) {
            if (currentIndex() < d->model->count() - 1 || (d->wrap && !event->isAutoRepeat())) {
                incrementCurrentIndex();
                event->accept();
                return;
            } else if (d->wrap) {
                event->accept();
                return;
            }
        }
    }
    event->ignore();
    QDeclarative1Flickable::keyPressEvent(event);
}

void QDeclarative1ListView::geometryChanged(const QRectF &newGeometry,
                             const QRectF &oldGeometry)
{
    Q_D(QDeclarative1ListView);
    d->maxExtentDirty = true;
    d->minExtentDirty = true;
    if (d->isRightToLeft() && d->orient == QDeclarative1ListView::Horizontal) {
        // maintain position relative to the right edge
        int dx = newGeometry.width() - oldGeometry.width();
        setContentX(contentX() - dx);
    }
    QDeclarative1Flickable::geometryChanged(newGeometry, oldGeometry);
}


/*!
    \qmlmethod QtQuick1::ListView::incrementCurrentIndex()

    Increments the current index.  The current index will wrap
    if keyNavigationWraps is true and it is currently at the end.
    This method has no effect if the \l count is zero.

    \bold Note: methods should only be called after the Component has completed.
*/
void QDeclarative1ListView::incrementCurrentIndex()
{
    Q_D(QDeclarative1ListView);
    int count = d->model ? d->model->count() : 0;
    if (count && (currentIndex() < count - 1 || d->wrap)) {
        d->moveReason = QDeclarative1ListViewPrivate::SetIndex;
        int index = currentIndex()+1;
        setCurrentIndex((index >= 0 && index < count) ? index : 0);
    }
}

/*!
    \qmlmethod QtQuick1::ListView::decrementCurrentIndex()

    Decrements the current index.  The current index will wrap
    if keyNavigationWraps is true and it is currently at the beginning.
    This method has no effect if the \l count is zero.

    \bold Note: methods should only be called after the Component has completed.
*/
void QDeclarative1ListView::decrementCurrentIndex()
{
    Q_D(QDeclarative1ListView);
    int count = d->model ? d->model->count() : 0;
    if (count && (currentIndex() > 0 || d->wrap)) {
        d->moveReason = QDeclarative1ListViewPrivate::SetIndex;
        int index = currentIndex()-1;
        setCurrentIndex((index >= 0 && index < count) ? index : count-1);
    }
}

void QDeclarative1ListViewPrivate::positionViewAtIndex(int index, int mode)
{
    Q_Q(QDeclarative1ListView);
    if (!isValid())
        return;
    if (mode < QDeclarative1ListView::Beginning || mode > QDeclarative1ListView::Contain)
        return;
    int idx = qMax(qMin(index, model->count()-1), 0);

    if (layoutScheduled)
        layout();
    qreal pos = isRightToLeft() ? -position() - size() : position();
    FxListItem1 *item = visibleItem(idx);
    qreal maxExtent;
    if (orient == QDeclarative1ListView::Vertical)
        maxExtent = -q->maxYExtent();
    else
        maxExtent = isRightToLeft() ? q->minXExtent()-size(): -q->maxXExtent();

    if (!item) {
        int itemPos = positionAt(idx);
        // save the currently visible items in case any of them end up visible again
        QList<FxListItem1*> oldVisible = visibleItems;
        visibleItems.clear();
        visiblePos = itemPos;
        visibleIndex = idx;
        setPosition(qMin(qreal(itemPos), maxExtent));
        // now release the reference to all the old visible items.
        for (int i = 0; i < oldVisible.count(); ++i)
            releaseItem(oldVisible.at(i));
        item = visibleItem(idx);
    }
    if (item) {
        const qreal itemPos = item->position();
        switch (mode) {
        case QDeclarative1ListView::Beginning:
            pos = itemPos;
            if (index < 0 && header)
                pos -= header->size();
            break;
        case QDeclarative1ListView::Center:
            pos = itemPos - (size() - item->size())/2;
            break;
        case QDeclarative1ListView::End:
            pos = itemPos - size() + item->size();
            if (index >= model->count() && footer)
                pos += footer->size();
            break;
        case QDeclarative1ListView::Visible:
            if (itemPos > pos + size())
                pos = itemPos - size() + item->size();
            else if (item->endPosition() < pos)
                pos = itemPos;
            break;
        case QDeclarative1ListView::Contain:
            if (item->endPosition() > pos + size())
                pos = itemPos - size() + item->size();
            if (itemPos < pos)
                pos = itemPos;
        }
        pos = qMin(pos, maxExtent);
        qreal minExtent;
        if (orient == QDeclarative1ListView::Vertical) {
            minExtent = -q->minYExtent();
        } else {
            minExtent = isRightToLeft() ? q->maxXExtent()-size(): -q->minXExtent();
        }
        pos = qMax(pos, minExtent);
        moveReason = QDeclarative1ListViewPrivate::Other;
        q->cancelFlick();
        setPosition(pos);
        if (highlight) {
            if (autoHighlight) {
                highlight->setPosition(currentItem->itemPosition());
                highlight->setSize(currentItem->itemSize());
            }
            updateHighlight();
        }
    }
    fixupPosition();
}

/*!
    \qmlmethod QtQuick1::ListView::positionViewAtIndex(int index, PositionMode mode)

    Positions the view such that the \a index is at the position specified by
    \a mode:

    \list
    \o ListView.Beginning - position item at the top (or left for horizontal orientation) of the view.
    \o ListView.Center - position item in the center of the view.
    \o ListView.End - position item at bottom (or right for horizontal orientation) of the view.
    \o ListView.Visible - if any part of the item is visible then take no action, otherwise
    bring the item into view.
    \o ListView.Contain - ensure the entire item is visible.  If the item is larger than
    the view the item is positioned at the top (or left for horizontal orientation) of the view.
    \endlist

    If positioning the view at \a index would cause empty space to be displayed at
    the beginning or end of the view, the view will be positioned at the boundary.

    It is not recommended to use \l {Flickable::}{contentX} or \l {Flickable::}{contentY} to position the view
    at a particular index.  This is unreliable since removing items from the start
    of the list does not cause all other items to be repositioned, and because
    the actual start of the view can vary based on the size of the delegates.
    The correct way to bring an item into view is with \c positionViewAtIndex.

    \bold Note: methods should only be called after the Component has completed.  To position
    the view at startup, this method should be called by Component.onCompleted.  For
    example, to position the view at the end:

    \code
    Component.onCompleted: positionViewAtIndex(count - 1, ListView.Beginning)
    \endcode
*/
void QDeclarative1ListView::positionViewAtIndex(int index, int mode)
{
    Q_D(QDeclarative1ListView);
    if (!d->isValid() || index < 0 || index >= d->model->count())
        return;
    d->positionViewAtIndex(index, mode);
}

/*!
    \qmlmethod QtQuick1::ListView::positionViewAtBeginning()
    \qmlmethod QtQuick1::ListView::positionViewAtEnd()
    \since Quick 1.1

    Positions the view at the beginning or end, taking into account any header or footer.

    It is not recommended to use \l {Flickable::}{contentX} or \l {Flickable::}{contentY} to position the view
    at a particular index.  This is unreliable since removing items from the start
    of the list does not cause all other items to be repositioned, and because
    the actual start of the view can vary based on the size of the delegates.

    \bold Note: methods should only be called after the Component has completed.  To position
    the view at startup, this method should be called by Component.onCompleted.  For
    example, to position the view at the end on startup:

    \code
    Component.onCompleted: positionViewAtEnd()
    \endcode
*/
void QDeclarative1ListView::positionViewAtBeginning()
{
    Q_D(QDeclarative1ListView);
    if (!d->isValid())
        return;
    d->positionViewAtIndex(-1, Beginning);
}

void QDeclarative1ListView::positionViewAtEnd()
{
    Q_D(QDeclarative1ListView);
    if (!d->isValid())
        return;
    d->positionViewAtIndex(d->model->count(), End);
}

/*!
    \qmlmethod int QtQuick1::ListView::indexAt(int x, int y)

    Returns the index of the visible item containing the point \a x, \a y in content
    coordinates.  If there is no item at the point specified, or the item is
    not visible -1 is returned.

    If the item is outside the visible area, -1 is returned, regardless of
    whether an item will exist at that point when scrolled into view.

    \bold Note: methods should only be called after the Component has completed.
*/
int QDeclarative1ListView::indexAt(qreal x, qreal y) const
{
    Q_D(const QDeclarative1ListView);
    for (int i = 0; i < d->visibleItems.count(); ++i) {
        const FxListItem1 *listItem = d->visibleItems.at(i);
        if(listItem->contains(x, y))
            return listItem->index;
    }

    return -1;
}

void QDeclarative1ListView::componentComplete()
{
    Q_D(QDeclarative1ListView);
    QDeclarative1Flickable::componentComplete();
    updateSections();
    d->updateHeader();
    d->updateFooter();
    if (d->isValid()) {
        refill();
        d->moveReason = QDeclarative1ListViewPrivate::SetIndex;
        if (d->currentIndex < 0 && !d->currentIndexCleared)
            d->updateCurrent(0);
        else
            d->updateCurrent(d->currentIndex);
        if (d->highlight && d->currentItem) {
            if (d->autoHighlight)
                d->highlight->setPosition(d->currentItem->position());
            d->updateTrackedItem();
        }
        d->moveReason = QDeclarative1ListViewPrivate::Other;
        d->fixupPosition();
    }
}

void QDeclarative1ListView::updateSections()
{
    Q_D(QDeclarative1ListView);
    if (isComponentComplete() && d->model) {
        QList<QByteArray> roles;
        if (d->sectionCriteria && !d->sectionCriteria->property().isEmpty())
            roles << d->sectionCriteria->property().toUtf8();
        d->model->setWatchedRoles(roles);
        d->updateSections();
        if (d->itemCount)
            d->layout();
    }
}

void QDeclarative1ListView::refill()
{
    Q_D(QDeclarative1ListView);
    if (d->isRightToLeft())
        d->refill(-d->position()-d->size()+1, -d->position());
    else
        d->refill(d->position(), d->position()+d->size()-1);
}

void QDeclarative1ListView::trackedPositionChanged()
{
    Q_D(QDeclarative1ListView);
    if (!d->trackedItem || !d->currentItem)
        return;
    if (d->moveReason == QDeclarative1ListViewPrivate::SetIndex) {
        qreal trackedPos = qCeil(d->trackedItem->position());
        qreal trackedSize = d->trackedItem->size();
        if (d->trackedItem != d->currentItem) {
            trackedSize += d->currentItem->sectionSize();
        }
        qreal viewPos;
        qreal highlightStart;
        qreal highlightEnd;
        if (d->isRightToLeft()) {
            viewPos = -d->position()-d->size();
            highlightStart = d->highlightRangeStartValid ? d->size()-d->highlightRangeEnd : d->highlightRangeStart;
            highlightEnd = d->highlightRangeEndValid ? d->size()-d->highlightRangeStart : d->highlightRangeEnd;
        } else {
            viewPos = d->position();
            highlightStart = d->highlightRangeStart;
            highlightEnd = d->highlightRangeEnd;
        }
        qreal pos = viewPos;
        if (d->haveHighlightRange) {
            if (d->highlightRange == StrictlyEnforceRange) {
                if (trackedPos > pos + highlightEnd - d->trackedItem->size())
                    pos = trackedPos - highlightEnd + d->trackedItem->size();
                if (trackedPos < pos + highlightStart)
                    pos = trackedPos - highlightStart;
            } else {
                if (trackedPos < d->startPosition() + highlightStart) {
                    pos = d->startPosition();
                } else if (d->trackedItem->endPosition() > d->endPosition() - d->size() + highlightEnd) {
                    pos = d->endPosition() - d->size() + 1;
                    if (pos < d->startPosition())
                        pos = d->startPosition();
                } else {
                    if (trackedPos > pos + highlightEnd - trackedSize)
                        pos = trackedPos - highlightEnd + trackedSize;
                    if (trackedPos < pos + highlightStart)
                        pos = trackedPos - highlightStart;
                }
            }
        } else {
            if (trackedPos < viewPos && d->currentItem->position() < viewPos) {
                pos = d->currentItem->position() < trackedPos ? trackedPos : d->currentItem->position();
            } else if (d->trackedItem->endPosition() >= viewPos + d->size()
                        && d->currentItem->endPosition() >= viewPos + d->size()) {
                if (d->trackedItem->endPosition() <= d->currentItem->endPosition()) {
                    pos = d->trackedItem->endPosition() - d->size() + 1;
                     if (trackedSize > d->size())
                        pos = trackedPos;
                } else {
                    pos = d->currentItem->endPosition() - d->size() + 1;
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

void QDeclarative1ListView::itemsInserted(int modelIndex, int count)
{
    Q_D(QDeclarative1ListView);
    if (!isComponentComplete() || !d->model || !d->model->isValid())
        return;
    d->updateUnrequestedIndexes();
    d->moveReason = QDeclarative1ListViewPrivate::Other;

    qreal tempPos = d->isRightToLeft() ? -d->position()-d->size() : d->position();
    int index = d->visibleItems.count() ? d->mapFromModel(modelIndex) : 0;

    if (index < 0) {
        int i = d->visibleItems.count() - 1;
        while (i > 0 && d->visibleItems.at(i)->index == -1)
            --i;
        if (i == 0 && d->visibleItems.first()->index == -1) {
            // there are no visible items except items marked for removal
            index = d->visibleItems.count();
        } else if (d->visibleItems.at(i)->index + 1 == modelIndex
            && d->visibleItems.at(i)->endPosition() < d->buffer+tempPos+d->size()-1) {
            // Special case of appending an item to the model.
            index = d->visibleItems.count();
        } else {
            if (modelIndex < d->visibleIndex) {
                // Insert before visible items
                d->visibleIndex += count;
                for (int i = 0; i < d->visibleItems.count(); ++i) {
                    FxListItem1 *listItem = d->visibleItems.at(i);
                    if (listItem->index != -1 && listItem->index >= modelIndex)
                        listItem->index += count;
                }
            }
            if (d->currentIndex >= modelIndex) {
                // adjust current item index
                d->currentIndex += count;
                if (d->currentItem)
                    d->currentItem->index = d->currentIndex;
                emit currentIndexChanged();
            }
            d->scheduleLayout();
            d->itemCount += count;
            emit countChanged();
            return;
        }
    }

    // index can be the next item past the end of the visible items list (i.e. appended)
    int pos = 0;
    if (d->visibleItems.count()) {
        pos = index < d->visibleItems.count() ? d->visibleItems.at(index)->position()
                                                : d->visibleItems.last()->endPosition()+d->spacing+1;
    } else if (d->itemCount == 0 && d->header) {
        pos = d->header->size();
    }

    int initialPos = pos;
    int diff = 0;
    QList<FxListItem1*> added;
    bool addedVisible = false;
    FxListItem1 *firstVisible = d->firstVisibleItem();
    if (firstVisible && pos < firstVisible->position()) {
        // Insert items before the visible item.
        int insertionIdx = index;
        int i = 0;
        int from = tempPos - d->buffer;
        for (i = count-1; i >= 0 && pos > from; --i) {
            if (!addedVisible) {
                d->scheduleLayout();
                addedVisible = true;
            }
            FxListItem1 *item = d->createItem(modelIndex + i);
            d->visibleItems.insert(insertionIdx, item);
            pos -= item->size() + d->spacing;
            item->setPosition(pos);
            index++;
        }
        if (i >= 0) {
            // If we didn't insert all our new items - anything
            // before the current index is not visible - remove it.
            while (insertionIdx--) {
                FxListItem1 *item = d->visibleItems.takeFirst();
                if (item->index != -1)
                    d->visibleIndex++;
                d->releaseItem(item);
            }
        } else {
            // adjust pos of items before inserted items.
            for (int i = insertionIdx-1; i >= 0; i--) {
                FxListItem1 *listItem = d->visibleItems.at(i);
                listItem->setPosition(listItem->position() - (initialPos - pos));
            }
        }
    } else {
        int i = 0;
        int to = d->buffer+tempPos+d->size();
        for (i = 0; i < count && pos <= to; ++i) {
            if (!addedVisible) {
                d->scheduleLayout();
                addedVisible = true;
            }
            FxListItem1 *item = d->createItem(modelIndex + i);
            d->visibleItems.insert(index, item);
            item->setPosition(pos);
            added.append(item);
            pos += item->size() + d->spacing;
            ++index;
        }
        if (i != count) {
            // We didn't insert all our new items, which means anything
            // beyond the current index is not visible - remove it.
            while (d->visibleItems.count() > index)
                d->releaseItem(d->visibleItems.takeLast());
        }
        diff = pos - initialPos;
    }
    if (d->itemCount && d->currentIndex >= modelIndex) {
        // adjust current item index
        d->currentIndex += count;
        if (d->currentItem) {
            d->currentItem->index = d->currentIndex;
            d->currentItem->setPosition(d->currentItem->position() + diff);
        }
        emit currentIndexChanged();
    } else if (!d->itemCount && (!d->currentIndex || (d->currentIndex < 0 && !d->currentIndexCleared))) {
        d->updateCurrent(0);
    }
    // Update the indexes of the following visible items.
    for (; index < d->visibleItems.count(); ++index) {
        FxListItem1 *listItem = d->visibleItems.at(index);
        if (d->currentItem && listItem->item != d->currentItem->item)
            listItem->setPosition(listItem->position() + diff);
        if (listItem->index != -1)
            listItem->index += count;
    }
    // everything is in order now - emit add() signal
    for (int j = 0; j < added.count(); ++j)
        added.at(j)->attached->emitAdd();

    d->updateSections();
    d->itemCount += count;
    emit countChanged();
}

void QDeclarative1ListView::itemsRemoved(int modelIndex, int count)
{
    Q_D(QDeclarative1ListView);
    if (!isComponentComplete() || !d->model || !d->model->isValid())
        return;
    d->moveReason = QDeclarative1ListViewPrivate::Other;
    d->updateUnrequestedIndexes();
    d->itemCount -= count;

    FxListItem1 *firstVisible = d->firstVisibleItem();
    int preRemovedSize = 0;
    bool removedVisible = false;
    // Remove the items from the visible list, skipping anything already marked for removal
    QList<FxListItem1*>::Iterator it = d->visibleItems.begin();
    while (it != d->visibleItems.end()) {
        FxListItem1 *item = *it;
        if (item->index == -1 || item->index < modelIndex) {
            // already removed, or before removed items
            ++it;
        } else if (item->index >= modelIndex + count) {
            // after removed items
            item->index -= count;
            ++it;
        } else {
            // removed item
            if (!removedVisible) {
                d->scheduleLayout();
                removedVisible = true;
            }
            item->attached->emitRemove();
            if (item->attached->delayRemove()) {
                item->index = -1;
                connect(item->attached, SIGNAL(delayRemoveChanged()), this, SLOT(destroyRemoved()), Qt::QueuedConnection);
                ++it;
            } else {
                if (item == firstVisible)
                    firstVisible = 0;
                if (firstVisible && item->position() < firstVisible->position())
                    preRemovedSize += item->size();
                it = d->visibleItems.erase(it);
                d->releaseItem(item);
            }
        }
    }

    if (firstVisible && d->visibleItems.first() != firstVisible)
        d->visibleItems.first()->setPosition(d->visibleItems.first()->position() + preRemovedSize);

    // update visibleIndex
    bool haveVisibleIndex = false;
    for (it = d->visibleItems.begin(); it != d->visibleItems.end(); ++it) {
        if ((*it)->index != -1) {
            d->visibleIndex = (*it)->index;
            haveVisibleIndex = true;
            break;
        }
    }

    // fix current
    if (d->currentIndex >= modelIndex + count) {
        d->currentIndex -= count;
        if (d->currentItem)
            d->currentItem->index -= count;
        emit currentIndexChanged();
    } else if (d->currentIndex >= modelIndex && d->currentIndex < modelIndex + count) {
        // current item has been removed.
        d->currentItem->attached->setIsCurrentItem(false);
        d->releaseItem(d->currentItem);
        d->currentItem = 0;
        d->currentIndex = -1;
        if (d->itemCount)
            d->updateCurrent(qMin(modelIndex, d->itemCount-1));
        else
            emit currentIndexChanged();
    }

    if (!haveVisibleIndex) {
        d->timeline.clear();
        if (removedVisible && d->itemCount == 0) {
            d->visibleIndex = 0;
            d->visiblePos = d->header ? d->header->size() : 0;
            d->setPosition(0);
            d->updateHeader();
            d->updateFooter();
            update();
        } else {
            if (modelIndex < d->visibleIndex)
                d->visibleIndex = modelIndex+1;
            d->visibleIndex = qMax(qMin(d->visibleIndex, d->itemCount-1), 0);
        }
    }

    d->updateSections();
    emit countChanged();
}

void QDeclarative1ListView::destroyRemoved()
{
    Q_D(QDeclarative1ListView);
    for (QList<FxListItem1*>::Iterator it = d->visibleItems.begin();
            it != d->visibleItems.end();) {
        FxListItem1 *listItem = *it;
        if (listItem->index == -1 && listItem->attached->delayRemove() == false) {
            d->releaseItem(listItem);
            it = d->visibleItems.erase(it);
        } else {
            ++it;
        }
    }

    // Correct the positioning of the items
    d->updateSections();
    d->layout();
}

void QDeclarative1ListView::itemsMoved(int from, int to, int count)
{
    Q_D(QDeclarative1ListView);
    if (!isComponentComplete() || !d->isValid())
        return;
    d->updateUnrequestedIndexes();

    if (d->visibleItems.isEmpty()) {
        refill();
        return;
    }

    d->moveReason = QDeclarative1ListViewPrivate::Other;
    FxListItem1 *firstVisible = d->firstVisibleItem();
    qreal firstItemPos = firstVisible->position();
    QHash<int,FxListItem1*> moved;
    int moveBy = 0;

    QList<FxListItem1*>::Iterator it = d->visibleItems.begin();
    while (it != d->visibleItems.end()) {
        FxListItem1 *item = *it;
        if (item->index >= from && item->index < from + count) {
            // take the items that are moving
            item->index += (to-from);
            moved.insert(item->index, item);
            if (item->position() < firstItemPos)
                moveBy += item->size();
            it = d->visibleItems.erase(it);
        } else {
            // move everything after the moved items.
            if (item->index > from && item->index != -1)
                item->index -= count;
            ++it;
        }
    }

    int remaining = count;
    int endIndex = d->visibleIndex;
    it = d->visibleItems.begin();
    while (it != d->visibleItems.end()) {
        FxListItem1 *item = *it;
        if (remaining && item->index >= to && item->index < to + count) {
            // place items in the target position, reusing any existing items
            FxListItem1 *movedItem = moved.take(item->index);
            if (!movedItem)
                movedItem = d->createItem(item->index);
            if (item->index <= firstVisible->index)
                moveBy -= movedItem->size();
            it = d->visibleItems.insert(it, movedItem);
            ++it;
            --remaining;
        } else {
            if (item->index != -1) {
                if (item->index >= to) {
                    // update everything after the moved items.
                    item->index += count;
                }
                endIndex = item->index;
            }
            ++it;
        }
    }

    // If we have moved items to the end of the visible items
    // then add any existing moved items that we have
    while (FxListItem1 *item = moved.take(endIndex+1)) {
        d->visibleItems.append(item);
        ++endIndex;
    }

    // update visibleIndex
    for (it = d->visibleItems.begin(); it != d->visibleItems.end(); ++it) {
        if ((*it)->index != -1) {
            d->visibleIndex = (*it)->index;
            break;
        }
    }

    // Fix current index
    if (d->currentIndex >= 0 && d->currentItem) {
        int oldCurrent = d->currentIndex;
        d->currentIndex = d->model->indexOf(d->currentItem->item, this);
        if (oldCurrent != d->currentIndex) {
            d->currentItem->index = d->currentIndex;
            emit currentIndexChanged();
        }
    }

    // Whatever moved items remain are no longer visible items.
    while (moved.count()) {
        int idx = moved.begin().key();
        FxListItem1 *item = moved.take(idx);
        if (d->currentItem && item->item == d->currentItem->item)
            item->setPosition(d->positionAt(idx));
        d->releaseItem(item);
    }

    // Ensure we don't cause an ugly list scroll.
    d->visibleItems.first()->setPosition(d->visibleItems.first()->position() + moveBy);

    d->updateSections();
    d->layout();
}

void QDeclarative1ListView::itemsChanged(int, int)
{
    Q_D(QDeclarative1ListView);
    d->updateSections();
    d->layout();
}

void QDeclarative1ListView::modelReset()
{
    Q_D(QDeclarative1ListView);
    d->moveReason = QDeclarative1ListViewPrivate::SetIndex;
    d->regenerate();
    if (d->highlight && d->currentItem) {
        if (d->autoHighlight)
            d->highlight->setPosition(d->currentItem->position());
        d->updateTrackedItem();
    }
    d->moveReason = QDeclarative1ListViewPrivate::Other;
    emit countChanged();
}

void QDeclarative1ListView::createdItem(int index, QDeclarativeItem *item)
{
    Q_D(QDeclarative1ListView);
    if (d->requestedIndex != index) {
        item->setParentItem(contentItem());
        d->unrequestedItems.insert(item, index);
        if (d->orient == QDeclarative1ListView::Vertical) {
            item->setY(d->positionAt(index));
        } else {
            if (d->isRightToLeft())
                item->setX(-d->positionAt(index)-item->width());
            else
                item->setX(d->positionAt(index));
        }
    }
}

void QDeclarative1ListView::destroyingItem(QDeclarativeItem *item)
{
    Q_D(QDeclarative1ListView);
    d->unrequestedItems.remove(item);
}

void QDeclarative1ListView::animStopped()
{
    Q_D(QDeclarative1ListView);
    d->bufferMode = QDeclarative1ListViewPrivate::NoBuffer;
    if (d->haveHighlightRange && d->highlightRange == QDeclarative1ListView::StrictlyEnforceRange)
        d->updateHighlight();
}

QDeclarative1ListViewAttached *QDeclarative1ListView::qmlAttachedProperties(QObject *obj)
{
    return new QDeclarative1ListViewAttached(obj);
}



QT_END_NAMESPACE
