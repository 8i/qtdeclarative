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

#include "qquickitem.h"

#include "qquickcanvas.h"
#include <QtDeclarative/qjsengine.h>
#include "qquickcanvas_p.h"

#include "qquickevents_p_p.h"

#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtGui/qpen.h>
#include <QtGui/qcursor.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qinputpanel.h>
#include <QtCore/qdebug.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qnumeric.h>

#include <private/qdeclarativeengine_p.h>
#include <private/qdeclarativestategroup_p.h>
#include <private/qdeclarativeopenmetaobject_p.h>
#include <private/qdeclarativestate_p.h>
#include <private/qlistmodelinterface_p.h>
#include <private/qquickitem_p.h>

#include <float.h>

// XXX todo Readd parentNotifier for faster parent bindings
// XXX todo Check that elements that create items handle memory correctly after visual ownership change

QT_BEGIN_NAMESPACE

/*!
    \qmlclass Transform QQuickTransform
    \inqmlmodule QtQuick 2
    \ingroup qml-transform-elements
    \brief The Transform elements provide a way of building advanced transformations on Items.

    The Transform element is a base type which cannot be instantiated directly.
    The following concrete Transform types are available:

    \list
    \o \l Rotation
    \o \l Scale
    \o \l Translate
    \endlist

    The Transform elements let you create and control advanced transformations that can be configured
    independently using specialized properties.

    You can assign any number of Transform elements to an \l Item. Each Transform is applied in order,
    one at a time.
*/

/*!
    \qmlclass Translate QQuickTranslate
    \inqmlmodule QtQuick 2
    \ingroup qml-transform-elements
    \brief The Translate object provides a way to move an Item without changing its x or y properties.

    The Translate object provides independent control over position in addition to the Item's x and y properties.

    The following example moves the Y axis of the \l Rectangle elements while still allowing the \l Row element
    to lay the items out as if they had not been transformed:
    \qml
    import QtQuick 1.0

    Row {
        Rectangle {
            width: 100; height: 100
            color: "blue"
            transform: Translate { y: 20 }
        }
        Rectangle {
            width: 100; height: 100
            color: "red"
            transform: Translate { y: -20 }
        }
    }
    \endqml

    \image translate.png
*/

/*!
    \qmlproperty real QtQuick2::Translate::x

    The translation along the X axis.
*/

/*!
    \qmlproperty real QtQuick2::Translate::y

    The translation along the Y axis.
*/

/*!
    \qmlclass Scale QQuickScale
    \inqmlmodule QtQuick 2
    \ingroup qml-transform-elements
    \brief The Scale element provides a way to scale an Item.

    The Scale element gives more control over scaling than using \l Item's \l{Item::scale}{scale} property. Specifically,
    it allows a different scale for the x and y axes, and allows the scale to be relative to an
    arbitrary point.

    The following example scales the X axis of the Rectangle, relative to its interior point 25, 25:
    \qml
    Rectangle {
        width: 100; height: 100
        color: "blue"
        transform: Scale { origin.x: 25; origin.y: 25; xScale: 3}
    }
    \endqml

    \sa Rotation, Translate
*/

/*!
    \qmlproperty real QtQuick2::Scale::origin.x
    \qmlproperty real QtQuick2::Scale::origin.y

    The point that the item is scaled from (i.e., the point that stays fixed relative to the parent as
    the rest of the item grows). By default the origin is 0, 0.
*/

/*!
    \qmlproperty real QtQuick2::Scale::xScale

    The scaling factor for the X axis.
*/

/*!
    \qmlproperty real QtQuick2::Scale::yScale

    The scaling factor for the Y axis.
*/

/*!
    \qmlclass Rotation QQuickRotation
    \inqmlmodule QtQuick 2
    \ingroup qml-transform-elements
    \brief The Rotation object provides a way to rotate an Item.

    The Rotation object gives more control over rotation than using \l Item's \l{Item::rotation}{rotation} property.
    Specifically, it allows (z axis) rotation to be relative to an arbitrary point.

    The following example rotates a Rectangle around its interior point 25, 25:
    \qml
    Rectangle {
        width: 100; height: 100
        color: "blue"
        transform: Rotation { origin.x: 25; origin.y: 25; angle: 45}
    }
    \endqml

    Rotation also provides a way to specify 3D-like rotations for Items. For these types of
    rotations you must specify the axis to rotate around in addition to the origin point.

    The following example shows various 3D-like rotations applied to an \l Image.
    \snippet doc/src/snippets/declarative/rotation.qml 0

    \image axisrotation.png

    \sa {declarative/ui-components/dialcontrol}{Dial Control example}, {declarative/toys/clocks}{Clocks example}
*/

/*!
    \qmlproperty real QtQuick2::Rotation::origin.x
    \qmlproperty real QtQuick2::Rotation::origin.y

    The origin point of the rotation (i.e., the point that stays fixed relative to the parent as
    the rest of the item rotates). By default the origin is 0, 0.
*/

/*!
    \qmlproperty real QtQuick2::Rotation::axis.x
    \qmlproperty real QtQuick2::Rotation::axis.y
    \qmlproperty real QtQuick2::Rotation::axis.z

    The axis to rotate around. For simple (2D) rotation around a point, you do not need to specify an axis,
    as the default axis is the z axis (\c{ axis { x: 0; y: 0; z: 1 } }).

    For a typical 3D-like rotation you will usually specify both the origin and the axis.

    \image 3d-rotation-axis.png
*/

/*!
    \qmlproperty real QtQuick2::Rotation::angle

    The angle to rotate, in degrees clockwise.
*/

QQuickTransformPrivate::QQuickTransformPrivate()
{
}

QQuickTransform::QQuickTransform(QObject *parent)
: QObject(*(new QQuickTransformPrivate), parent)
{
}

QQuickTransform::QQuickTransform(QQuickTransformPrivate &dd, QObject *parent)
: QObject(dd, parent)
{
}

QQuickTransform::~QQuickTransform()
{
    Q_D(QQuickTransform);
    for (int ii = 0; ii < d->items.count(); ++ii) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(d->items.at(ii));
        p->transforms.removeOne(this);
        p->dirty(QQuickItemPrivate::Transform);
    }
}

void QQuickTransform::update()
{
    Q_D(QQuickTransform);
    for (int ii = 0; ii < d->items.count(); ++ii) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(d->items.at(ii));
        p->dirty(QQuickItemPrivate::Transform);
    }
}

QQuickContents::QQuickContents(QQuickItem *item)
: m_item(item), m_x(0), m_y(0), m_width(0), m_height(0)
{
}

QQuickContents::~QQuickContents()
{
    QList<QQuickItem *> children = m_item->childItems();
    for (int i = 0; i < children.count(); ++i) {
        QQuickItem *child = children.at(i);
        QQuickItemPrivate::get(child)->removeItemChangeListener(this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Destroyed);
    }
}

bool QQuickContents::calcHeight(QQuickItem *changed)
{
    qreal oldy = m_y;
    qreal oldheight = m_height;

    if (changed) {
        qreal top = oldy;
        qreal bottom = oldy + oldheight;
        qreal y = changed->y();
        if (y + changed->height() > bottom)
            bottom = y + changed->height();
        if (y < top)
            top = y;
        m_y = top;
        m_height = bottom - top;
    } else {
        qreal top = FLT_MAX;
        qreal bottom = 0;
        QList<QQuickItem *> children = m_item->childItems();
        for (int i = 0; i < children.count(); ++i) {
            QQuickItem *child = children.at(i);
            qreal y = child->y();
            if (y + child->height() > bottom)
                bottom = y + child->height();
            if (y < top)
                top = y;
        }
        if (!children.isEmpty())
            m_y = top;
        m_height = qMax(bottom - top, qreal(0.0));
    }

    return (m_height != oldheight || m_y != oldy);
}

bool QQuickContents::calcWidth(QQuickItem *changed)
{
    qreal oldx = m_x;
    qreal oldwidth = m_width;

    if (changed) {
        qreal left = oldx;
        qreal right = oldx + oldwidth;
        qreal x = changed->x();
        if (x + changed->width() > right)
            right = x + changed->width();
        if (x < left)
            left = x;
        m_x = left;
        m_width = right - left;
    } else {
        qreal left = FLT_MAX;
        qreal right = 0;
        QList<QQuickItem *> children = m_item->childItems();
        for (int i = 0; i < children.count(); ++i) {
            QQuickItem *child = children.at(i);
            qreal x = child->x();
            if (x + child->width() > right)
                right = x + child->width();
            if (x < left)
                left = x;
        }
        if (!children.isEmpty())
            m_x = left;
        m_width = qMax(right - left, qreal(0.0));
    }

    return (m_width != oldwidth || m_x != oldx);
}

void QQuickContents::complete()
{
    QQuickItemPrivate::get(m_item)->addItemChangeListener(this, QQuickItemPrivate::Children);

    QList<QQuickItem *> children = m_item->childItems();
    for (int i = 0; i < children.count(); ++i) {
        QQuickItem *child = children.at(i);
        QQuickItemPrivate::get(child)->addItemChangeListener(this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Destroyed);
        //###what about changes to visibility?
    }
    calcGeometry();
}

void QQuickContents::updateRect()
{
    QQuickItemPrivate::get(m_item)->emitChildrenRectChanged(rectF());
}

void QQuickContents::itemGeometryChanged(QQuickItem *changed, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(changed)
    bool wChanged = false;
    bool hChanged = false;
    //### we can only pass changed if the left edge has moved left, or the right edge has moved right
    if (newGeometry.width() != oldGeometry.width() || newGeometry.x() != oldGeometry.x())
        wChanged = calcWidth(/*changed*/);
    if (newGeometry.height() != oldGeometry.height() || newGeometry.y() != oldGeometry.y())
        hChanged = calcHeight(/*changed*/);
    if (wChanged || hChanged)
        updateRect();
}

void QQuickContents::itemDestroyed(QQuickItem *item)
{
    if (item)
        QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Destroyed);
    calcGeometry();
}

void QQuickContents::itemChildRemoved(QQuickItem *, QQuickItem *item)
{
    if (item)
        QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Destroyed);
    calcGeometry();
}

void QQuickContents::itemChildAdded(QQuickItem *, QQuickItem *item)
{
    if (item)
        QQuickItemPrivate::get(item)->addItemChangeListener(this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Destroyed);
    calcGeometry(item);
}

QQuickItemKeyFilter::QQuickItemKeyFilter(QQuickItem *item)
: m_processPost(false), m_next(0)
{
    QQuickItemPrivate *p = item?QQuickItemPrivate::get(item):0;
    if (p) {
        m_next = p->keyHandler;
        p->keyHandler = this;
    }
}

QQuickItemKeyFilter::~QQuickItemKeyFilter()
{
}

void QQuickItemKeyFilter::keyPressed(QKeyEvent *event, bool post)
{
    if (m_next) m_next->keyPressed(event, post);
}

void QQuickItemKeyFilter::keyReleased(QKeyEvent *event, bool post)
{
    if (m_next) m_next->keyReleased(event, post);
}

void QQuickItemKeyFilter::inputMethodEvent(QInputMethodEvent *event, bool post)
{
    if (m_next)
        m_next->inputMethodEvent(event, post);
    else
        event->ignore();
}

QVariant QQuickItemKeyFilter::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (m_next) return m_next->inputMethodQuery(query);
    return QVariant();
}

void QQuickItemKeyFilter::componentComplete()
{
    if (m_next) m_next->componentComplete();
}
/*!
    \qmlclass KeyNavigation QQuickKeyNavigationAttached
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-interaction-elements
    \brief The KeyNavigation attached property supports key navigation by arrow keys.

    Key-based user interfaces commonly allow the use of arrow keys to navigate between
    focusable items.  The KeyNavigation attached property enables this behavior by providing a
    convenient way to specify the item that should gain focus when an arrow or tab key is pressed.

    The following example provides key navigation for a 2x2 grid of items:

    \snippet doc/src/snippets/declarative/keynavigation.qml 0

    The top-left item initially receives focus by setting \l {Item::}{focus} to
    \c true. When an arrow key is pressed, the focus will move to the
    appropriate item, as defined by the value that has been set for
    the KeyNavigation \l left, \l right, \l up or \l down properties.

    Note that if a KeyNavigation attached property receives the key press and release
    events for a requested arrow or tab key, the event is accepted and does not
    propagate any further.

    By default, KeyNavigation receives key events after the item to which it is attached.
    If the item accepts the key event, the KeyNavigation attached property will not
    receive an event for that key.  Setting the \l priority property to
    \c KeyNavigation.BeforeItem allows the event to be used for key navigation
    before the item, rather than after.

    If item to which the focus is switching is not enabled or visible, an attempt will
    be made to skip this item and focus on the next. This is possible if there are
    a chain of items with the same KeyNavigation handler. If multiple items in a row are not enabled
    or visible, they will also be skipped.

    KeyNavigation will implicitly set the other direction to return focus to this item. So if you set
    \l left to another item, \l right will be set on that item's KeyNavigation to set focus back to this
    item. However, if that item's KeyNavigation has had right explicitly set then no change will occur.
    This means that the above example could have been written, with the same behaviour, without specifing
    KeyNavigation.right or KeyNavigation.down for any of the items.

    \sa {Keys}{Keys attached property}
*/

/*!
    \qmlproperty Item QtQuick2::KeyNavigation::left
    \qmlproperty Item QtQuick2::KeyNavigation::right
    \qmlproperty Item QtQuick2::KeyNavigation::up
    \qmlproperty Item QtQuick2::KeyNavigation::down
    \qmlproperty Item QtQuick2::KeyNavigation::tab
    \qmlproperty Item QtQuick2::KeyNavigation::backtab

    These properties hold the item to assign focus to
    when the left, right, up or down cursor keys, or the
    tab key are pressed.
*/

/*!
    \qmlproperty Item QtQuick2::KeyNavigation::tab
    \qmlproperty Item QtQuick2::KeyNavigation::backtab

    These properties hold the item to assign focus to
    when the Tab key or Shift+Tab key combination (Backtab) are pressed.
*/

QQuickKeyNavigationAttached::QQuickKeyNavigationAttached(QObject *parent)
: QObject(*(new QQuickKeyNavigationAttachedPrivate), parent),
  QQuickItemKeyFilter(qobject_cast<QQuickItem*>(parent))
{
    m_processPost = true;
}

QQuickKeyNavigationAttached *
QQuickKeyNavigationAttached::qmlAttachedProperties(QObject *obj)
{
    return new QQuickKeyNavigationAttached(obj);
}

QQuickItem *QQuickKeyNavigationAttached::left() const
{
    Q_D(const QQuickKeyNavigationAttached);
    return d->left;
}

void QQuickKeyNavigationAttached::setLeft(QQuickItem *i)
{
    Q_D(QQuickKeyNavigationAttached);
    if (d->left == i)
        return;
    d->left = i;
    d->leftSet = true;
    QQuickKeyNavigationAttached* other =
            qobject_cast<QQuickKeyNavigationAttached*>(qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(i));
    if (other && !other->d_func()->rightSet){
        other->d_func()->right = qobject_cast<QQuickItem*>(parent());
        emit other->rightChanged();
    }
    emit leftChanged();
}

QQuickItem *QQuickKeyNavigationAttached::right() const
{
    Q_D(const QQuickKeyNavigationAttached);
    return d->right;
}

void QQuickKeyNavigationAttached::setRight(QQuickItem *i)
{
    Q_D(QQuickKeyNavigationAttached);
    if (d->right == i)
        return;
    d->right = i;
    d->rightSet = true;
    QQuickKeyNavigationAttached* other =
            qobject_cast<QQuickKeyNavigationAttached*>(qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(i));
    if (other && !other->d_func()->leftSet){
        other->d_func()->left = qobject_cast<QQuickItem*>(parent());
        emit other->leftChanged();
    }
    emit rightChanged();
}

QQuickItem *QQuickKeyNavigationAttached::up() const
{
    Q_D(const QQuickKeyNavigationAttached);
    return d->up;
}

void QQuickKeyNavigationAttached::setUp(QQuickItem *i)
{
    Q_D(QQuickKeyNavigationAttached);
    if (d->up == i)
        return;
    d->up = i;
    d->upSet = true;
    QQuickKeyNavigationAttached* other =
            qobject_cast<QQuickKeyNavigationAttached*>(qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(i));
    if (other && !other->d_func()->downSet){
        other->d_func()->down = qobject_cast<QQuickItem*>(parent());
        emit other->downChanged();
    }
    emit upChanged();
}

QQuickItem *QQuickKeyNavigationAttached::down() const
{
    Q_D(const QQuickKeyNavigationAttached);
    return d->down;
}

void QQuickKeyNavigationAttached::setDown(QQuickItem *i)
{
    Q_D(QQuickKeyNavigationAttached);
    if (d->down == i)
        return;
    d->down = i;
    d->downSet = true;
    QQuickKeyNavigationAttached* other =
            qobject_cast<QQuickKeyNavigationAttached*>(qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(i));
    if (other && !other->d_func()->upSet) {
        other->d_func()->up = qobject_cast<QQuickItem*>(parent());
        emit other->upChanged();
    }
    emit downChanged();
}

QQuickItem *QQuickKeyNavigationAttached::tab() const
{
    Q_D(const QQuickKeyNavigationAttached);
    return d->tab;
}

void QQuickKeyNavigationAttached::setTab(QQuickItem *i)
{
    Q_D(QQuickKeyNavigationAttached);
    if (d->tab == i)
        return;
    d->tab = i;
    d->tabSet = true;
    QQuickKeyNavigationAttached* other =
            qobject_cast<QQuickKeyNavigationAttached*>(qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(i));
    if (other && !other->d_func()->backtabSet) {
        other->d_func()->backtab = qobject_cast<QQuickItem*>(parent());
        emit other->backtabChanged();
    }
    emit tabChanged();
}

