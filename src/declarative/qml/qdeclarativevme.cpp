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

#include "qdeclarativevme_p.h"

#include "qdeclarativecompiler_p.h"
#include "qdeclarativeboundsignal_p.h"
#include "qdeclarativestringconverters_p.h"
#include <private/qmetaobjectbuilder_p.h>
#include <private/qfastmetabuilder_p.h>
#include "qdeclarativedata_p.h"
#include "qdeclarative.h"
#include "qdeclarativecustomparser_p.h"
#include "qdeclarativeengine.h"
#include "qdeclarativecontext.h"
#include "qdeclarativecomponent.h"
#include "qdeclarativecomponentattached_p.h"
#include "qdeclarativebinding_p.h"
#include "qdeclarativeengine_p.h"
#include "qdeclarativecomponent_p.h"
#include "qdeclarativevmemetaobject_p.h"
#include "qdeclarativebinding_p_p.h"
#include "qdeclarativecontext_p.h"
#include <private/qv4bindings_p.h>
#include <private/qv8bindings_p.h>
#include "qdeclarativeglobal_p.h"
#include <private/qfinitestack_p.h>
#include "qdeclarativescriptstring.h"
#include "qdeclarativescriptstring_p.h"
#include "qdeclarativepropertyvalueinterceptor_p.h"

#include <QStack>
#include <QColor>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QtCore/qdebug.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qvarlengtharray.h>
#include <QtDeclarative/qjsvalue.h>

QT_BEGIN_NAMESPACE

using namespace QDeclarativeVMETypes;

#define VME_EXCEPTION(desc, line) \
    { \
        QDeclarativeError error; \
        error.setDescription(desc.trimmed()); \
        error.setLine(line); \
        error.setUrl(COMP->url); \
        *errors << error; \
        goto exceptionExit; \
    }

void QDeclarativeVME::init(QDeclarativeContextData *ctxt, QDeclarativeCompiledData *comp, int start,
                           QDeclarativeContextData *creation)
{
    Q_ASSERT(ctxt);
    Q_ASSERT(comp);

    if (start == -1) {
        start = 0;
    } else {
        creationContext = creation;
    }

    State initState;
    initState.context = ctxt;
    initState.compiledData = comp;
    initState.instructionStream = comp->bytecode.constData() + start;
    states.push(initState);

    typedef QDeclarativeInstruction I;
    I *i = (I *)initState.instructionStream;

    Q_ASSERT(comp->instructionType(i) == I::Init);

    objects.allocate(i->init.objectStackSize);
    lists.allocate(i->init.listStackSize);
    bindValues.allocate(i->init.bindingsSize);
    parserStatus.allocate(i->init.parserStatusSize);

#ifdef QML_ENABLE_TRACE
    parserStatusData.allocate(i->init.parserStatusSize);
    rootComponent = comp;
#endif

    rootContext = 0;
    engine = ctxt->engine;
}

bool QDeclarativeVME::initDeferred(QObject *object)
{
    QDeclarativeData *data = QDeclarativeData::get(object);

    if (!data || !data->context || !data->deferredComponent)
        return false;

    QDeclarativeContextData *ctxt = data->context;
    QDeclarativeCompiledData *comp = data->deferredComponent;
    int start = data->deferredIdx;

    State initState;
    initState.flags = State::Deferred;
    initState.context = ctxt;
    initState.compiledData = comp;
    initState.instructionStream = comp->bytecode.constData() + start;
    states.push(initState);

    typedef QDeclarativeInstruction I;
    I *i = (I *)initState.instructionStream;

    Q_ASSERT(comp->instructionType(i) == I::DeferInit);

    objects.allocate(i->deferInit.objectStackSize);
    lists.allocate(i->deferInit.listStackSize);
    bindValues.allocate(i->deferInit.bindingsSize);
    parserStatus.allocate(i->deferInit.parserStatusSize);

    objects.push(object);

#ifdef QML_ENABLE_TRACE
    parserStatusData.allocate(i->deferInit.parserStatusSize);
    rootComponent = comp;
#endif

    rootContext = 0;
    engine = ctxt->engine;

    return true;
}

namespace {
struct ActiveVMERestorer 
{
    ActiveVMERestorer(QDeclarativeVME *me, QDeclarativeEnginePrivate *ep) 
    : ep(ep), oldVME(ep->activeVME) { ep->activeVME = me; }
    ~ActiveVMERestorer() { ep->activeVME = oldVME; }

    QDeclarativeEnginePrivate *ep;
    QDeclarativeVME *oldVME;
};
}

QObject *QDeclarativeVME::execute(QList<QDeclarativeError> *errors, const Interrupt &interrupt)
{
    Q_ASSERT(states.count() >= 1);

#ifdef QML_ENABLE_TRACE
    QDeclarativeTrace trace("VME Execute");
    trace.addDetail("URL", rootComponent->url);
#endif

    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(states.at(0).context->engine);

    ActiveVMERestorer restore(this, ep);

    QObject *rv = run(errors, interrupt);

    return rv;
}

inline bool fastHasBinding(QObject *o, int index) 
{
    QDeclarativeData *ddata = static_cast<QDeclarativeData *>(QObjectPrivate::get(o)->declarativeData);

    return ddata && (ddata->bindingBitsSize > index) && 
           (ddata->bindingBits[index / 32] & (1 << (index % 32)));
}

static void removeBindingOnProperty(QObject *o, int index)
{
    QDeclarativeAbstractBinding *binding = QDeclarativePropertyPrivate::setBinding(o, index, -1, 0);
    if (binding) binding->destroy();
}

static QVariant variantFromString(const QString &string)
{
    return QDeclarativeStringConverters::variantFromString(string);
}

// XXX we probably need some form of "work count" here to prevent us checking this 
// for every instruction.
#define QML_BEGIN_INSTR_COMMON(I) { \
    const QDeclarativeInstructionMeta<(int)QDeclarativeInstruction::I>::DataType &instr = QDeclarativeInstructionMeta<(int)QDeclarativeInstruction::I>::data(*genericInstr); \
    INSTRUCTIONSTREAM += QDeclarativeInstructionMeta<(int)QDeclarativeInstruction::I>::Size; \
    Q_UNUSED(instr);

#ifdef QML_THREADED_VME_INTERPRETER
#  define QML_BEGIN_INSTR(I) op_##I: \
    QML_BEGIN_INSTR_COMMON(I)

