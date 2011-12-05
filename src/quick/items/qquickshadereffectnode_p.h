/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QQUICKSHADEREFFECTNODE_P_H
#define QQUICKSHADEREFFECTNODE_P_H

#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgmaterial.h>
#include <QtQuick/private/qsgtextureprovider_p.h>
#include <QtQuick/qquickitem.h>

#include <QtCore/qsharedpointer.h>
#include <QtCore/qpointer.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

struct QQuickShaderEffectMaterialKey {
    QByteArray vertexCode;
    QByteArray fragmentCode;
    const char *className;

    bool operator == (const QQuickShaderEffectMaterialKey &other) const;
};

uint qHash(const QQuickShaderEffectMaterialKey &key);

// TODO: Implement support for multisampling.
struct QQuickShaderEffectProgram : public QQuickShaderEffectMaterialKey
{
    QQuickShaderEffectProgram() : respectsOpacity(false), respectsMatrix(false) {}

    QVector<QByteArray> attributeNames;
    QSet<QByteArray> uniformNames;

    uint respectsOpacity : 1;
    uint respectsMatrix : 1;
};


class QQuickCustomMaterialShader;
class QQuickShaderEffectMaterial : public QSGMaterial
{
public:
    enum CullMode
    {
        NoCulling,
        BackFaceCulling,
        FrontFaceCulling
    };

    QQuickShaderEffectMaterial();
    virtual QSGMaterialType *type() const;
    virtual QSGMaterialShader *createShader() const;
    virtual int compare(const QSGMaterial *other) const;

    void setCullMode(CullMode face);
    CullMode cullMode() const;

    void setProgramSource(const QQuickShaderEffectProgram &);
    void setUniforms(const QVector<QPair<QByteArray, QVariant> > &uniformValues);
    void setTextureProviders(const QVector<QPair<QByteArray, QSGTextureProvider *> > &textures);
    const QVector<QPair<QByteArray, QSGTextureProvider *> > &textureProviders() const;
    void updateTextures() const;

protected:
    friend class QQuickCustomMaterialShader;

    // The type pointer needs to be unique. It is not safe to let the type object be part of the
    // QQuickShaderEffectMaterial, since it can be deleted and a new one constructed on top of the old
    // one. The new QQuickShaderEffectMaterial would then get the same type pointer as the old one, and
    // CustomMaterialShaders based on the old one would incorrectly be used together with the new
    // one. To guarantee that the type pointer is unique, the type object must live as long as
    // there are any CustomMaterialShaders of that type.
    QSharedPointer<QSGMaterialType> m_type;

    QQuickShaderEffectProgram m_source;
    QVector<QPair<QByteArray, QVariant> > m_uniformValues;
    QVector<QPair<QByteArray, QSGTextureProvider *> > m_textures;
    CullMode m_cullMode;

    static QHash<QQuickShaderEffectMaterialKey, QSharedPointer<QSGMaterialType> > materialMap;
};


class QSGShaderEffectMesh;

class QQuickShaderEffectNode : public QObject, public QSGGeometryNode
{
    Q_OBJECT
public:
    QQuickShaderEffectNode();
    virtual ~QQuickShaderEffectNode();

    virtual void preprocess();

    QQuickShaderEffectMaterial *shaderMaterial() { return &m_material; }

private Q_SLOTS:
    void markDirtyTexture();

private:
    QQuickShaderEffectMaterial m_material;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQUICKSHADEREFFECTNODE_P_H
