/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qquickitemsmodule_p.h"

#include "qquickitem.h"
#include "qquickitem_p.h"
#include "qquickevents_p_p.h"
#include "qquickrectangle_p.h"
#include "qquickfocusscope_p.h"
#include "qquicktext_p.h"
#include "qquicktextinput_p.h"
#include "qquicktextedit_p.h"
#include "qquickimage_p.h"
#include "qquickborderimage_p.h"
#include "qquickscalegrid_p_p.h"
#include "qquickmousearea_p.h"
#include "qquickpincharea_p.h"
#include "qquickflickable_p.h"
#include "qquickflickable_p_p.h"
#include "qquicklistview_p.h"
#include "qquickvisualitemmodel_p.h"
#include "qquickvisualdatamodel_p.h"
#include "qquickgridview_p.h"
#include "qquickpathview_p.h"
#include <private/qdeclarativepath_p.h>
#include <private/qdeclarativepathinterpolator_p.h>
#include "qquickpositioners_p.h"
#include "qquickrepeater_p.h"
#include "qquickloader_p.h"
#include "qquickanimatedimage_p.h"
#include "qquickflipable_p.h"
#include "qquicktranslate_p.h"
#include "qquickstateoperations_p.h"
#include "qquickanimation_p.h"
#include <private/qquickshadereffect_p.h>
#include <QtQuick/private/qquickshadereffectsource_p.h>
//#include <private/qquickpincharea_p.h>
#include <QtQuick/private/qquickcanvasitem_p.h>
#include <QtQuick/private/qquickcontext2d_p.h>
#include "qquicksprite_p.h"
#include "qquickspriteimage_p.h"
#include "qquickdrag_p.h"
#include "qquickdroparea_p.h"
#include "qquickmultipointtoucharea_p.h"
#include <private/qdeclarativemetatype_p.h>

static QDeclarativePrivate::AutoParentResult qquickitem_autoParent(QObject *obj, QObject *parent)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(obj);
    if (!item)
        return QDeclarativePrivate::IncompatibleObject;

    QQuickItem *parentItem = qobject_cast<QQuickItem *>(parent);
    if (!parentItem)
        return QDeclarativePrivate::IncompatibleParent;

    item->setParentItem(parentItem);
    return QDeclarativePrivate::Parented;
}

static bool compareQQuickAnchorLines(const void *p1, const void *p2)
{
    const QQuickAnchorLine &l1 = *static_cast<const QQuickAnchorLine*>(p1);
    const QQuickAnchorLine &l2 = *static_cast<const QQuickAnchorLine*>(p2);
    return l1 == l2;
}