#  define QML_NEXT_INSTR(I) { \
    if (watcher.hasRecursed()) return 0; \
    genericInstr = reinterpret_cast<const QDeclarativeInstruction *>(INSTRUCTIONSTREAM); \
    goto *genericInstr->common.code; \
    }

#  define QML_END_INSTR(I) } \
    if (watcher.hasRecursed()) return 0; \
    genericInstr = reinterpret_cast<const QDeclarativeInstruction *>(INSTRUCTIONSTREAM); \
    if (interrupt.shouldInterrupt()) return 0; \
    goto *genericInstr->common.code;

#else
#  define QML_BEGIN_INSTR(I) \
    case QDeclarativeInstruction::I: \
    QML_BEGIN_INSTR_COMMON(I)

#  define QML_NEXT_INSTR(I) { \
    if (watcher.hasRecursed()) return 0; \
    break; \
    }

#  define QML_END_INSTR(I) \
    if (watcher.hasRecursed() || interrupt.shouldInterrupt()) return 0; \
    } break;
#endif

#define QML_STORE_VALUE(name, cpptype, value) \
    QML_BEGIN_INSTR(name) \
        cpptype v = value; \
        void *a[] = { (void *)&v, 0, &status, &flags }; \
        QObject *target = objects.top(); \
        CLEAN_PROPERTY(target, instr.propertyIndex); \
        QMetaObject::metacall(target, QMetaObject::WriteProperty, instr.propertyIndex, a); \
    QML_END_INSTR(name)

#define QML_STORE_LIST(name, cpptype, value) \
    QML_BEGIN_INSTR(name) \
        cpptype v; \
        v.append(value); \
        void *a[] = { (void *)&v, 0, &status, &flags }; \
        QObject *target = objects.top(); \
        CLEAN_PROPERTY(target, instr.propertyIndex); \
        QMetaObject::metacall(target, QMetaObject::WriteProperty, instr.propertyIndex, a); \
    QML_END_INSTR(name)

#define QML_STORE_VAR(name, value) \
    QML_BEGIN_INSTR(name) \
        v8::Handle<v8::Value> v8value = value; \
        QObject *target = objects.top(); \
        CLEAN_PROPERTY(target, instr.propertyIndex); \
        QMetaObject *mo = const_cast<QMetaObject *>(target->metaObject()); \
        QDeclarativeVMEMetaObject *vmemo = static_cast<QDeclarativeVMEMetaObject *>(mo); \
        vmemo->setVMEProperty(instr.propertyIndex, v8value); \
    QML_END_INSTR(name)

#define QML_STORE_POINTER(name, value) \
    QML_BEGIN_INSTR(name) \
        void *a[] = { (void *)value, 0, &status, &flags }; \
        QObject *target = objects.top(); \
        CLEAN_PROPERTY(target, instr.propertyIndex); \
        QMetaObject::metacall(target, QMetaObject::WriteProperty, instr.propertyIndex, a); \
    QML_END_INSTR(name)

#define CLEAN_PROPERTY(o, index) \
    if (fastHasBinding(o, index)) \
        removeBindingOnProperty(o, index)

QObject *QDeclarativeVME::run(QList<QDeclarativeError> *errors,
                              const Interrupt &interrupt
#ifdef QML_THREADED_VME_INTERPRETER
                              , void ***storeJumpTable
#endif
                              )
{
#ifdef QML_THREADED_VME_INTERPRETER
    if (storeJumpTable) {
#define QML_INSTR_ADDR(I, FMT) &&op_##I,
        static void *jumpTable[] = {
            FOR_EACH_QML_INSTR(QML_INSTR_ADDR)
        };
#undef QML_INSTR_ADDR
        *storeJumpTable = jumpTable;
        return 0;
    }
#endif
    Q_ASSERT(errors->isEmpty());
    Q_ASSERT(states.count() >= 1);

    QDeclarativeEngine *engine = states.at(0).context->engine;
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);

    // Need a v8 handle scope and execution context for StoreVar instructions.
    v8::HandleScope handleScope;
    v8::Context::Scope contextScope(ep->v8engine()->context());

    int status = -1; // needed for dbus
    QDeclarativePropertyPrivate::WriteFlags flags = QDeclarativePropertyPrivate::BypassInterceptor |
                                                    QDeclarativePropertyPrivate::RemoveBindingOnAliasWrite;

    QRecursionWatcher<QDeclarativeVME, &QDeclarativeVME::recursion> watcher(this);

#define COMP states.top().compiledData
#define CTXT states.top().context
#define INSTRUCTIONSTREAM states.top().instructionStream
#define BINDINGSKIPLIST states.top().bindingSkipList

#define TYPES COMP->types
#define PRIMITIVES COMP->primitives
#define DATAS COMP->datas
#define PROPERTYCACHES COMP->propertyCaches
#define SCRIPTS COMP->scripts
#define URLS COMP->urls

#ifdef QML_THREADED_VME_INTERPRETER
    const QDeclarativeInstruction *genericInstr = reinterpret_cast<const QDeclarativeInstruction *>(INSTRUCTIONSTREAM);
    goto *genericInstr->common.code;
