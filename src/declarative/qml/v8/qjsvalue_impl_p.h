/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtScript module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-ONLY$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

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

#ifndef QJSVALUE_IMPL_P_H
#define QJSVALUE_IMPL_P_H

#include "qjsconverter_p.h"
#include "qjsvalue_p.h"
#include "qv8engine_p.h"
#include "qscriptisolate_p.h"

QT_BEGIN_NAMESPACE

// This template is used indirectly by the Q_GLOBAL_STATIC macro below
template<>
class QGlobalStaticDeleter<QJSValuePrivate>
{
public:
    QGlobalStatic<QJSValuePrivate> &globalStatic;
    QGlobalStaticDeleter(QGlobalStatic<QJSValuePrivate> &_globalStatic)
        : globalStatic(_globalStatic)
    {
        globalStatic.pointer.load()->ref.ref();
    }

    inline ~QGlobalStaticDeleter()
    {
        if (!globalStatic.pointer.load()->ref.deref()) { // Logic copy & paste from SharedDataPointer
            delete globalStatic.pointer.load();
        }
        globalStatic.pointer.store(0);
        globalStatic.destroyed = true;
    }
};

Q_GLOBAL_STATIC(QJSValuePrivate, InvalidValue)

QJSValuePrivate* QJSValuePrivate::get(const QJSValue& q) { Q_ASSERT(q.d_ptr.data()); return q.d_ptr.data(); }

QJSValue QJSValuePrivate::get(const QJSValuePrivate* d)
{
    Q_ASSERT(d);
    return QJSValue(const_cast<QJSValuePrivate*>(d));
}

QJSValue QJSValuePrivate::get(QScriptPassPointer<QJSValuePrivate> d)
{
    Q_ASSERT(d);
    return QJSValue(d);
}

QJSValue QJSValuePrivate::get(QJSValuePrivate* d)
{
    Q_ASSERT(d);
    return QJSValue(d);
}

QJSValuePrivate::QJSValuePrivate()
    : m_engine(0), m_state(Invalid)
{
}

QJSValuePrivate::QJSValuePrivate(bool value)
    : m_engine(0), m_state(CBool), u(value)
{
}

QJSValuePrivate::QJSValuePrivate(int value)
    : m_engine(0), m_state(CNumber), u(value)
{
}

QJSValuePrivate::QJSValuePrivate(uint value)
    : m_engine(0), m_state(CNumber), u(value)
{
}

QJSValuePrivate::QJSValuePrivate(double value)
    : m_engine(0), m_state(CNumber), u(value)
{
}

QJSValuePrivate::QJSValuePrivate(const QString& value)
    : m_engine(0), m_state(CString), u(new QString(value))
{
}

QJSValuePrivate::QJSValuePrivate(QJSValue::SpecialValue value)
    : m_engine(0), m_state(value == QJSValue::NullValue ? CNull : CUndefined)
{
}

QJSValuePrivate::QJSValuePrivate(QV8Engine* engine, bool value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    m_value = v8::Persistent<v8::Value>::New(m_engine->makeJSValue(value));
    m_engine->registerValue(this);
}

QJSValuePrivate::QJSValuePrivate(QV8Engine* engine, int value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    m_value = v8::Persistent<v8::Value>::New(m_engine->makeJSValue(value));
    m_engine->registerValue(this);
}

QJSValuePrivate::QJSValuePrivate(QV8Engine* engine, uint value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    m_value = v8::Persistent<v8::Value>::New(m_engine->makeJSValue(value));
    m_engine->registerValue(this);
}

QJSValuePrivate::QJSValuePrivate(QV8Engine* engine, double value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    m_value = v8::Persistent<v8::Value>::New(m_engine->makeJSValue(value));
    m_engine->registerValue(this);
}

QJSValuePrivate::QJSValuePrivate(QV8Engine* engine, const QString& value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    m_value = v8::Persistent<v8::Value>::New(m_engine->makeJSValue(value));
    m_engine->registerValue(this);
}

QJSValuePrivate::QJSValuePrivate(QV8Engine* engine, QJSValue::SpecialValue value)
    : m_engine(engine), m_state(JSValue)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    m_value = v8::Persistent<v8::Value>::New(m_engine->makeJSValue(value));
    m_engine->registerValue(this);
}

