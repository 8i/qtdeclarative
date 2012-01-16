/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QV8DEBUGSERVICE_P_H
#define QV8DEBUGSERVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qdeclarativedebugservice_p.h"
#include <private/qv8debug_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QV8Engine;
class QV8DebugServicePrivate;

class QV8DebugService : public QDeclarativeDebugService
{
    Q_OBJECT
public:
    QV8DebugService(QObject *parent = 0);
    ~QV8DebugService();

    static QV8DebugService *instance();
    static void initialize(const QV8Engine *engine);

    void debugMessageHandler(const QString &message, const v8::DebugEvent &event);

    void signalEmitted(const QString &signal);

public slots:
    void processDebugMessages();

private slots:
    void scheduledDebugBreak(bool schedule);
    void sendDebugMessage(const QString &message);
    void init();

protected:
    void statusChanged(Status newStatus);
    void messageReceived(const QByteArray &);

private:
    void setEngine(const QV8Engine *engine);

private:
    Q_DISABLE_COPY(QV8DebugService)
    Q_DECLARE_PRIVATE(QV8DebugService)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QV8DEBUGSERVICE_P_H
