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

#include "designersupport.h"
#include <private/qquickitem_p.h>

#include <QtQuick/private/qquickshadereffectsource_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <private/qdeclarativeengine_p.h>
#include <private/qquickview_p.h>
#include <QtQuick/private/qdeclarativestategroup_p.h>
#include <QtGui/QImage>

QT_BEGIN_NAMESPACE

DesignerSupport::DesignerSupport()
{
}

DesignerSupport::~DesignerSupport()
{
    QHash<QQuickItem*, QQuickShaderEffectTexture*>::iterator iterator;

    for (iterator = m_itemTextureHash.begin(); iterator != m_itemTextureHash.end(); ++iterator) {
        QQuickShaderEffectTexture *texture = iterator.value();
        QQuickItem *item = iterator.key();
        QQuickItemPrivate::get(item)->derefFromEffectItem(true);
        delete texture;
    }
}

void DesignerSupport::refFromEffectItem(QQuickItem *referencedItem, bool hide)
{
    if (referencedItem == 0)
        return;

    QQuickItemPrivate::get(referencedItem)->refFromEffectItem(hide);
    QQuickCanvasPrivate::get(referencedItem->canvas())->updateDirtyNode(referencedItem);

    Q_ASSERT(QQuickItemPrivate::get(referencedItem)->rootNode);

    if (!m_itemTextureHash.contains(referencedItem)) {
        QQuickShaderEffectTexture *texture = new QQuickShaderEffectTexture(referencedItem);

        texture->setLive(true);
        texture->setItem(QQuickItemPrivate::get(referencedItem)->rootNode);
        texture->setRect(referencedItem->boundingRect());
        texture->setSize(referencedItem->boundingRect().size().toSize());
        texture->setRecursive(true);
#ifndef QT_OPENGL_ES
        texture->setFormat(GL_RGBA8);
#else
        texture->setFormat(GL_RGBA);
#endif
        texture->setHasMipmaps(false);

        m_itemTextureHash.insert(referencedItem, texture);
    }
}

void DesignerSupport::derefFromEffectItem(QQuickItem *referencedItem, bool unhide)
{
    if (referencedItem == 0)
        return;

    delete m_itemTextureHash.take(referencedItem);
    QQuickItemPrivate::get(referencedItem)->derefFromEffectItem(unhide);
}

QImage DesignerSupport::renderImageForItem(QQuickItem *referencedItem, const QRectF &boundingRect, const QSize &imageSize)
{
    if (referencedItem == 0 || referencedItem->parentItem() == 0) {
        qDebug() << __FILE__ << __LINE__ << "Warning: Item can be rendered.";
        return QImage();
    }

    QQuickShaderEffectTexture *renderTexture = m_itemTextureHash.value(referencedItem);

    Q_ASSERT(renderTexture);
    if (renderTexture == 0)
         return QImage();
    renderTexture->setRect(boundingRect);
    renderTexture->setSize(imageSize);
    renderTexture->updateTexture();

    QImage renderImage = renderTexture->toImage();
    renderImage = renderImage.mirrored(false, true);

    if (renderImage.size().isEmpty())
        qDebug() << __FILE__ << __LINE__ << "Warning: Image is empty.";

    return renderImage;
}

bool DesignerSupport::isDirty(QQuickItem *referencedItem, DirtyType dirtyType)
{
    if (referencedItem == 0)
        return false;

    return QQuickItemPrivate::get(referencedItem)->dirtyAttributes & dirtyType;
}

void DesignerSupport::resetDirty(QQuickItem *referencedItem)
{
    if (referencedItem == 0)
        return;

    QQuickItemPrivate::get(referencedItem)->dirtyAttributes = 0x0;
    QQuickItemPrivate::get(referencedItem)->removeFromDirtyList();
}

QTransform DesignerSupport::canvasTransform(QQuickItem *referencedItem)
{
    if (referencedItem == 0)
        return QTransform();

    return QQuickItemPrivate::get(referencedItem)->itemToCanvasTransform();
}

QTransform DesignerSupport::parentTransform(QQuickItem *referencedItem)
{
    if (referencedItem == 0)
        return QTransform();

    QTransform parentTransform;

    QQuickItemPrivate::get(referencedItem)->itemToParentTransform(parentTransform);

    return parentTransform;
}

