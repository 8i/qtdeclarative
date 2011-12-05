// Commit: ebd4bc73c46c2962742a682b6a391fb68c482aec
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

#ifndef QQUICKBORDERIMAGE_P_H
#define QQUICKBORDERIMAGE_P_H

#include "qquickimagebase_p.h"

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

class QQuickScaleGrid;
class QQuickGridScaledImage;
class QQuickBorderImagePrivate;
class Q_AUTOTEST_EXPORT QQuickBorderImage : public QQuickImageBase
{
    Q_OBJECT
    Q_ENUMS(TileMode)

    Q_PROPERTY(QQuickScaleGrid *border READ border CONSTANT)
    Q_PROPERTY(TileMode horizontalTileMode READ horizontalTileMode WRITE setHorizontalTileMode NOTIFY horizontalTileModeChanged)
    Q_PROPERTY(TileMode verticalTileMode READ verticalTileMode WRITE setVerticalTileMode NOTIFY verticalTileModeChanged)
    // read-only for BorderImage
    Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)

public:
    QQuickBorderImage(QQuickItem *parent=0);
    ~QQuickBorderImage();

    QQuickScaleGrid *border();

    enum TileMode { Stretch = Qt::StretchTile, Repeat = Qt::RepeatTile, Round = Qt::RoundTile };

    TileMode horizontalTileMode() const;
    void setHorizontalTileMode(TileMode);

    TileMode verticalTileMode() const;
    void setVerticalTileMode(TileMode);

    void setSource(const QUrl &url);

Q_SIGNALS:
    void horizontalTileModeChanged();
    void verticalTileModeChanged();
    void sourceSizeChanged();

protected:
    virtual void load();
    virtual void pixmapChange();
    virtual QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

private:
    void setGridScaledImage(const QQuickGridScaledImage& sci);

private Q_SLOTS:
    void doUpdate();
    void requestFinished();
    void sciRequestFinished();

private:
    Q_DISABLE_COPY(QQuickBorderImage)
    Q_DECLARE_PRIVATE(QQuickBorderImage)
};

QT_END_NAMESPACE
QML_DECLARE_TYPE(QQuickBorderImage)
QT_END_HEADER

#endif // QQUICKBORDERIMAGE_P_H