QQuickItem *QQuickKeyNavigationAttached::backtab() const
{
    Q_D(const QQuickKeyNavigationAttached);
    return d->backtab;
}

void QQuickKeyNavigationAttached::setBacktab(QQuickItem *i)
{
    Q_D(QQuickKeyNavigationAttached);
    if (d->backtab == i)
        return;
    d->backtab = i;
    d->backtabSet = true;
    QQuickKeyNavigationAttached* other =
            qobject_cast<QQuickKeyNavigationAttached*>(qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(i));
    if (other && !other->d_func()->tabSet) {
        other->d_func()->tab = qobject_cast<QQuickItem*>(parent());
        emit other->tabChanged();
    }
    emit backtabChanged();
}

/*!
    \qmlproperty enumeration QtQuick2::KeyNavigation::priority

    This property determines whether the keys are processed before
    or after the attached item's own key handling.

    \list
    \o KeyNavigation.BeforeItem - process the key events before normal
    item key processing.  If the event is used for key navigation, it will be accepted and will not
    be passed on to the item.
    \o KeyNavigation.AfterItem (default) - process the key events after normal item key
    handling.  If the item accepts the key event it will not be
    handled by the KeyNavigation attached property handler.
    \endlist
*/
QQuickKeyNavigationAttached::Priority QQuickKeyNavigationAttached::priority() const
{
    return m_processPost ? AfterItem : BeforeItem;
}

void QQuickKeyNavigationAttached::setPriority(Priority order)
{
    bool processPost = order == AfterItem;
    if (processPost != m_processPost) {
        m_processPost = processPost;
        emit priorityChanged();
    }
}

void QQuickKeyNavigationAttached::keyPressed(QKeyEvent *event, bool post)
{
    Q_D(QQuickKeyNavigationAttached);
    event->ignore();

    if (post != m_processPost) {
        QQuickItemKeyFilter::keyPressed(event, post);
        return;
    }

    bool mirror = false;
    switch (event->key()) {
    case Qt::Key_Left: {
        if (QQuickItem *parentItem = qobject_cast<QQuickItem*>(parent()))
            mirror = QQuickItemPrivate::get(parentItem)->effectiveLayoutMirror;
        QQuickItem* leftItem = mirror ? d->right : d->left;
        if (leftItem) {
            setFocusNavigation(leftItem, mirror ? "right" : "left");
            event->accept();
        }
        break;
    }
    case Qt::Key_Right: {
        if (QQuickItem *parentItem = qobject_cast<QQuickItem*>(parent()))
            mirror = QQuickItemPrivate::get(parentItem)->effectiveLayoutMirror;
        QQuickItem* rightItem = mirror ? d->left : d->right;
        if (rightItem) {
            setFocusNavigation(rightItem, mirror ? "left" : "right");
            event->accept();
        }
        break;
    }
    case Qt::Key_Up:
        if (d->up) {
            setFocusNavigation(d->up, "up");
            event->accept();
        }
        break;
    case Qt::Key_Down:
        if (d->down) {
            setFocusNavigation(d->down, "down");
            event->accept();
        }
        break;
    case Qt::Key_Tab:
        if (d->tab) {
            setFocusNavigation(d->tab, "tab");
            event->accept();
        }
        break;
    case Qt::Key_Backtab:
        if (d->backtab) {
            setFocusNavigation(d->backtab, "backtab");
            event->accept();
        }
        break;
    default:
        break;
    }

    if (!event->isAccepted()) QQuickItemKeyFilter::keyPressed(event, post);
}

void QQuickKeyNavigationAttached::keyReleased(QKeyEvent *event, bool post)
{
    Q_D(QQuickKeyNavigationAttached);
    event->ignore();

    if (post != m_processPost) {
        QQuickItemKeyFilter::keyReleased(event, post);
        return;
    }

    bool mirror = false;
    switch (event->key()) {
    case Qt::Key_Left:
        if (QQuickItem *parentItem = qobject_cast<QQuickItem*>(parent()))
            mirror = QQuickItemPrivate::get(parentItem)->effectiveLayoutMirror;
        if (mirror ? d->right : d->left)
            event->accept();
        break;
    case Qt::Key_Right:
        if (QQuickItem *parentItem = qobject_cast<QQuickItem*>(parent()))
            mirror = QQuickItemPrivate::get(parentItem)->effectiveLayoutMirror;
        if (mirror ? d->left : d->right)
            event->accept();
        break;
    case Qt::Key_Up:
        if (d->up) {
            event->accept();
        }
        break;
    case Qt::Key_Down:
        if (d->down) {
            event->accept();
        }
        break;
    case Qt::Key_Tab:
        if (d->tab) {
            event->accept();
        }
        break;
    case Qt::Key_Backtab:
        if (d->backtab) {
            event->accept();
        }
        break;
    default:
        break;
    }

    if (!event->isAccepted()) QQuickItemKeyFilter::keyReleased(event, post);
}

void QQuickKeyNavigationAttached::setFocusNavigation(QQuickItem *currentItem, const char *dir)
{
    QQuickItem *initialItem = currentItem;
    bool isNextItem = false;
    do {
        isNextItem = false;
        if (currentItem->isVisible() && currentItem->isEnabled()) {
            currentItem->setFocus(true);
        } else {
            QObject *attached =
                qmlAttachedPropertiesObject<QQuickKeyNavigationAttached>(currentItem, false);
            if (attached) {
                QQuickItem *tempItem = qvariant_cast<QQuickItem*>(attached->property(dir));
                if (tempItem) {
                    currentItem = tempItem;
                    isNextItem = true;
                }
            }
        }
    }
    while (currentItem != initialItem && isNextItem);
}

const QQuickKeysAttached::SigMap QQuickKeysAttached::sigMap[] = {
    { Qt::Key_Left, "leftPressed" },
    { Qt::Key_Right, "rightPressed" },
    { Qt::Key_Up, "upPressed" },
    { Qt::Key_Down, "downPressed" },
    { Qt::Key_Tab, "tabPressed" },
    { Qt::Key_Backtab, "backtabPressed" },
    { Qt::Key_Asterisk, "asteriskPressed" },
    { Qt::Key_NumberSign, "numberSignPressed" },
    { Qt::Key_Escape, "escapePressed" },
    { Qt::Key_Return, "returnPressed" },
    { Qt::Key_Enter, "enterPressed" },
    { Qt::Key_Delete, "deletePressed" },
    { Qt::Key_Space, "spacePressed" },
    { Qt::Key_Back, "backPressed" },
    { Qt::Key_Cancel, "cancelPressed" },
    { Qt::Key_Select, "selectPressed" },
    { Qt::Key_Yes, "yesPressed" },
    { Qt::Key_No, "noPressed" },
    { Qt::Key_Context1, "context1Pressed" },
    { Qt::Key_Context2, "context2Pressed" },
    { Qt::Key_Context3, "context3Pressed" },
    { Qt::Key_Context4, "context4Pressed" },
    { Qt::Key_Call, "callPressed" },
    { Qt::Key_Hangup, "hangupPressed" },
    { Qt::Key_Flip, "flipPressed" },
    { Qt::Key_Menu, "menuPressed" },
    { Qt::Key_VolumeUp, "volumeUpPressed" },
    { Qt::Key_VolumeDown, "volumeDownPressed" },
    { 0, 0 }
};

bool QQuickKeysAttachedPrivate::isConnected(const char *signalName)
{
    return isSignalConnected(signalIndex(signalName));
}

/*!
    \qmlclass Keys QQuickKeysAttached
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-interaction-elements
    \brief The Keys attached property provides key handling to Items.

    All visual primitives support key handling via the Keys
    attached property.  Keys can be handled via the onPressed
    and onReleased signal properties.

    The signal properties have a \l KeyEvent parameter, named
    \e event which contains details of the event.  If a key is
    handled \e event.accepted should be set to true to prevent the
    event from propagating up the item hierarchy.

    \section1 Example Usage

    The following example shows how the general onPressed handler can
    be used to test for a certain key; in this case, the left cursor
    key:

    \snippet doc/src/snippets/declarative/keys/keys-pressed.qml key item

    Some keys may alternatively be handled via specific signal properties,
    for example \e onSelectPressed.  These handlers automatically set
    \e event.accepted to true.

    \snippet doc/src/snippets/declarative/keys/keys-handler.qml key item

    See \l{Qt::Key}{Qt.Key} for the list of keyboard codes.

    \section1 Key Handling Priorities

    The Keys attached property can be configured to handle key events
    before or after the item it is attached to. This makes it possible
    to intercept events in order to override an item's default behavior,
    or act as a fallback for keys not handled by the item.

    If \l priority is Keys.BeforeItem (default) the order of key event processing is:

    \list 1
    \o Items specified in \c forwardTo
    \o specific key handlers, e.g. onReturnPressed
    \o onKeyPress, onKeyRelease handlers
    \o Item specific key handling, e.g. TextInput key handling
    \o parent item
    \endlist

    If priority is Keys.AfterItem the order of key event processing is:

    \list 1
    \o Item specific key handling, e.g. TextInput key handling
    \o Items specified in \c forwardTo
    \o specific key handlers, e.g. onReturnPressed
    \o onKeyPress, onKeyRelease handlers
    \o parent item
    \endlist

    If the event is accepted during any of the above steps, key
    propagation stops.

    \sa KeyEvent, {KeyNavigation}{KeyNavigation attached property}
*/

/*!
    \qmlproperty bool QtQuick2::Keys::enabled

    This flags enables key handling if true (default); otherwise
    no key handlers will be called.
*/

/*!
    \qmlproperty enumeration QtQuick2::Keys::priority

    This property determines whether the keys are processed before
    or after the attached item's own key handling.

    \list
    \o Keys.BeforeItem (default) - process the key events before normal
    item key processing.  If the event is accepted it will not
    be passed on to the item.
    \o Keys.AfterItem - process the key events after normal item key
    handling.  If the item accepts the key event it will not be
    handled by the Keys attached property handler.
    \endlist
*/

/*!
    \qmlproperty list<Object> QtQuick2::Keys::forwardTo

    This property provides a way to forward key presses, key releases, and keyboard input
    coming from input methods to other items. This can be useful when you want
    one item to handle some keys (e.g. the up and down arrow keys), and another item to
    handle other keys (e.g. the left and right arrow keys).  Once an item that has been
    forwarded keys accepts the event it is no longer forwarded to items later in the
    list.

    This example forwards key events to two lists:
    \qml
    Item {
        ListView {
            id: list1
            // ...
        }
        ListView {
            id: list2
            // ...
        }
        Keys.forwardTo: [list1, list2]
        focus: true
    }
    \endqml
*/

/*!
    \qmlsignal QtQuick2::Keys::onPressed(KeyEvent event)

    This handler is called when a key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onReleased(KeyEvent event)

    This handler is called when a key has been released. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit0Pressed(KeyEvent event)

    This handler is called when the digit '0' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit1Pressed(KeyEvent event)

    This handler is called when the digit '1' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit2Pressed(KeyEvent event)

    This handler is called when the digit '2' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit3Pressed(KeyEvent event)

    This handler is called when the digit '3' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit4Pressed(KeyEvent event)

    This handler is called when the digit '4' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit5Pressed(KeyEvent event)

    This handler is called when the digit '5' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit6Pressed(KeyEvent event)

    This handler is called when the digit '6' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit7Pressed(KeyEvent event)

    This handler is called when the digit '7' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit8Pressed(KeyEvent event)

    This handler is called when the digit '8' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDigit9Pressed(KeyEvent event)

    This handler is called when the digit '9' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onLeftPressed(KeyEvent event)

    This handler is called when the Left arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onRightPressed(KeyEvent event)

    This handler is called when the Right arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onUpPressed(KeyEvent event)

    This handler is called when the Up arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDownPressed(KeyEvent event)

    This handler is called when the Down arrow has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onTabPressed(KeyEvent event)

    This handler is called when the Tab key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onBacktabPressed(KeyEvent event)

    This handler is called when the Shift+Tab key combination (Backtab) has
    been pressed. The \a event parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onAsteriskPressed(KeyEvent event)

    This handler is called when the Asterisk '*' has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onEscapePressed(KeyEvent event)

    This handler is called when the Escape key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onReturnPressed(KeyEvent event)

    This handler is called when the Return key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onEnterPressed(KeyEvent event)

    This handler is called when the Enter key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onDeletePressed(KeyEvent event)

    This handler is called when the Delete key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onSpacePressed(KeyEvent event)

    This handler is called when the Space key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onBackPressed(KeyEvent event)

    This handler is called when the Back key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onCancelPressed(KeyEvent event)

    This handler is called when the Cancel key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onSelectPressed(KeyEvent event)

    This handler is called when the Select key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onYesPressed(KeyEvent event)

    This handler is called when the Yes key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onNoPressed(KeyEvent event)

    This handler is called when the No key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onContext1Pressed(KeyEvent event)

    This handler is called when the Context1 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onContext2Pressed(KeyEvent event)

    This handler is called when the Context2 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onContext3Pressed(KeyEvent event)

    This handler is called when the Context3 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onContext4Pressed(KeyEvent event)

    This handler is called when the Context4 key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onCallPressed(KeyEvent event)

    This handler is called when the Call key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onHangupPressed(KeyEvent event)

    This handler is called when the Hangup key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onFlipPressed(KeyEvent event)

    This handler is called when the Flip key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onMenuPressed(KeyEvent event)

    This handler is called when the Menu key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onVolumeUpPressed(KeyEvent event)

    This handler is called when the VolumeUp key has been pressed. The \a event
    parameter provides information about the event.
*/

/*!
    \qmlsignal QtQuick2::Keys::onVolumeDownPressed(KeyEvent event)

    This handler is called when the VolumeDown key has been pressed. The \a event
    parameter provides information about the event.
*/

QQuickKeysAttached::QQuickKeysAttached(QObject *parent)
: QObject(*(new QQuickKeysAttachedPrivate), parent),
  QQuickItemKeyFilter(qobject_cast<QQuickItem*>(parent))
{
    Q_D(QQuickKeysAttached);
    m_processPost = false;
    d->item = qobject_cast<QQuickItem*>(parent);
}

QQuickKeysAttached::~QQuickKeysAttached()
{
}

QQuickKeysAttached::Priority QQuickKeysAttached::priority() const
{
    return m_processPost ? AfterItem : BeforeItem;
}

void QQuickKeysAttached::setPriority(Priority order)
{
    bool processPost = order == AfterItem;
    if (processPost != m_processPost) {
        m_processPost = processPost;
        emit priorityChanged();
    }
}

void QQuickKeysAttached::componentComplete()
{
    Q_D(QQuickKeysAttached);
    if (d->item) {
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QQuickItem *targetItem = d->targets.at(ii);
            if (targetItem && (targetItem->flags() & QQuickItem::ItemAcceptsInputMethod)) {
                d->item->setFlag(QQuickItem::ItemAcceptsInputMethod);
                break;
            }
        }
    }
}

void QQuickKeysAttached::keyPressed(QKeyEvent *event, bool post)
{
    Q_D(QQuickKeysAttached);
    if (post != m_processPost || !d->enabled || d->inPress) {
        event->ignore();
        QQuickItemKeyFilter::keyPressed(event, post);
        return;
    }

    // first process forwards
    if (d->item && d->item->canvas()) {
        d->inPress = true;
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QQuickItem *i = d->targets.at(ii);
            if (i && i->isVisible()) {
                d->item->canvas()->sendEvent(i, event);
                if (event->isAccepted()) {
                    d->inPress = false;
                    return;
                }
            }
        }
        d->inPress = false;
    }

    QQuickKeyEvent ke(*event);
    QByteArray keySignal = keyToSignal(event->key());
    if (!keySignal.isEmpty()) {
        keySignal += "(QQuickKeyEvent*)";
        if (d->isConnected(keySignal)) {
            // If we specifically handle a key then default to accepted
            ke.setAccepted(true);
            int idx = QQuickKeysAttached::staticMetaObject.indexOfSignal(keySignal);
            metaObject()->method(idx).invoke(this, Qt::DirectConnection, Q_ARG(QQuickKeyEvent*, &ke));
        }
    }
    if (!ke.isAccepted())
        emit pressed(&ke);
    event->setAccepted(ke.isAccepted());

    if (!event->isAccepted()) QQuickItemKeyFilter::keyPressed(event, post);
}

void QQuickKeysAttached::keyReleased(QKeyEvent *event, bool post)
{
    Q_D(QQuickKeysAttached);
    if (post != m_processPost || !d->enabled || d->inRelease) {
        event->ignore();
        QQuickItemKeyFilter::keyReleased(event, post);
        return;
    }

    if (d->item && d->item->canvas()) {
        d->inRelease = true;
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QQuickItem *i = d->targets.at(ii);
            if (i && i->isVisible()) {
                d->item->canvas()->sendEvent(i, event);
                if (event->isAccepted()) {
                    d->inRelease = false;
                    return;
                }
            }
        }
        d->inRelease = false;
    }

    QQuickKeyEvent ke(*event);
    emit released(&ke);
    event->setAccepted(ke.isAccepted());

    if (!event->isAccepted()) QQuickItemKeyFilter::keyReleased(event, post);
}

void QQuickKeysAttached::inputMethodEvent(QInputMethodEvent *event, bool post)
{
    Q_D(QQuickKeysAttached);
    if (post == m_processPost && d->item && !d->inIM && d->item->canvas()) {
        d->inIM = true;
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QQuickItem *i = d->targets.at(ii);
            if (i && i->isVisible() && (i->flags() & QQuickItem::ItemAcceptsInputMethod)) {
                d->item->canvas()->sendEvent(i, event);
                if (event->isAccepted()) {
                    d->imeItem = i;
                    d->inIM = false;
                    return;
                }
            }
        }
        d->inIM = false;
    }
    QQuickItemKeyFilter::inputMethodEvent(event, post);
}

