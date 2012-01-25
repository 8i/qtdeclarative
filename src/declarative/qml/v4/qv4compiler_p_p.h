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

#ifndef QV4COMPILER_P_P_H
#define QV4COMPILER_P_P_H

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

#include "qv4instruction_p.h"
#include "qv4ir_p.h"
#include <private/qdeclarativescript_p.h>
#include <private/qdeclarativeimport_p.h>
#include <private/qdeclarativeengine_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

// NOTE: This is a copy of QDeclarative1AnchorLine: src/qtquick1/graphicsitems/qdeclarativeanchors_p_p.h
class QGraphicsObject;
class QDeclarative1AnchorLine
{
public:
    QDeclarative1AnchorLine() : item(0), anchorLine(Invalid) {}

    enum AnchorLine {
        Invalid = 0x0,
        Left = 0x01,
        Right = 0x02,
        Top = 0x04,
        Bottom = 0x08,
        HCenter = 0x10,
        VCenter = 0x20,
        Baseline = 0x40,
        Horizontal_Mask = Left | Right | HCenter,
        Vertical_Mask = Top | Bottom | VCenter | Baseline
    };

    QGraphicsObject *item;
    AnchorLine anchorLine;
};

inline bool operator==(const QDeclarative1AnchorLine& a, const QDeclarative1AnchorLine& b)
{
    return a.item == b.item && a.anchorLine == b.anchorLine;
}


template <typename _Key, typename _Value>
class QDeclarativeAssociationList
{
public:
    typedef QVarLengthArray<QPair<_Key, _Value>, 8> Container;
    typedef typename Container::const_iterator const_iterator;
    typedef typename Container::const_iterator ConstIterator;

    const_iterator begin() const { return _container.begin(); }
    const_iterator end() const { return _container.end(); }
    int count() const { return _container.count(); }
    void clear() { _container.clear(); }

    _Value *value(const _Key &key) {
        for (int i = 0; i < _container.size(); ++i) {
            QPair<_Key, _Value> &p = _container[i];
            if (p.first == key)
                return &p.second;
        }
        return 0;
    }

    _Value &operator[](const _Key &key) {
        for (int i = 0; i < _container.size(); ++i) {
            QPair<_Key, _Value> &p = _container[i];
            if (p.first == key)
                return p.second;
        }
        int index = _container.size();
        _container.append(qMakePair(key, _Value()));
        return _container[index].second;
    }

    void insert(const _Key &key, _Value &value) {
        for (int i = 0; i < _container.size(); ++i) {
            QPair<_Key, _Value> &p = _container[i];
            if (p.first == key) {
                p.second = value;
                return;
            }
        }
        _container.append(qMakePair(key, value));
    }

private:
    Container _container;
};

