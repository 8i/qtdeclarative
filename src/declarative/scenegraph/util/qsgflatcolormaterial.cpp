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

#include "qsgflatcolormaterial.h"

#include <qopenglshaderprogram.h>

QT_BEGIN_NAMESPACE

class FlatColorMaterialShader : public QSGMaterialShader
{
public:
    virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect);
    virtual char const *const *attributeNames() const;

    static QSGMaterialType type;

private:
    virtual void initialize();
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;

    int m_matrix_id;
    int m_color_id;
};

QSGMaterialType FlatColorMaterialShader::type;

void FlatColorMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    Q_ASSERT(oldEffect == 0 || newEffect->type() == oldEffect->type());

    QSGFlatColorMaterial *oldMaterial = static_cast<QSGFlatColorMaterial *>(oldEffect);
    QSGFlatColorMaterial *newMaterial = static_cast<QSGFlatColorMaterial *>(newEffect);

    const QColor &c = newMaterial->color();

    if (oldMaterial == 0 || c != oldMaterial->color() || state.isOpacityDirty()) {
        float opacity = state.opacity();
        QVector4D v(c.redF() * c.alphaF() * opacity,
                    c.greenF() * c.alphaF() * opacity,
                    c.blueF() * c.alphaF() * opacity,
                    c.alphaF() * opacity);
        program()->setUniformValue(m_color_id, v);
    }

    if (state.isMatrixDirty())
        program()->setUniformValue(m_matrix_id, state.combinedMatrix());
}

char const *const *FlatColorMaterialShader::attributeNames() const
{
    static char const *const attr[] = { "vCoord", 0 };
    return attr;
}

void FlatColorMaterialShader::initialize()
{
    m_matrix_id = program()->uniformLocation("matrix");
    m_color_id = program()->uniformLocation("color");
}

const char *FlatColorMaterialShader::vertexShader() const {
    return
        "attribute highp vec4 vCoord;                   \n"
        "uniform highp mat4 matrix;                     \n"
        "void main() {                                  \n"
        "    gl_Position = matrix * vCoord;             \n"
        "}";
}

const char *FlatColorMaterialShader::fragmentShader() const {
    return
        "uniform lowp vec4 color;                       \n"
        "void main() {                                  \n"
        "    gl_FragColor = color;                      \n"
        "}";
}



/*!
    \class QSGFlatColorMaterial
    \brief The QSGFlatColorMaterial class provides a convenient way of rendering
    solid colored geometry in the scene graph.

    \inmodule QtDeclarative

    The flat color material will fill every pixel in a geometry using
    a solid color. The color can contain transparency.

    The geometry to be rendered with a flat color material requires
    vertices in attribute location 0 in the QSGGeometry object to render
    correctly. The QSGGeometry::defaultAttributes_Point2D() returns an attribute
    set compatible with this material.

    The flat color material respects both current opacity and current matrix
    when updating it's rendering state.
 */


/*!
    Constructs a new flat color material.

    The default color is white.
 */

QSGFlatColorMaterial::QSGFlatColorMaterial() : m_color(QColor(255, 255, 255))
{
}



/*!
    \fn QColor QSGFlatColorMaterial::color() const

    Returns this flat color material's color.

    The default color is white.
 */



/*!
    Sets this flat color material's color to \a color.
 */

void QSGFlatColorMaterial::setColor(const QColor &color)
{
    m_color = color;
    setFlag(Blending, m_color.alpha() != 0xff);
}



/*!
    \internal
 */

QSGMaterialType *QSGFlatColorMaterial::type() const
{
    return &FlatColorMaterialShader::type;
}



/*!
    \internal
 */

QSGMaterialShader *QSGFlatColorMaterial::createShader() const
{
    return new FlatColorMaterialShader;
}


int QSGFlatColorMaterial::compare(const QSGMaterial *other) const
{
    const QSGFlatColorMaterial *flat = static_cast<const QSGFlatColorMaterial *>(other);
    return m_color.rgba() - flat->color().rgba();

}

QT_END_NAMESPACE
