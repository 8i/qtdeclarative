/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include "debugutil_p.h"

#include <QEventLoop>
#include <QTimer>

#include <private/qdeclarativedebugclient_p.h>
#include <private/qdeclarativedebugservice_p.h>

bool QDeclarativeDebugTest::waitForSignal(QObject *receiver, const char *member, int timeout) {
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    QObject::connect(receiver, member, &loop, SLOT(quit()));
    timer.start(timeout);
    loop.exec();
    return timer.isActive();
}

QDeclarativeDebugTestService::QDeclarativeDebugTestService(const QString &s, float version, QObject *parent)
    : QDeclarativeDebugService(s, version, parent)
{
    registerService();
}

void QDeclarativeDebugTestService::messageReceived(const QByteArray &ba)
{
    sendMessage(ba);
}

void QDeclarativeDebugTestService::statusChanged(Status)
{
    emit statusHasChanged();
}

QDeclarativeDebugTestClient::QDeclarativeDebugTestClient(const QString &s, QDeclarativeDebugConnection *c)
    : QDeclarativeDebugClient(s, c)
{
}

QByteArray QDeclarativeDebugTestClient::waitForResponse()
{
    lastMsg.clear();
    QDeclarativeDebugTest::waitForSignal(this, SIGNAL(serverMessage(QByteArray)));
    if (lastMsg.isEmpty()) {
        qWarning() << "tst_QDeclarativeDebugClient: no response from server!";
        return QByteArray();
    }
    return lastMsg;
}

void QDeclarativeDebugTestClient::statusChanged(Status stat)
{
    QCOMPARE(stat, status());
    emit statusHasChanged();
}

void QDeclarativeDebugTestClient::messageReceived(const QByteArray &ba)
{
    lastMsg = ba;
    emit serverMessage(ba);
}

QDeclarativeDebugProcess::QDeclarativeDebugProcess(const QString &executable)
    : m_executable(executable)
    , m_started(false)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    m_timer.setSingleShot(true);
    m_timer.setInterval(5000);
    connect(&m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(processAppOutput()));
    connect(&m_timer, SIGNAL(timeout()), &m_eventLoop, SLOT(quit()));
}

QDeclarativeDebugProcess::~QDeclarativeDebugProcess()
{
    stop();
}

void QDeclarativeDebugProcess::start(const QStringList &arguments)
{
    m_mutex.lock();
    m_process.setEnvironment(m_environment);
    m_process.start(m_executable, arguments);
    m_process.waitForStarted();
    m_timer.start();
    m_mutex.unlock();
}

void QDeclarativeDebugProcess::stop()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(5000);
    }
}

bool QDeclarativeDebugProcess::waitForSessionStart()
{
    if (m_process.state() != QProcess::Running) {
        qWarning() << "Could not start up " << m_executable;
        return false;
    }
    m_eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

    return m_started;
}

void QDeclarativeDebugProcess::setEnvironment(const QStringList &environment)
{
    m_environment = environment;
}

QString QDeclarativeDebugProcess::output() const
{
    return m_output;
}

void QDeclarativeDebugProcess::processAppOutput()
{
    m_mutex.lock();

    QString newOutput = m_process.readAll();
    m_output.append(newOutput);
    m_outputBuffer.append(newOutput);

    while (true) {
        const int nlIndex = m_outputBuffer.indexOf(QLatin1Char('\n'));
        if (nlIndex < 0) // no further complete lines
            break;
        const QString line = m_outputBuffer.left(nlIndex);
        m_outputBuffer = m_outputBuffer.right(m_outputBuffer.size() - nlIndex - 1);

        if (line.startsWith("Qml debugging is enabled")) // ignore
            continue;
        if (line.startsWith("QDeclarativeDebugServer:")) {
            if (line.contains("Waiting for connection ")) {
                m_started = true;
                m_eventLoop.quit();
                continue;
            }
            if (line.contains("Connection established")) {
                continue;
            }
        }
    }
    m_mutex.unlock();
}