QJSValuePrivate::QJSValuePrivate(QV8Engine *engine, v8::Handle<v8::Value> value)
    : m_engine(engine), m_state(JSValue), m_value(v8::Persistent<v8::Value>::New(value))
{
    Q_ASSERT(engine);
    // It shouldn't happen, v8 shows errors by returning an empty handler. This is important debug
    // information and it can't be simply ignored.
    Q_ASSERT(!value.IsEmpty());
    m_engine->registerValue(this);
}

QJSValuePrivate::~QJSValuePrivate()
{
    if (isJSBased()) {
        m_engine->unregisterValue(this);
        QScriptIsolate api(m_engine);
        m_value.Dispose();
    } else if (isStringBased()) {
        delete u.m_string;
    }
}

bool QJSValuePrivate::toBool() const
{
    switch (m_state) {
    case JSValue:
        {
            v8::HandleScope scope;
            return m_value->ToBoolean()->Value();
        }
    case CNumber:
        return !(qIsNaN(u.m_number) || !u.m_number);
    case CBool:
        return u.m_bool;
    case Invalid:
    case CNull:
    case CUndefined:
        return false;
    case CString:
        return u.m_string->length();
    }

    Q_ASSERT_X(false, "toBool()", "Not all states are included in the previous switch statement.");
    return false; // Avoid compiler warning.
}

double QJSValuePrivate::toNumber() const
{
    switch (m_state) {
    case JSValue:
    {
        v8::HandleScope scope;
        return m_value->ToNumber()->Value();
    }
    case CNumber:
        return u.m_number;
    case CBool:
        return u.m_bool ? 1 : 0;
    case CNull:
    case Invalid:
        return 0;
    case CUndefined:
        return qQNaN();
    case CString:
        bool ok;
        double result = u.m_string->toDouble(&ok);
        if (ok)
            return result;
        result = u.m_string->toInt(&ok, 0); // Try other bases.
        if (ok)
            return result;
        if (*u.m_string == QLatin1String("Infinity"))
            return qInf();
        if (*u.m_string == QLatin1String("-Infinity"))
            return -qInf();
        return u.m_string->length() ? qQNaN() : 0;
    }

    Q_ASSERT_X(false, "toNumber()", "Not all states are included in the previous switch statement.");
    return 0; // Avoid compiler warning.
}

QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::toObject(QV8Engine* engine) const
{
    Q_ASSERT(engine);
    if (this->engine() && engine != this->engine()) {
        qWarning("QJSEngine::toObject: cannot convert value created in a different engine");
        return InvalidValue();
    }

    v8::HandleScope scope;
    switch (m_state) {
    case Invalid:
    case CNull:
    case CUndefined:
        return new QJSValuePrivate;
    case CString:
        return new QJSValuePrivate(engine, engine->makeJSValue(*u.m_string)->ToObject());
    case CNumber:
        return new QJSValuePrivate(engine, engine->makeJSValue(u.m_number)->ToObject());
    case CBool:
        return new QJSValuePrivate(engine, engine->makeJSValue(u.m_bool)->ToObject());
    case JSValue:
        if (m_value->IsObject())
            return const_cast<QJSValuePrivate*>(this);
        if (m_value->IsNull() || m_value->IsUndefined()) // avoid "Uncaught TypeError: Cannot convert null to object"
            return InvalidValue();
        return new QJSValuePrivate(engine, m_value->ToObject());
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Not all states are included in this switch");
        return InvalidValue();
    }
}

/*!
  This method is created only for QJSValue::toObject() purpose which is obsolete.
  \internal
 */
QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::toObject() const
{
    if (isJSBased())
        return toObject(engine());

    // Without an engine there is not much we can do.
    return new QJSValuePrivate;
}

QString QJSValuePrivate::toString() const
{
    switch (m_state) {
    case Invalid:
        return QString();
    case CBool:
        return u.m_bool ? QString::fromLatin1("true") : QString::fromLatin1("false");
    case CString:
        return *u.m_string;
    case CNumber:
        return QJSConverter::toString(u.m_number);
    case CNull:
        return QString::fromLatin1("null");
    case CUndefined:
        return QString::fromLatin1("undefined");
    case JSValue:
        Q_ASSERT(!m_value.IsEmpty());
        v8::HandleScope handleScope;
        v8::TryCatch tryCatch;
        v8::Local<v8::String> result = m_value->ToString();
        if (result.IsEmpty()) {
            result = tryCatch.Exception()->ToString();
            m_engine->setException(tryCatch.Exception(), tryCatch.Message());
        }
        return QJSConverter::toString(result);
    }

    Q_ASSERT_X(false, "toString()", "Not all states are included in the previous switch statement.");
    return QString(); // Avoid compiler warning.
}