QString propertyNameForAnchorLine(const QQuickAnchorLine::AnchorLine &anchorLine)
{
    switch (anchorLine) {
        case QQuickAnchorLine::Left: return QLatin1String("left");
        case QQuickAnchorLine::Right: return QLatin1String("right");
        case QQuickAnchorLine::Top: return QLatin1String("top");
        case QQuickAnchorLine::Bottom: return QLatin1String("bottom");
        case QQuickAnchorLine::HCenter: return QLatin1String("horizontalCenter");
        case QQuickAnchorLine::VCenter: return QLatin1String("verticalCenter");
        case QQuickAnchorLine::Baseline: return QLatin1String("baseline");
        case QQuickAnchorLine::Invalid:
        default: return QString();
    }
}

bool isValidAnchorName(const QString &name)
{
    static QStringList anchorNameList(QStringList() << QLatin1String("anchors.top")
                                                    << QLatin1String("anchors.left")
                                                    << QLatin1String("anchors.right")
                                                    << QLatin1String("anchors.bottom")
                                                    << QLatin1String("anchors.verticalCenter")
                                                    << QLatin1String("anchors.horizontalCenter")
                                                    << QLatin1String("anchors.fill")
                                                    << QLatin1String("anchors.centerIn")
                                                    << QLatin1String("anchors.baseline"));

    return anchorNameList.contains(name);
}

bool DesignerSupport::isAnchoredTo(QQuickItem *fromItem, QQuickItem *toItem)
{
    Q_ASSERT(dynamic_cast<QQuickItemPrivate*>(QQuickItemPrivate::get(fromItem)));
    QQuickItemPrivate *fromItemPrivate = static_cast<QQuickItemPrivate*>(QQuickItemPrivate::get(fromItem));
    QQuickAnchors *anchors = fromItemPrivate->anchors();
    return anchors->fill() == toItem
            || anchors->centerIn() == toItem
            || anchors->bottom().item == toItem
            || anchors->top().item == toItem
            || anchors->left().item == toItem
            || anchors->right().item == toItem
            || anchors->verticalCenter().item == toItem
            || anchors->horizontalCenter().item == toItem
            || anchors->baseline().item == toItem;
}

bool DesignerSupport::areChildrenAnchoredTo(QQuickItem *fromItem, QQuickItem *toItem)
{
    foreach (QQuickItem *childItem, fromItem->childItems()) {
        if (childItem) {
            if (isAnchoredTo(childItem, toItem))
                return true;

            if (areChildrenAnchoredTo(childItem, toItem))
                return true;
        }
    }

    return false;
}

QQuickAnchors *anchors(QQuickItem *item)
{
    QQuickItemPrivate *itemPrivate = static_cast<QQuickItemPrivate*>(QQuickItemPrivate::get(item));
    return itemPrivate->anchors();
}

QQuickAnchors::Anchor anchorLineFlagForName(const QString &name)
{
    if (name == QLatin1String("anchors.top"))
        return QQuickAnchors::TopAnchor;

    if (name == QLatin1String("anchors.left"))
        return QQuickAnchors::LeftAnchor;

    if (name == QLatin1String("anchors.bottom"))
         return QQuickAnchors::BottomAnchor;

    if (name == QLatin1String("anchors.right"))
        return QQuickAnchors::RightAnchor;

    if (name == QLatin1String("anchors.horizontalCenter"))
        return QQuickAnchors::HCenterAnchor;

    if (name == QLatin1String("anchors.verticalCenter"))
         return QQuickAnchors::VCenterAnchor;

    if (name == QLatin1String("anchors.baseline"))
         return QQuickAnchors::BaselineAnchor;


    Q_ASSERT_X(false, Q_FUNC_INFO, "wrong anchor name - this should never happen");
    return QQuickAnchors::LeftAnchor;
}

bool DesignerSupport::hasAnchor(QQuickItem *item, const QString &name)
{
    if (!isValidAnchorName(name))
        return false;

    if (name == QLatin1String("anchors.fill"))
        return anchors(item)->fill() != 0;

    if (name == QLatin1String("anchors.centerIn"))
        return anchors(item)->centerIn() != 0;

    if (name == QLatin1String("anchors.right"))
        return anchors(item)->right().item != 0;

    if (name == QLatin1String("anchors.top"))
        return anchors(item)->top().item != 0;

    if (name == QLatin1String("anchors.left"))
        return anchors(item)->left().item != 0;

    if (name == QLatin1String("anchors.bottom"))
        return anchors(item)->bottom().item != 0;

    if (name == QLatin1String("anchors.horizontalCenter"))
        return anchors(item)->horizontalCenter().item != 0;

    if (name == QLatin1String("anchors.verticalCenter"))
        return anchors(item)->verticalCenter().item != 0;

    if (name == QLatin1String("anchors.baseline"))
        return anchors(item)->baseline().item != 0;

    return anchors(item)->usedAnchors().testFlag(anchorLineFlagForName(name));
}

