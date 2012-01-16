/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include <private/qdeclarativejsengine_p.h>
#include <private/qdeclarativejsparser_p.h>
#include <private/qdeclarativejslexer_p.h>
#include <private/qdeclarativejsastvisitor_p.h>
#include <private/qdeclarativejsast_p.h>

#include <qtest.h>
#include <QDir>
#include <QDebug>
#include <cstdlib>

class tst_qdeclarativeparser : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativeparser();

private slots:
    void initTestCase();
    void qmlParser_data();
    void qmlParser();

private:
    QStringList excludedDirs;

    QStringList findFiles(const QDir &);
};

namespace check {

using namespace QDeclarativeJS;

class Check: public AST::Visitor
{
    Engine *engine;
    QList<AST::Node *> nodeStack;

public:
    Check(Engine *engine)
        : engine(engine)
    {
    }

    void operator()(AST::Node *node)
    {
        AST::Node::accept(node, this);
    }

    void checkNode(AST::Node *node)
    {
        if (! nodeStack.isEmpty()) {
            AST::Node *parent = nodeStack.last();
            const quint32 parentBegin = parent->firstSourceLocation().begin();
            const quint32 parentEnd = parent->lastSourceLocation().end();

            QVERIFY(node->firstSourceLocation().begin() >= parentBegin);
            QVERIFY(node->lastSourceLocation().end() <= parentEnd);
        }
    }

    virtual bool preVisit(AST::Node *node)
    {
        checkNode(node);
        nodeStack.append(node);
        return true;
    }

    virtual void postVisit(AST::Node *)
    {
        nodeStack.removeLast();
    }
};

}

tst_qdeclarativeparser::tst_qdeclarativeparser()
{
}

void tst_qdeclarativeparser::initTestCase()
{
    // Add directories you want excluded here

    // These snippets are not expected to run on their own.
    excludedDirs << "doc/src/snippets/declarative/visualdatamodel_rootindex";
    excludedDirs << "doc/src/snippets/declarative/qtbinding";
    excludedDirs << "doc/src/snippets/declarative/imports";
    excludedDirs << "doc/src/snippets/qtquick1/visualdatamodel_rootindex";
    excludedDirs << "doc/src/snippets/qtquick1/qtbinding";
    excludedDirs << "doc/src/snippets/qtquick1/imports";
}

QStringList tst_qdeclarativeparser::findFiles(const QDir &d)
{
    for (int ii = 0; ii < excludedDirs.count(); ++ii) {
        QString s = excludedDirs.at(ii);
        if (d.absolutePath().endsWith(s))
            return QStringList();
    }

    QStringList rv;

    QStringList files = d.entryList(QStringList() << QLatin1String("*.qml") << QLatin1String("*.js"),
                                    QDir::Files);
    foreach (const QString &file, files) {
        rv << d.absoluteFilePath(file);
    }

    QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                                   QDir::NoSymLinks);
    foreach (const QString &dir, dirs) {
        QDir sub = d;
        sub.cd(dir);
        rv << findFiles(sub);
    }

    return rv;
}

/*
This test checks all the qml and js files in the declarative UI source tree
and ensures that the subnode's source locations are inside parent node's source locations
*/

void tst_qdeclarativeparser::qmlParser_data()
{
    QTest::addColumn<QString>("file");

    QString examples = QLatin1String(SRCDIR) + "/../../../../examples/";
    QString tests = QLatin1String(SRCDIR) + "/../../../../tests/";

    QStringList files;
    files << findFiles(QDir(examples));
    files << findFiles(QDir(tests));

    foreach (const QString &file, files)
        QTest::newRow(qPrintable(file)) << file;
}

void tst_qdeclarativeparser::qmlParser()
{
    QFETCH(QString, file);

    using namespace QDeclarativeJS;

    QString code;

    QFile f(file);
    if (f.open(QFile::ReadOnly))
        code = QString::fromUtf8(f.readAll());

    const bool qmlMode = file.endsWith(QLatin1String(".qml"));

    Engine engine;
    Lexer lexer(&engine);
    lexer.setCode(code, 1, qmlMode);
    Parser parser(&engine);
    if (qmlMode)
        parser.parse();
    else
        parser.parseProgram();

    check::Check chk(&engine);
    chk(parser.rootNode());
}

QTEST_MAIN(tst_qdeclarativeparser)

#include "tst_qdeclarativeparser.moc"