QVariant QJSValuePrivate::toVariant() const
{
    switch (m_state) {
        case Invalid:
            return QVariant();
        case CBool:
            return QVariant(u.m_bool);
        case CString:
            return QVariant(*u.m_string);
        case CNumber:
            return QVariant(u.m_number);
        case CNull:
            return QVariant();
        case CUndefined:
            return QVariant();
        case JSValue:
            break;
    }

    Q_ASSERT(m_state == JSValue);
    Q_ASSERT(!m_value.IsEmpty());
    Q_ASSERT(m_engine);

    v8::HandleScope handleScope;
    return m_engine->variantFromJS(m_value);
}

inline QDateTime QJSValuePrivate::toDataTime() const
{
    if (!isDate())
        return QDateTime();

    v8::HandleScope handleScope;
    return QJSConverter::toDateTime(v8::Handle<v8::Date>::Cast(m_value));

}

inline QRegExp QJSValuePrivate::toRegExp() const
{
    if (!isRegExp())
        return QRegExp();

    v8::HandleScope handleScope;
    return QJSConverter::toRegExp(v8::Handle<v8::RegExp>::Cast(m_value));
}

QObject* QJSValuePrivate::toQObject() const
{
    if (!isJSBased())
        return 0;

    v8::HandleScope handleScope;
    return engine()->qtObjectFromJS(m_value);
}

double QJSValuePrivate::toInteger() const
{
    double result = toNumber();
    if (qIsNaN(result))
        return 0;
    if (qIsInf(result))
        return result;
    return (result > 0) ? qFloor(result) : -1 * qFloor(-result);
}

qint32 QJSValuePrivate::toInt32() const
{
    double result = toInteger();
    // Orginaly it should look like that (result == 0 || qIsInf(result) || qIsNaN(result)), but
    // some of these operation are invoked in toInteger subcall.
    if (qIsInf(result))
        return 0;
    return result;
}

quint32 QJSValuePrivate::toUInt32() const
{
    double result = toInteger();
    // Orginaly it should look like that (result == 0 || qIsInf(result) || qIsNaN(result)), but
    // some of these operation are invoked in toInteger subcall.
    if (qIsInf(result))
        return 0;
    return result;
}

quint16 QJSValuePrivate::toUInt16() const
{
    return toInt32();
}

inline bool QJSValuePrivate::isArray() const
{
    return isJSBased() && m_value->IsArray();
}

inline bool QJSValuePrivate::isBool() const
{
    return m_state == CBool || (isJSBased() && m_value->IsBoolean());
}

inline bool QJSValuePrivate::isCallable() const
{
    if (isFunction())
        return true;
    if (isObject()) {
        // Our C++ wrappers register function handlers but not always act as callables.
        return v8::Object::Cast(*m_value)->IsCallable();
    }
    return false;
}

inline bool QJSValuePrivate::isError() const
{
    if (!isJSBased())
        return false;
    v8::HandleScope handleScope;
    return m_value->IsError();
}

inline bool QJSValuePrivate::isFunction() const
{
    return isJSBased() && m_value->IsFunction();
}

inline bool QJSValuePrivate::isNull() const
{
    return m_state == CNull || (isJSBased() && m_value->IsNull());
}

inline bool QJSValuePrivate::isNumber() const
{
    return m_state == CNumber || (isJSBased() && m_value->IsNumber());
}

inline bool QJSValuePrivate::isObject() const
{
    return isJSBased() && m_value->IsObject();
}

inline bool QJSValuePrivate::isString() const
{
    return m_state == CString || (isJSBased() && m_value->IsString());
}

inline bool QJSValuePrivate::isUndefined() const
{
    return m_state == CUndefined || (isJSBased() && m_value->IsUndefined());
}

inline bool QJSValuePrivate::isValid() const
{
    return m_state != Invalid;
}