QVariant QQuickKeysAttached::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QQuickKeysAttached);
    if (d->item) {
        for (int ii = 0; ii < d->targets.count(); ++ii) {
            QQuickItem *i = d->targets.at(ii);
            if (i && i->isVisible() && (i->flags() & QQuickItem::ItemAcceptsInputMethod) && i == d->imeItem) {
                //### how robust is i == d->imeItem check?
                QVariant v = i->inputMethodQuery(query);
                if (v.userType() == QVariant::RectF)
                    v = d->item->mapRectFromItem(i, v.toRectF());  //### cost?
                return v;
            }
        }
    }
    return QQuickItemKeyFilter::inputMethodQuery(query);
}

QQuickKeysAttached *QQuickKeysAttached::qmlAttachedProperties(QObject *obj)
{
    return new QQuickKeysAttached(obj);
}

/*!
    \qmlclass LayoutMirroring QQuickLayoutMirroringAttached
    \inqmlmodule QtQuick 2
    \ingroup qml-utility-elements
    \brief The LayoutMirroring attached property is used to mirror layout behavior.

    The LayoutMirroring attached property is used to horizontally mirror \l {anchor-layout}{Item anchors},
    \l{Using QML Positioner and Repeater Items}{positioner} elements (such as \l Row and \l Grid)
    and views (such as \l GridView and horizontal \l ListView). Mirroring is a visual change: left
    anchors become right anchors, and positioner elements like \l Grid and \l Row reverse the
    horizontal layout of child items.

    Mirroring is enabled for an item by setting the \l enabled property to true. By default, this
    only affects the item itself; setting the \l childrenInherit property to true propagates the mirroring
    behavior to all child elements as well. If the \c LayoutMirroring attached property has not been defined
    for an item, mirroring is not enabled.

    The following example shows mirroring in action. The \l Row below is specified as being anchored
    to the left of its parent. However, since mirroring has been enabled, the anchor is horizontally
    reversed and it is now anchored to the right. Also, since items in a \l Row are positioned
    from left to right by default, they are now positioned from right to left instead, as demonstrated
    by the numbering and opacity of the items:

    \snippet doc/src/snippets/declarative/layoutmirroring.qml 0

    \image layoutmirroring.png

    Layout mirroring is useful when it is necessary to support both left-to-right and right-to-left
    layout versions of an application to target different language areas. The \l childrenInherit
    property allows layout mirroring to be applied without manually setting layout configurations
    for every item in an application. Keep in mind, however, that mirroring does not affect any
    positioning that is defined by the \l Item \l {Item::}{x} coordinate value, so even with
    mirroring enabled, it will often be necessary to apply some layout fixes to support the
    desired layout direction. Also, it may be necessary to disable the mirroring of individual
    child items (by setting \l {enabled}{LayoutMirroring.enabled} to false for such items) if
    mirroring is not the desired behavior, or if the child item already implements mirroring in
    some custom way.

    See \l {QML Right-to-left User Interfaces} for further details on using \c LayoutMirroring and
    other related features to implement right-to-left support for an application.
*/

/*!
    \qmlproperty bool QtQuick2::LayoutMirroring::enabled

    This property holds whether the item's layout is mirrored horizontally. Setting this to true
    horizontally reverses \l {anchor-layout}{anchor} settings such that left anchors become right,
    and right anchors become left. For \l{Using QML Positioner and Repeater Items}{positioner} elements
    (such as \l Row and \l Grid) and view elements (such as \l {GridView}{GridView} and \l {ListView}{ListView})
    this also mirrors the horizontal layout direction of the item.

    The default value is false.
*/

/*!
    \qmlproperty bool QtQuick2::LayoutMirroring::childrenInherit

    This property holds whether the \l {enabled}{LayoutMirroring.enabled} value for this item
    is inherited by its children.

    The default value is false.
*/


QQuickLayoutMirroringAttached::QQuickLayoutMirroringAttached(QObject *parent) : QObject(parent), itemPrivate(0)
{
    if (QQuickItem *item = qobject_cast<QQuickItem*>(parent)) {
        itemPrivate = QQuickItemPrivate::get(item);
        itemPrivate->attachedLayoutDirection = this;
    } else
        qmlInfo(parent) << tr("LayoutDirection attached property only works with Items");
}

QQuickLayoutMirroringAttached * QQuickLayoutMirroringAttached::qmlAttachedProperties(QObject *object)
{
    return new QQuickLayoutMirroringAttached(object);
}

bool QQuickLayoutMirroringAttached::enabled() const
{
    return itemPrivate ? itemPrivate->effectiveLayoutMirror : false;
}

void QQuickLayoutMirroringAttached::setEnabled(bool enabled)
{
    if (!itemPrivate)
        return;

    itemPrivate->isMirrorImplicit = false;
    if (enabled != itemPrivate->effectiveLayoutMirror) {
        itemPrivate->setLayoutMirror(enabled);
        if (itemPrivate->inheritMirrorFromItem)
             itemPrivate->resolveLayoutMirror();
    }
}

void QQuickLayoutMirroringAttached::resetEnabled()
{
    if (itemPrivate && !itemPrivate->isMirrorImplicit) {
        itemPrivate->isMirrorImplicit = true;
        itemPrivate->resolveLayoutMirror();
    }
}

bool QQuickLayoutMirroringAttached::childrenInherit() const
{
    return itemPrivate ? itemPrivate->inheritMirrorFromItem : false;
}

void QQuickLayoutMirroringAttached::setChildrenInherit(bool childrenInherit) {
    if (itemPrivate && childrenInherit != itemPrivate->inheritMirrorFromItem) {
        itemPrivate->inheritMirrorFromItem = childrenInherit;
        itemPrivate->resolveLayoutMirror();
        childrenInheritChanged();
    }
}

void QQuickItemPrivate::resolveLayoutMirror()
{
    Q_Q(QQuickItem);
    if (QQuickItem *parentItem = q->parentItem()) {
        QQuickItemPrivate *parentPrivate = QQuickItemPrivate::get(parentItem);
        setImplicitLayoutMirror(parentPrivate->inheritedLayoutMirror, parentPrivate->inheritMirrorFromParent);
    } else {
        setImplicitLayoutMirror(isMirrorImplicit ? false : effectiveLayoutMirror, inheritMirrorFromItem);
    }
}

void QQuickItemPrivate::setImplicitLayoutMirror(bool mirror, bool inherit)
{
    inherit = inherit || inheritMirrorFromItem;
    if (!isMirrorImplicit && inheritMirrorFromItem)
        mirror = effectiveLayoutMirror;
    if (mirror == inheritedLayoutMirror && inherit == inheritMirrorFromParent)
        return;

    inheritMirrorFromParent = inherit;
    inheritedLayoutMirror = inheritMirrorFromParent ? mirror : false;

    if (isMirrorImplicit)
        setLayoutMirror(inherit ? inheritedLayoutMirror : false);
    for (int i = 0; i < childItems.count(); ++i) {
        if (QQuickItem *child = qobject_cast<QQuickItem *>(childItems.at(i))) {
            QQuickItemPrivate *childPrivate = QQuickItemPrivate::get(child);
            childPrivate->setImplicitLayoutMirror(inheritedLayoutMirror, inheritMirrorFromParent);
        }
    }
}

void QQuickItemPrivate::setLayoutMirror(bool mirror)
{
    if (mirror != effectiveLayoutMirror) {
        effectiveLayoutMirror = mirror;
        if (_anchors) {
            QQuickAnchorsPrivate *anchor_d = QQuickAnchorsPrivate::get(_anchors);
            anchor_d->fillChanged();
            anchor_d->centerInChanged();
            anchor_d->updateHorizontalAnchors();
            emit _anchors->mirroredChanged();
        }
        mirrorChange();
        if (attachedLayoutDirection) {
            emit attachedLayoutDirection->enabledChanged();
        }
    }
}

/*!
    \class QQuickItem
    \brief The QQuickItem class provides the most basic of all visual items in QML.

    All visual items in Qt Declarative inherit from QQuickItem.  Although QQuickItem
    has no visual appearance, it defines all the properties that are
    common across visual items - such as the x and y position, the
    width and height, \l {anchor-layout}{anchoring} and key handling.

    You can subclass QQuickItem to provide your own custom visual item that inherits
    these features. Note that, because it does not draw anything, QQuickItem sets the
    QGraphicsItem::ItemHasNoContents flag. If you subclass QQuickItem to create a visual
    item, you will need to unset this flag.

*/

/*!
    \qmlclass Item QQuickItem
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-visual-elements
    \brief The Item is the most basic of all visual items in QML.

    All visual items in Qt Declarative inherit from Item.  Although Item
    has no visual appearance, it defines all the properties that are
    common across visual items - such as the x and y position, the
    width and height, \l {anchor-layout}{anchoring} and key handling.

    Item is also useful for grouping items together.

    \qml
    Item {
        Image {
            source: "tile.png"
        }
        Image {
            x: 80
            width: 100
            height: 100
            source: "tile.png"
        }
        Image {
            x: 190
            width: 100
            height: 100
            fillMode: Image.Tile
            source: "tile.png"
        }
    }
    \endqml


    \section1 Key Handling

    Key handling is available to all Item-based visual elements via the \l {Keys}{Keys}
    attached property.  The \e Keys attached property provides basic handlers such
    as \l {Keys::onPressed}{onPressed} and \l {Keys::onReleased}{onReleased},
    as well as handlers for specific keys, such as
    \l {Keys::onCancelPressed}{onCancelPressed}.  The example below
    assigns \l {qmlfocus}{focus} to the item and handles
    the Left key via the general \e onPressed handler and the Select key via the
    onSelectPressed handler:

    \qml
    Item {
        focus: true
        Keys.onPressed: {
            if (event.key == Qt.Key_Left) {
                console.log("move left");
                event.accepted = true;
            }
        }
        Keys.onSelectPressed: console.log("Selected");
    }
    \endqml

    See the \l {Keys}{Keys} attached property for detailed documentation.

    \section1 Layout Mirroring

    Item layouts can be mirrored using the \l {LayoutMirroring}{LayoutMirroring} attached property.

*/

/*!
    \fn void QQuickItem::childrenRectChanged(const QRectF &)
    \internal
*/

/*!
    \fn void QQuickItem::baselineOffsetChanged(qreal)
    \internal
*/

/*!
    \fn void QQuickItem::stateChanged(const QString &state)
    \internal
*/

/*!
    \fn void QQuickItem::parentChanged(QQuickItem *)
    \internal
*/

/*!
    \fn void QQuickItem::smoothChanged(bool)
    \internal
*/

/*!
    \fn void QQuickItem::clipChanged(bool)
    \internal
*/

/*! \fn void QQuickItem::transformOriginChanged(TransformOrigin)
  \internal
*/

/*!
    \fn void QQuickItem::focusChanged(bool)
    \internal
*/

/*!
    \fn void QQuickItem::activeFocusChanged(bool)
    \internal
*/
/*!
    \fn QQuickItem::QQuickItem(QQuickItem *parent)

    Constructs a QQuickItem with the given \a parent.
*/
QQuickItem::QQuickItem(QQuickItem* parent)
: QObject(*(new QQuickItemPrivate), parent)
{
    Q_D(QQuickItem);
    d->init(parent);
}

/*! \internal
*/
QQuickItem::QQuickItem(QQuickItemPrivate &dd, QQuickItem *parent)
: QObject(dd, parent)
{
    Q_D(QQuickItem);
    d->init(parent);
}

#ifndef QT_NO_DEBUG
static int qt_item_count = 0;

static void qt_print_item_count()
{
    qDebug("Number of leaked items: %i", qt_item_count);
    qt_item_count = -1;
}
#endif

/*!
    Destroys the QQuickItem.
*/
QQuickItem::~QQuickItem()
{
#ifndef QT_NO_DEBUG
    --qt_item_count;
    if (qt_item_count < 0)
        qDebug("Item destroyed after qt_print_item_count() was called.");
#endif

    Q_D(QQuickItem);

    if (d->parentItem)
        setParentItem(0);
    else if (d->canvas && d->itemNodeInstance)
        QQuickCanvasPrivate::get(d->canvas)->cleanup(d->itemNodeInstance); // cleanup root
    // XXX todo - optimize
    while (!d->childItems.isEmpty())
        d->childItems.first()->setParentItem(0);

    for (int ii = 0; ii < d->changeListeners.count(); ++ii) {
        QQuickAnchorsPrivate *anchor = d->changeListeners.at(ii).listener->anchorPrivate();
        if (anchor)
            anchor->clearItem(this);
    }

    /*
        update item anchors that depended on us unless they are our child (and will also be destroyed),
        or our sibling, and our parent is also being destroyed.
    */
    for (int ii = 0; ii < d->changeListeners.count(); ++ii) {
        QQuickAnchorsPrivate *anchor = d->changeListeners.at(ii).listener->anchorPrivate();
        if (anchor && anchor->item && anchor->item->parentItem() && anchor->item->parentItem() != this)
            anchor->update();
    }

    for (int ii = 0; ii < d->changeListeners.count(); ++ii) {
        const QQuickItemPrivate::ChangeListener &change = d->changeListeners.at(ii);
        if (change.types & QQuickItemPrivate::Destroyed)
            change.listener->itemDestroyed(this);
    }
    d->changeListeners.clear();
    delete d->_anchorLines; d->_anchorLines = 0;
    delete d->_anchors; d->_anchors = 0;
    delete d->_stateGroup; d->_stateGroup = 0;
    delete d->_contents; d->_contents = 0;
}

/*!
    \qmlproperty enumeration QtQuick2::Item::transformOrigin
    This property holds the origin point around which scale and rotation transform.

    Nine transform origins are available, as shown in the image below.

    \image declarative-transformorigin.png

    This example rotates an image around its bottom-right corner.
    \qml
    Image {
        source: "myimage.png"
        transformOrigin: Item.BottomRight
        rotation: 45
    }
    \endqml

    The default transform origin is \c Item.Center.

    To set an arbitrary transform origin point use the \l Scale or \l Rotation
    transform elements.
*/

/*!
    \qmlproperty Item QtQuick2::Item::parent
    This property holds the parent of the item.
*/

/*!
    \property QQuickItem::parent
    This property holds the parent of the item.
*/
void QQuickItem::setParentItem(QQuickItem *parentItem)
{
    Q_D(QQuickItem);
    if (parentItem == d->parentItem)
        return;

    d->removeFromDirtyList();

    QQuickItem *oldParentItem = d->parentItem;
    QQuickItem *scopeFocusedItem = 0;

    if (oldParentItem) {
        QQuickItemPrivate *op = QQuickItemPrivate::get(oldParentItem);

        QQuickItem *scopeItem = 0;

        if (d->canvas && hasFocus()) {
            scopeItem = oldParentItem;
            while (!scopeItem->isFocusScope()) scopeItem = scopeItem->parentItem();
            scopeFocusedItem = this;
        } else if (d->canvas && !isFocusScope() && d->subFocusItem) {
            scopeItem = oldParentItem;
            while (!scopeItem->isFocusScope()) scopeItem = scopeItem->parentItem();
            scopeFocusedItem = d->subFocusItem;
        }

        if (scopeFocusedItem)
            QQuickCanvasPrivate::get(d->canvas)->clearFocusInScope(scopeItem, scopeFocusedItem,
                                                                QQuickCanvasPrivate::DontChangeFocusProperty);

        op->removeChild(this);
    }

    d->parentItem = parentItem;

    QQuickCanvas *parentCanvas = parentItem?QQuickItemPrivate::get(parentItem)->canvas:0;
    if (d->canvas != parentCanvas) {
        QQuickItemPrivate::InitializationState initState;
        initState.clear();
        d->initCanvas(&initState, parentCanvas);
    }

    d->dirty(QQuickItemPrivate::ParentChanged);

    if (d->parentItem)
        QQuickItemPrivate::get(d->parentItem)->addChild(this);

    d->setEffectiveVisibleRecur(d->calcEffectiveVisible());
    d->setEffectiveEnableRecur(d->calcEffectiveEnable());

    if (scopeFocusedItem && d->parentItem && d->canvas) {
        // We need to test whether this item becomes scope focused
        QQuickItem *scopeItem = 0;
        scopeItem = d->parentItem;
        while (!scopeItem->isFocusScope()) scopeItem = scopeItem->parentItem();

        if (scopeItem->scopedFocusItem()) {
            QQuickItemPrivate::get(scopeFocusedItem)->focus = false;
            emit scopeFocusedItem->focusChanged(false);
        } else {
            QQuickCanvasPrivate::get(d->canvas)->setFocusInScope(scopeItem, scopeFocusedItem,
                                                              QQuickCanvasPrivate::DontChangeFocusProperty);
        }
    }

    d->resolveLayoutMirror();

    d->itemChange(ItemParentHasChanged, d->parentItem);

    emit parentChanged(d->parentItem);
}

void QQuickItem::stackBefore(const QQuickItem *sibling)
{
    Q_D(QQuickItem);
    if (!sibling || sibling == this || !d->parentItem || d->parentItem != QQuickItemPrivate::get(sibling)->parentItem) {
        qWarning("QQuickItem::stackBefore: Cannot stack before %p, which must be a sibling", sibling);
        return;
    }

    QQuickItemPrivate *parentPrivate = QQuickItemPrivate::get(d->parentItem);

    int myIndex = parentPrivate->childItems.indexOf(this);
    int siblingIndex = parentPrivate->childItems.indexOf(const_cast<QQuickItem *>(sibling));

    Q_ASSERT(myIndex != -1 && siblingIndex != -1);

    if (myIndex == siblingIndex - 1)
        return;

    parentPrivate->childItems.removeAt(myIndex);

    if (myIndex < siblingIndex) --siblingIndex;

    parentPrivate->childItems.insert(siblingIndex, this);

    parentPrivate->dirty(QQuickItemPrivate::ChildrenStackingChanged);
    parentPrivate->markSortedChildrenDirty(this);

    for (int ii = qMin(siblingIndex, myIndex); ii < parentPrivate->childItems.count(); ++ii)
        QQuickItemPrivate::get(parentPrivate->childItems.at(ii))->siblingOrderChanged();
}

