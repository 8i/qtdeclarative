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

#ifndef ITEMPARTICLE_H
#define ITEMPARTICLE_H
#include "qsgparticlepainter_p.h"
#include <QPointer>
#include <QSet>
QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)
class QSGVisualDataModel;
class QSGItemParticleAttached;

class QSGItemParticle : public QSGParticlePainter
{
    Q_OBJECT
    Q_PROPERTY(bool fade READ fade WRITE setFade NOTIFY fadeChanged)
    Q_PROPERTY(QDeclarativeComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
public:
    explicit QSGItemParticle(QSGItem *parent = 0);

    bool fade() const { return m_fade; }

    virtual QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

    static QSGItemParticleAttached *qmlAttachedProperties(QObject *object);
    QDeclarativeComponent* delegate() const
    {
        return m_delegate;
    }

signals:
    void fadeChanged();

    void delegateChanged(QDeclarativeComponent* arg);

public slots:
    //TODO: Add a follow mode, where moving the delegate causes the logical particle to go with it?
    void freeze(QSGItem* item);
    void unfreeze(QSGItem* item);
    void take(QSGItem* item,bool prioritize=false);//take by modelparticle
    void give(QSGItem* item);//give from modelparticle

    void setFade(bool arg){if(arg == m_fade) return; m_fade = arg; emit fadeChanged();}
    void setDelegate(QDeclarativeComponent* arg)
    {
        if (m_delegate != arg) {
            m_delegate = arg;
            emit delegateChanged(arg);
        }
    }

protected:
    virtual void reset();
    virtual void reload(int idx);
    virtual void initialize(int idx);
    void prepareNextFrame();
private slots:
    void tick();
private:
    QList<QSGItem* > m_deletables;
    QList< int > m_loadables;
    bool m_fade;

    QList<QSGItem*> m_pendingItems;
    QList<int> m_available;
    QSet<QSGItem*> m_stasis;
    qreal m_lastT;
    int m_activeCount;
    QDeclarativeComponent* m_delegate;
};

class QSGItemParticleAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QSGItemParticle* particle READ particle CONSTANT);
public:
    QSGItemParticleAttached(QObject* parent)
        : QObject(parent), m_mp(0)
    {;}
    QSGItemParticle* particle() {return m_mp;}
    void detach(){emit detached();}
    void attach(){emit attached();}
private:
    QSGItemParticle* m_mp;
    friend class QSGItemParticle;
Q_SIGNALS:
    void detached();
    void attached();
};

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QSGItemParticle, QML_HAS_ATTACHED_PROPERTIES)

QT_END_HEADER
#endif // ITEMPARTICLE_H