inline bool QJSValuePrivate::isVariant() const
{
    return isJSBased() && m_engine->isVariant(m_value);
}

bool QJSValuePrivate::isDate() const
{
    return (isJSBased() && m_value->IsDate());
}

bool QJSValuePrivate::isRegExp() const
{
    return (isJSBased() && m_value->IsRegExp());
}

bool QJSValuePrivate::isQObject() const
{
    return isJSBased() && engine()->isQObject(m_value);
}

inline bool QJSValuePrivate::equals(QJSValuePrivate* other)
{
    if (!isValid())
        return !other->isValid();

    if (!other->isValid())
        return false;

    if (!isJSBased() && !other->isJSBased()) {
        switch (m_state) {
        case CNull:
        case CUndefined:
            return other->isUndefined() || other->isNull();
        case CNumber:
            switch (other->m_state) {
            case CBool:
            case CString:
                return u.m_number == other->toNumber();
            case CNumber:
                return u.m_number == other->u.m_number;
            default:
                return false;
            }
        case CBool:
            switch (other->m_state) {
            case CBool:
                return u.m_bool == other->u.m_bool;
            case CNumber:
                return toNumber() == other->u.m_number;
            case CString:
                return toNumber() == other->toNumber();
            default:
                return false;
            }
        case CString:
            switch (other->m_state) {
            case CBool:
                return toNumber() == other->toNumber();
            case CNumber:
                return toNumber() == other->u.m_number;
            case CString:
                return *u.m_string == *other->u.m_string;
            default:
                return false;
            }
        default:
            Q_ASSERT_X(false, "QJSValue::equals", "Not all states are included in the previous switch statement.");
        }
    }

    v8::HandleScope handleScope;
    if (isJSBased() && !other->isJSBased()) {
        if (!other->assignEngine(engine())) {
            qWarning("QJSValue::equals: cannot compare to a value created in a different engine");
            return false;
        }
    } else if (!isJSBased() && other->isJSBased()) {
        if (!assignEngine(other->engine())) {
            qWarning("QJSValue::equals: cannot compare to a value created in a different engine");
            return false;
        }
    }

    Q_ASSERT(this->engine() && other->engine());
    if (this->engine() != other->engine()) {
        qWarning("QJSValue::equals: cannot compare to a value created in a different engine");
        return false;
    }
    return m_value->Equals(other->m_value);
}

inline bool QJSValuePrivate::strictlyEquals(QJSValuePrivate* other)
{
    if (isJSBased()) {
        // We can't compare these two values without binding to the same engine.
        if (!other->isJSBased()) {
            if (other->assignEngine(engine()))
                return m_value->StrictEquals(other->m_value);
            return false;
        }
        if (other->engine() != engine()) {
            qWarning("QJSValue::strictlyEquals: cannot compare to a value created in a different engine");
            return false;
        }
        return m_value->StrictEquals(other->m_value);
    }
    if (isStringBased()) {
        if (other->isStringBased())
            return *u.m_string == *(other->u.m_string);
        if (other->isJSBased()) {
            assignEngine(other->engine());
            return m_value->StrictEquals(other->m_value);
        }
    }
    if (isNumberBased()) {
        if (other->isJSBased()) {
            assignEngine(other->engine());
            return m_value->StrictEquals(other->m_value);
        }
        if (m_state != other->m_state)
            return false;
        if (m_state == CNumber)
            return u.m_number == other->u.m_number;
        Q_ASSERT(m_state == CBool);
        return u.m_bool == other->u.m_bool;
    }

    if (!isValid() && !other->isValid())
        return true;

    return false;
}

