/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgmaskextruder_p.h"
#include <QImage>
#include <QDebug>
QT_BEGIN_NAMESPACE
QSGMaskExtruder::QSGMaskExtruder(QObject *parent) :
    QSGParticleExtruder(parent)
  , m_lastWidth(-1)
  , m_lastHeight(-1)
{
}

QPointF QSGMaskExtruder::extrude(const QRectF &r)
{
    ensureInitialized(r);
    if(!m_mask.count() || m_img.isNull())
        return r.topLeft();
    const QPointF p = m_mask[rand() % m_mask.count()];
    //### Should random sub-pixel positioning be added?
    return p + r.topLeft();
}

bool QSGMaskExtruder::contains(const QRectF &bounds, const QPointF &point)
{
    ensureInitialized(bounds);//###Current usage patterns WILL lead to different bounds/r calls. Separate list?
    if(m_img.isNull())
        return false;
    QPoint p = point.toPoint() - bounds.topLeft().toPoint();
    return m_img.rect().contains(p) && (bool)m_img.pixelIndex(p);
}

void QSGMaskExtruder::ensureInitialized(const QRectF &r)
{
    if(m_lastWidth == r.width() && m_lastHeight == r.height())
        return;//Same as before
    m_lastWidth = r.width();
    m_lastHeight = r.height();

    m_img = QImage();
    m_mask.clear();
    if(m_source.isEmpty())
        return;
    m_img = QImage(m_source.toLocalFile());
    if(m_img.isNull()){
        qWarning() << "MaskShape: Cannot load" << qPrintable(m_source.toLocalFile());
        return;
    }
    m_img = m_img.createAlphaMask();
    m_img = m_img.convertToFormat(QImage::Format_Mono);//Else LSB, but I think that's easier
    m_img = m_img.scaled(r.size().toSize());//TODO: Do they need aspect ratio stuff? Or tiling?
    for(int i=0; i<r.width(); i++){
        for(int j=0; j<r.height(); j++){
            if(m_img.pixelIndex(i,j))//Direct bit manipulation is presumably more efficient
                m_mask << QPointF(i,j);
        }
    }
}
QT_END_NAMESPACE