#else
    for (;;) {
        const QDeclarativeInstruction *genericInstr = reinterpret_cast<const QDeclarativeInstruction *>(INSTRUCTIONSTREAM);

        switch (genericInstr->common.instructionType) {
#endif

        // Store a created object in a property.  These all pop from the objects stack.
        QML_STORE_VALUE(StoreObject, QObject *, objects.pop());
        QML_STORE_VALUE(StoreVariantObject, QVariant, QVariant::fromValue(objects.pop()));
        QML_STORE_VAR(StoreVarObject, ep->v8engine()->newQObject(objects.pop()));

        // Store a literal value in a corresponding property
        QML_STORE_VALUE(StoreFloat, float, instr.value);
        QML_STORE_VALUE(StoreDouble, double, instr.value);
        QML_STORE_VALUE(StoreBool, bool, instr.value);
        QML_STORE_VALUE(StoreInteger, int, instr.value);
        QML_STORE_VALUE(StoreColor, QColor, QColor::fromRgba(instr.value));
        QML_STORE_VALUE(StoreDate, QDate, QDate::fromJulianDay(instr.value));
        QML_STORE_VALUE(StoreDateTime, QDateTime,
                        QDateTime(QDate::fromJulianDay(instr.date), *(QTime *)&instr.time));
        QML_STORE_POINTER(StoreTime, (QTime *)&instr.time);
        QML_STORE_POINTER(StorePoint, (QPoint *)&instr.point);
        QML_STORE_POINTER(StorePointF, (QPointF *)&instr.point);
        QML_STORE_POINTER(StoreSize, (QSize *)&instr.size);
        QML_STORE_POINTER(StoreSizeF, (QSizeF *)&instr.size);
        QML_STORE_POINTER(StoreRect, (QRect *)&instr.rect);
        QML_STORE_POINTER(StoreRectF, (QRectF *)&instr.rect);
        QML_STORE_POINTER(StoreVector3D, (QVector3D *)&instr.vector);
        QML_STORE_POINTER(StoreVector4D, (QVector4D *)&instr.vector);
        QML_STORE_POINTER(StoreString, &PRIMITIVES.at(instr.value));
        QML_STORE_POINTER(StoreByteArray, &DATAS.at(instr.value));
        QML_STORE_POINTER(StoreUrl, &URLS.at(instr.value));
        QML_STORE_VALUE(StoreTrString, QString,
                        QCoreApplication::translate(DATAS.at(instr.context).constData(),
                                                    DATAS.at(instr.text).constData(),
                                                    DATAS.at(instr.comment).constData(),
                                                    QCoreApplication::UnicodeUTF8,
                                                    instr.n));
        QML_STORE_VALUE(StoreTrIdString, QString, qtTrId(DATAS.at(instr.text).constData(), instr.n));

        // Store a literal value in a QList
        QML_STORE_LIST(StoreStringList, QStringList, PRIMITIVES.at(instr.value));
        QML_STORE_LIST(StoreStringQList, QList<QString>, PRIMITIVES.at(instr.value));
        QML_STORE_LIST(StoreUrlQList, QList<QUrl>, URLS.at(instr.value));
        QML_STORE_LIST(StoreDoubleQList, QList<double>, instr.value);
        QML_STORE_LIST(StoreBoolQList, QList<bool>, instr.value);
        QML_STORE_LIST(StoreIntegerQList, QList<int>, instr.value);

        // Store a literal value in a QVariant property
        QML_STORE_VALUE(StoreVariant, QVariant, variantFromString(PRIMITIVES.at(instr.value)));
        QML_STORE_VALUE(StoreVariantInteger, QVariant, QVariant(instr.value));
        QML_STORE_VALUE(StoreVariantDouble, QVariant, QVariant(instr.value));
        QML_STORE_VALUE(StoreVariantBool, QVariant, QVariant(instr.value));

        // Store a literal value in a var property.
        // We deliberately do not use string converters here
        QML_STORE_VAR(StoreVar, ep->v8engine()->fromVariant(PRIMITIVES.at(instr.value)));
        QML_STORE_VAR(StoreVarInteger, v8::Integer::New(instr.value));
        QML_STORE_VAR(StoreVarDouble, v8::Number::New(instr.value));
        QML_STORE_VAR(StoreVarBool, v8::Boolean::New(instr.value));


        QML_BEGIN_INSTR(Init)
            // Ensure that the compiled data has been initialized
            if (!COMP->isInitialized()) COMP->initialize(engine);

            QDeclarativeContextData *parentCtxt = CTXT;
            CTXT = new QDeclarativeContextData;
            CTXT->isInternal = true;
            CTXT->url = COMP->url;
            CTXT->imports = COMP->importCache;
            CTXT->imports->addref();
            CTXT->setParent(parentCtxt);
            if (instr.contextCache != -1) 
                CTXT->setIdPropertyData(COMP->contextCaches.at(instr.contextCache));
            if (instr.compiledBinding != -1) {
                const char *v4data = DATAS.at(instr.compiledBinding).constData();
                CTXT->v4bindings = new QV4Bindings(v4data, CTXT, COMP);
            }
            if (states.count() == 1) {
                rootContext = CTXT;
                rootContext->activeVMEData = data;
            }
            if (states.count() == 1 && !creationContext.isNull()) {
                // A component that is logically created within another component instance shares the 
                // same instances of script imports.  For example:
                //
                //     import QtQuick 1.0
                //     import "test.js" as Test
                //     ListView {
                //         model: Test.getModel()
                //         delegate: Component {
                //             Text { text: Test.getValue(index); }
                //         }
                //     }
                //
                // Has the same "Test" instance.  To implement this, we simply copy the v8 handles into
                // the inner context.  We have to create a fresh persistent handle for each to prevent 
                // double dispose.  It is possible we could do this more efficiently using some form of
                // referencing instead.
                CTXT->importedScripts = creationContext->importedScripts;
                for (int ii = 0; ii < CTXT->importedScripts.count(); ++ii)
                    CTXT->importedScripts[ii] = qPersistentNew<v8::Object>(CTXT->importedScripts[ii]);
            }
        QML_END_INSTR(Init)

        QML_BEGIN_INSTR(DeferInit)
        QML_END_INSTR(DeferInit)

        QML_BEGIN_INSTR(Done)
            states.pop();

            if (states.isEmpty())
                goto normalExit;
        QML_END_INSTR(Done)

        QML_BEGIN_INSTR(CreateQMLObject)
            const QDeclarativeCompiledData::TypeReference &type = TYPES.at(instr.type);
            Q_ASSERT(type.component);

            states.push(State());

            State *cState = &states[states.count() - 2];
            State *nState = &states[states.count() - 1];

            nState->context = cState->context;
            nState->compiledData = type.component;
            nState->instructionStream = type.component->bytecode.constData();

            if (instr.bindingBits != -1) {
                const QByteArray &bits = cState->compiledData->datas.at(instr.bindingBits);
                nState->bindingSkipList = QBitField((const quint32*)bits.constData(),
                                                    bits.size() * 8);
            }
            if (instr.isRoot)
                nState->bindingSkipList = nState->bindingSkipList.united(cState->bindingSkipList);

            // As the state in the state stack changed, execution will continue in the new program.
        QML_END_INSTR(CreateQMLObject)

        QML_BEGIN_INSTR(CompleteQMLObject)
            QObject *o = objects.top();

            QDeclarativeData *ddata = QDeclarativeData::get(o);
            Q_ASSERT(ddata);

            if (instr.isRoot) {
                if (ddata->context) {
                    Q_ASSERT(ddata->context != CTXT);
                    Q_ASSERT(ddata->outerContext);
                    Q_ASSERT(ddata->outerContext != CTXT);
                    QDeclarativeContextData *c = ddata->context;
                    while (c->linkedContext) c = c->linkedContext;
                    c->linkedContext = CTXT;
                } else {
                    CTXT->addObject(o);
                }

                ddata->ownContext = true;
            } else if (!ddata->context) {
                CTXT->addObject(o);
            }

            ddata->setImplicitDestructible();
            ddata->outerContext = CTXT;
            ddata->lineNumber = instr.line;
            ddata->columnNumber = instr.column;
        QML_END_INSTR(CompleteQMLObject)

        QML_BEGIN_INSTR(CreateCppObject)
            const QDeclarativeCompiledData::TypeReference &type = TYPES.at(instr.type);
            Q_ASSERT(type.type);

            QObject *o = 0;
            void *memory = 0;
            type.type->create(&o, &memory, sizeof(QDeclarativeData));
            QDeclarativeData *ddata = new (memory) QDeclarativeData;
            ddata->ownMemory = false;
            QObjectPrivate::get(o)->declarativeData = ddata;

            if (type.typePropertyCache && !ddata->propertyCache) {
                ddata->propertyCache = type.typePropertyCache;
                ddata->propertyCache->addref();
            }

            if (!o) 
                VME_EXCEPTION(tr("Unable to create object of type %1").arg(type.className), instr.line);

            if (instr.isRoot) {
                if (ddata->context) {
                    Q_ASSERT(ddata->context != CTXT);
                    Q_ASSERT(ddata->outerContext);
                    Q_ASSERT(ddata->outerContext != CTXT);
                    QDeclarativeContextData *c = ddata->context;
                    while (c->linkedContext) c = c->linkedContext;
                    c->linkedContext = CTXT;
                } else {
                    CTXT->addObject(o);
                }

                ddata->ownContext = true;
            } else if (!ddata->context) {
                CTXT->addObject(o);
            }

            ddata->setImplicitDestructible();
            ddata->outerContext = CTXT;
            ddata->lineNumber = instr.line;
            ddata->columnNumber = instr.column;

            if (instr.data != -1) {
                QDeclarativeCustomParser *customParser =
                    TYPES.at(instr.type).type->customParser();
                customParser->setCustomData(o, DATAS.at(instr.data));
            }
            if (!objects.isEmpty()) {
                QObject *parent = objects.top();
#if 0 // ### refactor
                if (o->isWidgetType() && parent->isWidgetType()) 
                    static_cast<QWidget*>(o)->setParent(static_cast<QWidget*>(parent));
                else 
#endif
                    QDeclarative_setParent_noEvent(o, parent);
            }
            objects.push(o);
        QML_END_INSTR(CreateCppObject)

        QML_BEGIN_INSTR(CreateSimpleObject)
            QObject *o = (QObject *)operator new(instr.typeSize + sizeof(QDeclarativeData));   
            ::memset(o, 0, instr.typeSize + sizeof(QDeclarativeData));
            instr.create(o);

            QDeclarativeData *ddata = (QDeclarativeData *)(((const char *)o) + instr.typeSize);
            const QDeclarativeCompiledData::TypeReference &ref = TYPES.at(instr.type);
            if (!ddata->propertyCache && ref.typePropertyCache) {
                ddata->propertyCache = ref.typePropertyCache;
                ddata->propertyCache->addref();
            }
            ddata->lineNumber = instr.line;
            ddata->columnNumber = instr.column;

            QObjectPrivate::get(o)->declarativeData = ddata;                                                      
            ddata->context = ddata->outerContext = CTXT;
            ddata->nextContextObject = CTXT->contextObjects; 
            if (ddata->nextContextObject) 
                ddata->nextContextObject->prevContextObject = &ddata->nextContextObject; 
            ddata->prevContextObject = &CTXT->contextObjects; 
            CTXT->contextObjects = ddata; 

            QObject *parent = objects.top();                                                                    
            QDeclarative_setParent_noEvent(o, parent);                                                        

            objects.push(o);
        QML_END_INSTR(CreateSimpleObject)

        QML_BEGIN_INSTR(SetId)
            QObject *target = objects.top();
            CTXT->setIdProperty(instr.index, target);
        QML_END_INSTR(SetId)

        QML_BEGIN_INSTR(SetDefault)
            CTXT->contextObject = objects.top();
        QML_END_INSTR(SetDefault)

        QML_BEGIN_INSTR(CreateComponent)
            QDeclarativeComponent *qcomp = 
                new QDeclarativeComponent(CTXT->engine, COMP, INSTRUCTIONSTREAM - COMP->bytecode.constData(),
                                          objects.isEmpty() ? 0 : objects.top());

            QDeclarativeData *ddata = QDeclarativeData::get(qcomp, true);
            Q_ASSERT(ddata);

            CTXT->addObject(qcomp);

            if (instr.isRoot)
                ddata->ownContext = true;

            ddata->setImplicitDestructible();
            ddata->outerContext = CTXT;
            ddata->lineNumber = instr.line;
            ddata->columnNumber = instr.column;

            QDeclarativeComponentPrivate::get(qcomp)->creationContext = CTXT;

            objects.push(qcomp);
            INSTRUCTIONSTREAM += instr.count;
        QML_END_INSTR(CreateComponent)

        QML_BEGIN_INSTR(StoreMetaObject)
            QObject *target = objects.top();

            QMetaObject mo;
            const QByteArray &metadata = DATAS.at(instr.data);
            QFastMetaBuilder::fromData(&mo, 0, metadata);

            const QDeclarativeVMEMetaData *data = 
                (const QDeclarativeVMEMetaData *)DATAS.at(instr.aliasData).constData();

            (void)new QDeclarativeVMEMetaObject(target, &mo, data, COMP);

            if (instr.propertyCache != -1) {
                QDeclarativeData *ddata = QDeclarativeData::get(target, true);
                if (ddata->propertyCache) ddata->propertyCache->release();
                ddata->propertyCache = PROPERTYCACHES.at(instr.propertyCache);
                ddata->propertyCache->addref();
            }
        QML_END_INSTR(StoreMetaObject)

        QML_BEGIN_INSTR(AssignCustomType)
            QObject *target = objects.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            const QString &primitive = PRIMITIVES.at(instr.primitive);
            int type = instr.type;
            QDeclarativeMetaType::StringConverter converter = QDeclarativeMetaType::customStringConverter(type);
            QVariant v = (*converter)(primitive);

            QMetaProperty prop = 
                    target->metaObject()->property(instr.propertyIndex);
            if (v.isNull() || ((int)prop.type() != type && prop.userType() != type)) 
                VME_EXCEPTION(tr("Cannot assign value %1 to property %2").arg(primitive).arg(QString::fromUtf8(prop.name())), instr.line);

            void *a[] = { (void *)v.data(), 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(AssignCustomType)

        QML_BEGIN_INSTR(AssignSignalObject)
            // XXX optimize

            QObject *assign = objects.pop();
            QObject *target = objects.top();
            int sigIdx = instr.signal;
            const QString &pr = PRIMITIVES.at(sigIdx);

            QDeclarativeProperty prop(target, pr);
            if (prop.type() & QDeclarativeProperty::SignalProperty) {

                QMetaMethod method = QDeclarativeMetaType::defaultMethod(assign);
                if (method.signature() == 0)
                    VME_EXCEPTION(tr("Cannot assign object type %1 with no default method").arg(QString::fromLatin1(assign->metaObject()->className())), instr.line);

                if (!QMetaObject::checkConnectArgs(prop.method().signature(), method.signature()))
                    VME_EXCEPTION(tr("Cannot connect mismatched signal/slot %1 %vs. %2").arg(QString::fromLatin1(method.signature())).arg(QString::fromLatin1(prop.method().signature())), instr.line);

                QDeclarativePropertyPrivate::connect(target, prop.index(), assign, method.methodIndex());

            } else {
                VME_EXCEPTION(tr("Cannot assign an object to signal property %1").arg(pr), instr.line);
            }


        QML_END_INSTR(AssignSignalObject)

        QML_BEGIN_INSTR(StoreSignal)
            QObject *target = objects.top();
            QObject *context = objects.at(objects.count() - 1 - instr.context);

            QMetaMethod signal = target->metaObject()->method(instr.signalIndex);

            QDeclarativeBoundSignal *bs = new QDeclarativeBoundSignal(target, signal, target);
            QDeclarativeExpression *expr = 
                new QDeclarativeExpression(CTXT, context, PRIMITIVES.at(instr.value), true, COMP->name, instr.line, *new QDeclarativeExpressionPrivate);
            bs->setExpression(expr);
        QML_END_INSTR(StoreSignal)

        QML_BEGIN_INSTR(StoreImportedScript)
            CTXT->importedScripts << run(CTXT, SCRIPTS.at(instr.value));
        QML_END_INSTR(StoreImportedScript)

        QML_BEGIN_INSTR(StoreScriptString)
            QObject *target = objects.top();
            QObject *scope = objects.at(objects.count() - 1 - instr.scope);
            QDeclarativeScriptString ss;
            ss.setContext(CTXT->asQDeclarativeContext());
            ss.setScopeObject(scope);
            ss.setScript(PRIMITIVES.at(instr.value));
            ss.d.data()->bindingId = instr.bindingId;
            ss.d.data()->lineNumber = instr.line;

            void *a[] = { &ss, 0, &status, &flags };
            QMetaObject::metacall(target, QMetaObject::WriteProperty, 
                                  instr.propertyIndex, a);
        QML_END_INSTR(StoreScriptString)

        QML_BEGIN_INSTR(BeginObject)
            QObject *target = objects.top();
            QDeclarativeParserStatus *status = reinterpret_cast<QDeclarativeParserStatus *>(reinterpret_cast<char *>(target) + instr.castValue);
            parserStatus.push(status);
#ifdef QML_ENABLE_TRACE
            Q_ASSERT(QObjectPrivate::get(target)->declarativeData);
            parserStatusData.push(static_cast<QDeclarativeData *>(QObjectPrivate::get(target)->declarativeData));
#endif
            status->d = &parserStatus.top();

            status->classBegin();
        QML_END_INSTR(BeginObject)

        QML_BEGIN_INSTR(InitV8Bindings)
            CTXT->v8bindings = new QV8Bindings(PRIMITIVES.at(instr.program), instr.programIndex, 
                                                       instr.line, COMP, CTXT);
        QML_END_INSTR(InitV8Bindings)

        QML_BEGIN_INSTR(StoreBinding)
            QObject *target = 
                objects.at(objects.count() - 1 - instr.owner);
            QObject *context = 
                objects.at(objects.count() - 1 - instr.context);

            if (instr.isRoot && BINDINGSKIPLIST.testBit(instr.property.coreIndex))
                QML_NEXT_INSTR(StoreBinding);

            QDeclarativeBinding *bind = new QDeclarativeBinding(PRIMITIVES.at(instr.value), true, 
                                                                context, CTXT, COMP->name, instr.line,
                                                                instr.column);
            bindValues.push(bind);
            bind->m_mePtr = &bindValues.top();
            bind->setTarget(target, instr.property, CTXT);

            bind->addToObject(target, QDeclarativePropertyPrivate::bindingIndex(instr.property));
        QML_END_INSTR(StoreBinding)

        QML_BEGIN_INSTR(StoreBindingOnAlias)
            QObject *target = 
                objects.at(objects.count() - 1 - instr.owner);
            QObject *context = 
                objects.at(objects.count() - 1 - instr.context);

            if (instr.isRoot && BINDINGSKIPLIST.testBit(instr.property.coreIndex))
                QML_NEXT_INSTR(StoreBindingOnAlias);

            QDeclarativeBinding *bind = new QDeclarativeBinding(PRIMITIVES.at(instr.value), true,
                                                                context, CTXT, COMP->name, instr.line,
                                                                instr.column);
            bindValues.push(bind);
            bind->m_mePtr = &bindValues.top();
            bind->setTarget(target, instr.property, CTXT);

            QDeclarativeAbstractBinding *old =
                QDeclarativePropertyPrivate::setBindingNoEnable(target, instr.property.coreIndex,
                                                                instr.property.getValueTypeCoreIndex(),
                                                                bind);
            if (old) { old->destroy(); }
        QML_END_INSTR(StoreBindingOnAlias)

        QML_BEGIN_INSTR(StoreV4Binding)
            QObject *target = 
                objects.at(objects.count() - 1 - instr.owner);
            QObject *scope = 
                objects.at(objects.count() - 1 - instr.context);

            int property = instr.property;
            if (instr.isRoot && BINDINGSKIPLIST.testBit(property & 0xFFFF))
                QML_NEXT_INSTR(StoreV4Binding);

            QDeclarativeAbstractBinding *binding = 
                CTXT->v4bindings->configBinding(instr.value, target, scope, property,
                                                instr.line, instr.column);
            bindValues.push(binding);
            binding->m_mePtr = &bindValues.top();
            binding->addToObject(target, property);
        QML_END_INSTR(StoreV4Binding)

        QML_BEGIN_INSTR(StoreV8Binding)
            QObject *target = 
                objects.at(objects.count() - 1 - instr.owner);
            QObject *scope = 
                objects.at(objects.count() - 1 - instr.context);

            if (instr.isRoot && BINDINGSKIPLIST.testBit(instr.property.coreIndex))
                QML_NEXT_INSTR(StoreV8Binding);

            QDeclarativeAbstractBinding *binding = 
                CTXT->v8bindings->configBinding(instr.value, target, scope, 
                                                instr.property, instr.line,
                                                instr.column);
            bindValues.push(binding);
            binding->m_mePtr = &bindValues.top();
            binding->addToObject(target, QDeclarativePropertyPrivate::bindingIndex(instr.property));
        QML_END_INSTR(StoreV8Binding)

        QML_BEGIN_INSTR(StoreValueSource)
            QObject *obj = objects.pop();
            QDeclarativePropertyValueSource *vs = reinterpret_cast<QDeclarativePropertyValueSource *>(reinterpret_cast<char *>(obj) + instr.castValue);
            QObject *target = objects.at(objects.count() - 1 - instr.owner);

            obj->setParent(target);
            vs->setTarget(QDeclarativePropertyPrivate::restore(target, instr.property, CTXT));
        QML_END_INSTR(StoreValueSource)

        QML_BEGIN_INSTR(StoreValueInterceptor)
            QObject *obj = objects.pop();
            QDeclarativePropertyValueInterceptor *vi = reinterpret_cast<QDeclarativePropertyValueInterceptor *>(reinterpret_cast<char *>(obj) + instr.castValue);
            QObject *target = objects.at(objects.count() - 1 - instr.owner);
            QDeclarativeProperty prop = 
                QDeclarativePropertyPrivate::restore(target, instr.property, CTXT);
            obj->setParent(target);
            vi->setTarget(prop);
            QDeclarativeVMEMetaObject *mo = static_cast<QDeclarativeVMEMetaObject *>((QMetaObject*)target->metaObject());
            mo->registerInterceptor(prop.index(), QDeclarativePropertyPrivate::valueTypeCoreIndex(prop), vi);
        QML_END_INSTR(StoreValueInterceptor)

        QML_BEGIN_INSTR(StoreObjectQList)
            QObject *assign = objects.pop();

            const List &list = lists.top();
            list.qListProperty.append((QDeclarativeListProperty<void>*)&list.qListProperty, assign);
        QML_END_INSTR(StoreObjectQList)

        QML_BEGIN_INSTR(AssignObjectList)
            // This is only used for assigning interfaces
            QObject *assign = objects.pop();
            const List &list = lists.top();

            int type = list.type;

            void *ptr = 0;

            const char *iid = QDeclarativeMetaType::interfaceIId(type);
            if (iid) 
                ptr = assign->qt_metacast(iid);
            if (!ptr) 
                VME_EXCEPTION(tr("Cannot assign object to list"), instr.line);


            list.qListProperty.append((QDeclarativeListProperty<void>*)&list.qListProperty, ptr);
        QML_END_INSTR(AssignObjectList)

        QML_BEGIN_INSTR(StoreInterface)
            QObject *assign = objects.pop();
            QObject *target = objects.top();
            CLEAN_PROPERTY(target, instr.propertyIndex);

            int coreIdx = instr.propertyIndex;
            QMetaProperty prop = target->metaObject()->property(coreIdx);
            int t = prop.userType();
            const char *iid = QDeclarativeMetaType::interfaceIId(t);
            bool ok = false;
            if (iid) {
                void *ptr = assign->qt_metacast(iid);
                if (ptr) {
                    void *a[] = { &ptr, 0, &status, &flags };
                    QMetaObject::metacall(target, 
                                          QMetaObject::WriteProperty,
                                          coreIdx, a);
                    ok = true;
                }
            } 

            if (!ok) 
                VME_EXCEPTION(tr("Cannot assign object to interface property"), instr.line);
        QML_END_INSTR(StoreInterface)
            
        QML_BEGIN_INSTR(FetchAttached)
            QObject *target = objects.top();

            QObject *qmlObject = qmlAttachedPropertiesObjectById(instr.id, target);

            if (!qmlObject)
                VME_EXCEPTION(tr("Unable to create attached object"), instr.line);

            objects.push(qmlObject);
        QML_END_INSTR(FetchAttached)

        QML_BEGIN_INSTR(FetchQList)
            QObject *target = objects.top();

            lists.push(List(instr.type));

            void *a[1];
            a[0] = (void *)&(lists.top().qListProperty);
            QMetaObject::metacall(target, QMetaObject::ReadProperty, 
                                  instr.property, a);
        QML_END_INSTR(FetchQList)

        QML_BEGIN_INSTR(FetchObject)
            QObject *target = objects.top();

            QObject *obj = 0;
            // NOTE: This assumes a cast to QObject does not alter the 
            // object pointer
            void *a[1];
            a[0] = &obj;
            QMetaObject::metacall(target, QMetaObject::ReadProperty, 
                                  instr.property, a);

            if (!obj)
                VME_EXCEPTION(tr("Cannot set properties on %1 as it is null").arg(QString::fromUtf8(target->metaObject()->property(instr.property).name())), instr.line);

            objects.push(obj);
        QML_END_INSTR(FetchObject)

        QML_BEGIN_INSTR(PopQList)
            lists.pop();
        QML_END_INSTR(PopQList)

        QML_BEGIN_INSTR(Defer)
            if (instr.deferCount) {
                QObject *target = objects.top();
                QDeclarativeData *data = 
                    QDeclarativeData::get(target, true);
                COMP->addref();
                data->deferredComponent = COMP;
                data->deferredIdx = INSTRUCTIONSTREAM - COMP->bytecode.constData();
                INSTRUCTIONSTREAM += instr.deferCount;
            }
        QML_END_INSTR(Defer)

        QML_BEGIN_INSTR(PopFetchedObject)
            objects.pop();
        QML_END_INSTR(PopFetchedObject)

        QML_BEGIN_INSTR(FetchValueType)
            QObject *target = objects.top();

            if (instr.bindingSkipList != 0) {
                // Possibly need to clear bindings
                QDeclarativeData *targetData = QDeclarativeData::get(target);
                if (targetData) {
                    QDeclarativeAbstractBinding *binding = 
                        QDeclarativePropertyPrivate::binding(target, instr.property, -1);

                    if (binding && binding->bindingType() != QDeclarativeAbstractBinding::ValueTypeProxy) {
                        QDeclarativePropertyPrivate::setBinding(target, instr.property, -1, 0);
                        binding->destroy();
                    } else if (binding) {
                        QDeclarativeValueTypeProxyBinding *proxy = 
                            static_cast<QDeclarativeValueTypeProxyBinding *>(binding);
                        proxy->removeBindings(instr.bindingSkipList);
                    }
                }
            }

            QDeclarativeValueType *valueHandler = ep->valueTypes[instr.type];
            valueHandler->read(target, instr.property);
            objects.push(valueHandler);
        QML_END_INSTR(FetchValueType)

        QML_BEGIN_INSTR(PopValueType)
            QDeclarativeValueType *valueHandler = 
                static_cast<QDeclarativeValueType *>(objects.pop());
            QObject *target = objects.top();
            valueHandler->write(target, instr.property, QDeclarativePropertyPrivate::BypassInterceptor);
        QML_END_INSTR(PopValueType)

#ifdef QML_THREADED_VME_INTERPRETER
    // nothing to do
#else
        default:
            qFatal("QDeclarativeCompiledData: Internal error - unknown instruction %d", genericInstr->common.instructionType);
            break;
        }
    }
#endif

exceptionExit:
    Q_ASSERT(!states.isEmpty());
    Q_ASSERT(!errors->isEmpty());

    reset();

    return 0;

normalExit:
    Q_ASSERT(objects.count() == 1);

    QObject *rv = objects.top();

    objects.deallocate();
    lists.deallocate();
    states.clear();

    return rv;
}

