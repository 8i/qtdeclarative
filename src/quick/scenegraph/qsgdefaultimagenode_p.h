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


#ifndef DEFAULT_PIXMAPNODE_H
#define DEFAULT_PIXMAPNODE_H

#include <private/qsgadaptationlayer_p.h>

#include <QtQuick/qsgtexturematerial.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QSGDefaultImageNode : public QSGImageNode
{
public:
    QSGDefaultImageNode();
    virtual void setTargetRect(const QRectF &rect);
    virtual void setSourceRect(const QRectF &rect);
    virtual void setTexture(QSGTexture *t);
    virtual void update();

    virtual void setMipmapFiltering(QSGTexture::Filtering filtering);
    virtual void setFiltering(QSGTexture::Filtering filtering);
    virtual void setHorizontalWrapMode(QSGTexture::WrapMode wrapMode);
    virtual void setVerticalWrapMode(QSGTexture::WrapMode wrapMode);

    virtual void preprocess();

private:
    void updateGeometry();

    QRectF m_targetRect;
    QRectF m_sourceRect;

    QSGOpaqueTextureMaterial m_material;
    QSGTextureMaterial m_materialO;

    uint m_dirtyGeometry : 1;

    QSGGeometry m_geometry;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
