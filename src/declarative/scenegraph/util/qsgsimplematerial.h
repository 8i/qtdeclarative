/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGSIMPLEMATERIAL_H
#define QSGSIMPLEMATERIAL_H

#include <qsgmaterial.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

template <typename State>
class QSGSimpleMaterialShader : public QSGMaterialShader
{
public:
    void initialize() {
        QSGMaterialShader::initialize();

        m_id_matrix = program()->uniformLocation(uniformMatrixName());
        if (m_id_matrix < 0) {
            qFatal("QSGSimpleMaterialShader does not implement 'uniform highp mat4 %s;' in its vertex shader",
                   uniformMatrixName());
        }

        const char *opacity = uniformOpacityName();
        if (opacity) {
            m_id_opacity = program()->uniformLocation(uniformOpacityName());
            if (m_id_opacity < 0) {
                qFatal("QSGSimpleMaterialShader does not implement 'uniform lowp float %s' in its fragment shader",
                       uniformOpacityName());
            }
        } else {
            m_id_opacity = -1;
        }

        resolveUniforms();
    }

    const char *uniformMatrixName() const { return "qt_ModelViewProjectionMatrix"; }
    const char *uniformOpacityName() const { return "qt_Opacity"; }

    void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial);

    virtual void updateState(const State *newState, const State *oldState) = 0;

    virtual void resolveUniforms() {}

    virtual QList<QByteArray> attributes() const = 0;

    char const *const *attributeNames() const
    {
        if (m_attribute_pointers.size())
            return m_attribute_pointers.constData();

        QList<QByteArray> names = attributes();

        // Calculate the total number of bytes needed, so we don't get rellocs and
        // bad pointers while copying over the individual names.
        // Add an extra byte pr entry for the '\0' char.
        int total = 0;
        for (int i=0; i<names.size(); ++i)
            total += names.at(i).size() + 1;
        m_attribute_name_data.reserve(total);

        // Copy over the names
        for (int i=0; i<names.size(); ++i) {
            m_attribute_pointers << m_attribute_name_data.constData() + m_attribute_name_data.size();
            m_attribute_name_data.append(names.at(i));
            m_attribute_name_data.append('\0');
        }

        // Append the "null" terminator
        m_attribute_pointers << 0;

        return m_attribute_pointers.constData();
    }

private:
    int m_id_matrix;
    int m_id_opacity;

    mutable QByteArray m_attribute_name_data;
    mutable QVector<const char *> m_attribute_pointers;
};

#define QSG_DECLARE_SIMPLE_SHADER(Shader, State)                \
static QSGMaterialShader *createShader()                        \
{                                                               \
    return new Shader;                                          \
}                                                               \
public:                                                         \
static QSGSimpleMaterial<State> *createMaterial()               \
{                                                               \
    return new QSGSimpleMaterial<State>(createShader);          \
}


typedef QSGMaterialShader *(*PtrShaderCreateFunc)();


template <typename State>
class QSGSimpleMaterial : public QSGMaterial
{

public:
    QSGSimpleMaterial(const State &state, PtrShaderCreateFunc func)
        : m_state(state)
        , m_func(func)
    {
    }

    QSGSimpleMaterial(PtrShaderCreateFunc func)
        : m_func(func)
    {
    }

    QSGMaterialShader *createShader() const { return m_func(); }
    QSGMaterialType *type() const { return &m_type; }

    State *state() { return &m_state; }
    const State *state() const { return &m_state; }

private:
    static QSGMaterialType m_type;
    State m_state;
    PtrShaderCreateFunc m_func;
};

#define QSG_DECLARE_SIMPLE_COMPARABLE_SHADER(Shader, State)                     \
static QSGMaterialShader *createShader()                        \
{                                                               \
    return new Shader;                                          \
}                                                               \
public:                                                         \
static QSGSimpleMaterialComparableMaterial<State> *createMaterial()                            \
{                                                               \
    return new QSGSimpleMaterialComparableMaterial<State>(createShader);    \
}

template <typename State>
class QSGSimpleMaterialComparableMaterial : public QSGSimpleMaterial<State>
{

public:
    QSGSimpleMaterialComparableMaterial(const State &state, PtrShaderCreateFunc func)
        : QSGSimpleMaterial<State>(state, func) {}

    QSGSimpleMaterialComparableMaterial(PtrShaderCreateFunc func)
        : QSGSimpleMaterial<State>(func) {}

    int compare(const QSGMaterial *other) const {
        return QSGSimpleMaterialComparableMaterial<State>::state()->compare(static_cast<const QSGSimpleMaterialComparableMaterial<State> *>(other)->state());
    }
};


template <typename State>
QSGMaterialType QSGSimpleMaterial<State>::m_type;


template <typename State>
Q_INLINE_TEMPLATE void QSGSimpleMaterialShader<State>::updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    if (state.isMatrixDirty())
        program()->setUniformValue(m_id_matrix, state.combinedMatrix());
    if (state.isOpacityDirty() && m_id_opacity >= 0)
        program()->setUniformValue(m_id_opacity, state.opacity());

    State *ns = static_cast<QSGSimpleMaterial<State> *>(newMaterial)->state();
    State *old = 0;
    if (oldMaterial)
        old = static_cast<QSGSimpleMaterial<State> *>(oldMaterial)->state();
    updateState(ns, old);
}

QT_END_NAMESPACE

QT_END_HEADER


#endif
