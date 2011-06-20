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

#ifndef MASKEXTRUDER_H
#define MASKEXTRUDER_H
#include "qsgparticleextruder_p.h"
#include <QUrl>
#include <QImage>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGMaskExtruder : public QSGParticleExtruder
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
public:
    explicit QSGMaskExtruder(QObject *parent = 0);
    virtual QPointF extrude(const QRectF &);
    virtual bool contains(const QRectF &bounds, const QPointF &point);

    QUrl source() const
    {
        return m_source;
    }

signals:

    void sourceChanged(QUrl arg);

public slots:

    void setSource(QUrl arg)
    {
        if (m_source != arg) {
            m_source = arg;
            m_lastHeight = -1;//Trigger reset
            m_lastWidth = -1;
            emit sourceChanged(arg);
        }
    }
private:
    QUrl m_source;

    void ensureInitialized(const QRectF &r);
    int m_lastWidth;
    int m_lastHeight;
    QImage m_img;
    QList<QPointF> m_mask;//TODO: More memory efficient datastructures
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // MASKEXTRUDER_H