inline bool QJSValuePrivate::lessThan(QJSValuePrivate *other) const
{
    if (engine() != other->engine() && engine() && other->engine()) {
        qWarning("QJSValue::lessThan: cannot compare to a value created in a different engine");
        return false;
    }

    if (!isValid() || !other->isValid())
        return false;

    if (isString() && other->isString())
        return toString() < other->toString();

    if (isObject() || other->isObject()) {
        v8::HandleScope handleScope;
        QV8Engine *eng = m_engine ? engine() : other->engine();
        // FIXME: lessThan can throw an exception which will be dropped by this code:
        Q_ASSERT(eng);
        eng->saveException();
        QScriptSharedDataPointer<QJSValuePrivate> cmp(eng->evaluate(QString::fromLatin1("(function(a,b){return a<b})")));
        Q_ASSERT(cmp->isFunction());
        v8::Handle<v8::Value> args[2];
        cmp->prepareArgumentsForCall(args, QJSValueList() << QJSValuePrivate::get(this) << QJSValuePrivate::get(other));
        QScriptSharedDataPointer<QJSValuePrivate> resultValue(cmp->call(0, 2, args));
        bool result = resultValue->toBool();
        eng->restoreException();
        return result;
    }

    double nthis = toNumber();
    double nother = other->toNumber();
    if (qIsNaN(nthis) || qIsNaN(nother)) {
        // Should return undefined in ECMA standard.
        return false;
    }
    return nthis < nother;
}

inline bool QJSValuePrivate::instanceOf(QJSValuePrivate* other) const
{
    if (!isObject() || !other->isFunction())
        return false;
    if (engine() != other->engine()) {
        qWarning("QJSValue::instanceof: cannot perform operation on a value created in a different engine");
        return false;
    }
    v8::HandleScope handleScope;
    return instanceOf(v8::Handle<v8::Object>::Cast(other->m_value));
}