static void qt_quickitems_defineModule(const char *uri, int major, int minor)
{
    QDeclarativePrivate::RegisterAutoParent autoparent = { 0, &qquickitem_autoParent };
    QDeclarativePrivate::qmlregister(QDeclarativePrivate::AutoParentRegistration, &autoparent);
    QQuickItemPrivate::registerAccessorProperties();

#ifdef QT_NO_MOVIE
    qmlRegisterTypeNotAvailable(uri,major,minor,"AnimatedImage", qApp->translate("QQuickAnimatedImage","Qt was built without support for QMovie"));
#else
    qmlRegisterType<QQuickAnimatedImage>(uri,major,minor,"AnimatedImage");
#endif
    qmlRegisterType<QQuickBorderImage>(uri,major,minor,"BorderImage");
    qmlRegisterType<QQuickColumn>(uri,major,minor,"Column");
    qmlRegisterType<QQuickFlickable>(uri,major,minor,"Flickable");
    qmlRegisterType<QQuickFlipable>(uri,major,minor,"Flipable");
    qmlRegisterType<QQuickFlow>(uri,major,minor,"Flow");
//    qmlRegisterType<QDeclarativeFocusPanel>(uri,major,minor,"FocusPanel");
    qmlRegisterType<QQuickFocusScope>(uri,major,minor,"FocusScope");
    qmlRegisterType<QQuickGradient>(uri,major,minor,"Gradient");
    qmlRegisterType<QQuickGradientStop>(uri,major,minor,"GradientStop");
    qmlRegisterType<QQuickGrid>(uri,major,minor,"Grid");
    qmlRegisterType<QQuickGridView>(uri,major,minor,"GridView");
    qmlRegisterType<QQuickImage>(uri,major,minor,"Image");
    qmlRegisterType<QQuickItem>(uri,major,minor,"Item");
    qmlRegisterType<QQuickListView>(uri,major,minor,"ListView");
    qmlRegisterType<QQuickLoader>(uri,major,minor,"Loader");
    qmlRegisterType<QQuickMouseArea>(uri,major,minor,"MouseArea");
    qmlRegisterType<QDeclarativePath>(uri,major,minor,"Path");
    qmlRegisterType<QDeclarativePathAttribute>(uri,major,minor,"PathAttribute");
    qmlRegisterType<QDeclarativePathCubic>(uri,major,minor,"PathCubic");
    qmlRegisterType<QDeclarativePathLine>(uri,major,minor,"PathLine");
    qmlRegisterType<QDeclarativePathPercent>(uri,major,minor,"PathPercent");
    qmlRegisterType<QDeclarativePathQuad>(uri,major,minor,"PathQuad");
    qmlRegisterType<QDeclarativePathCatmullRomCurve>("QtQuick",2,0,"PathCurve");
    qmlRegisterType<QDeclarativePathArc>("QtQuick",2,0,"PathArc");
    qmlRegisterType<QDeclarativePathSvg>("QtQuick",2,0,"PathSvg");
    qmlRegisterType<QQuickPathView>(uri,major,minor,"PathView");
    qmlRegisterUncreatableType<QQuickBasePositioner>(uri,major,minor,"Positioner",
                                                  QStringLiteral("Positioner is an abstract type that is only available as an attached property."));
#ifndef QT_NO_VALIDATOR
    qmlRegisterType<QIntValidator>(uri,major,minor,"IntValidator");
    qmlRegisterType<QDoubleValidator>(uri,major,minor,"DoubleValidator");
    qmlRegisterType<QRegExpValidator>(uri,major,minor,"RegExpValidator");
#endif
    qmlRegisterType<QQuickRectangle>(uri,major,minor,"Rectangle");
    qmlRegisterType<QQuickRepeater>(uri,major,minor,"Repeater");
    qmlRegisterType<QQuickRow>(uri,major,minor,"Row");
    qmlRegisterType<QQuickTranslate>(uri,major,minor,"Translate");
    qmlRegisterType<QQuickRotation>(uri,major,minor,"Rotation");
    qmlRegisterType<QQuickScale>(uri,major,minor,"Scale");
    qmlRegisterType<QQuickText>(uri,major,minor,"Text");
    qmlRegisterType<QQuickTextEdit>(uri,major,minor,"TextEdit");
    qmlRegisterType<QQuickTextInput>(uri,major,minor,"TextInput");
    qmlRegisterType<QQuickViewSection>(uri,major,minor,"ViewSection");
    qmlRegisterType<QQuickVisualDataModel>(uri,major,minor,"VisualDataModel");
    qmlRegisterType<QQuickVisualDataGroup>(uri,major,minor,"VisualDataGroup");
    qmlRegisterType<QQuickVisualItemModel>(uri,major,minor,"VisualItemModel");

    qmlRegisterType<QQuickAnchors>();
    qmlRegisterType<QQuickKeyEvent>();
    qmlRegisterType<QQuickMouseEvent>();
    qmlRegisterType<QQuickTransform>();
    qmlRegisterType<QDeclarativePathElement>();
    qmlRegisterType<QDeclarativeCurve>();
    qmlRegisterType<QQuickScaleGrid>();
    qmlRegisterType<QQuickTextLine>();
#ifndef QT_NO_VALIDATOR
    qmlRegisterType<QValidator>();
#endif
    qmlRegisterType<QQuickVisualModel>();
    qmlRegisterType<QQuickPen>();
    qmlRegisterType<QQuickFlickableVisibleArea>();
    qRegisterMetaType<QQuickAnchorLine>("QQuickAnchorLine");
    QDeclarativeMetaType::setQQuickAnchorLineCompareFunction(compareQQuickAnchorLines);

    qmlRegisterUncreatableType<QQuickKeyNavigationAttached>(uri,major,minor,"KeyNavigation",QQuickKeyNavigationAttached::tr("KeyNavigation is only available via attached properties"));
    qmlRegisterUncreatableType<QQuickKeysAttached>(uri,major,minor,"Keys",QQuickKeysAttached::tr("Keys is only available via attached properties"));
    qmlRegisterUncreatableType<QQuickLayoutMirroringAttached>(uri,major,minor,"LayoutMirroring", QQuickLayoutMirroringAttached::tr("LayoutMirroring is only available via attached properties"));

    qmlRegisterType<QQuickPinchArea>(uri,major,minor,"PinchArea");
    qmlRegisterType<QQuickPinch>(uri,major,minor,"Pinch");
    qmlRegisterType<QQuickPinchEvent>();

    qmlRegisterType<QQuickShaderEffect>("QtQuick", 2, 0, "ShaderEffect");
    qmlRegisterType<QQuickShaderEffectSource>("QtQuick", 2, 0, "ShaderEffectSource");
    qmlRegisterUncreatableType<QQuickShaderEffectMesh>("QtQuick", 2, 0, "ShaderEffectMesh", QQuickShaderEffectMesh::tr("Cannot create instance of abstract class ShaderEffectMesh."));
    qmlRegisterType<QQuickGridMesh>("QtQuick", 2, 0, "GridMesh");

    qmlRegisterUncreatableType<QQuickPaintedItem>("QtQuick", 2, 0, "PaintedItem", QQuickPaintedItem::tr("Cannot create instance of abstract class PaintedItem"));

    qmlRegisterType<QQuickCanvasItem>("QtQuick", 2, 0, "Canvas");

    qmlRegisterType<QQuickSprite>("QtQuick", 2, 0, "Sprite");
    qmlRegisterType<QQuickSpriteImage>("QtQuick", 2, 0, "SpriteImage");

    qmlRegisterType<QQuickParentChange>(uri, major, minor,"ParentChange");
    qmlRegisterType<QQuickAnchorChanges>(uri, major, minor,"AnchorChanges");
    qmlRegisterType<QQuickAnchorSet>();
    qmlRegisterType<QQuickAnchorAnimation>(uri, major, minor,"AnchorAnimation");
    qmlRegisterType<QQuickParentAnimation>(uri, major, minor,"ParentAnimation");
    qmlRegisterType<QQuickPathAnimation>("QtQuick",2,0,"PathAnimation");
    qmlRegisterType<QDeclarativePathInterpolator>("QtQuick",2,0,"PathInterpolator");

    qmlRegisterType<QQuickDropArea>("QtQuick", 2, 0, "DropArea");
    qmlRegisterType<QQuickDropEvent>();
    qmlRegisterType<QQuickDropAreaDrag>();
    qmlRegisterUncreatableType<QQuickDrag>("QtQuick", 2, 0, "Drag", QQuickDragAttached::tr("Drag is only available via attached properties"));

    qmlRegisterType<QQuickMultiPointTouchArea>("QtQuick", 2, 0, "MultiPointTouchArea");
    qmlRegisterType<QQuickTouchPoint>("QtQuick", 2, 0, "TouchPoint");
    qmlRegisterType<QQuickGrabGestureEvent>();
}

void QQuickItemsModule::defineModule()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

    // XXX todo -  Remove before final integration...
    QByteArray mode = qgetenv("QMLSCENE_IMPORT_NAME");
    QByteArray name = "QtQuick";
    int majorVersion = 2;
    int minorVersion = 0;
    if (mode == "quick1") {
        majorVersion = 1;
    } else if (mode == "qt") {
        name = "Qt";
        majorVersion = 4;
        minorVersion = 7;
    }

    qt_quickitems_defineModule(name, majorVersion, minorVersion);
}

