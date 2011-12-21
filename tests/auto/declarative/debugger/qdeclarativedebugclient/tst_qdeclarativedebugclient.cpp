/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
#include <qtest.h>
#include <QSignalSpy>
#include <QTimer>
#include <QHostAddress>
#include <QDebug>
#include <QThread>

#include <QtDeclarative/qdeclarativeengine.h>

#include "../shared/debugutil_p.h"

#define PORT 13770
#define STR_PORT "13770"

class tst_QDeclarativeDebugClient : public QObject
{
    Q_OBJECT

private:
    QDeclarativeDebugConnection *m_conn;

private slots:
    void initTestCase();

    void name();
    void status();
    void sendMessage();
    void parallelConnect();
    void sequentialConnect();
};

void tst_QDeclarativeDebugClient::initTestCase()
{
    const QString waitingMsg = QString("QDeclarativeDebugServer: Waiting for connection on port %1...").arg(PORT);
    QTest::ignoreMessage(QtWarningMsg, waitingMsg.toAscii().constData());
    new QDeclarativeEngine(this);

    m_conn = new QDeclarativeDebugConnection(this);

    QDeclarativeDebugTestClient client("tst_QDeclarativeDebugClient::handshake()", m_conn);
    QDeclarativeDebugTestService service("tst_QDeclarativeDebugClient::handshake()");

    QTest::ignoreMessage(QtWarningMsg, "QDeclarativeDebugServer: Connection established");
    for (int i = 0; i < 50; ++i) {
        // try for 5 seconds ...
        m_conn->connectToHost("127.0.0.1", PORT);
        if (m_conn->waitForConnected())
            break;
        QTest::qSleep(100);
    }

    QVERIFY(m_conn->isConnected());

    QTRY_VERIFY(QDeclarativeDebugService::hasDebuggingClient());
    QTRY_COMPARE(client.status(), QDeclarativeDebugClient::Enabled);
}

void tst_QDeclarativeDebugClient::name()
{
    QString name = "tst_QDeclarativeDebugClient::name()";

    QDeclarativeDebugClient client(name, m_conn);
    QCOMPARE(client.name(), name);
}

void tst_QDeclarativeDebugClient::status()
{
    {
        QDeclarativeDebugConnection dummyConn;
        QDeclarativeDebugClient client("tst_QDeclarativeDebugClient::status()", &dummyConn);
        QCOMPARE(client.status(), QDeclarativeDebugClient::NotConnected);
    }

    QDeclarativeDebugTestClient client("tst_QDeclarativeDebugClient::status()", m_conn);
    QCOMPARE(client.status(), QDeclarativeDebugClient::Unavailable);

    {
        QDeclarativeDebugTestService service("tst_QDeclarativeDebugClient::status()");
        QTRY_COMPARE(client.status(), QDeclarativeDebugClient::Enabled);
    }

    QTRY_COMPARE(client.status(), QDeclarativeDebugClient::Unavailable);

    // duplicate plugin name
    QTest::ignoreMessage(QtWarningMsg, "QDeclarativeDebugClient: Conflicting plugin name \"tst_QDeclarativeDebugClient::status()\" ");
    QDeclarativeDebugClient client2("tst_QDeclarativeDebugClient::status()", m_conn);
    QCOMPARE(client2.status(), QDeclarativeDebugClient::NotConnected);

    QDeclarativeDebugClient client3("tst_QDeclarativeDebugClient::status3()", 0);
    QCOMPARE(client3.status(), QDeclarativeDebugClient::NotConnected);
}

void tst_QDeclarativeDebugClient::sendMessage()
{
    QDeclarativeDebugTestService service("tst_QDeclarativeDebugClient::sendMessage()");
    QDeclarativeDebugTestClient client("tst_QDeclarativeDebugClient::sendMessage()", m_conn);

    QByteArray msg = "hello!";

    QTRY_COMPARE(client.status(), QDeclarativeDebugClient::Enabled);

    client.sendMessage(msg);
    QByteArray resp = client.waitForResponse();
    QCOMPARE(resp, msg);
}

void tst_QDeclarativeDebugClient::parallelConnect()
{
    QDeclarativeDebugConnection connection2;

    QTest::ignoreMessage(QtWarningMsg, "QDeclarativeDebugServer: Another client is already connected");
    // will connect & immediately disconnect
    connection2.connectToHost("127.0.0.1", PORT);
    QVERIFY(connection2.waitForConnected());
    QTRY_COMPARE(connection2.state(), QAbstractSocket::UnconnectedState);
    QVERIFY(m_conn->isConnected());
}

void tst_QDeclarativeDebugClient::sequentialConnect()
{
    QDeclarativeDebugConnection connection2;
    QDeclarativeDebugTestClient client2("tst_QDeclarativeDebugClient::handshake()", &connection2);
    QDeclarativeDebugTestService service("tst_QDeclarativeDebugClient::handshake()");

    m_conn->close();
    QVERIFY(!m_conn->isConnected());
    QCOMPARE(m_conn->state(), QAbstractSocket::UnconnectedState);

    // Make sure that the disconnect is actually delivered to the server
    QTest::qWait(100);

    connection2.connectToHost("127.0.0.1", PORT);
    QTest::ignoreMessage(QtWarningMsg, "QDeclarativeDebugServer: Connection established");
    QVERIFY(connection2.waitForConnected());
    QVERIFY(connection2.isConnected());
    QTRY_VERIFY(client2.status() == QDeclarativeDebugClient::Enabled);
}

int main(int argc, char *argv[])
{
    int _argc = argc + 1;
    char **_argv = new char*[_argc];
    for (int i = 0; i < argc; ++i)
        _argv[i] = argv[i];
    char arg[] = "-qmljsdebugger=port:" STR_PORT;
    _argv[_argc - 1] = arg;

    QGuiApplication app(_argc, _argv);
    tst_QDeclarativeDebugClient tc;
    return QTest::qExec(&tc, _argc, _argv);
    delete _argv;
}

#include "tst_qdeclarativedebugclient.moc"