void QQuickItem::stackAfter(const QQuickItem *sibling)
{
    Q_D(QQuickItem);
    if (!sibling || sibling == this || !d->parentItem || d->parentItem != QQuickItemPrivate::get(sibling)->parentItem) {
        qWarning("QQuickItem::stackAfter: Cannot stack after %p, which must be a sibling", sibling);
        return;
    }

    QQuickItemPrivate *parentPrivate = QQuickItemPrivate::get(d->parentItem);

    int myIndex = parentPrivate->childItems.indexOf(this);
    int siblingIndex = parentPrivate->childItems.indexOf(const_cast<QQuickItem *>(sibling));

    Q_ASSERT(myIndex != -1 && siblingIndex != -1);

    if (myIndex == siblingIndex + 1)
        return;

    parentPrivate->childItems.removeAt(myIndex);

    if (myIndex < siblingIndex) --siblingIndex;

    parentPrivate->childItems.insert(siblingIndex + 1, this);

    parentPrivate->dirty(QQuickItemPrivate::ChildrenStackingChanged);
    parentPrivate->markSortedChildrenDirty(this);

    for (int ii = qMin(myIndex, siblingIndex + 1); ii < parentPrivate->childItems.count(); ++ii)
        QQuickItemPrivate::get(parentPrivate->childItems.at(ii))->siblingOrderChanged();
}

/*!
    Returns the QQuickItem parent of this item.
*/
QQuickItem *QQuickItem::parentItem() const
{
    Q_D(const QQuickItem);
    return d->parentItem;
}

QSGEngine *QQuickItem::sceneGraphEngine() const
{
    return canvas()->sceneGraphEngine();
}

QQuickCanvas *QQuickItem::canvas() const
{
    Q_D(const QQuickItem);
    return d->canvas;
}

static bool itemZOrder_sort(QQuickItem *lhs, QQuickItem *rhs)
{
    return lhs->z() < rhs->z();
}

QList<QQuickItem *> QQuickItemPrivate::paintOrderChildItems() const
{
    if (sortedChildItems)
        return *sortedChildItems;

    // If none of the items have set Z then the paint order list is the same as
    // the childItems list.  This is by far the most common case.
    bool haveZ = false;
    for (int i = 0; i < childItems.count(); ++i) {
        if (QQuickItemPrivate::get(childItems.at(i))->z != 0.) {
            haveZ = true;
            break;
        }
    }
    if (haveZ) {
        sortedChildItems = new QList<QQuickItem*>(childItems);
        qStableSort(sortedChildItems->begin(), sortedChildItems->end(), itemZOrder_sort);
        return *sortedChildItems;
    }

    sortedChildItems = const_cast<QList<QQuickItem*>*>(&childItems);

    return childItems;
}

void QQuickItemPrivate::addChild(QQuickItem *child)
{
    Q_Q(QQuickItem);

    Q_ASSERT(!childItems.contains(child));

    childItems.append(child);

    markSortedChildrenDirty(child);
    dirty(QQuickItemPrivate::ChildrenChanged);

    itemChange(QQuickItem::ItemChildAddedChange, child);

    emit q->childrenChanged();
}

void QQuickItemPrivate::removeChild(QQuickItem *child)
{
    Q_Q(QQuickItem);

    Q_ASSERT(child);
    Q_ASSERT(childItems.contains(child));
    childItems.removeOne(child);
    Q_ASSERT(!childItems.contains(child));

    markSortedChildrenDirty(child);
    dirty(QQuickItemPrivate::ChildrenChanged);

    itemChange(QQuickItem::ItemChildRemovedChange, child);

    emit q->childrenChanged();
}

void QQuickItemPrivate::InitializationState::clear()
{
    focusScope = 0;
}

void QQuickItemPrivate::InitializationState::clear(QQuickItem *fs)
{
    focusScope = fs;
}

QQuickItem *QQuickItemPrivate::InitializationState::getFocusScope(QQuickItem *item)
{
    if (!focusScope) {
        QQuickItem *fs = item->parentItem();
        while (!fs->isFocusScope())
            fs = fs->parentItem();
        focusScope = fs;
    }
    return focusScope;
}

void QQuickItemPrivate::initCanvas(InitializationState *state, QQuickCanvas *c)
{
    Q_Q(QQuickItem);

    if (canvas) {
        removeFromDirtyList();
        QQuickCanvasPrivate *c = QQuickCanvasPrivate::get(canvas);
        if (polishScheduled)
            c->itemsToPolish.remove(q);
        if (c->mouseGrabberItem == q)
            c->mouseGrabberItem = 0;
        if ( hoverEnabled )
            c->hoverItems.removeAll(q);
        if (itemNodeInstance)
            c->cleanup(itemNodeInstance);
    }

    canvas = c;

    if (canvas && polishScheduled)
        QQuickCanvasPrivate::get(canvas)->itemsToPolish.insert(q);

    itemNodeInstance = 0;
    opacityNode = 0;
    clipNode = 0;
    rootNode = 0;
    groupNode = 0;
    paintNode = 0;
    beforePaintNode = 0;

    InitializationState _dummy;
    InitializationState *childState = state;

    if (c && q->isFocusScope()) {
        _dummy.clear(q);
        childState = &_dummy;
    }

    for (int ii = 0; ii < childItems.count(); ++ii) {
        QQuickItem *child = childItems.at(ii);
        QQuickItemPrivate::get(child)->initCanvas(childState, c);
    }

    if (c && focus) {
        // Fixup
        if (state->getFocusScope(q)->scopedFocusItem()) {
            focus = false;
            emit q->focusChanged(false);
        } else {
            QQuickCanvasPrivate::get(canvas)->setFocusInScope(state->getFocusScope(q), q);
        }
    }

    dirty(Canvas);

    itemChange(QQuickItem::ItemSceneChange, c);
}

/*!
Returns a transform that maps points from canvas space into item space.
*/
QTransform QQuickItemPrivate::canvasToItemTransform() const
{
    // XXX todo - optimize
    return itemToCanvasTransform().inverted();
}

/*!
Returns a transform that maps points from item space into canvas space.
*/
QTransform QQuickItemPrivate::itemToCanvasTransform() const
{
    // XXX todo
    QTransform rv = parentItem?QQuickItemPrivate::get(parentItem)->itemToCanvasTransform():QTransform();
    itemToParentTransform(rv);
    return rv;
}

/*!
Motifies \a t with this items local transform relative to its parent.
*/
void QQuickItemPrivate::itemToParentTransform(QTransform &t) const
{
    if (x || y)
        t.translate(x, y);

    if (!transforms.isEmpty()) {
        QMatrix4x4 m(t);
        for (int ii = transforms.count() - 1; ii >= 0; --ii)
            transforms.at(ii)->applyTo(&m);
        t = m.toTransform();
    }

    if (scale != 1. || rotation != 0.) {
        QPointF tp = computeTransformOrigin();
        t.translate(tp.x(), tp.y());
        t.scale(scale, scale);
        t.rotate(rotation);
        t.translate(-tp.x(), -tp.y());
    }
}


/*!
    \qmlproperty real QtQuick2::Item::childrenRect.x
    \qmlproperty real QtQuick2::Item::childrenRect.y
    \qmlproperty real QtQuick2::Item::childrenRect.width
    \qmlproperty real QtQuick2::Item::childrenRect.height

    The childrenRect properties allow an item access to the geometry of its
    children. This property is useful if you have an item that needs to be
    sized to fit its children.
*/


/*!
    \qmlproperty list<Item> QtQuick2::Item::children
    \qmlproperty list<Object> QtQuick2::Item::resources

    The children property contains the list of visual children of this item.
    The resources property contains non-visual resources that you want to
    reference by name.

    Generally you can rely on Item's default property to handle all this for
    you, but it can come in handy in some cases.

    \qml
    Item {
        children: [
            Text {},
            Rectangle {}
        ]
        resources: [
            Component {
                id: myComponent
                Text {}
            }
        ]
    }
    \endqml
*/

/*!
    Returns true if construction of the QML component is complete; otherwise
    returns false.

    It is often desirable to delay some processing until the component is
    completed.

    \sa componentComplete()
*/
bool QQuickItem::isComponentComplete() const
{
    Q_D(const QQuickItem);
    return d->componentComplete;
}

QQuickItemPrivate::QQuickItemPrivate()
: _anchors(0), _contents(0), baselineOffset(0), _anchorLines(0), _stateGroup(0), origin(QQuickItem::Center),

  flags(0), widthValid(false), heightValid(false), componentComplete(true),
  keepMouse(false), keepTouch(false), hoverEnabled(false), smooth(false), focus(false), activeFocus(false), notifiedFocus(false),
  notifiedActiveFocus(false), filtersChildMouseEvents(false), explicitVisible(true),
  effectiveVisible(true), explicitEnable(true), effectiveEnable(true), polishScheduled(false),
  inheritedLayoutMirror(false), effectiveLayoutMirror(false), isMirrorImplicit(true),
  inheritMirrorFromParent(false), inheritMirrorFromItem(false), childrenDoNotOverlap(false),

  canvas(0), parentItem(0), sortedChildItems(&childItems),

  subFocusItem(0),

  x(0), y(0), width(0), height(0), implicitWidth(0), implicitHeight(0),
  z(0), scale(1), rotation(0), opacity(1),

  attachedLayoutDirection(0), acceptedMouseButtons(0),
  imHints(Qt::ImhMultiLine),

  keyHandler(0),

  dirtyAttributes(0), nextDirtyItem(0), prevDirtyItem(0),

  itemNodeInstance(0), opacityNode(0), clipNode(0), rootNode(0), groupNode(0), paintNode(0)
  , beforePaintNode(0), effectRefCount(0), hideRefCount(0)
{
}

QQuickItemPrivate::~QQuickItemPrivate()
{
    if (sortedChildItems != &childItems)
        delete sortedChildItems;
}

void QQuickItemPrivate::init(QQuickItem *parent)
{
#ifndef QT_NO_DEBUG
    ++qt_item_count;
    static bool atexit_registered = false;
    if (!atexit_registered) {
        atexit(qt_print_item_count);
        atexit_registered = true;
    }
#endif

    Q_Q(QQuickItem);
    baselineOffset.invalidate();

    if (parent) {
        q->setParentItem(parent);
        QQuickItemPrivate *parentPrivate = QQuickItemPrivate::get(parent);
        setImplicitLayoutMirror(parentPrivate->inheritedLayoutMirror, parentPrivate->inheritMirrorFromParent);
    }
}

void QQuickItemPrivate::data_append(QDeclarativeListProperty<QObject> *prop, QObject *o)
{
    if (!o)
        return;

    QQuickItem *that = static_cast<QQuickItem *>(prop->object);

    // This test is measurably (albeit only slightly) faster than qobject_cast<>()
    const QMetaObject *mo = o->metaObject();
    while (mo && mo != &QQuickItem::staticMetaObject) {
        mo = mo->d.superdata;
    }

    if (mo) {
        QQuickItem *item = static_cast<QQuickItem *>(o);
        item->setParentItem(that);
    } else {
        if (o->inherits("QGraphicsItem"))
            qWarning("Cannot add a QtQuick 1.0 item (%s) into a QtQuick 2.0 scene!", o->metaObject()->className());

        // XXX todo - do we really want this behavior?
        o->setParent(that);
    }
}

/*!
    \qmlproperty list<Object> QtQuick2::Item::data
    \default

    The data property allows you to freely mix visual children and resources
    in an item.  If you assign a visual item to the data list it becomes
    a child and if you assign any other object type, it is added as a resource.

    So you can write:
    \qml
    Item {
        Text {}
        Rectangle {}
        Timer {}
    }
    \endqml

    instead of:
    \qml
    Item {
        children: [
            Text {},
            Rectangle {}
        ]
        resources: [
            Timer {}
        ]
    }
    \endqml

    data is a behind-the-scenes property: you should never need to explicitly
    specify it.
 */

int QQuickItemPrivate::data_count(QDeclarativeListProperty<QObject> *prop)
{
    Q_UNUSED(prop);
    // XXX todo
    return 0;
}

QObject *QQuickItemPrivate::data_at(QDeclarativeListProperty<QObject> *prop, int i)
{
    Q_UNUSED(prop);
    Q_UNUSED(i);
    // XXX todo
    return 0;
}

void QQuickItemPrivate::data_clear(QDeclarativeListProperty<QObject> *prop)
{
    Q_UNUSED(prop);
    // XXX todo
}

QObject *QQuickItemPrivate::resources_at(QDeclarativeListProperty<QObject> *prop, int index)
{
    const QObjectList children = prop->object->children();
    if (index < children.count())
        return children.at(index);
    else
        return 0;
}

void QQuickItemPrivate::resources_append(QDeclarativeListProperty<QObject> *prop, QObject *o)
{
    // XXX todo - do we really want this behavior?
    o->setParent(prop->object);
}

int QQuickItemPrivate::resources_count(QDeclarativeListProperty<QObject> *prop)
{
    return prop->object->children().count();
}

void QQuickItemPrivate::resources_clear(QDeclarativeListProperty<QObject> *prop)
{
    // XXX todo - do we really want this behavior?
    const QObjectList children = prop->object->children();
    for (int index = 0; index < children.count(); index++)
        children.at(index)->setParent(0);
}

QQuickItem *QQuickItemPrivate::children_at(QDeclarativeListProperty<QQuickItem> *prop, int index)
{
    QQuickItemPrivate *p = QQuickItemPrivate::get(static_cast<QQuickItem *>(prop->object));
    if (index >= p->childItems.count() || index < 0)
        return 0;
    else
        return p->childItems.at(index);
}

void QQuickItemPrivate::children_append(QDeclarativeListProperty<QQuickItem> *prop, QQuickItem *o)
{
    if (!o)
        return;

    QQuickItem *that = static_cast<QQuickItem *>(prop->object);
    if (o->parentItem() == that)
        o->setParentItem(0);

    o->setParentItem(that);
}

int QQuickItemPrivate::children_count(QDeclarativeListProperty<QQuickItem> *prop)
{
    QQuickItemPrivate *p = QQuickItemPrivate::get(static_cast<QQuickItem *>(prop->object));
    return p->childItems.count();
}

void QQuickItemPrivate::children_clear(QDeclarativeListProperty<QQuickItem> *prop)
{
    QQuickItem *that = static_cast<QQuickItem *>(prop->object);
    QQuickItemPrivate *p = QQuickItemPrivate::get(that);
    while (!p->childItems.isEmpty())
        p->childItems.at(0)->setParentItem(0);
}

int QQuickItemPrivate::transform_count(QDeclarativeListProperty<QQuickTransform> *prop)
{
    QQuickItem *that = static_cast<QQuickItem *>(prop->object);
    return QQuickItemPrivate::get(that)->transforms.count();
}

void QQuickTransform::appendToItem(QQuickItem *item)
{
    Q_D(QQuickTransform);
    if (!item)
        return;

    QQuickItemPrivate *p = QQuickItemPrivate::get(item);

    if (!d->items.isEmpty() && !p->transforms.isEmpty() && p->transforms.contains(this)) {
        p->transforms.removeOne(this);
        p->transforms.append(this);
    } else {
        p->transforms.append(this);
        d->items.append(item);
    }

    p->dirty(QQuickItemPrivate::Transform);
}

void QQuickTransform::prependToItem(QQuickItem *item)
{
    Q_D(QQuickTransform);
    if (!item)
        return;

    QQuickItemPrivate *p = QQuickItemPrivate::get(item);

    if (!d->items.isEmpty() && !p->transforms.isEmpty() && p->transforms.contains(this)) {
        p->transforms.removeOne(this);
        p->transforms.prepend(this);
    } else {
        p->transforms.prepend(this);
        d->items.append(item);
    }

    p->dirty(QQuickItemPrivate::Transform);
}

void QQuickItemPrivate::transform_append(QDeclarativeListProperty<QQuickTransform> *prop, QQuickTransform *transform)
{
    if (!transform)
        return;

    QQuickItem *that = static_cast<QQuickItem *>(prop->object);
    transform->appendToItem(that);
}

QQuickTransform *QQuickItemPrivate::transform_at(QDeclarativeListProperty<QQuickTransform> *prop, int idx)
{
    QQuickItem *that = static_cast<QQuickItem *>(prop->object);
    QQuickItemPrivate *p = QQuickItemPrivate::get(that);

    if (idx < 0 || idx >= p->transforms.count())
        return 0;
    else
        return p->transforms.at(idx);
}

void QQuickItemPrivate::transform_clear(QDeclarativeListProperty<QQuickTransform> *prop)
{
    QQuickItem *that = static_cast<QQuickItem *>(prop->object);
    QQuickItemPrivate *p = QQuickItemPrivate::get(that);

    for (int ii = 0; ii < p->transforms.count(); ++ii) {
        QQuickTransform *t = p->transforms.at(ii);
        QQuickTransformPrivate *tp = QQuickTransformPrivate::get(t);
        tp->items.removeOne(that);
    }

    p->transforms.clear();

    p->dirty(QQuickItemPrivate::Transform);
}

/*!
    \property QQuickItem::childrenRect
    \brief The geometry of an item's children.

    This property holds the (collective) position and size of the item's children.
*/

/*!
  \qmlproperty real QtQuick2::Item::x
  \qmlproperty real QtQuick2::Item::y
  \qmlproperty real QtQuick2::Item::width
  \qmlproperty real QtQuick2::Item::height

  Defines the item's position and size relative to its parent.

  \qml
  Item { x: 100; y: 100; width: 100; height: 100 }
  \endqml
 */