void QDeclarativeVME::reset()
{
    Q_ASSERT(!states.isEmpty() || objects.isEmpty());

    QRecursionWatcher<QDeclarativeVME, &QDeclarativeVME::recursion> watcher(this);

    if (!objects.isEmpty() && !(states.at(0).flags & State::Deferred))
        delete objects.at(0); 
    
    if (!rootContext.isNull()) 
        rootContext->activeVMEData = 0;

    // Remove the QDeclarativeParserStatus and QDeclarativeAbstractBinding back pointers
    blank(parserStatus);
    blank(bindValues);

    while (componentAttached) {
        QDeclarativeComponentAttached *a = componentAttached;
        a->rem();
    }
    
    engine = 0;
    objects.deallocate();
    lists.deallocate();
    bindValues.deallocate();
    parserStatus.deallocate();
#ifdef QML_ENABLE_TRACE
    parserStatusData.deallocate();
#endif
    finalizeCallbacks.clear();
    states.clear();
    rootContext = 0;
    creationContext = 0;
}

// Must be called with a handle scope and context
void QDeclarativeScriptData::initialize(QDeclarativeEngine *engine)
{
    Q_ASSERT(m_program.IsEmpty());
    Q_ASSERT(engine);
    Q_ASSERT(!hasEngine());

    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);
    QV8Engine *v8engine = ep->v8engine();

    // If compilation throws an error, a surrounding v8::TryCatch will record it.
    v8::Local<v8::Script> program = v8engine->qmlModeCompile(m_programSource, url.toString(), 1);
    if (program.IsEmpty())
        return;

    m_program = qPersistentNew<v8::Script>(program);

    addToEngine(engine);

    addref();
}

