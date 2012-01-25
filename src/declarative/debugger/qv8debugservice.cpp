/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include "qv8debugservice_p.h"
#include "qdeclarativedebugservice_p_p.h"
#include <private/qjsconverter_impl_p.h>
#include <private/qv8engine_p.h>

#include <QtCore/QHash>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>

//V8 DEBUG SERVICE PROTOCOL
// <HEADER><COMMAND><DATA>
// <HEADER> : "V8DEBUG"
// <COMMAND> : ["connect", "disconnect", "interrupt",
//              "v8request", "v8message", "breakonsignal",
//              "breakaftercompile"]
// <DATA> : connect, disconnect, interrupt: empty
//          v8request, v8message: <JSONrequest_string>
//          breakonsignal: <signalname_string><enabled_bool>
//          breakaftercompile: <enabled_bool>

const char *V8_DEBUGGER_KEY_VERSION = "version";
const char *V8_DEBUGGER_KEY_CONNECT = "connect";
const char *V8_DEBUGGER_KEY_INTERRUPT = "interrupt";
const char *V8_DEBUGGER_KEY_DISCONNECT = "disconnect";
const char *V8_DEBUGGER_KEY_REQUEST = "v8request";
const char *V8_DEBUGGER_KEY_V8MESSAGE = "v8message";
const char *V8_DEBUGGER_KEY_BREAK_ON_SIGNAL = "breakonsignal";
const char *V8_DEBUGGER_KEY_BREAK_AFTER_COMPILE = "breakaftercompile";

QT_BEGIN_NAMESPACE

struct SignalHandlerData
{
    QString functionName;
    bool enabled;
};

Q_GLOBAL_STATIC(QV8DebugService, v8ServiceInstance)

// DebugMessageHandler will call back already when the QV8DebugService constructor is
// running, we therefore need a plain pointer.
static QV8DebugService *v8ServiceInstancePtr = 0;

void DebugMessageDispatchHandler()
{
    QMetaObject::invokeMethod(v8ServiceInstancePtr, "processDebugMessages", Qt::QueuedConnection);
}

void DebugMessageHandler(const v8::Debug::Message& message)
{
    v8::DebugEvent event = message.GetEvent();

    if (event != v8::Break && event != v8::Exception &&
            event != v8::AfterCompile && event != v8::BeforeCompile)
            return;
    v8ServiceInstancePtr->debugMessageHandler(QJSConverter::toString(message.GetJSON()), event);
}

class QV8DebugServicePrivate : public QDeclarativeDebugServicePrivate
{
public:
    QV8DebugServicePrivate()
        : connectReceived(false)
        , breakAfterCompile(false)
        , engine(0)
    {
    }

    void initializeDebuggerThread();

    static QByteArray packMessage(const QString &type, const QString &message = QString());

    bool connectReceived;
    bool breakAfterCompile;
    QMutex initializeMutex;
    QStringList breakOnSignals;
    const QV8Engine *engine;
};

QV8DebugService::QV8DebugService(QObject *parent)
    : QDeclarativeDebugService(*(new QV8DebugServicePrivate()),
                               QLatin1String("V8Debugger"), 2, parent)
{
    Q_D(QV8DebugService);
    v8ServiceInstancePtr = this;
    // wait for statusChanged() -> initialize()
    d->initializeMutex.lock();
    if (registerService() == Enabled) {
        init();
        // ,block mode, client attached
        while (!d->connectReceived) {
            waitForMessage();
        }
    } else {
        d->initializeMutex.unlock();
    }
}

QV8DebugService::~QV8DebugService()
{
}

QV8DebugService *QV8DebugService::instance()
{
    return v8ServiceInstance();
}

void QV8DebugService::initialize(const QV8Engine *engine)
{
    // just make sure that the service is properly registered
    v8ServiceInstance()->setEngine(engine);
}

void QV8DebugService::setEngine(const QV8Engine *engine)
{
    Q_D(QV8DebugService);

    d->engine = engine;
}

void QV8DebugService::debugMessageHandler(const QString &message, const v8::DebugEvent &event)
{
    Q_D(QV8DebugService);
    sendMessage(QV8DebugServicePrivate::packMessage(QLatin1String(V8_DEBUGGER_KEY_V8MESSAGE), message));
    if (event == v8::AfterCompile && d->breakAfterCompile)
        scheduledDebugBreak(true);
}