/*!
  \qmlproperty real QtQuick2::Item::z

  Sets the stacking order of sibling items.  By default the stacking order is 0.

  Items with a higher stacking value are drawn on top of siblings with a
  lower stacking order.  Items with the same stacking value are drawn
  bottom up in the order they appear.  Items with a negative stacking
  value are drawn under their parent's content.

  The following example shows the various effects of stacking order.

  \table
  \row
  \o \image declarative-item_stacking1.png
  \o Same \c z - later children above earlier children:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
      }
      Rectangle {
          color: "blue"
          x: 50; y: 50; width: 100; height: 100
      }
  }
  \endqml
  \row
  \o \image declarative-item_stacking2.png
  \o Higher \c z on top:
  \qml
  Item {
      Rectangle {
          z: 1
          color: "red"
          width: 100; height: 100
      }
      Rectangle {
          color: "blue"
          x: 50; y: 50; width: 100; height: 100
      }
  }
  \endqml
  \row
  \o \image declarative-item_stacking3.png
  \o Same \c z - children above parents:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
          Rectangle {
              color: "blue"
              x: 50; y: 50; width: 100; height: 100
          }
      }
  }
  \endqml
  \row
  \o \image declarative-item_stacking4.png
  \o Lower \c z below:
  \qml
  Item {
      Rectangle {
          color: "red"
          width: 100; height: 100
          Rectangle {
              z: -1
              color: "blue"
              x: 50; y: 50; width: 100; height: 100
          }
      }
  }
  \endqml
  \endtable
 */

/*!
    \qmlproperty bool QtQuick2::Item::visible

    This property holds whether the item is visible. By default this is true.

    Setting this property directly affects the \c visible value of child
    items. When set to \c false, the \c visible values of all child items also
    become \c false. When set to \c true, the \c visible values of child items
    are returned to \c true, unless they have explicitly been set to \c false.

    (Because of this flow-on behavior, using the \c visible property may not
    have the intended effect if a property binding should only respond to
    explicit property changes. In such cases it may be better to use the
    \l opacity property instead.)

    Setting this property to \c false automatically causes \l focus to be set
    to \c false, and this item will longer receive mouse and keyboard events.
    (In contrast, setting the \l opacity to 0 does not affect the \l focus
    property and the receiving of key events.)

    \note This property's value is only affected by changes to this property or
    the parent's \c visible property. It does not change, for example, if this
    item moves off-screen, or if the \l opacity changes to 0.
*/


/*!
  \qmlproperty AnchorLine QtQuick2::Item::anchors.top
  \qmlproperty AnchorLine QtQuick2::Item::anchors.bottom
  \qmlproperty AnchorLine QtQuick2::Item::anchors.left
  \qmlproperty AnchorLine QtQuick2::Item::anchors.right
  \qmlproperty AnchorLine QtQuick2::Item::anchors.horizontalCenter
  \qmlproperty AnchorLine QtQuick2::Item::anchors.verticalCenter
  \qmlproperty AnchorLine QtQuick2::Item::anchors.baseline

  \qmlproperty Item QtQuick2::Item::anchors.fill
  \qmlproperty Item QtQuick2::Item::anchors.centerIn

  \qmlproperty real QtQuick2::Item::anchors.margins
  \qmlproperty real QtQuick2::Item::anchors.topMargin
  \qmlproperty real QtQuick2::Item::anchors.bottomMargin
  \qmlproperty real QtQuick2::Item::anchors.leftMargin
  \qmlproperty real QtQuick2::Item::anchors.rightMargin
  \qmlproperty real QtQuick2::Item::anchors.horizontalCenterOffset
  \qmlproperty real QtQuick2::Item::anchors.verticalCenterOffset
  \qmlproperty real QtQuick2::Item::anchors.baselineOffset

  \qmlproperty bool QtQuick2::Item::anchors.mirrored

  Anchors provide a way to position an item by specifying its
  relationship with other items.

  Margins apply to top, bottom, left, right, and fill anchors.
  The \c anchors.margins property can be used to set all of the various margins at once, to the same value.
  Note that margins are anchor-specific and are not applied if an item does not
  use anchors.

  Offsets apply for horizontal center, vertical center, and baseline anchors.

  \table
  \row
  \o \image declarative-anchors_example.png
  \o Text anchored to Image, horizontally centered and vertically below, with a margin.
  \qml
  Item {
      Image {
          id: pic
          // ...
      }
      Text {
          id: label
          anchors.horizontalCenter: pic.horizontalCenter
          anchors.top: pic.bottom
          anchors.topMargin: 5
          // ...
      }
  }
  \endqml
  \row
  \o \image declarative-anchors_example2.png
  \o
  Left of Text anchored to right of Image, with a margin. The y
  property of both defaults to 0.

  \qml
  Item {
      Image {
          id: pic
          // ...
      }
      Text {
          id: label
          anchors.left: pic.right
          anchors.leftMargin: 5
          // ...
      }
  }
  \endqml
  \endtable

  \c anchors.fill provides a convenient way for one item to have the
  same geometry as another item, and is equivalent to connecting all
  four directional anchors.

  To clear an anchor value, set it to \c undefined.

  \c anchors.mirrored returns true it the layout has been \l {LayoutMirroring}{mirrored}.

  \note You can only anchor an item to siblings or a parent.

  For more information see \l {anchor-layout}{Anchor Layouts}.
*/

/*!
  \property QQuickItem::baselineOffset
  \brief The position of the item's baseline in local coordinates.

  The baseline of a \l Text item is the imaginary line on which the text
  sits. Controls containing text usually set their baseline to the
  baseline of their text.

  For non-text items, a default baseline offset of 0 is used.
*/
QQuickAnchors *QQuickItemPrivate::anchors() const
{
    if (!_anchors) {
        Q_Q(const QQuickItem);
        _anchors = new QQuickAnchors(const_cast<QQuickItem *>(q));
        if (!componentComplete)
            _anchors->classBegin();
    }
    return _anchors;
}

QQuickItemPrivate::AnchorLines *QQuickItemPrivate::anchorLines() const
{
    Q_Q(const QQuickItem);
    if (!_anchorLines) _anchorLines =
        new AnchorLines(const_cast<QQuickItem *>(q));
    return _anchorLines;
}

void QQuickItemPrivate::siblingOrderChanged()
{
    Q_Q(QQuickItem);
    for (int ii = 0; ii < changeListeners.count(); ++ii) {
        const QQuickItemPrivate::ChangeListener &change = changeListeners.at(ii);
        if (change.types & QQuickItemPrivate::SiblingOrder) {
            change.listener->itemSiblingOrderChanged(q);
        }
    }
}

QDeclarativeListProperty<QObject> QQuickItemPrivate::data()
{
    return QDeclarativeListProperty<QObject>(q_func(), 0, QQuickItemPrivate::data_append,
                                             QQuickItemPrivate::data_count,
                                             QQuickItemPrivate::data_at,
                                             QQuickItemPrivate::data_clear);
}

QRectF QQuickItem::childrenRect()
{
    Q_D(QQuickItem);
    if (!d->_contents) {
        d->_contents = new QQuickContents(this);
        if (d->componentComplete)
            d->_contents->complete();
    }
    return d->_contents->rectF();
}

QList<QQuickItem *> QQuickItem::childItems() const
{
    Q_D(const QQuickItem);
    return d->childItems;
}

bool QQuickItem::clip() const
{
    return flags() & ItemClipsChildrenToShape;
}

void QQuickItem::setClip(bool c)
{
    if (clip() == c)
        return;

    setFlag(ItemClipsChildrenToShape, c);

    emit clipChanged(c);
}


/*!
  This function is called to handle this item's changes in
  geometry from \a oldGeometry to \a newGeometry. If the two
  geometries are the same, it doesn't do anything.
 */
void QQuickItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickItem);

    if (d->_anchors)
        QQuickAnchorsPrivate::get(d->_anchors)->updateMe();

    bool xChange = (newGeometry.x() != oldGeometry.x());
    bool yChange = (newGeometry.y() != oldGeometry.y());
    bool widthChange = (newGeometry.width() != oldGeometry.width());
    bool heightChange = (newGeometry.height() != oldGeometry.height());

    for (int ii = 0; ii < d->changeListeners.count(); ++ii) {
        const QQuickItemPrivate::ChangeListener &change = d->changeListeners.at(ii);
        if (change.types & QQuickItemPrivate::Geometry) {
            if (change.gTypes == QQuickItemPrivate::GeometryChange) {
                change.listener->itemGeometryChanged(this, newGeometry, oldGeometry);
            } else if ((xChange && (change.gTypes & QQuickItemPrivate::XChange)) ||
                       (yChange && (change.gTypes & QQuickItemPrivate::YChange)) ||
                       (widthChange && (change.gTypes & QQuickItemPrivate::WidthChange)) ||
                       (heightChange && (change.gTypes & QQuickItemPrivate::HeightChange))) {
                change.listener->itemGeometryChanged(this, newGeometry, oldGeometry);
            }
        }
    }

    if (xChange)
        emit xChanged();
    if (yChange)
        emit yChanged();
    if (widthChange)
        emit widthChanged();
    if (heightChange)
        emit heightChanged();
}

/*!
    Called by the rendering thread when it is time to sync the state of the QML objects with the
    scene graph objects. The function should return the root of the scene graph subtree for
    this item. \a oldNode is the node that was returned the last time the function was called.

    The main thread is blocked while this function is executed so it is safe to read
    values from the QQuickItem instance and other objects in the main thread.

    \warning This is the only function in which it is allowed to make use of scene graph
    objects from the main thread. Use of scene graph objects outside this function will
    result in race conditions and potential crashes.
 */

QSGNode *QQuickItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    delete oldNode;
    return 0;
}

QSGTransformNode *QQuickItemPrivate::createTransformNode()
{
    return new QSGTransformNode;
}

void QQuickItem::updatePolish()
{
}

void QQuickItemPrivate::removeItemChangeListener(QQuickItemChangeListener *listener, ChangeTypes types)
{
    ChangeListener change(listener, types);
    changeListeners.removeOne(change);
}

void QQuickItemPrivate::updateOrAddGeometryChangeListener(QQuickItemChangeListener *listener, GeometryChangeTypes types)
{
    ChangeListener change(listener, types);
    int index = changeListeners.find(change);
    if (index > -1)
        changeListeners[index].gTypes = change.gTypes;  //we may have different GeometryChangeTypes
    else
        changeListeners.append(change);
}

void QQuickItemPrivate::updateOrRemoveGeometryChangeListener(QQuickItemChangeListener *listener, GeometryChangeTypes types)
{
    ChangeListener change(listener, types);
    if (types == NoChange) {
        changeListeners.removeOne(change);
    } else {
        int index = changeListeners.find(change);
        if (index > -1)
            changeListeners[index].gTypes = change.gTypes;  //we may have different GeometryChangeTypes
    }
}

void QQuickItem::keyPressEvent(QKeyEvent *event)
{
    event->ignore();
}

void QQuickItem::keyReleaseEvent(QKeyEvent *event)
{
    event->ignore();
}

void QQuickItem::inputMethodEvent(QInputMethodEvent *event)
{
    event->ignore();
}

void QQuickItem::focusInEvent(QFocusEvent *)
{
}

void QQuickItem::focusOutEvent(QFocusEvent *)
{
}

void QQuickItem::mousePressEvent(QMouseEvent *event)
{
    event->ignore();
}

void QQuickItem::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();
}

void QQuickItem::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();
}

void QQuickItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    mousePressEvent(event);
}

void QQuickItem::mouseUngrabEvent()
{
    // XXX todo
}

void QQuickItem::touchUngrabEvent()
{
    // XXX todo
}

void QQuickItem::wheelEvent(QWheelEvent *event)
{
    event->ignore();
}

void QQuickItem::touchEvent(QTouchEvent *event)
{
    event->ignore();
}

void QQuickItem::hoverEnterEvent(QHoverEvent *event)
{
    Q_UNUSED(event);
}

void QQuickItem::hoverMoveEvent(QHoverEvent *event)
{
    Q_UNUSED(event);
}

void QQuickItem::hoverLeaveEvent(QHoverEvent *event)
{
    Q_UNUSED(event);
}

void QQuickItem::dragEnterEvent(QDragEnterEvent *event)
{
    Q_UNUSED(event);
}

void QQuickItem::dragMoveEvent(QDragMoveEvent *event)
{

    Q_UNUSED(event);
}

void QQuickItem::dragLeaveEvent(QDragLeaveEvent *event)
{

    Q_UNUSED(event);
}

void QQuickItem::dropEvent(QDropEvent *event)
{
    Q_UNUSED(event);
}

bool QQuickItem::childMouseEventFilter(QQuickItem *, QEvent *)
{
    return false;
}

void QQuickItem::windowDeactivateEvent()
{
    foreach (QQuickItem* item, childItems()) {
        item->windowDeactivateEvent();
    }
}

Qt::InputMethodHints QQuickItem::inputMethodHints() const
{
    Q_D(const QQuickItem);
    return d->imHints;
}

void QQuickItem::setInputMethodHints(Qt::InputMethodHints hints)
{
    Q_D(QQuickItem);
    d->imHints = hints;

    if (!d->canvas || d->canvas->activeFocusItem() != this)
        return;

    QInputPanel *p = qApp->inputPanel();
    if (p->inputItem() == this)
        qApp->inputPanel()->update(Qt::ImHints);
}

void QQuickItem::updateMicroFocus()
{
    QInputPanel *p = qApp->inputPanel();
    if (p->inputItem() == this)
        qApp->inputPanel()->update(Qt::ImQueryInput);
}

QVariant QQuickItem::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QQuickItem);
    QVariant v;

    switch (query) {
    case Qt::ImEnabled:
        v = (bool)(flags() & ItemAcceptsInputMethod);
        break;
    case Qt::ImHints:
        v = (int)inputMethodHints();
        break;
    case Qt::ImCursorRectangle:
    case Qt::ImFont:
    case Qt::ImCursorPosition:
    case Qt::ImSurroundingText:
    case Qt::ImCurrentSelection:
    case Qt::ImMaximumTextLength:
    case Qt::ImAnchorPosition:
    case Qt::ImPreferredLanguage:
        if (d->keyHandler)
            v = d->keyHandler->inputMethodQuery(query);
    default:
        break;
    }

    return v;
}

QQuickAnchorLine QQuickItemPrivate::left() const
{
    return anchorLines()->left;
}

QQuickAnchorLine QQuickItemPrivate::right() const
{
    return anchorLines()->right;
}

QQuickAnchorLine QQuickItemPrivate::horizontalCenter() const
{
    return anchorLines()->hCenter;
}

QQuickAnchorLine QQuickItemPrivate::top() const
{
    return anchorLines()->top;
}

QQuickAnchorLine QQuickItemPrivate::bottom() const
{
    return anchorLines()->bottom;
}

QQuickAnchorLine QQuickItemPrivate::verticalCenter() const
{
    return anchorLines()->vCenter;
}

QQuickAnchorLine QQuickItemPrivate::baseline() const
{
    return anchorLines()->baseline;
}

qreal QQuickItem::baselineOffset() const
{
    Q_D(const QQuickItem);
    if (!d->baselineOffset.isValid()) {
        return 0.0;
    } else
        return d->baselineOffset;
}

void QQuickItem::setBaselineOffset(qreal offset)
{
    Q_D(QQuickItem);
    if (offset == d->baselineOffset)
        return;

    d->baselineOffset = offset;

    for (int ii = 0; ii < d->changeListeners.count(); ++ii) {
        const QQuickItemPrivate::ChangeListener &change = d->changeListeners.at(ii);
        if (change.types & QQuickItemPrivate::Geometry) {
            QQuickAnchorsPrivate *anchor = change.listener->anchorPrivate();
            if (anchor)
                anchor->updateVerticalAnchors();
        }
    }
    emit baselineOffsetChanged(offset);
}

void QQuickItem::update()
{
    Q_D(QQuickItem);
    Q_ASSERT(flags() & ItemHasContents);
    d->dirty(QQuickItemPrivate::Content);
}

void QQuickItem::polish()
{
    Q_D(QQuickItem);
    if (!d->polishScheduled) {
        d->polishScheduled = true;
        if (d->canvas) {
            QQuickCanvasPrivate *p = QQuickCanvasPrivate::get(d->canvas);
            bool maybeupdate = p->itemsToPolish.isEmpty();
            p->itemsToPolish.insert(this);
            if (maybeupdate) d->canvas->maybeUpdate();
        }
    }
}

void QQuickItem::mapFromItem(QDeclarativeV8Function *args) const
{
    if (args->Length() != 0) {
        v8::Local<v8::Value> item = (*args)[0];
        QV8Engine *engine = args->engine();

        QQuickItem *itemObj = 0;
        if (!item->IsNull())
            itemObj = qobject_cast<QQuickItem*>(engine->toQObject(item));

        if (!itemObj && !item->IsNull()) {
            qmlInfo(this) << "mapFromItem() given argument \"" << engine->toString(item->ToString())
                          << "\" which is neither null nor an Item";
            return;
        }

        v8::Local<v8::Object> rv = v8::Object::New();
        args->returnValue(rv);

        qreal x = (args->Length() > 1)?(*args)[1]->NumberValue():0;
        qreal y = (args->Length() > 2)?(*args)[2]->NumberValue():0;

        QPointF p = mapFromItem(itemObj, QPointF(x, y));

        rv->Set(v8::String::New("x"), v8::Number::New(p.x()));
        rv->Set(v8::String::New("y"), v8::Number::New(p.y()));
    }
}

QTransform QQuickItem::itemTransform(QQuickItem *other, bool *ok) const
{
    Q_D(const QQuickItem);

    // XXX todo - we need to be able to handle common parents better and detect
    // invalid cases
    if (ok) *ok = true;

    QTransform t = d->itemToCanvasTransform();
    if (other) t *= QQuickItemPrivate::get(other)->canvasToItemTransform();

    return t;
}

