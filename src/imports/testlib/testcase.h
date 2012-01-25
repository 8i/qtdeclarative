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

#ifndef TESTCASE_H
#define TESTCASE_H

// This is a dummy header for defining the interface of "TestCase.qml" to qdoc.

#include <QtDeclarative/qdeclarativeitem.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class TestCase : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool when READ when WRITE setWhen NOTIFY whenChanged)
    Q_PROPERTY(bool optional READ optional WRITE setOptional NOTIFY optionalChanged)
    Q_PROPERTY(bool completed READ completed NOTIFY completedChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool windowShown READ windowShown NOTIFY windowShownChanged)
public:
    TestCase(QDeclarativeItem *parent) : QDeclarativeItem(parent) {}
    ~TestCase()

    QString name() const;
    void setName(const QString &name);

    bool when() const;
    void setWhen(bool when);

    bool optional() const;
    void setOptional(bool optional);

    bool completed() const;
    bool running() const;
    bool windowShown() const;

Q_SIGNALS:
    void nameChanged();
    void whenChanged();
    void optionalChanged();
    void completedChanged();
    void runningChanged();
    void windowShownChanged();
};

QML_DECLARE_TYPE(TestCase)

QT_END_NAMESPACE

QT_END_HEADER

#endif
