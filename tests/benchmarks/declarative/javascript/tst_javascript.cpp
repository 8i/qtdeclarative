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

#include <QDir>
#include <QDebug>
#include <qtest.h>
#include <QDeclarativeEngine>
#include <QDeclarativeComponent>

#include "testtypes.h"

class tst_javascript : public QObject
{
    Q_OBJECT

public:
    tst_javascript();
    virtual ~tst_javascript();

private slots:
    void run_data();
    void run();

private:
    QDeclarativeEngine engine;
};

tst_javascript::tst_javascript()
{
    registerTypes();
}

tst_javascript::~tst_javascript()
{
}

void tst_javascript::run_data()
{
    QTest::addColumn<QString>("file");

    QDir dir(SRCDIR "/data");

    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    for (int ii = 0; ii < files.count(); ++ii) {
        QString file = files.at(ii);
        if (file.endsWith(".qml") && file.at(0).isLower()) {

            QString testName = file.left(file.length() - 4 /* strlen(".qml") */);
            QString fileName = QLatin1String(SRCDIR) + QLatin1String("/data/") + file;


            QTest::newRow(qPrintable(testName)) << fileName;

        }
    }
}

void tst_javascript::run()
{
    QFETCH(QString, file);

    QDeclarativeComponent c(&engine, file);

    if (c.isError()) {
        qWarning() << c.errors();
    }

    QVERIFY(!c.isError());

    QObject *o = c.create();
    QVERIFY(o != 0);

    QMetaMethod method = o->metaObject()->method(o->metaObject()->indexOfMethod("runtest()"));

    QBENCHMARK {
        method.invoke(o);
    }

    delete o;
}

QTEST_MAIN(tst_javascript)

#include "tst_javascript.moc"