void QQuickItem::mapToItem(QDeclarativeV8Function *args) const
{
    if (args->Length() != 0) {
        v8::Local<v8::Value> item = (*args)[0];
        QV8Engine *engine = args->engine();

        QQuickItem *itemObj = 0;
        if (!item->IsNull())
            itemObj = qobject_cast<QQuickItem*>(engine->toQObject(item));

        if (!itemObj && !item->IsNull()) {
            qmlInfo(this) << "mapToItem() given argument \"" << engine->toString(item->ToString())
                          << "\" which is neither null nor an Item";
            return;
        }

        v8::Local<v8::Object> rv = v8::Object::New();
        args->returnValue(rv);

        qreal x = (args->Length() > 1)?(*args)[1]->NumberValue():0;
        qreal y = (args->Length() > 2)?(*args)[2]->NumberValue():0;

        QPointF p = mapToItem(itemObj, QPointF(x, y));

        rv->Set(v8::String::New("x"), v8::Number::New(p.x()));
        rv->Set(v8::String::New("y"), v8::Number::New(p.y()));
    }
}

void QQuickItem::forceActiveFocus()
{
    setFocus(true);
    QQuickItem *parent = parentItem();
    while (parent) {
        if (parent->flags() & QQuickItem::ItemIsFocusScope) {
            parent->setFocus(true);
        }
        parent = parent->parentItem();
    }
}

QQuickItem *QQuickItem::childAt(qreal x, qreal y) const
{
    // XXX todo - should this include transform etc.?
    const QList<QQuickItem *> children = childItems();
    for (int i = children.count()-1; i >= 0; --i) {
        QQuickItem *child = children.at(i);
        if (child->isVisible() && child->x() <= x
                && child->x() + child->width() >= x
                && child->y() <= y
                && child->y() + child->height() >= y)
            return child;
    }
    return 0;
}

QDeclarativeListProperty<QObject> QQuickItemPrivate::resources()
{
    return QDeclarativeListProperty<QObject>(q_func(), 0, QQuickItemPrivate::resources_append,
                                             QQuickItemPrivate::resources_count,
                                             QQuickItemPrivate::resources_at,
                                             QQuickItemPrivate::resources_clear);
}

QDeclarativeListProperty<QQuickItem> QQuickItemPrivate::children()
{
    return QDeclarativeListProperty<QQuickItem>(q_func(), 0, QQuickItemPrivate::children_append,
                                             QQuickItemPrivate::children_count,
                                             QQuickItemPrivate::children_at,
                                             QQuickItemPrivate::children_clear);

}

QDeclarativeListProperty<QDeclarativeState> QQuickItemPrivate::states()
{
    return _states()->statesProperty();
}

QDeclarativeListProperty<QDeclarativeTransition> QQuickItemPrivate::transitions()
{
    return _states()->transitionsProperty();
}

QString QQuickItemPrivate::state() const
{
    if (!_stateGroup)
        return QString();
    else
        return _stateGroup->state();
}

void QQuickItemPrivate::setState(const QString &state)
{
    _states()->setState(state);
}

QString QQuickItem::state() const
{
    Q_D(const QQuickItem);
    return d->state();
}

void QQuickItem::setState(const QString &state)
{
    Q_D(QQuickItem);
    d->setState(state);
}

QDeclarativeListProperty<QQuickTransform> QQuickItem::transform()
{
    return QDeclarativeListProperty<QQuickTransform>(this, 0, QQuickItemPrivate::transform_append,
                                                     QQuickItemPrivate::transform_count,
                                                     QQuickItemPrivate::transform_at,
                                                     QQuickItemPrivate::transform_clear);
}

void QQuickItem::classBegin()
{
    Q_D(QQuickItem);
    d->componentComplete = false;
    if (d->_stateGroup)
        d->_stateGroup->classBegin();
    if (d->_anchors)
        d->_anchors->classBegin();
}

void QQuickItem::componentComplete()
{
    Q_D(QQuickItem);
    d->componentComplete = true;
    if (d->_stateGroup)
        d->_stateGroup->componentComplete();
    if (d->_anchors) {
        d->_anchors->componentComplete();
        QQuickAnchorsPrivate::get(d->_anchors)->updateOnComplete();
    }
    if (d->keyHandler)
        d->keyHandler->componentComplete();
    if (d->_contents)
        d->_contents->complete();
}

QDeclarativeStateGroup *QQuickItemPrivate::_states()
{
    Q_Q(QQuickItem);
    if (!_stateGroup) {
        _stateGroup = new QDeclarativeStateGroup;
        if (!componentComplete)
            _stateGroup->classBegin();
        FAST_CONNECT(_stateGroup, SIGNAL(stateChanged(QString)),
                     q, SIGNAL(stateChanged(QString)))
    }

    return _stateGroup;
}

QQuickItemPrivate::AnchorLines::AnchorLines(QQuickItem *q)
{
    left.item = q;
    left.anchorLine = QQuickAnchorLine::Left;
    right.item = q;
    right.anchorLine = QQuickAnchorLine::Right;
    hCenter.item = q;
    hCenter.anchorLine = QQuickAnchorLine::HCenter;
    top.item = q;
    top.anchorLine = QQuickAnchorLine::Top;
    bottom.item = q;
    bottom.anchorLine = QQuickAnchorLine::Bottom;
    vCenter.item = q;
    vCenter.anchorLine = QQuickAnchorLine::VCenter;
    baseline.item = q;
    baseline.anchorLine = QQuickAnchorLine::Baseline;
}

QPointF QQuickItemPrivate::computeTransformOrigin() const
{
    switch (origin) {
    default:
    case QQuickItem::TopLeft:
        return QPointF(0, 0);
    case QQuickItem::Top:
        return QPointF(width / 2., 0);
    case QQuickItem::TopRight:
        return QPointF(width, 0);
    case QQuickItem::Left:
        return QPointF(0, height / 2.);
    case QQuickItem::Center:
        return QPointF(width / 2., height / 2.);
    case QQuickItem::Right:
        return QPointF(width, height / 2.);
    case QQuickItem::BottomLeft:
        return QPointF(0, height);
    case QQuickItem::Bottom:
        return QPointF(width / 2., height);
    case QQuickItem::BottomRight:
        return QPointF(width, height);
    }
}

void QQuickItemPrivate::transformChanged()
{
}

void QQuickItemPrivate::deliverKeyEvent(QKeyEvent *e)
{
    Q_Q(QQuickItem);

    Q_ASSERT(e->isAccepted());
    if (keyHandler) {
        if (e->type() == QEvent::KeyPress)
            keyHandler->keyPressed(e, false);
        else
            keyHandler->keyReleased(e, false);

        if (e->isAccepted())
            return;
        else
            e->accept();
    }

    if (e->type() == QEvent::KeyPress)
        q->keyPressEvent(e);
    else
        q->keyReleaseEvent(e);

    if (e->isAccepted())
        return;

    if (keyHandler) {
        e->accept();

        if (e->type() == QEvent::KeyPress)
            keyHandler->keyPressed(e, true);
        else
            keyHandler->keyReleased(e, true);
    }
}

void QQuickItemPrivate::deliverInputMethodEvent(QInputMethodEvent *e)
{
    Q_Q(QQuickItem);

    Q_ASSERT(e->isAccepted());
    if (keyHandler) {
        keyHandler->inputMethodEvent(e, false);

        if (e->isAccepted())
            return;
        else
            e->accept();
    }

    q->inputMethodEvent(e);

    if (e->isAccepted())
        return;

    if (keyHandler) {
        e->accept();

        keyHandler->inputMethodEvent(e, true);
    }
}

void QQuickItemPrivate::deliverFocusEvent(QFocusEvent *e)
{
    Q_Q(QQuickItem);

    if (e->type() == QEvent::FocusIn) {
        q->focusInEvent(e);
    } else {
        q->focusOutEvent(e);
    }
}

void QQuickItemPrivate::deliverMouseEvent(QMouseEvent *e)
{
    Q_Q(QQuickItem);

    Q_ASSERT(e->isAccepted());

    switch (e->type()) {
    default:
        Q_ASSERT(!"Unknown event type");
    case QEvent::MouseMove:
        q->mouseMoveEvent(e);
        break;
    case QEvent::MouseButtonPress:
        q->mousePressEvent(e);
        break;
    case QEvent::MouseButtonRelease:
        q->mouseReleaseEvent(e);
        break;
    case QEvent::MouseButtonDblClick:
        q->mouseDoubleClickEvent(e);
        break;
    }
}

void QQuickItemPrivate::deliverWheelEvent(QWheelEvent *e)
{
    Q_Q(QQuickItem);
    q->wheelEvent(e);
}

void QQuickItemPrivate::deliverTouchEvent(QTouchEvent *e)
{
    Q_Q(QQuickItem);
    q->touchEvent(e);
}

void QQuickItemPrivate::deliverHoverEvent(QHoverEvent *e)
{
    Q_Q(QQuickItem);
    switch (e->type()) {
    default:
        Q_ASSERT(!"Unknown event type");
    case QEvent::HoverEnter:
        q->hoverEnterEvent(e);
        break;
    case QEvent::HoverLeave:
        q->hoverLeaveEvent(e);
        break;
    case QEvent::HoverMove:
        q->hoverMoveEvent(e);
        break;
    }
}

void QQuickItemPrivate::deliverDragEvent(QEvent *e)
{
    Q_Q(QQuickItem);
    switch (e->type()) {
    default:
        Q_ASSERT(!"Unknown event type");
    case QEvent::DragEnter:
        q->dragEnterEvent(static_cast<QDragEnterEvent *>(e));
        break;
    case QEvent::DragLeave:
        q->dragLeaveEvent(static_cast<QDragLeaveEvent *>(e));
        break;
    case QEvent::DragMove:
        q->dragMoveEvent(static_cast<QDragMoveEvent *>(e));
        break;
    case QEvent::Drop:
        q->dropEvent(static_cast<QDropEvent *>(e));
        break;
    }
}

void QQuickItem::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_UNUSED(change);
    Q_UNUSED(value);
}

/*! \internal */
// XXX todo - do we want/need this anymore?
// Note that it's now used for varying clip rect
QRectF QQuickItem::boundingRect() const
{
    Q_D(const QQuickItem);
    return QRectF(0, 0, d->width, d->height);
}

QQuickItem::TransformOrigin QQuickItem::transformOrigin() const
{
    Q_D(const QQuickItem);
    return d->origin;
}

void QQuickItem::setTransformOrigin(TransformOrigin origin)
{
    Q_D(QQuickItem);
    if (origin == d->origin)
        return;

    d->origin = origin;
    d->dirty(QQuickItemPrivate::TransformOrigin);

    emit transformOriginChanged(d->origin);
}

QPointF QQuickItem::transformOriginPoint() const
{
    Q_D(const QQuickItem);
    if (!d->transformOriginPoint.isNull())
        return d->transformOriginPoint;
    return d->computeTransformOrigin();
}

void QQuickItem::setTransformOriginPoint(const QPointF &point)
{
    Q_D(QQuickItem);
    if (d->transformOriginPoint == point)
        return;

    d->transformOriginPoint = point;
    d->dirty(QQuickItemPrivate::TransformOrigin);
}

qreal QQuickItem::z() const
{
    Q_D(const QQuickItem);
    return d->z;
}

void QQuickItem::setZ(qreal v)
{
    Q_D(QQuickItem);
    if (d->z == v)
        return;

    d->z = v;

    d->dirty(QQuickItemPrivate::ZValue);
    if (d->parentItem) {
        QQuickItemPrivate::get(d->parentItem)->dirty(QQuickItemPrivate::ChildrenStackingChanged);
        QQuickItemPrivate::get(d->parentItem)->markSortedChildrenDirty(this);
    }

    emit zChanged();
}


/*!
  \qmlproperty real QtQuick2::Item::rotation
  This property holds the rotation of the item in degrees clockwise.

  This specifies how many degrees to rotate the item around its transformOrigin.
  The default rotation is 0 degrees (i.e. not rotated at all).

  \table
  \row
  \o \image declarative-rotation.png
  \o
  \qml
  Rectangle {
      color: "blue"
      width: 100; height: 100
      Rectangle {
          color: "red"
          x: 25; y: 25; width: 50; height: 50
          rotation: 30
      }
  }
  \endqml
  \endtable

  \sa transform, Rotation
*/

/*!
  \qmlproperty real QtQuick2::Item::scale
  This property holds the scale of the item.

  A scale of less than 1 means the item will be displayed smaller than
  normal, and a scale of greater than 1 means the item will be
  displayed larger than normal.  A negative scale means the item will
  be mirrored.

  By default, items are displayed at a scale of 1 (i.e. at their
  normal size).

  Scaling is from the item's transformOrigin.

  \table
  \row
  \o \image declarative-scale.png
  \o
  \qml
  Rectangle {
      color: "blue"
      width: 100; height: 100
      Rectangle {
          color: "green"
          width: 25; height: 25
      }
      Rectangle {
          color: "red"
          x: 25; y: 25; width: 50; height: 50
          scale: 1.4
      }
  }
  \endqml
  \endtable

  \sa transform, Scale
*/

/*!
  \qmlproperty real QtQuick2::Item::opacity

  This property holds the opacity of the item.  Opacity is specified as a
  number between 0 (fully transparent) and 1 (fully opaque).  The default is 1.

  When this property is set, the specified opacity is also applied
  individually to child items.  In almost all cases this is what you want,
  but in some cases it may produce undesired results. For example in the
  second set of rectangles below, the red rectangle has specified an opacity
  of 0.5, which affects the opacity of its blue child rectangle even though
  the child has not specified an opacity.

  \table
  \row
  \o \image declarative-item_opacity1.png
  \o
  \qml
    Item {
        Rectangle {
            color: "red"
            width: 100; height: 100
            Rectangle {
                color: "blue"
                x: 50; y: 50; width: 100; height: 100
            }
        }
    }
  \endqml
  \row
  \o \image declarative-item_opacity2.png
  \o
  \qml
    Item {
        Rectangle {
            opacity: 0.5
            color: "red"
            width: 100; height: 100
            Rectangle {
                color: "blue"
                x: 50; y: 50; width: 100; height: 100
            }
        }
    }
  \endqml
  \endtable

  If an item's opacity is set to 0, the item will no longer receive mouse
  events, but will continue to receive key events and will retain the keyboard
  \l focus if it has been set. (In contrast, setting the \l visible property
  to \c false stops both mouse and keyboard events, and also removes focus
  from the item.)
*/

/*!
  Returns a value indicating whether mouse input should
  remain with this item exclusively.

  \sa setKeepMouseGrab()
 */

qreal QQuickItem::rotation() const
{
    Q_D(const QQuickItem);
    return d->rotation;
}

void QQuickItem::setRotation(qreal r)
{
    Q_D(QQuickItem);
    if (d->rotation == r)
        return;

    d->rotation = r;

    d->dirty(QQuickItemPrivate::BasicTransform);

    d->itemChange(ItemRotationHasChanged, r);

    emit rotationChanged();
}

qreal QQuickItem::scale() const
{
    Q_D(const QQuickItem);
    return d->scale;
}

void QQuickItem::setScale(qreal s)
{
    Q_D(QQuickItem);
    if (d->scale == s)
        return;

    d->scale = s;

    d->dirty(QQuickItemPrivate::BasicTransform);

    emit scaleChanged();
}

qreal QQuickItem::opacity() const
{
    Q_D(const QQuickItem);
    return d->opacity;
}

void QQuickItem::setOpacity(qreal o)
{
    Q_D(QQuickItem);
    if (d->opacity == o)
        return;

    d->opacity = o;

    d->dirty(QQuickItemPrivate::OpacityValue);

    d->itemChange(ItemOpacityHasChanged, o);

    emit opacityChanged();
}

bool QQuickItem::isVisible() const
{
    Q_D(const QQuickItem);
    return d->effectiveVisible;
}

void QQuickItem::setVisible(bool v)
{
    Q_D(QQuickItem);
    if (v == d->explicitVisible)
        return;

    d->explicitVisible = v;

    d->setEffectiveVisibleRecur(d->calcEffectiveVisible());
}

bool QQuickItem::isEnabled() const
{
    Q_D(const QQuickItem);
    return d->effectiveEnable;
}

void QQuickItem::setEnabled(bool e)
{
    Q_D(QQuickItem);
    if (e == d->explicitEnable)
        return;

    d->explicitEnable = e;

    d->setEffectiveEnableRecur(d->calcEffectiveEnable());
}

bool QQuickItemPrivate::calcEffectiveVisible() const
{
    // XXX todo - Should the effective visible of an element with no parent just be the current
    // effective visible?  This would prevent pointless re-processing in the case of an element
    // moving to/from a no-parent situation, but it is different from what graphics view does.
    return explicitVisible && (!parentItem || QQuickItemPrivate::get(parentItem)->effectiveVisible);
}

void QQuickItemPrivate::setEffectiveVisibleRecur(bool newEffectiveVisible)
{
    Q_Q(QQuickItem);

    if (newEffectiveVisible && !explicitVisible) {
        // This item locally overrides visibility
        return;
    }

    if (newEffectiveVisible == effectiveVisible) {
        // No change necessary
        return;
    }

    effectiveVisible = newEffectiveVisible;
    dirty(Visible);
    if (parentItem) QQuickItemPrivate::get(parentItem)->dirty(ChildrenStackingChanged);

    if (canvas) {
        QQuickCanvasPrivate *canvasPriv = QQuickCanvasPrivate::get(canvas);
        if (canvasPriv->mouseGrabberItem == q)
            q->ungrabMouse();
    }

    for (int ii = 0; ii < childItems.count(); ++ii)
        QQuickItemPrivate::get(childItems.at(ii))->setEffectiveVisibleRecur(newEffectiveVisible);

    for (int ii = 0; ii < changeListeners.count(); ++ii) {
        const QQuickItemPrivate::ChangeListener &change = changeListeners.at(ii);
        if (change.types & QQuickItemPrivate::Visibility)
            change.listener->itemVisibilityChanged(q);
    }

    emit q->visibleChanged();
}