v8::Persistent<v8::Object> QDeclarativeVME::run(QDeclarativeContextData *parentCtxt, QDeclarativeScriptData *script)
{
    if (script->m_loaded)
        return qPersistentNew<v8::Object>(script->m_value);

    Q_ASSERT(parentCtxt && parentCtxt->engine);
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(parentCtxt->engine);
    QV8Engine *v8engine = ep->v8engine();

    bool shared = script->pragmas & QDeclarativeScript::Object::ScriptBlock::Shared;

    QDeclarativeContextData *effectiveCtxt = parentCtxt;
    if (shared)
        effectiveCtxt = 0;

    // Create the script context if required
    QDeclarativeContextData *ctxt = new QDeclarativeContextData;
    ctxt->isInternal = true;
    ctxt->isJSContext = true;
    if (shared)
        ctxt->isPragmaLibraryContext = true;
    else
        ctxt->isPragmaLibraryContext = parentCtxt->isPragmaLibraryContext;
    ctxt->url = script->url;

    // For backward compatibility, if there are no imports, we need to use the
    // imports from the parent context.  See QTBUG-17518.
    if (!script->importCache->isEmpty()) {
        ctxt->imports = script->importCache;
    } else if (effectiveCtxt) {
        ctxt->imports = effectiveCtxt->imports;
        ctxt->importedScripts = effectiveCtxt->importedScripts;
        for (int ii = 0; ii < ctxt->importedScripts.count(); ++ii)
            ctxt->importedScripts[ii] = qPersistentNew<v8::Object>(ctxt->importedScripts[ii]);
    }

    if (ctxt->imports) {
        ctxt->imports->addref();
    }

    if (effectiveCtxt) {
        ctxt->setParent(effectiveCtxt, true);
    } else {
        ctxt->engine = parentCtxt->engine; // Fix for QTBUG-21620
    }

    for (int ii = 0; ii < script->scripts.count(); ++ii) {
        ctxt->importedScripts << run(ctxt, script->scripts.at(ii)->scriptData());
    }

    v8::HandleScope handle_scope;
    v8::Context::Scope scope(v8engine->context());

    v8::TryCatch try_catch;
    if (!script->isInitialized())
        script->initialize(parentCtxt->engine);

    v8::Local<v8::Object> qmlglobal = v8engine->qmlScope(ctxt, 0);

    if (!script->m_program.IsEmpty()) {
        script->m_program->Run(qmlglobal);
    } else {
        // Compilation failed.
        Q_ASSERT(try_catch.HasCaught());
    }

    v8::Persistent<v8::Object> rv;
    
    if (try_catch.HasCaught()) {
        v8::Local<v8::Message> message = try_catch.Message();
        if (!message.IsEmpty()) {
            QDeclarativeError error;
            QDeclarativeExpressionPrivate::exceptionToError(message, error);
            ep->warning(error);
        }
    } 

    rv = qPersistentNew<v8::Object>(qmlglobal);
    if (shared) {
        script->m_value = qPersistentNew<v8::Object>(qmlglobal);
        script->m_loaded = true;
    }

    return rv;
}

