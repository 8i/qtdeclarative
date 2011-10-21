/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QSGNINEPATCHNODE_H
#define QSGNINEPATCHNODE_H

#include "qsgnode.h"
#include "qsgtexturematerial.h"
#include "qsgborderimage_p.h"

class TextureReference;

class QSGNinePatchNode : public QSGGeometryNode
{
public:
    QSGNinePatchNode();

    void setTexture(QSGTexture *texture);
    QSGTexture *texture() const;

    void setRect(const QRectF &rect);
    QRectF rect() const { return m_targetRect; }

    void setInnerRect(const QRectF &rect);
    QRectF innerRect() const { return m_innerRect; }

    void setFiltering(QSGTexture::Filtering filtering);
    QSGTexture::Filtering filtering() const;

    void setHorzontalTileMode(QSGBorderImage::TileMode mode);
    QSGBorderImage::TileMode horizontalTileMode() const {
        return (QSGBorderImage::TileMode) m_horizontalTileMode;
    }

    void setVerticalTileMode(QSGBorderImage::TileMode mode);
    QSGBorderImage::TileMode verticalTileMode() const {
        return (QSGBorderImage::TileMode) m_verticalTileMode;
    }

    void setMirror(bool m);
    bool mirror() const { return m_mirror; }

    void update();

private:
    void fillRow(QSGGeometry::TexturedPoint2D *&v, float y, float ty, int xChunkCount, float xChunkSize, const QRectF &tsr, const QSize &ts);
    QRectF m_targetRect;
    QRectF m_innerRect;
    QSGOpaqueTextureMaterial m_material;
    QSGTextureMaterial m_materialO;
    QSGGeometry m_geometry;

    uint m_horizontalTileMode : 2;
    uint m_verticalTileMode : 2;

    uint m_dirtyGeometry : 1;
    uint m_mirror : 1;
};

#endif // QSGNINEPATCHNODE_H