bool QQuickItemPrivate::calcEffectiveEnable() const
{
    // XXX todo - Should the effective enable of an element with no parent just be the current
    // effective enable?  This would prevent pointless re-processing in the case of an element
    // moving to/from a no-parent situation, but it is different from what graphics view does.
    return explicitEnable && (!parentItem || QQuickItemPrivate::get(parentItem)->effectiveEnable);
}

void QQuickItemPrivate::setEffectiveEnableRecur(bool newEffectiveEnable)
{
    Q_Q(QQuickItem);

    // XXX todo - need to fixup focus

    if (newEffectiveEnable && !explicitEnable) {
        // This item locally overrides enable
        return;
    }

    if (newEffectiveEnable == effectiveEnable) {
        // No change necessary
        return;
    }

    effectiveEnable = newEffectiveEnable;

    if (canvas) {
        QQuickCanvasPrivate *canvasPriv = QQuickCanvasPrivate::get(canvas);
        if (canvasPriv->mouseGrabberItem == q)
            q->ungrabMouse();
    }

    for (int ii = 0; ii < childItems.count(); ++ii)
        QQuickItemPrivate::get(childItems.at(ii))->setEffectiveEnableRecur(newEffectiveEnable);

    emit q->enabledChanged();
}

QString QQuickItemPrivate::dirtyToString() const
{
#define DIRTY_TO_STRING(value) if (dirtyAttributes & value) { \
    if (!rv.isEmpty()) \
        rv.append(QLatin1String("|")); \
    rv.append(QLatin1String(#value)); \
}

//    QString rv = QLatin1String("0x") + QString::number(dirtyAttributes, 16);
    QString rv;

    DIRTY_TO_STRING(TransformOrigin);
    DIRTY_TO_STRING(Transform);
    DIRTY_TO_STRING(BasicTransform);
    DIRTY_TO_STRING(Position);
    DIRTY_TO_STRING(Size);
    DIRTY_TO_STRING(ZValue);
    DIRTY_TO_STRING(Content);
    DIRTY_TO_STRING(Smooth);
    DIRTY_TO_STRING(OpacityValue);
    DIRTY_TO_STRING(ChildrenChanged);
    DIRTY_TO_STRING(ChildrenStackingChanged);
    DIRTY_TO_STRING(ParentChanged);
    DIRTY_TO_STRING(Clip);
    DIRTY_TO_STRING(Canvas);
    DIRTY_TO_STRING(EffectReference);
    DIRTY_TO_STRING(Visible);
    DIRTY_TO_STRING(HideReference);

    return rv;
}

void QQuickItemPrivate::dirty(DirtyType type)
{
    Q_Q(QQuickItem);
    if (type & (TransformOrigin | Transform | BasicTransform | Position | Size))
        transformChanged();

    if (!(dirtyAttributes & type) || (canvas && !prevDirtyItem)) {
        dirtyAttributes |= type;
        if (canvas) {
            addToDirtyList();
            QQuickCanvasPrivate::get(canvas)->dirtyItem(q);
        }
    }
}

void QQuickItemPrivate::addToDirtyList()
{
    Q_Q(QQuickItem);

    Q_ASSERT(canvas);
    if (!prevDirtyItem) {
        Q_ASSERT(!nextDirtyItem);

        QQuickCanvasPrivate *p = QQuickCanvasPrivate::get(canvas);
        nextDirtyItem = p->dirtyItemList;
        if (nextDirtyItem) QQuickItemPrivate::get(nextDirtyItem)->prevDirtyItem = &nextDirtyItem;
        prevDirtyItem = &p->dirtyItemList;
        p->dirtyItemList = q;
        p->dirtyItem(q);
    }
    Q_ASSERT(prevDirtyItem);
}

void QQuickItemPrivate::removeFromDirtyList()
{
    if (prevDirtyItem) {
        if (nextDirtyItem) QQuickItemPrivate::get(nextDirtyItem)->prevDirtyItem = prevDirtyItem;
        *prevDirtyItem = nextDirtyItem;
        prevDirtyItem = 0;
        nextDirtyItem = 0;
    }
    Q_ASSERT(!prevDirtyItem);
    Q_ASSERT(!nextDirtyItem);
}

void QQuickItemPrivate::refFromEffectItem(bool hide)
{
    ++effectRefCount;
    if (1 == effectRefCount) {
        dirty(EffectReference);
        if (parentItem) QQuickItemPrivate::get(parentItem)->dirty(ChildrenStackingChanged);
    }
    if (hide) {
        if (++hideRefCount == 1)
            dirty(HideReference);
    }
}

void QQuickItemPrivate::derefFromEffectItem(bool unhide)
{
    Q_ASSERT(effectRefCount);
    --effectRefCount;
    if (0 == effectRefCount) {
        dirty(EffectReference);
        if (parentItem) QQuickItemPrivate::get(parentItem)->dirty(ChildrenStackingChanged);
    }
    if (unhide) {
        if (--hideRefCount == 0)
            dirty(HideReference);
    }
}

void QQuickItemPrivate::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    Q_Q(QQuickItem);
    switch (change) {
    case QQuickItem::ItemChildAddedChange:
        q->itemChange(change, data);
        for (int ii = 0; ii < changeListeners.count(); ++ii) {
            const QQuickItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QQuickItemPrivate::Children) {
                change.listener->itemChildAdded(q, data.item);
            }
        }
        break;
    case QQuickItem::ItemChildRemovedChange:
        q->itemChange(change, data);
        for (int ii = 0; ii < changeListeners.count(); ++ii) {
            const QQuickItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QQuickItemPrivate::Children) {
                change.listener->itemChildRemoved(q, data.item);
            }
        }
        break;
    case QQuickItem::ItemSceneChange:
        q->itemChange(change, data);
        break;
    case QQuickItem::ItemVisibleHasChanged:
        q->itemChange(change, data);
        for (int ii = 0; ii < changeListeners.count(); ++ii) {
            const QQuickItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QQuickItemPrivate::Visibility) {
                change.listener->itemVisibilityChanged(q);
            }
        }
        break;
    case QQuickItem::ItemParentHasChanged:
        q->itemChange(change, data);
        for (int ii = 0; ii < changeListeners.count(); ++ii) {
            const QQuickItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QQuickItemPrivate::Parent) {
                change.listener->itemParentChanged(q, data.item);
            }
        }
        break;
    case QQuickItem::ItemOpacityHasChanged:
        q->itemChange(change, data);
        for (int ii = 0; ii < changeListeners.count(); ++ii) {
            const QQuickItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QQuickItemPrivate::Opacity) {
                change.listener->itemOpacityChanged(q);
            }
        }
        break;
    case QQuickItem::ItemActiveFocusHasChanged:
        q->itemChange(change, data);
        break;
    case QQuickItem::ItemRotationHasChanged:
        q->itemChange(change, data);
        for (int ii = 0; ii < changeListeners.count(); ++ii) {
            const QQuickItemPrivate::ChangeListener &change = changeListeners.at(ii);
            if (change.types & QQuickItemPrivate::Rotation) {
                change.listener->itemRotationChanged(q);
            }
        }
        break;
    }
}

/*!
    \property QQuickItem::smooth
    \brief whether the item is smoothly transformed.

    This property is provided purely for the purpose of optimization. Turning
    smooth transforms off is faster, but looks worse; turning smooth
    transformations on is slower, but looks better.

    By default smooth transformations are off.
*/

/*!
    Returns true if the item should be drawn with antialiasing and
    smooth pixmap filtering, false otherwise.

    The default is false.

    \sa setSmooth()
*/
bool QQuickItem::smooth() const
{
    Q_D(const QQuickItem);
    return d->smooth;
}

/*!
    Sets whether the item should be drawn with antialiasing and
    smooth pixmap filtering to \a smooth.

    \sa smooth()
*/
void QQuickItem::setSmooth(bool smooth)
{
    Q_D(QQuickItem);
    if (d->smooth == smooth)
        return;

    d->smooth = smooth;
    d->dirty(QQuickItemPrivate::Smooth);

    emit smoothChanged(smooth);
}

QQuickItem::Flags QQuickItem::flags() const
{
    Q_D(const QQuickItem);
    return (QQuickItem::Flags)d->flags;
}

void QQuickItem::setFlag(Flag flag, bool enabled)
{
    Q_D(QQuickItem);
    if (enabled)
        setFlags((Flags)(d->flags | (quint32)flag));
    else
        setFlags((Flags)(d->flags & ~(quint32)flag));
}

void QQuickItem::setFlags(Flags flags)
{
    Q_D(QQuickItem);

    if ((flags & ItemIsFocusScope) != (d->flags & ItemIsFocusScope)) {
        if (flags & ItemIsFocusScope && !d->childItems.isEmpty() && d->canvas) {
            qWarning("QQuickItem: Cannot set FocusScope once item has children and is in a canvas.");
            flags &= ~ItemIsFocusScope;
        } else if (d->flags & ItemIsFocusScope) {
            qWarning("QQuickItem: Cannot unset FocusScope flag.");
            flags |= ItemIsFocusScope;
        }
    }

    if ((flags & ItemClipsChildrenToShape ) != (d->flags & ItemClipsChildrenToShape))
        d->dirty(QQuickItemPrivate::Clip);

    d->flags = flags;
}

qreal QQuickItem::x() const
{
    Q_D(const QQuickItem);
    return d->x;
}

qreal QQuickItem::y() const
{
    Q_D(const QQuickItem);
    return d->y;
}

QPointF QQuickItem::pos() const
{
    Q_D(const QQuickItem);
    return QPointF(d->x, d->y);
}

void QQuickItem::setX(qreal v)
{
    Q_D(QQuickItem);
    if (d->x == v)
        return;

    qreal oldx = d->x;
    d->x = v;

    d->dirty(QQuickItemPrivate::Position);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(oldx, y(), width(), height()));
}

void QQuickItem::setY(qreal v)
{
    Q_D(QQuickItem);
    if (d->y == v)
        return;

    qreal oldy = d->y;
    d->y = v;

    d->dirty(QQuickItemPrivate::Position);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), oldy, width(), height()));
}

void QQuickItem::setPos(const QPointF &pos)
{
    Q_D(QQuickItem);
    if (QPointF(d->x, d->y) == pos)
        return;

    qreal oldx = d->x;
    qreal oldy = d->y;

    d->x = pos.x();
    d->y = pos.y();

    d->dirty(QQuickItemPrivate::Position);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(oldx, oldy, width(), height()));
}

qreal QQuickItem::width() const
{
    Q_D(const QQuickItem);
    return d->width;
}

void QQuickItem::setWidth(qreal w)
{
    Q_D(QQuickItem);
    if (qIsNaN(w))
        return;

    d->widthValid = true;
    if (d->width == w)
        return;

    qreal oldWidth = d->width;
    d->width = w;

    d->dirty(QQuickItemPrivate::Size);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), oldWidth, height()));
}

void QQuickItem::resetWidth()
{
    Q_D(QQuickItem);
    d->widthValid = false;
    setImplicitWidth(implicitWidth());
}

void QQuickItemPrivate::implicitWidthChanged()
{
    Q_Q(QQuickItem);
    emit q->implicitWidthChanged();
}

qreal QQuickItemPrivate::getImplicitWidth() const
{
    return implicitWidth;
}
/*!
    Returns the width of the item that is implied by other properties that determine the content.
*/
qreal QQuickItem::implicitWidth() const
{
    Q_D(const QQuickItem);
    return d->getImplicitWidth();
}

/*!
    \qmlproperty real QtQuick2::Item::implicitWidth
    \qmlproperty real QtQuick2::Item::implicitHeight

    Defines the natural width or height of the Item if no \l width or \l height is specified.

    The default implicit size for most items is 0x0, however some elements have an inherent
    implicit size which cannot be overridden, e.g. Image, Text.

    Setting the implicit size is useful for defining components that have a preferred size
    based on their content, for example:

    \qml
    // Label.qml
    import QtQuick 1.1

    Item {
        property alias icon: image.source
        property alias label: text.text
        implicitWidth: text.implicitWidth + image.implicitWidth
        implicitHeight: Math.max(text.implicitHeight, image.implicitHeight)
        Image { id: image }
        Text {
            id: text
            wrapMode: Text.Wrap
            anchors.left: image.right; anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    \endqml

    \bold Note: using implicitWidth of Text or TextEdit and setting the width explicitly
    incurs a performance penalty as the text must be laid out twice.
*/

/*!
    Sets the implied width of the item to \a w.
    This is the width implied by other properties that determine the content.
*/
void QQuickItem::setImplicitWidth(qreal w)
{
    Q_D(QQuickItem);
    bool changed = w != d->implicitWidth;
    d->implicitWidth = w;
    if (d->width == w || widthValid()) {
        if (changed)
            d->implicitWidthChanged();
        return;
    }

    qreal oldWidth = d->width;
    d->width = w;

    d->dirty(QQuickItemPrivate::Size);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), oldWidth, height()));

    if (changed)
        d->implicitWidthChanged();
}

/*!
    Returns whether the width property has been set explicitly.
*/
bool QQuickItem::widthValid() const
{
    Q_D(const QQuickItem);
    return d->widthValid;
}

qreal QQuickItem::height() const
{
    Q_D(const QQuickItem);
    return d->height;
}

void QQuickItem::setHeight(qreal h)
{
    Q_D(QQuickItem);
    if (qIsNaN(h))
        return;

    d->heightValid = true;
    if (d->height == h)
        return;

    qreal oldHeight = d->height;
    d->height = h;

    d->dirty(QQuickItemPrivate::Size);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), width(), oldHeight));
}

void QQuickItem::resetHeight()
{
    Q_D(QQuickItem);
    d->heightValid = false;
    setImplicitHeight(implicitHeight());
}

void QQuickItemPrivate::implicitHeightChanged()
{
    Q_Q(QQuickItem);
    emit q->implicitHeightChanged();
}

qreal QQuickItemPrivate::getImplicitHeight() const
{
    return implicitHeight;
}

/*!
    Returns the height of the item that is implied by other properties that determine the content.
*/
qreal QQuickItem::implicitHeight() const
{
    Q_D(const QQuickItem);
    return d->getImplicitHeight();
}


/*!
    Sets the implied height of the item to \a h.
    This is the height implied by other properties that determine the content.
*/
void QQuickItem::setImplicitHeight(qreal h)
{
    Q_D(QQuickItem);
    bool changed = h != d->implicitHeight;
    d->implicitHeight = h;
    if (d->height == h || heightValid()) {
        if (changed)
            d->implicitHeightChanged();
        return;
    }

    qreal oldHeight = d->height;
    d->height = h;

    d->dirty(QQuickItemPrivate::Size);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), width(), oldHeight));

    if (changed)
        d->implicitHeightChanged();
}

void QQuickItem::setImplicitSize(qreal w, qreal h)
{
    Q_D(QQuickItem);
    bool wChanged = w != d->implicitWidth;
    bool hChanged = h != d->implicitHeight;

    d->implicitWidth = w;
    d->implicitHeight = h;

    bool wDone = false;
    bool hDone = false;
    if (d->width == w || widthValid()) {
        if (wChanged)
            d->implicitWidthChanged();
        wDone = true;
    }
    if (d->height == h || heightValid()) {
        if (hChanged)
            d->implicitHeightChanged();
        hDone = true;
    }
    if (wDone && hDone)
        return;

    qreal oldWidth = d->width;
    qreal oldHeight = d->height;
    if (!wDone)
        d->width = w;
    if (!hDone)
        d->height = h;

    d->dirty(QQuickItemPrivate::Size);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), oldWidth, oldHeight));

    if (!wDone && wChanged)
        d->implicitWidthChanged();
    if (!hDone && hChanged)
        d->implicitHeightChanged();
}

/*!
    Returns whether the height property has been set explicitly.
*/
bool QQuickItem::heightValid() const
{
    Q_D(const QQuickItem);
    return d->heightValid;
}

void QQuickItem::setSize(const QSizeF &size)
{
    Q_D(QQuickItem);
    d->heightValid = true;
    d->widthValid = true;

    if (QSizeF(d->width, d->height) == size)
        return;

    qreal oldHeight = d->height;
    qreal oldWidth = d->width;
    d->height = size.height();
    d->width = size.width();

    d->dirty(QQuickItemPrivate::Size);

    geometryChanged(QRectF(x(), y(), width(), height()),
                    QRectF(x(), y(), oldWidth, oldHeight));
}

bool QQuickItem::hasActiveFocus() const
{
    Q_D(const QQuickItem);
    return d->activeFocus;
}

bool QQuickItem::hasFocus() const
{
    Q_D(const QQuickItem);
    return d->focus;
}

void QQuickItem::setFocus(bool focus)
{
    Q_D(QQuickItem);
    if (d->focus == focus)
        return;

    if (d->canvas) {
        // Need to find our nearest focus scope
        QQuickItem *scope = parentItem();
        while (scope && !scope->isFocusScope())
            scope = scope->parentItem();
        if (focus)
            QQuickCanvasPrivate::get(d->canvas)->setFocusInScope(scope, this);
        else
            QQuickCanvasPrivate::get(d->canvas)->clearFocusInScope(scope, this);
    } else {
        d->focus = focus;
        emit focusChanged(focus);
    }
}

bool QQuickItem::isFocusScope() const
{
    return flags() & ItemIsFocusScope;
}