void QV8DebugService::signalEmitted(const QString &signal)
{
    //This function is only called by QDeclarativeBoundSignal
    //only if there is a slot connected to the signal. Hence, there
    //is no need for additional check.
    Q_D(QV8DebugService);

    //Parse just the name and remove the class info
    //Normalize to Lower case.
    QString signalName = signal.left(signal.indexOf(QLatin1String("("))).toLower();

    foreach (const QString &signal, d->breakOnSignals) {
        if (signal == signalName) {
            scheduledDebugBreak(true);
            break;
        }
    }
}

// executed in the gui thread
void QV8DebugService::init()
{
    Q_D(QV8DebugService);
    v8::Debug::SetMessageHandler2(DebugMessageHandler);
    v8::Debug::SetDebugMessageDispatchHandler(DebugMessageDispatchHandler);
    d->initializeMutex.unlock();
}

// executed in the gui thread
void QV8DebugService::scheduledDebugBreak(bool schedule)
{
    if (schedule)
        v8::Debug::DebugBreak();
    else
        v8::Debug::CancelDebugBreak();
}

// executed in the debugger thread
void QV8DebugService::statusChanged(QDeclarativeDebugService::Status newStatus)
{
    Q_D(QV8DebugService);
    if (newStatus == Enabled) {
        // execute in GUI thread
        d->initializeMutex.lock();
        QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
    }
}

// executed in the debugger thread
void QV8DebugService::messageReceived(const QByteArray &message)
{
    Q_D(QV8DebugService);

    QDataStream ds(message);
    QByteArray header;
    ds >> header;

    if (header == "V8DEBUG") {
        QByteArray command;
        QByteArray data;
        ds >> command >> data;

        if (command == V8_DEBUGGER_KEY_CONNECT) {
            QMutexLocker locker(&d->initializeMutex);
            d->connectReceived = true;
            sendMessage(QV8DebugServicePrivate::packMessage(QLatin1String(V8_DEBUGGER_KEY_CONNECT)));

        } else if (command == V8_DEBUGGER_KEY_INTERRUPT) {
            // break has to be executed in gui thread
            QMetaObject::invokeMethod(this, "scheduledDebugBreak", Qt::QueuedConnection, Q_ARG(bool, true));
            sendMessage(QV8DebugServicePrivate::packMessage(QLatin1String(V8_DEBUGGER_KEY_INTERRUPT)));

        } else if (command == V8_DEBUGGER_KEY_DISCONNECT) {
            // cancel break has to be executed in gui thread
            QMetaObject::invokeMethod(this, "scheduledDebugBreak", Qt::QueuedConnection, Q_ARG(bool, false));
            sendDebugMessage(QString::fromUtf8(data));

        } else if (command == V8_DEBUGGER_KEY_REQUEST) {
            sendDebugMessage(QString::fromUtf8(data));

        } else if (command == V8_DEBUGGER_KEY_BREAK_ON_SIGNAL) {
            QDataStream rs(data);
            QByteArray signal;
            bool enabled;
            rs >> signal >> enabled;
             //Normalize to lower case.
            QString signalName(QString::fromUtf8(signal).toLower());
            if (enabled)
                d->breakOnSignals.append(signalName);
            else
                d->breakOnSignals.removeOne(signalName);
            sendMessage(QV8DebugServicePrivate::packMessage(QLatin1String(V8_DEBUGGER_KEY_BREAK_ON_SIGNAL)));

        } else if (command == V8_DEBUGGER_KEY_BREAK_AFTER_COMPILE) {
            QDataStream rs(data);
            rs >> d->breakAfterCompile;
            sendMessage(QV8DebugServicePrivate::packMessage(QLatin1String(V8_DEBUGGER_KEY_BREAK_AFTER_COMPILE)));

        }
    }
}

void QV8DebugService::sendDebugMessage(const QString &message)
{
    v8::Debug::SendCommand(message.utf16(), message.size());
}

void QV8DebugService::processDebugMessages()
{
    Q_D(QV8DebugService);
    v8::HandleScope handleScope;
    v8::Context::Scope contextScope(d->engine->context());
    v8::Debug::ProcessDebugMessages();
}

QByteArray QV8DebugServicePrivate::packMessage(const QString &type, const QString &message)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd("V8DEBUG");
    rs << cmd << type.toUtf8() << message.toUtf8();
    return reply;
}

QT_END_NAMESPACE