inline bool QJSValuePrivate::instanceOf(v8::Handle<v8::Object> other) const
{
    Q_ASSERT(isObject());
    Q_ASSERT(other->IsFunction());

    v8::Handle<v8::Object> self = v8::Handle<v8::Object>::Cast(m_value);
    v8::Handle<v8::Value> selfPrototype = self->GetPrototype();
    v8::Handle<v8::Value> otherPrototype = other->Get(v8::String::New("prototype"));

    while (!selfPrototype->IsNull()) {
        if (selfPrototype->StrictEquals(otherPrototype))
            return true;
        // In general a prototype can be an object or null, but in the loop it can't be null, so
        // we can cast it safely.
        selfPrototype = v8::Handle<v8::Object>::Cast(selfPrototype)->GetPrototype();
    }
    return false;
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::prototype() const
{
    if (isObject()) {
        v8::HandleScope handleScope;
        return new QJSValuePrivate(engine(), v8::Handle<v8::Object>::Cast(m_value)->GetPrototype());
    }
    return InvalidValue();
}

inline void QJSValuePrivate::setPrototype(QJSValuePrivate* prototype)
{
    if (isObject() && (prototype->isObject() || prototype->isNull())) {
        if (engine() != prototype->engine()) {
            if (prototype->engine()) {
                qWarning("QJSValue::setPrototype() failed: cannot set a prototype created in a different engine");
                return;
            }
            prototype->assignEngine(engine());
        }
        v8::HandleScope handleScope;
        if (!v8::Handle<v8::Object>::Cast(m_value)->SetPrototype(*prototype))
            qWarning("QJSValue::setPrototype() failed: cyclic prototype value");
    }
}

inline void QJSValuePrivate::setProperty(const QString& name, QJSValuePrivate* value, uint attribs)
{
    if (!isObject())
        return;
    v8::HandleScope handleScope;
    setProperty(QJSConverter::toString(name), value, attribs);
}

inline void QJSValuePrivate::setProperty(v8::Handle<v8::String> name, QJSValuePrivate* value, uint attribs)
{
    if (!isObject())
        return;

    if (!value->isJSBased())
        value->assignEngine(engine());

    if (!value->isValid()) {
        // Remove the property.
        v8::HandleScope handleScope;
        v8::TryCatch tryCatch;
        v8::Handle<v8::Object> recv(v8::Object::Cast(*m_value));
//        if (attribs & QJSValue::PropertyGetter && !(attribs & QJSValue::PropertySetter)) {
//            v8::Local<v8::Object> descriptor = engine()->originalGlobalObject()->getOwnPropertyDescriptor(recv, name);
//            if (!descriptor.IsEmpty()) {
//                v8::Local<v8::Value> setter = descriptor->Get(v8::String::New("set"));
//                if (!setter.IsEmpty() && !setter->IsUndefined()) {
//                    recv->Delete(name);
//                    engine()->originalGlobalObject()->defineGetterOrSetter(recv, name, setter, QJSValue::PropertySetter);
//                    if (tryCatch.HasCaught())
//                        engine()->setException(tryCatch.Exception(), tryCatch.Message());
//                    return;
//                }
//            }
//        } else if (attribs & QJSValue::PropertySetter && !(attribs & QJSValue::PropertyGetter)) {
//            v8::Local<v8::Object> descriptor = engine()->originalGlobalObject()->getOwnPropertyDescriptor(recv, name);
//            if (!descriptor.IsEmpty()) {
//                v8::Local<v8::Value> getter = descriptor->Get(v8::String::New("get"));
//                if (!getter.IsEmpty() && !getter->IsUndefined()) {
//                    recv->Delete(name);
//                    engine()->originalGlobalObject()->defineGetterOrSetter(recv, name, getter, QJSValue::PropertyGetter);
//                    if (tryCatch.HasCaught())
//                        engine()->setException(tryCatch.Exception(), tryCatch.Message());
//                    return;
//                }
//            }
//        }
        recv->Delete(name);
        if (tryCatch.HasCaught())
            engine()->setException(tryCatch.Exception(), tryCatch.Message());
        return;
    }

    if (engine() != value->engine()) {
        qWarning("QJSValue::setProperty(%s) failed: "
                 "cannot set value created in a different engine",
                 qPrintable(QJSConverter::toString(name)));
        return;
    }

    v8::TryCatch tryCatch;
//    if (attribs & (QJSValue::PropertyGetter | QJSValue::PropertySetter)) {
//        engine()->originalGlobalObject()->defineGetterOrSetter(*this, name, value->m_value, attribs);
//    } else {
        v8::Object::Cast(*m_value)->Set(name, value->m_value, v8::PropertyAttribute(attribs & QJSConverter::PropertyAttributeMask));
//    }
    if (tryCatch.HasCaught())
        engine()->setException(tryCatch.Exception(), tryCatch.Message());
}

inline void QJSValuePrivate::setProperty(quint32 index, QJSValuePrivate* value, uint attribs)
{
    // FIXME this method should by integrated with other overloads to use the same code patch.
    // for now it is not possible as v8 doesn't allow to set property attributes using index based api.

    if (!isObject())
        return;

    if (attribs) {
        // FIXME we dont need to convert index to a string.
        //Object::Set(int,value) do not take attributes.
        setProperty(QString::number(index), value, attribs);
        return;
    }

    if (!value->isJSBased())
        value->assignEngine(engine());

    if (!value->isValid()) {
        // Remove the property.
        v8::HandleScope handleScope;
        v8::TryCatch tryCatch;
        v8::Object::Cast(*m_value)->Delete(index);
        if (tryCatch.HasCaught())
            engine()->setException(tryCatch.Exception(), tryCatch.Message());
        return;
    }

    if (engine() != value->engine()) {
        qWarning("QJSValue::setProperty() failed: cannot set value created in a different engine");
        return;
    }

    v8::HandleScope handleScope;
    v8::TryCatch tryCatch;
    v8::Object::Cast(*m_value)->Set(index, value->m_value);
    if (tryCatch.HasCaught())
        engine()->setException(tryCatch.Exception(), tryCatch.Message());
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::property(const QString& name) const
{
    if (!name.length())
        return InvalidValue();
    if (!isObject())
        return InvalidValue();

    v8::HandleScope handleScope;
    return property(QJSConverter::toString(name));
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::property(v8::Handle<v8::String> name) const
{
    Q_ASSERT(!name.IsEmpty());
    if (!isObject())
        return InvalidValue();
    return property<>(name);
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::property(quint32 index) const
{
    if (!isObject())
        return InvalidValue();
    return property<>(index);
}

template<typename T>
inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::property(T name) const
{
    Q_ASSERT(isObject());
    v8::HandleScope handleScope;
    v8::Handle<v8::Object> self(v8::Object::Cast(*m_value));

    v8::TryCatch tryCatch;
    v8::Handle<v8::Value> result = self->Get(name);
    if (tryCatch.HasCaught()) {
        result = tryCatch.Exception();
        engine()->setException(result, tryCatch.Message());
        return new QJSValuePrivate(engine(), result);
    }
    if (result.IsEmpty() || (result->IsUndefined() && !self->Has(name))) {
        // In QtScript we make a distinction between a property that exists and has value undefined,
        // and a property that doesn't exist; in the latter case, we should return an invalid value.
        return InvalidValue();
    }
    return new QJSValuePrivate(engine(), result);
}

inline bool QJSValuePrivate::deleteProperty(const QString& name)
{
    if (!isObject())
        return false;

    v8::HandleScope handleScope;
    v8::Handle<v8::Object> self(v8::Handle<v8::Object>::Cast(m_value));
    return self->Delete(QJSConverter::toString(name));
}

inline QJSValue::PropertyFlags QJSValuePrivate::propertyFlags(const QString& name) const
{
    if (!isObject())
        return QJSValue::PropertyFlags(0);

    v8::HandleScope handleScope;
    return engine()->getPropertyFlags(v8::Handle<v8::Object>::Cast(m_value), QJSConverter::toString(name));
}

inline QJSValue::PropertyFlags QJSValuePrivate::propertyFlags(v8::Handle<v8::String> name) const
{
    if (!isObject())
        return QJSValue::PropertyFlags(0);

    v8::HandleScope handleScope;
    return engine()->getPropertyFlags(v8::Handle<v8::Object>::Cast(m_value), name);
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::call(QJSValuePrivate* thisObject, const QJSValueList& args)
{
    if (!isCallable())
        return InvalidValue();

    v8::HandleScope handleScope;

    // Convert all arguments and bind to the engine.
    int argc = args.size();
    QVarLengthArray<v8::Handle<v8::Value>, 8> argv(argc);
    if (!prepareArgumentsForCall(argv.data(), args)) {
        qWarning("QJSValue::call() failed: cannot call function with argument created in a different engine");
        return InvalidValue();
    }

    return call(thisObject, argc, argv.data());
}

QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::call(QJSValuePrivate* thisObject, int argc, v8::Handle<v8::Value> *argv)
{
    QV8Engine *e = engine();

    v8::Handle<v8::Object> recv;

    if (!thisObject || !thisObject->isObject()) {
        recv = v8::Handle<v8::Object>(v8::Object::Cast(*e->global()));
    } else {
        if (!thisObject->assignEngine(e)) {
            qWarning("QJSValue::call() failed: cannot call function with thisObject created in a different engine");
            return InvalidValue();
        }

        recv = v8::Handle<v8::Object>(v8::Object::Cast(*thisObject->m_value));
    }

    if (argc < 0) {
        v8::Local<v8::Value> exeption = v8::Exception::TypeError(v8::String::New("Arguments must be an array"));
        e->setException(exeption);
        return new QJSValuePrivate(e, exeption);
    }

    v8::TryCatch tryCatch;
    v8::Handle<v8::Value> result = v8::Object::Cast(*m_value)->CallAsFunction(recv, argc, argv);

    if (result.IsEmpty()) {
        result = tryCatch.Exception();
        // TODO: figure out why v8 doesn't always produce an exception value.
        //Q_ASSERT(!result.IsEmpty());
        if (result.IsEmpty())
            result = v8::Exception::Error(v8::String::New("missing exception value"));
        e->setException(result, tryCatch.Message());
    }

    return new QJSValuePrivate(e, result);
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::construct(int argc, v8::Handle<v8::Value> *argv)
{
    QV8Engine *e = engine();

    if (argc < 0) {
        v8::Local<v8::Value> exeption = v8::Exception::TypeError(v8::String::New("Arguments must be an array"));
        e->setException(exeption);
        return new QJSValuePrivate(e, exeption);
    }

    v8::TryCatch tryCatch;
    v8::Handle<v8::Value> result = v8::Object::Cast(*m_value)->CallAsConstructor(argc, argv);

    if (result.IsEmpty()) {
        result = tryCatch.Exception();
        e->setException(result, tryCatch.Message());
    }

    return new QJSValuePrivate(e, result);
}

inline QScriptPassPointer<QJSValuePrivate> QJSValuePrivate::construct(const QJSValueList& args)
{
    if (!isCallable())
        return InvalidValue();

    v8::HandleScope handleScope;

    // Convert all arguments and bind to the engine.
    int argc = args.size();
    QVarLengthArray<v8::Handle<v8::Value>, 8> argv(argc);
    if (!prepareArgumentsForCall(argv.data(), args)) {
        qWarning("QJSValue::construct() failed: cannot construct function with argument created in a different engine");
        return InvalidValue();
    }

    return construct(argc, argv.data());
}

/*! \internal
 * Make sure this value is associated with a v8 value belogning to this engine.
 * If the value was invalid, or belogning to another engine, return false.
 */
bool QJSValuePrivate::assignEngine(QV8Engine* engine)
{
    Q_ASSERT(engine);
    v8::HandleScope handleScope;
    switch (m_state) {
    case Invalid:
        return false;
    case CBool:
        m_value = v8::Persistent<v8::Value>::New(engine->makeJSValue(u.m_bool));
        break;
    case CString:
        m_value = v8::Persistent<v8::Value>::New(engine->makeJSValue(*u.m_string));
        delete u.m_string;
        break;
    case CNumber:
        m_value = v8::Persistent<v8::Value>::New(engine->makeJSValue(u.m_number));
        break;
    case CNull:
        m_value = v8::Persistent<v8::Value>::New(engine->makeJSValue(QJSValue::NullValue));
        break;
    case CUndefined:
        m_value = v8::Persistent<v8::Value>::New(engine->makeJSValue(QJSValue::UndefinedValue));
        break;
    default:
        if (this->engine() == engine)
            return true;
        else if (!isJSBased())
            Q_ASSERT_X(!isJSBased(), "assignEngine()", "Not all states are included in the previous switch statement.");
        else
            qWarning("JSValue can't be rassigned to an another engine.");
        return false;
    }
    m_engine = engine;
    m_state = JSValue;

    m_engine->registerValue(this);
    return true;
}

/*!
  \internal
  Invalidates this value.

  Does not remove the value from the engine's list of
  registered values; that's the responsibility of the caller.
*/
void QJSValuePrivate::invalidate()
{
    if (isJSBased()) {
        m_value.Dispose();
        m_value.Clear();
    } else if (isStringBased()) {
        delete u.m_string;
    }
    m_engine = 0;
    m_state = Invalid;
}

QV8Engine* QJSValuePrivate::engine() const
{
    return m_engine;
}

inline QJSValuePrivate::operator v8::Handle<v8::Value>() const
{
    Q_ASSERT(isJSBased());
    return m_value;
}

inline QJSValuePrivate::operator v8::Handle<v8::Object>() const
{
    Q_ASSERT(isObject());
    return v8::Handle<v8::Object>::Cast(m_value);
}

/*!
 * Return a v8::Handle, assign to the engine if needed.
 */
v8::Handle<v8::Value> QJSValuePrivate::asV8Value(QV8Engine* engine)
{
    if (!m_engine) {
        if (!assignEngine(engine))
            return v8::Handle<v8::Value>();
    }
    Q_ASSERT(isJSBased());
    return m_value;
}

/*!
  \internal
  Returns true if QSV have an engine associated.
*/
bool QJSValuePrivate::isJSBased() const
{
#ifndef QT_NO_DEBUG
    // internals check.
    if (m_state >= JSValue)
        Q_ASSERT(!m_value.IsEmpty());
    else
        Q_ASSERT(m_value.IsEmpty());
#endif
    return m_state >= JSValue;
}

/*!
  \internal
  Returns true if current value of QSV is placed in m_number.
*/
bool QJSValuePrivate::isNumberBased() const { return m_state == CNumber || m_state == CBool; }

/*!
  \internal
  Returns true if current value of QSV is placed in m_string.
*/
bool QJSValuePrivate::isStringBased() const { return m_state == CString; }

/*!
  \internal
  Converts arguments and bind them to the engine.
  \attention argv should be big enough
*/
inline bool QJSValuePrivate::prepareArgumentsForCall(v8::Handle<v8::Value> argv[], const QJSValueList& args) const
{
    QJSValueList::const_iterator i = args.constBegin();
    for (int j = 0; i != args.constEnd(); j++, i++) {
        QJSValuePrivate* value = QJSValuePrivate::get(*i);
        if ((value->isJSBased() && engine() != value->engine())
                || (!value->isJSBased() && value->isValid() && !value->assignEngine(engine())))
            // Different engines are not allowed!
            return false;
        if (value->isValid())
            argv[j] = *value;
        else
            argv[j] = engine()->makeJSValue(QJSValue::UndefinedValue);
    }
    return true;
}

QT_END_NAMESPACE

#endif