class QV4CompilerPrivate: protected QDeclarativeJS::IR::ExprVisitor, 
                                     protected QDeclarativeJS::IR::StmtVisitor
{
public:
    QV4CompilerPrivate();

    void resetInstanceState();
    int commitCompile();

    const QV4Compiler::Expression *expression;
    QDeclarativeEnginePrivate *engine;

    QString contextName() const { return QLatin1String("$$$SCOPE_") + QString::number((quintptr)expression->context, 16); }

    bool compile(QDeclarativeJS::AST::Node *);

    int registerLiteralString(quint8 reg, const QStringRef &);
    int registerString(const QString &);
    QDeclarativeAssociationList<QString, QPair<int, int> > registeredStrings;
    QByteArray data;

    bool blockNeedsSubscription(const QStringList &);
    int subscriptionIndex(const QStringList &);
    quint32 subscriptionBlockMask(const QStringList &);

    quint8 exceptionId(quint32 line, quint32 column);
    quint8 exceptionId(QDeclarativeJS::AST::ExpressionNode *);
    QVector<quint64> exceptions;

    QDeclarativeAssociationList<int, quint32> usedSubscriptionIds;

    QDeclarativeAssociationList<QString, int> subscriptionIds;
    QDeclarativeJS::Bytecode bytecode;

    // back patching
    struct Patch {
        QDeclarativeJS::IR::BasicBlock *block; // the basic block
        int offset; // the index of the instruction to patch
        Patch(QDeclarativeJS::IR::BasicBlock *block = 0, int index = -1)
            : block(block), offset(index) {}
    };
    QVector<Patch> patches;
    QDeclarativePool pool;

    // Committed binding data
    struct {
        QList<int> offsets;
        QList<QDeclarativeAssociationList<int, quint32> > dependencies;

        //QDeclarativeJS::Bytecode bytecode;
        QByteArray bytecode;
        QByteArray data;
        QDeclarativeAssociationList<QString, int> subscriptionIds;
        QVector<quint64> exceptions;

        QDeclarativeAssociationList<QString, QPair<int, int> > registeredStrings;

        int count() const { return offsets.count(); }
    } committed;

    QByteArray buildSignalTable() const;
    QByteArray buildExceptionData() const;

    void convertToReal(QDeclarativeJS::IR::Expr *expr, int reg);    
    void convertToInt(QDeclarativeJS::IR::Expr *expr, int reg);
    void convertToBool(QDeclarativeJS::IR::Expr *expr, int reg);
    quint8 instructionOpcode(QDeclarativeJS::IR::Binop *e);

    struct Instr {
#define QML_V4_INSTR_DATA_TYPEDEF(I, FMT) typedef QDeclarativeJS::V4InstrData<QDeclarativeJS::V4Instr::I> I;
    FOR_EACH_V4_INSTR(QML_V4_INSTR_DATA_TYPEDEF)
#undef QML_v4_INSTR_DATA_TYPEDEF
    private:
        Instr();
    };

protected:
    //
    // tracing
    //
    void trace(int line, int column);
    void trace(QVector<QDeclarativeJS::IR::BasicBlock *> *blocks);
    void traceExpression(QDeclarativeJS::IR::Expr *e, quint8 r);

    template <int Instr>
    inline void gen(const QDeclarativeJS::V4InstrData<Instr> &i)
    { bytecode.append(i); }
    inline void gen(QDeclarativeJS::V4Instr::Type type, QDeclarativeJS::V4Instr &instr)
    { bytecode.append(type, instr); }

    inline QDeclarativeJS::V4Instr::Type instructionType(const QDeclarativeJS::V4Instr *i) const
    { return bytecode.instructionType(i); }

    //
    // expressions
    //
    virtual void visitConst(QDeclarativeJS::IR::Const *e);
    virtual void visitString(QDeclarativeJS::IR::String *e);
    virtual void visitName(QDeclarativeJS::IR::Name *e);
    virtual void visitTemp(QDeclarativeJS::IR::Temp *e);
    virtual void visitUnop(QDeclarativeJS::IR::Unop *e);
    virtual void visitBinop(QDeclarativeJS::IR::Binop *e);
    virtual void visitCall(QDeclarativeJS::IR::Call *e);

    //
    // statements
    //
    virtual void visitExp(QDeclarativeJS::IR::Exp *s);
    virtual void visitMove(QDeclarativeJS::IR::Move *s);
    virtual void visitJump(QDeclarativeJS::IR::Jump *s);
    virtual void visitCJump(QDeclarativeJS::IR::CJump *s);
    virtual void visitRet(QDeclarativeJS::IR::Ret *s);

private:
    QStringList _subscribeName;
    QDeclarativeJS::IR::Function *_function;
    QDeclarativeJS::IR::BasicBlock *_block;
    void discard() { _discarded = true; }
    bool _discarded;
    quint8 currentReg;

    bool usedSubscriptionIdsChanged;
    quint32 currentBlockMask;
};


QT_END_NAMESPACE

Q_DECLARE_METATYPE(QDeclarative1AnchorLine)

QT_END_HEADER

#endif // QV4COMPILER_P_P_H

