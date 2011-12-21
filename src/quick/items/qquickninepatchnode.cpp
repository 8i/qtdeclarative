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

#include "qquickninepatchnode_p.h"
#include <private/qsgadaptationlayer_p.h>
#include <private/qmath_p.h>

QQuickNinePatchNode::QQuickNinePatchNode()
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0)
    , m_horizontalTileMode(QQuickBorderImage::Stretch)
    , m_verticalTileMode(QQuickBorderImage::Stretch)
    , m_dirtyGeometry(false)
    , m_mirror(false)
{
    setOpaqueMaterial(&m_material);
    setMaterial(&m_materialO);
    setGeometry(&m_geometry);
    m_geometry.setDrawingMode(GL_TRIANGLES);
#ifdef QML_RUNTIME_TESTING
    description = QLatin1String("borderimage");
#endif
}

void QQuickNinePatchNode::setInnerRect(const QRectF &rect)
{
    if (m_innerRect == rect)
        return;
    m_innerRect = rect;
    m_dirtyGeometry = true;
}

void QQuickNinePatchNode::setRect(const QRectF &rect)
{
    if (m_targetRect == rect)
        return;
    m_targetRect = rect;
    m_dirtyGeometry = true;
}

void QQuickNinePatchNode::setHorzontalTileMode(QQuickBorderImage::TileMode mode)
{
    if (mode == QQuickBorderImage::TileMode(m_horizontalTileMode))
        return;
    m_horizontalTileMode = mode;
    m_dirtyGeometry = true;
}


void QQuickNinePatchNode::setVerticalTileMode(QQuickBorderImage::TileMode mode)
{
    if (mode == QQuickBorderImage::TileMode(m_verticalTileMode))
        return;
    m_verticalTileMode = mode;
    m_dirtyGeometry = true;
}


void QQuickNinePatchNode::setFiltering(QSGTexture::Filtering filtering)
{
    if (m_material.filtering() == filtering)
        return;

    m_material.setFiltering(filtering);
    m_materialO.setFiltering(filtering);
    markDirty(DirtyMaterial);
}

QSGTexture::Filtering QQuickNinePatchNode::filtering() const
{
    return m_material.filtering();
}

void QQuickNinePatchNode::setTexture(QSGTexture *texture)
{
    if (texture == m_material.texture())
        return;
    m_material.setTexture(texture);
    m_materialO.setTexture(texture);
    markDirty(DirtyMaterial);
}

QSGTexture *QQuickNinePatchNode::texture() const
{
    return m_material.texture();
}

void QQuickNinePatchNode::setMirror(bool m)
{
    if (m_mirror == m)
        return;
    m_mirror = m;
    m_dirtyGeometry = true;
}