QQuickItem *DesignerSupport::anchorFillTargetItem(QQuickItem *item)
{
    return anchors(item)->fill();
}

QQuickItem *DesignerSupport::anchorCenterInTargetItem(QQuickItem *item)
{
    return anchors(item)->centerIn();
}



QPair<QString, QObject*> DesignerSupport::anchorLineTarget(QQuickItem *item, const QString &name, QDeclarativeContext *context)
{
    QObject *targetObject = 0;
    QString targetName;

    if (name == QLatin1String("anchors.fill")) {
        targetObject = anchors(item)->fill();
    } else if (name == QLatin1String("anchors.centerIn")) {
        targetObject = anchors(item)->centerIn();
    } else {
        QDeclarativeProperty metaProperty(item, name, context);
        if (!metaProperty.isValid())
            return QPair<QString, QObject*>();

        QQuickAnchorLine anchorLine = metaProperty.read().value<QQuickAnchorLine>();
        if (anchorLine.anchorLine != QQuickAnchorLine::Invalid) {
            targetObject = anchorLine.item;
            targetName = propertyNameForAnchorLine(anchorLine.anchorLine);
        }

    }

    return QPair<QString, QObject*>(targetName, targetObject);
}

void DesignerSupport::resetAnchor(QQuickItem *item, const QString &name)
{
    if (name == QLatin1String("anchors.fill")) {
        anchors(item)->resetFill();
    } else if (name == QLatin1String("anchors.centerIn")) {
        anchors(item)->resetCenterIn();
    } else if (name == QLatin1String("anchors.top")) {
        anchors(item)->resetTop();
    } else if (name == QLatin1String("anchors.left")) {
        anchors(item)->resetLeft();
    } else if (name == QLatin1String("anchors.right")) {
        anchors(item)->resetRight();
    } else if (name == QLatin1String("anchors.bottom")) {
        anchors(item)->resetBottom();
    } else if (name == QLatin1String("anchors.horizontalCenter")) {
        anchors(item)->resetHorizontalCenter();
    } else if (name == QLatin1String("anchors.verticalCenter")) {
        anchors(item)->resetVerticalCenter();
    } else if (name == QLatin1String("anchors.baseline")) {
        anchors(item)->resetBaseline();
    }
}

QList<QObject*> DesignerSupport::statesForItem(QQuickItem *item)
{
    QList<QObject*> objectList;
    QList<QDeclarativeState *> stateList = QQuickItemPrivate::get(item)->_states()->states();
    qCopy(stateList.begin(), stateList.end(), objectList.begin());

    return objectList;
}

bool DesignerSupport::isComponentComplete(QQuickItem *item)
{
    return static_cast<QQuickItemPrivate*>(QQuickItemPrivate::get(item))->componentComplete;
}

int DesignerSupport::borderWidth(QQuickItem *item)
{
    QQuickRectangle *rectangle = qobject_cast<QQuickRectangle*>(item);
    if (rectangle)
        return rectangle->border()->width();

    return 0;
}

void DesignerSupport::refreshExpressions(QDeclarativeContext *context)
{
    QDeclarativeContextPrivate::get(context)->data->refreshExpressions();
}

void DesignerSupport::setRootItem(QQuickView *view, QQuickItem *item)
{
    QQuickViewPrivate::get(view)->setRootObject(item);
}

bool DesignerSupport::isValidWidth(QQuickItem *item)
{
    return QQuickItemPrivate::get(item)->heightValid;
}

bool DesignerSupport::isValidHeight(QQuickItem *item)
{
    return QQuickItemPrivate::get(item)->widthValid;
}

void DesignerSupport::updateDirtyNode(QQuickItem *item)
{
    QQuickCanvasPrivate::get(item->canvas())->updateDirtyNode(item);
}

QT_END_NAMESPACE
