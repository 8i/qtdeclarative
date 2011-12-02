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

#ifndef DISTANCEFIELD_GLYPHNODE_H
#define DISTANCEFIELD_GLYPHNODE_H

#include <private/qsgadaptationlayer_p.h>
#include "qsgtexture.h"

#include <private/qquicktext_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGDistanceFieldGlyphCacheManager;
class QSGDistanceFieldTextMaterial;
class QSGDistanceFieldGlyphNode: public QSGGlyphNode
{
public:
    QSGDistanceFieldGlyphNode(QSGDistanceFieldGlyphCacheManager *cacheManager);
    ~QSGDistanceFieldGlyphNode();

    virtual QPointF baseLine() const { return m_baseLine; }
    virtual void setGlyphs(const QPointF &position, const QGlyphRun &glyphs);
    virtual void setColor(const QColor &color);

    virtual void setPreferredAntialiasingMode(AntialiasingMode mode);

    virtual void setStyle(QQuickText::TextStyle style);
    virtual void setStyleColor(const QColor &color);

    virtual void update();

    void updateGeometry();

private:
    void updateMaterial();

    QColor m_color;
    QPointF m_baseLine;
    QSGDistanceFieldTextMaterial *m_material;
    QPointF m_position;
    QGlyphRun m_glyphs;
    QSGDistanceFieldGlyphCacheManager *m_glyph_cacheManager;
    QSGDistanceFieldGlyphCache *m_glyph_cache;
    QSGGeometry m_geometry;
    QQuickText::TextStyle m_style;
    QColor m_styleColor;
    AntialiasingMode m_antialiasingMode;
    QRectF m_boundingRect;

    struct GlyphInfo {
        quint32 glyphIndex;
        QPointF position;
    };
    QLinkedList<GlyphInfo> m_glyphsToAdd;

    uint m_dirtyGeometry: 1;
    uint m_dirtyMaterial: 1;
};

QT_END_HEADER

QT_END_NAMESPACE

#endif // DISTANCEFIELD_GLYPHNODE_H