void QQuickNinePatchNode::update()
{
    if (!m_dirtyGeometry)
        return;

    // For stretch this algorithm could be simplified to use less vertices
    // as more vertices could be reused then, but I doubt its where our main
    // problem will lie. This way, we at least share the algorithm between all

    Q_ASSERT(m_material.texture());

    float tw = m_material.texture()->textureSize().width();
    float th = m_material.texture()->textureSize().height();

    QRectF textureSubRect = m_material.texture()->textureSubRect();
    QSize textureSize = m_material.texture()->textureSize();

    float rightBorder = tw - m_innerRect.right();
    float bottomBorder = th - m_innerRect.bottom();

//    qDebug() << m_innerRect << m_targetRect << m_horizontalTileMode << m_verticalTileMode;

    int xChunkCount = 0; // Number of chunks
    float xChunkSize = 0; // Size of chunk in pixels
    float xTexSize = m_innerRect.width(); // Size of the texture to stretch/tile
    float xSize = m_targetRect.width() - m_innerRect.left() - rightBorder; // Size of area to fill with chunks

    if (m_horizontalTileMode == QQuickBorderImage::Repeat) {
        xChunkCount = qCeil(xSize / xTexSize);
        xChunkSize = xTexSize;
    } else if (m_horizontalTileMode == QQuickBorderImage::Round) {
        xChunkCount = qCeil(xSize / xTexSize);
        qreal fullWidth = xChunkCount * xTexSize;
        xChunkSize = xTexSize * xSize / fullWidth;
    } else {
        xChunkCount = 1;
        xChunkSize = xSize;
    }

    int yChunkCount = 0;
    float yChunkSize = 0; // Relative to target rect.
    float yTexSize = m_innerRect.height(); // Size of the texture to stretch/tile
    float ySize = m_targetRect.height() - m_innerRect.top() - bottomBorder;

    if (m_verticalTileMode == QQuickBorderImage::Repeat) {
        yChunkCount = qCeil(ySize / yTexSize);
        yChunkSize = yTexSize;
    } else if (m_verticalTileMode == QQuickBorderImage::Round) {
        yChunkCount = qCeil(ySize / yTexSize);
        qreal fullHeight = yChunkCount * yTexSize;
        yChunkSize = yTexSize * ySize / fullHeight;
    } else {
        yChunkCount = 1;
        yChunkSize = ySize;
    }

    int xTotalChunkCount = xChunkCount + 2;
    int yTotalChunkCount = yChunkCount + 2;

    int totalChunkCount = xTotalChunkCount * yTotalChunkCount;
    int vertexCount = totalChunkCount * 4;
    int indexCount = totalChunkCount * 6;

    if (vertexCount != m_geometry.vertexCount() || indexCount != m_geometry.indexCount())
        m_geometry.allocate(vertexCount, indexCount);

    QSGGeometry::TexturedPoint2D *v = m_geometry.vertexDataAsTexturedPoint2D();


    // Fill in the vertices.. The loop below is pretty much an exact replica
    // of the one inside fillRow.
    float yTexChunk1 = m_innerRect.top() / th;
    float yTexChunk2 = m_innerRect.bottom() / th;

    fillRow(v, 0, 0, xChunkCount, xChunkSize, textureSubRect, textureSize);
    fillRow(v, m_innerRect.y(), yTexChunk1, xChunkCount, xChunkSize, textureSubRect, textureSize);

    for (int yc=0; yc<yChunkCount; ++yc) {
        float yy = m_innerRect.y() + yChunkSize * yc;
        fillRow(v, yy, yTexChunk1, xChunkCount, xChunkSize, textureSubRect, textureSize);

        // Special case the last one
        if (yc == yChunkCount - 1) {
            float t = m_verticalTileMode == QQuickBorderImage::Repeat
                    ? yTexChunk1 + (yTexChunk2 - yTexChunk1) * (m_targetRect.height() - bottomBorder - yy) / yChunkSize
                    : yTexChunk2;
            fillRow(v, m_targetRect.height() - bottomBorder, t, xChunkCount, xChunkSize, textureSubRect, textureSize);
        } else {
            fillRow(v, yy + yChunkSize, yTexChunk2, xChunkCount, xChunkSize, textureSubRect, textureSize);
        }
    }

    fillRow(v, m_targetRect.height() - bottomBorder, yTexChunk2, xChunkCount, xChunkSize, textureSubRect, textureSize);
    fillRow(v, m_targetRect.height(), 1, xChunkCount, xChunkSize, textureSubRect, textureSize);

    if (m_mirror) {
        v = m_geometry.vertexDataAsTexturedPoint2D();
        for (int i=0; i<m_geometry.vertexCount(); ++i) {
            v->x = m_targetRect.width() - v->x;
            ++v;
        }
    }

//    v = m_geometry.vertexDataAsTexturedPoint2D();
//    for (int i=0; i<m_geometry.vertexCount(); ++i) {
//        printf("Vertex: %d:  (%.3f, %.3f) - (%.3f, %.3f)\n",
//               i,
//               v->x, v->y, v->tx, v->ty);
//        ++v;
//    }

    quint16 *i = m_geometry.indexDataAsUShort();
    int row = xTotalChunkCount * 2;
    for (int r=0; r<yTotalChunkCount; ++r) {
        int offset = r * row * 2;
        for (int c=0; c<xTotalChunkCount; ++c) {
            *i++ = offset + c * 2;
            *i++ = offset + c * 2 + 1;
            *i++ = offset + c * 2 + row;
            *i++ = offset + c * 2 + 1;
            *i++ = offset + c * 2 + row + 1;
            *i++ = offset + c * 2 + row;
        }
    }

//    i = m_geometry.indexDataAsUShort();
//    for (int idx=0; idx<m_geometry.indexCount(); idx+=6) {
//        printf("%2d: ", idx / 6);
//        for (int s=0; s<6; ++s)
//            printf(" %d", i[idx + s]);
//        printf("\n");
//    }

    markDirty(QSGNode::DirtyGeometry);
}

void QQuickNinePatchNode::fillRow(QSGGeometry::TexturedPoint2D *&v, float y, float ty, int xChunkCount, float xChunkSize,
                               const QRectF &tsr,   // texture sub rect, for atlasses
                               const QSize &ts)     // texture size in pixels
{
    ty = tsr.y() + ty * tsr.height();

    float tw = ts.width();
    float rightBorder = tw - m_innerRect.right();
    float xTexChunk1 = tsr.left() + tsr.width() * m_innerRect.left() / tw;
    float xTexChunk2 = tsr.left() + tsr.width() * m_innerRect.right() / tw;

    v++->set(0, y, tsr.left(), ty);
    v++->set(m_innerRect.x(), y, xTexChunk1, ty);

    for (int xc=0; xc<xChunkCount; ++xc) {
        float xx = m_innerRect.x() + xChunkSize * xc;
        v++->set(xx, y, xTexChunk1, ty);

        // Special case the last one
        if (xc == xChunkCount - 1) {
            float t = m_horizontalTileMode == QQuickBorderImage::Repeat
                    ? xTexChunk1 + (xTexChunk2 - xTexChunk1) * (m_targetRect.width() - rightBorder - xx) / xChunkSize
                    : xTexChunk2;
            v->set(m_targetRect.width() - rightBorder, y, t, ty);
        } else {
            v->set(xx + xChunkSize, y, xTexChunk2, ty);
        }
        ++v;
    }

    v++->set(m_targetRect.width() - rightBorder, y, xTexChunk2, ty);
    v++->set(m_targetRect.width(), y, tsr.right(), ty);
}