QQuickItem *QQuickItem::scopedFocusItem() const
{
    Q_D(const QQuickItem);
    if (!isFocusScope())
        return 0;
    else
        return d->subFocusItem;
}


Qt::MouseButtons QQuickItem::acceptedMouseButtons() const
{
    Q_D(const QQuickItem);
    return d->acceptedMouseButtons;
}

void QQuickItem::setAcceptedMouseButtons(Qt::MouseButtons buttons)
{
    Q_D(QQuickItem);
    d->acceptedMouseButtons = buttons;
}

bool QQuickItem::filtersChildMouseEvents() const
{
    Q_D(const QQuickItem);
    return d->filtersChildMouseEvents;
}

void QQuickItem::setFiltersChildMouseEvents(bool filter)
{
    Q_D(QQuickItem);
    d->filtersChildMouseEvents = filter;
}

bool QQuickItem::isUnderMouse() const
{
    Q_D(const QQuickItem);
    if (!d->canvas)
        return false;

    QPoint cursorPos = QCursor::pos();
    if (QRectF(0, 0, width(), height()).contains(mapFromScene(cursorPos))) // ### refactor: d->canvas->mapFromGlobal(cursorPos))))
        return true;
    return false;
}

bool QQuickItem::acceptHoverEvents() const
{
    Q_D(const QQuickItem);
    return d->hoverEnabled;
}

void QQuickItem::setAcceptHoverEvents(bool enabled)
{
    Q_D(QQuickItem);
    d->hoverEnabled = enabled;
}

void QQuickItem::grabMouse()
{
    Q_D(QQuickItem);
    if (!d->canvas)
        return;
    QQuickCanvasPrivate *canvasPriv = QQuickCanvasPrivate::get(d->canvas);
    if (canvasPriv->mouseGrabberItem == this)
        return;

    QQuickItem *oldGrabber = canvasPriv->mouseGrabberItem;
    canvasPriv->mouseGrabberItem = this;
    if (oldGrabber)
        oldGrabber->mouseUngrabEvent();
}

void QQuickItem::ungrabMouse()
{
    Q_D(QQuickItem);
    if (!d->canvas)
        return;
    QQuickCanvasPrivate *canvasPriv = QQuickCanvasPrivate::get(d->canvas);
    if (canvasPriv->mouseGrabberItem != this) {
        qWarning("QQuickItem::ungrabMouse(): Item is not the mouse grabber.");
        return;
    }

    canvasPriv->mouseGrabberItem = 0;
    mouseUngrabEvent();
}

bool QQuickItem::keepMouseGrab() const
{
    Q_D(const QQuickItem);
    return d->keepMouse;
}

/*!
  The flag indicating whether the mouse should remain
  with this item is set to \a keep.

  This is useful for items that wish to grab and keep mouse
  interaction following a predefined gesture.  For example,
  an item that is interested in horizontal mouse movement
  may set keepMouseGrab to true once a threshold has been
  exceeded.  Once keepMouseGrab has been set to true, filtering
  items will not react to mouse events.

  If the item does not indicate that it wishes to retain mouse grab,
  a filtering item may steal the grab. For example, Flickable may attempt
  to steal a mouse grab if it detects that the user has begun to
  move the viewport.

  \sa keepMouseGrab()
 */
void QQuickItem::setKeepMouseGrab(bool keep)
{
    Q_D(QQuickItem);
    d->keepMouse = keep;
}

/*!
    Grabs the touch points specified by \a ids.

    These touch points will be owned by the item until
    they are released. Alternatively, the grab can be stolen
    by a filtering item like Flickable. Use setKeepTouchGrab()
    to prevent the grab from being stolen.

    \sa ungrabTouchPoints(), setKeepTouchGrab()
*/
void QQuickItem::grabTouchPoints(const QList<int> &ids)
{
    Q_D(QQuickItem);
    if (!d->canvas)
        return;
    QQuickCanvasPrivate *canvasPriv = QQuickCanvasPrivate::get(d->canvas);

    QSet<QQuickItem*> ungrab;
    for (int i = 0; i < ids.count(); ++i) {
        QQuickItem *oldGrabber = canvasPriv->itemForTouchPointId.value(ids.at(i));
        if (oldGrabber == this)
            return;

        canvasPriv->itemForTouchPointId[ids.at(i)] = this;
        if (oldGrabber)
            ungrab.insert(oldGrabber);
    }
    foreach (QQuickItem *oldGrabber, ungrab)
        oldGrabber->touchUngrabEvent();
}

/*!
    Ungrabs the touch points owned by this item.

    \sa grabTouchPoints()
*/
void QQuickItem::ungrabTouchPoints()
{
    Q_D(QQuickItem);
    if (!d->canvas)
        return;
    QQuickCanvasPrivate *canvasPriv = QQuickCanvasPrivate::get(d->canvas);

    QMutableHashIterator<int, QQuickItem*> i(canvasPriv->itemForTouchPointId);
    while (i.hasNext()) {
        i.next();
        if (i.value() == this)
            i.remove();
    }
    touchUngrabEvent();
}

/*!
    Returns a value indicating whether the touch points grabbed by this item
    should remain with this item exclusively.

    \sa setKeepTouchGrab(), keepMouseGrab()
*/
bool QQuickItem::keepTouchGrab() const
{
    Q_D(const QQuickItem);
    return d->keepTouch;
}

/*!
  The flag indicating whether the touch points grabbed
  by this item should remain with this item is set to \a keep.

  This is useful for items that wish to grab and keep specific touch
  points following a predefined gesture.  For example,
  an item that is interested in horizontal touch point movement
  may set setKeepTouchGrab to true once a threshold has been
  exceeded.  Once setKeepTouchGrab has been set to true, filtering
  items will not react to the relevant touch points.

  If the item does not indicate that it wishes to retain touch point grab,
  a filtering item may steal the grab. For example, Flickable may attempt
  to steal a touch point grab if it detects that the user has begun to
  move the viewport.

  \sa keepTouchGrab(), setKeepMouseGrab()
 */
void QQuickItem::setKeepTouchGrab(bool keep)
{
    Q_D(QQuickItem);
    d->keepTouch = keep;
}

/*!
    \qmlmethod object QtQuick2::Item::mapFromItem(Item item, real x, real y)

    Maps the point (\a x, \a y), which is in \a item's coordinate system, to
    this item's coordinate system, and returns an object with \c x and \c y
    properties matching the mapped coordinate.

    If \a item is a \c null value, this maps the point from the coordinate
    system of the root QML view.
*/
/*!
    \qmlmethod object QtQuick2::Item::mapToItem(Item item, real x, real y)

    Maps the point (\a x, \a y), which is in this item's coordinate system, to
    \a item's coordinate system, and returns an object with \c x and \c y
    properties matching the mapped coordinate.

    If \a item is a \c null value, this maps \a x and \a y to the coordinate
    system of the root QML view.
*/
QPointF QQuickItem::mapToItem(const QQuickItem *item, const QPointF &point) const
{
    QPointF p = mapToScene(point);
    if (item)
        p = item->mapFromScene(p);
    return p;
}

QPointF QQuickItem::mapToScene(const QPointF &point) const
{
    Q_D(const QQuickItem);
    return d->itemToCanvasTransform().map(point);
}

QRectF QQuickItem::mapRectToItem(const QQuickItem *item, const QRectF &rect) const
{
    Q_D(const QQuickItem);
    QTransform t = d->itemToCanvasTransform();
    if (item)
        t *= QQuickItemPrivate::get(item)->canvasToItemTransform();
    return t.mapRect(rect);
}

QRectF QQuickItem::mapRectToScene(const QRectF &rect) const
{
    Q_D(const QQuickItem);
    return d->itemToCanvasTransform().mapRect(rect);
}

QPointF QQuickItem::mapFromItem(const QQuickItem *item, const QPointF &point) const
{
    QPointF p = item?item->mapToScene(point):point;
    return mapFromScene(p);
}

QPointF QQuickItem::mapFromScene(const QPointF &point) const
{
    Q_D(const QQuickItem);
    return d->canvasToItemTransform().map(point);
}

QRectF QQuickItem::mapRectFromItem(const QQuickItem *item, const QRectF &rect) const
{
    Q_D(const QQuickItem);
    QTransform t = item?QQuickItemPrivate::get(item)->itemToCanvasTransform():QTransform();
    t *= d->canvasToItemTransform();
    return t.mapRect(rect);
}

QRectF QQuickItem::mapRectFromScene(const QRectF &rect) const
{
    Q_D(const QQuickItem);
    return d->canvasToItemTransform().mapRect(rect);
}


/*!
    \qmlmethod QtQuick2::Item::forceActiveFocus()

    Forces active focus on the item.

    This method sets focus on the item and makes sure that all the focus scopes
    higher in the object hierarchy are also given the focus.
*/

/*!
    Forces active focus on the item.

    This method sets focus on the item and makes sure that all the focus scopes
    higher in the object hierarchy are also given the focus.
*/

/*!
  \qmlmethod QtQuick2::Item::childAt(real x, real y)

  Returns the visible child item at point (\a x, \a y), which is in this
  item's coordinate system, or \c null if there is no such item.
*/

/*!
  Returns the visible child item at point (\a x, \a y), which is in this
  item's coordinate system, or 0 if there is no such item.
*/

/*!
  \qmlproperty list<State> QtQuick2::Item::states
  This property holds a list of states defined by the item.

  \qml
  Item {
      states: [
          State {
              // ...
          },
          State {
              // ...
          }
          // ...
      ]
  }
  \endqml

  \sa {qmlstate}{States}
*/
/*!
  \qmlproperty list<Transition> QtQuick2::Item::transitions
  This property holds a list of transitions defined by the item.

  \qml
  Item {
      transitions: [
          Transition {
              // ...
          },
          Transition {
              // ...
          }
          // ...
      ]
  }
  \endqml

  \sa {QML Animation and Transitions}{Transitions}
*/
/*
  \qmlproperty list<Filter> QtQuick2::Item::filter
  This property holds a list of graphical filters to be applied to the item.

  \l {Filter}{Filters} include things like \l {Blur}{blurring}
  the item, or giving it a \l Reflection.  Some
  filters may not be available on all canvases; if a filter is not
  available on a certain canvas, it will simply not be applied for
  that canvas (but the QML will still be considered valid).

  \qml
  Item {
      filter: [
          Blur {
              // ...
          },
          Reflection {
              // ...
          }
          // ...
      ]
  }
  \endqml
*/

/*!
  \qmlproperty bool QtQuick2::Item::clip
  This property holds whether clipping is enabled. The default clip value is \c false.

  If clipping is enabled, an item will clip its own painting, as well
  as the painting of its children, to its bounding rectangle.

  Non-rectangular clipping regions are not supported for performance reasons.
*/

/*!
  \property QQuickItem::clip
  This property holds whether clipping is enabled. The default clip value is \c false.

  If clipping is enabled, an item will clip its own painting, as well
  as the painting of its children, to its bounding rectangle. If you set
  clipping during an item's paint operation, remember to re-set it to
  prevent clipping the rest of your scene.

  Non-rectangular clipping regions are not supported for performance reasons.
*/

/*!
  \qmlproperty string QtQuick2::Item::state

  This property holds the name of the current state of the item.

  This property is often used in scripts to change between states. For
  example:

  \js
  function toggle() {
      if (button.state == 'On')
          button.state = 'Off';
      else
          button.state = 'On';
  }
  \endjs

  If the item is in its base state (i.e. no explicit state has been
  set), \c state will be a blank string. Likewise, you can return an
  item to its base state by setting its current state to \c ''.

  \sa {qmlstates}{States}
*/

/*!
  \qmlproperty list<Transform> QtQuick2::Item::transform
  This property holds the list of transformations to apply.

  For more information see \l Transform.
*/

/*!
    \enum QQuickItem::TransformOrigin

    Controls the point about which simple transforms like scale apply.

    \value TopLeft The top-left corner of the item.
    \value Top The center point of the top of the item.
    \value TopRight The top-right corner of the item.
    \value Left The left most point of the vertical middle.
    \value Center The center of the item.
    \value Right The right most point of the vertical middle.
    \value BottomLeft The bottom-left corner of the item.
    \value Bottom The center point of the bottom of the item.
    \value BottomRight The bottom-right corner of the item.
*/


/*!
  \qmlproperty bool QtQuick2::Item::activeFocus

  This property indicates whether the item has active focus.

  An item with active focus will receive keyboard input,
  or is a FocusScope ancestor of the item that will receive keyboard input.

  Usually, activeFocus is gained by setting focus on an item and its enclosing
  FocusScopes. In the following example \c input will have activeFocus.
  \qml
  Rectangle {
      FocusScope {
          focus: true
          TextInput {
              id: input
              focus: true
          }
      }
  }
  \endqml

  \sa focus, {qmlfocus}{Keyboard Focus}
*/

/*!
  \qmlproperty bool QtQuick2::Item::focus
  This property indicates whether the item has focus within the enclosing focus scope. If true, this item
  will gain active focus when the enclosing focus scope gains active focus.
  In the following example, \c input will be given active focus when \c scope gains active focus.
  \qml
  Rectangle {
      FocusScope {
          id: scope
          TextInput {
              id: input
              focus: true
          }
      }
  }
  \endqml

  For the purposes of this property, the scene as a whole is assumed to act like a focus scope.
  On a practical level, that means the following QML will give active focus to \c input on startup.

  \qml
  Rectangle {
      TextInput {
          id: input
          focus: true
      }
  }
  \endqml

  \sa activeFocus, {qmlfocus}{Keyboard Focus}
*/


/*!
  \property QQuickItem::anchors
  \internal
*/

/*!
  \property QQuickItem::left
  \internal
*/

/*!
  \property QQuickItem::right
  \internal
*/

/*!
  \property QQuickItem::horizontalCenter
  \internal
*/

/*!
  \property QQuickItem::top
  \internal
*/

/*!
  \property QQuickItem::bottom
  \internal
*/

/*!
  \property QQuickItem::verticalCenter
  \internal
*/

/*!
  \property QQuickItem::focus
  \internal
*/

/*!
  \property QQuickItem::transform
  \internal
*/

/*!
  \property QQuickItem::transformOrigin
  \internal
*/

/*!
  \property QQuickItem::activeFocus
  \internal
*/

/*!
  \property QQuickItem::baseline
  \internal
*/

/*!
  \property QQuickItem::data
  \internal
*/

/*!
  \property QQuickItem::resources
  \internal
*/

/*!
  \property QQuickItem::state
  \internal
*/

/*!
  \property QQuickItem::states
  \internal
*/

/*!
  \property QQuickItem::transformOriginPoint
  \internal
*/

/*!
  \property QQuickItem::transitions
  \internal
*/

bool QQuickItem::event(QEvent *ev)
{
#if 0
    if (ev->type() == QEvent::PolishRequest) {
        Q_D(QQuickItem);
        d->polishScheduled = false;
        updatePolish();
        return true;
    } else {
        return QObject::event(ev);
    }
#endif
    if (ev->type() == QEvent::InputMethodQuery) {
        QInputMethodQueryEvent *query = static_cast<QInputMethodQueryEvent *>(ev);
        Qt::InputMethodQueries queries = query->queries();
        for (uint i = 0; i < 32; ++i) {
            Qt::InputMethodQuery q = (Qt::InputMethodQuery)(int)(queries & (1<<i));
            if (q) {
                QVariant v = inputMethodQuery(q);
                query->setValue(q, v);
            }
        }
        query->accept();
        return true;
    } else if (ev->type() == QEvent::InputMethod) {
        inputMethodEvent(static_cast<QInputMethodEvent *>(ev));
        return true;
    }
    return QObject::event(ev);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, QQuickItem *item)
{
    if (!item) {
        debug << "QQuickItem(0)";
        return debug;
    }

    debug << item->metaObject()->className() << "(this =" << ((void*)item)
          << ", name=" << item->objectName()
          << ", parent =" << ((void*)item->parentItem())
          << ", geometry =" << QRectF(item->pos(), QSizeF(item->width(), item->height()))
          << ", z =" << item->z() << ')';
    return debug;
}
#endif

qint64 QQuickItemPrivate::consistentTime = -1;
void QQuickItemPrivate::setConsistentTime(qint64 t)
{
    consistentTime = t;
}

class QElapsedTimerConsistentTimeHack
{
public:
    void start() {
        t1 = QQuickItemPrivate::consistentTime;
        t2 = 0;
    }
    qint64 elapsed() {
        return QQuickItemPrivate::consistentTime - t1;
    }
    qint64 restart() {
        qint64 val = QQuickItemPrivate::consistentTime - t1;
        t1 = QQuickItemPrivate::consistentTime;
        t2 = 0;
        return val;
    }

private:
    qint64 t1;
    qint64 t2;
};

void QQuickItemPrivate::start(QElapsedTimer &t)
{
    if (QQuickItemPrivate::consistentTime == -1)
        t.start();
    else
        ((QElapsedTimerConsistentTimeHack*)&t)->start();
}

qint64 QQuickItemPrivate::elapsed(QElapsedTimer &t)
{
    if (QQuickItemPrivate::consistentTime == -1)
        return t.elapsed();
    else
        return ((QElapsedTimerConsistentTimeHack*)&t)->elapsed();
}

qint64 QQuickItemPrivate::restart(QElapsedTimer &t)
{
    if (QQuickItemPrivate::consistentTime == -1)
        return t.restart();
    else
        return ((QElapsedTimerConsistentTimeHack*)&t)->restart();
}

/*!
    \fn bool QQuickItem::isTextureProvider() const

    Returns true if this item is a texture provider. The default
    implementation returns false.

    This function can be called from any thread.
 */

/*!
    \fn QSGTextureProvider *QQuickItem::textureProvider() const

    Returns the texture provider for an item. The default implementation
    returns 0.

    This function may only be called on the rendering thread.
 */

QT_END_NAMESPACE

#include <moc_qquickitem.cpp>