#ifdef QML_THREADED_VME_INTERPRETER
void **QDeclarativeVME::instructionJumpTable()
{
    static void **jumpTable = 0;
    if (!jumpTable) {
        QDeclarativeVME dummy;
        QDeclarativeVME::Interrupt i;
        dummy.run(0, i, &jumpTable);
    }
    return jumpTable;
}
#endif

QDeclarativeContextData *QDeclarativeVME::complete(const Interrupt &interrupt)
{
    Q_ASSERT(engine ||
             (bindValues.isEmpty() &&
              parserStatus.isEmpty() &&
              componentAttached == 0 &&
              rootContext.isNull() &&
              finalizeCallbacks.isEmpty()));

    if (!engine)
        return 0;

    QDeclarativeTrace trace("VME Complete");
#ifdef QML_ENABLE_TRACE
    trace.addDetail("URL", rootComponent->url);
#endif

    ActiveVMERestorer restore(this, QDeclarativeEnginePrivate::get(engine));
    QRecursionWatcher<QDeclarativeVME, &QDeclarativeVME::recursion> watcher(this);

    {
    QDeclarativeTrace trace("VME Binding Enable");
    trace.event("begin binding eval");
    while (!bindValues.isEmpty()) {
        QDeclarativeAbstractBinding *b = bindValues.pop();

        if(b) {
            b->m_mePtr = 0;
            b->setEnabled(true, QDeclarativePropertyPrivate::BypassInterceptor | 
                                QDeclarativePropertyPrivate::DontRemoveBinding);
        }

        if (watcher.hasRecursed() || interrupt.shouldInterrupt())
            return 0;
    }
    bindValues.deallocate();
    }

    {
    QDeclarativeTrace trace("VME Component Complete");
    while (!parserStatus.isEmpty()) {
        QDeclarativeParserStatus *status = parserStatus.pop();
#ifdef QML_ENABLE_TRACE
        QDeclarativeData *data = parserStatusData.pop();
#endif

        if (status && status->d) {
            status->d = 0;
#ifdef QML_ENABLE_TRACE
            QDeclarativeTrace trace("Component complete");
            trace.addDetail("URL", data->outerContext->url);
            trace.addDetail("Line", data->lineNumber);
#endif
            status->componentComplete();
        }
        
        if (watcher.hasRecursed() || interrupt.shouldInterrupt())
            return 0;
    }
    parserStatus.deallocate();
    }

    {
    QDeclarativeTrace trace("VME Finalize Callbacks");
    for (int ii = 0; ii < finalizeCallbacks.count(); ++ii) {
        QDeclarativeEnginePrivate::FinalizeCallback callback = finalizeCallbacks.at(ii);
        QObject *obj = callback.first;
        if (obj) {
            void *args[] = { 0 };
            QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod, callback.second, args);
        }
        if (watcher.hasRecursed())
            return 0;
    }
    finalizeCallbacks.clear();
    }

    {
    QDeclarativeTrace trace("VME Component.onCompleted Callbacks");
    while (componentAttached) {
        QDeclarativeComponentAttached *a = componentAttached;
        a->rem();
        QDeclarativeData *d = QDeclarativeData::get(a->parent());
        Q_ASSERT(d);
        Q_ASSERT(d->context);
        a->add(&d->context->componentAttached);
        emit a->completed();

        if (watcher.hasRecursed() || interrupt.shouldInterrupt())
            return 0;
    }
    }

    QDeclarativeContextData *rv = rootContext;

    reset();

    if (rv) rv->activeVMEData = data;

    return rv;
}

void QDeclarativeVME::blank(QFiniteStack<QDeclarativeAbstractBinding *> &bs)
{
    for (int ii = 0; ii < bs.count(); ++ii) {
        QDeclarativeAbstractBinding *b = bs.at(ii);
        if (b) b->m_mePtr = 0;
    }
}

void QDeclarativeVME::blank(QFiniteStack<QDeclarativeParserStatus *> &pss)
{
    for (int ii = 0; ii < pss.count(); ++ii) {
        QDeclarativeParserStatus *ps = pss.at(ii);
        if(ps) ps->d = 0;
    }
}

QDeclarativeVMEGuard::QDeclarativeVMEGuard()
: m_objectCount(0), m_objects(0), m_contextCount(0), m_contexts(0)
{
}

QDeclarativeVMEGuard::~QDeclarativeVMEGuard()
{
    clear();
}

void QDeclarativeVMEGuard::guard(QDeclarativeVME *vme)
{
    clear();
    
    m_objectCount = vme->objects.count();
    m_objects = new QDeclarativeGuard<QObject>[m_objectCount];
    for (int ii = 0; ii < m_objectCount; ++ii)
        m_objects[ii] = vme->objects[ii];

    m_contextCount = (vme->rootContext.isNull()?0:1) + vme->states.count();
    m_contexts = new QDeclarativeGuardedContextData[m_contextCount];
    for (int ii = 0; ii < vme->states.count(); ++ii) 
        m_contexts[ii] = vme->states.at(ii).context;
    if (!vme->rootContext.isNull())
        m_contexts[m_contextCount - 1] = vme->rootContext.contextData();
}

void QDeclarativeVMEGuard::clear()
{
    delete [] m_objects;
    delete [] m_contexts;

    m_objectCount = 0;
    m_objects = 0;
    m_contextCount = 0;
    m_contexts = 0;
}

bool QDeclarativeVMEGuard::isOK() const
{
    for (int ii = 0; ii < m_objectCount; ++ii)
        if (m_objects[ii].isNull())
            return false;

    for (int ii = 0; ii < m_contextCount; ++ii)
        if (m_contexts[ii].isNull())
            return false;

    return true;
}

QT_END_NAMESPACE
