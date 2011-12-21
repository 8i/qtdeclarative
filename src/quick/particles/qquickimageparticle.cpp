/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
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

#include <QtQuick/private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>
#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgtexturematerial.h>
#include <QtQuick/qsgtexture.h>
#include <QFile>
#include "qquickimageparticle_p.h"
#include "qquickparticleemitter_p.h"
#include <private/qquicksprite_p.h>
#include <private/qquickspriteengine_p.h>
#include <QOpenGLFunctions>
#include <QtQuick/qsgengine.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <private/qdeclarativeglobal_p.h>

QT_BEGIN_NAMESPACE
//###Switch to define later, for now user-friendly (no compilation) debugging is worth it
DEFINE_BOOL_CONFIG_OPTION(qmlParticlesDebug, QML_PARTICLES_DEBUG)

#ifndef QT_OPENGL_ES_2
#define SHADER_DEFINES "#version 120\n"
#else
#define SHADER_DEFINES ""
#endif

//TODO: Make it larger on desktop? Requires fixing up shader code with the same define
#define UNIFORM_ARRAY_SIZE 64

static const char vertexShaderCode[] =
    "#if defined(DEFORM)\n"
    "attribute highp vec4 vPosTex;\n"
    "#else\n"
    "attribute highp vec2 vPos;\n"
    "#endif\n"
    "attribute highp vec4 vData; //  x = time,  y = lifeSpan, z = size,  w = endSize\n"
    "attribute highp vec4 vVec; // x,y = constant speed,  z,w = acceleration\n"
    "uniform highp float entry;\n"
    "#if defined(COLOR)\n"
    "attribute highp vec4 vColor;\n"
    "#endif\n"
    "#if defined(DEFORM)\n"
    "attribute highp vec4 vDeformVec; //x,y x unit vector; z,w = y unit vector\n"
    "attribute highp vec3 vRotation; //x = radians of rotation, y=rotation speed, z= bool autoRotate\n"
    "#endif\n"
    "#if defined(SPRITE)\n"
    "attribute highp vec4 vAnimData;// interpolate(bool), duration, frameCount (this anim), timestamp (this anim)\n"
    "attribute highp vec4 vAnimPos;//sheet x,y, width/height of this anim\n"
    "uniform highp vec2 animSheetSize; //width/height of whole sheet\n"
    "#endif\n"
    "\n"
    "uniform highp mat4 qt_Matrix;\n"
    "uniform highp float timestamp;\n"
    "#if defined(TABLE)\n"
    "varying lowp vec2 tt;//y is progress if Sprite mode\n"
    "uniform highp float sizetable[64];\n"
    "uniform highp float opacitytable[64];\n"
    "#endif\n"
    "#if defined(SPRITE)\n"
    "varying highp vec4 fTexS;\n"
    "#elif defined(DEFORM)\n"
    "varying highp vec2 fTex;\n"
    "#endif\n"
    "#if defined(COLOR)\n"
    "varying lowp vec4 fColor;\n"
    "#else\n"
    "varying lowp float fFade;\n"
    "#endif\n"
    "\n"
    "\n"
    "void main() {\n"
    "\n"
    "    highp float t = (timestamp - vData.x) / vData.y;\n"
    "    if (t < 0. || t > 1.) {\n"
    "#if defined(DEFORM)\n"
    "        gl_Position = qt_Matrix * vec4(vPosTex.x, vPosTex.y, 0., 1.);\n"
    "#else\n"
    "        gl_PointSize = 0.;\n"
    "#endif\n"
    "    } else {\n"
    "#if defined(SPRITE)\n"
    "        //Calculate frame location in texture\n"
    "        highp float frameIndex = mod((((timestamp - vAnimData.w)*1000.)/vAnimData.y),vAnimData.z);\n"
    "        tt.y = mod((timestamp - vAnimData.w)*1000., vAnimData.y) / vAnimData.y;\n"
    "\n"
    "        frameIndex = floor(frameIndex);\n"
    "        fTexS.xy = vec2(((frameIndex + vPosTex.z) * vAnimPos.z / animSheetSize.x), ((vAnimPos.y + vPosTex.w * vAnimPos.w) / animSheetSize.y));\n"
    "\n"
    "        //Next frame is also passed, for interpolation\n"
    "        //### Should the next anim be precalculated to allow for interpolation there?\n"
    "        if (vAnimData.x == 1.0 && frameIndex != vAnimData.z - 1.)//Can't do it for the last frame though, this anim may not loop\n"
    "            frameIndex = mod(frameIndex+1., vAnimData.z);\n"
    "        fTexS.zw = vec2(((frameIndex + vPosTex.z) * vAnimPos.z / animSheetSize.x), ((vAnimPos.y + vPosTex.w * vAnimPos.w) / animSheetSize.y));\n"
    "#elif defined(DEFORM)\n"
    "        fTex = vPosTex.zw;\n"
    "#endif\n"
    "        highp float currentSize = mix(vData.z, vData.w, t * t);\n"
    "        lowp float fade = 1.;\n"
    "        highp float fadeIn = min(t * 10., 1.);\n"
    "        highp float fadeOut = 1. - clamp((t - 0.75) * 4.,0., 1.);\n"
    "\n"
    "#if defined(TABLE)\n"
    "        currentSize = currentSize * sizetable[int(floor(t*64.))];\n"
    "        fade = fade * opacitytable[int(floor(t*64.))];\n"
    "#endif\n"
    "\n"
    "        if (entry == 1.)\n"
    "            fade = fade * fadeIn * fadeOut;\n"
    "        else if (entry == 2.)\n"
    "            currentSize = currentSize * fadeIn * fadeOut;\n"
    "\n"
    "        if (currentSize <= 0.) {\n"
    "#if defined(DEFORM)\n"
    "            gl_Position = qt_Matrix * vec4(vPosTex.x, vPosTex.y, 0., 1.);\n"
    "#else\n"
    "            gl_PointSize = 0.;\n"
    "#endif\n"
    "        } else {\n"
    "            if (currentSize < 3.)//Sizes too small look jittery as they move\n"
    "                currentSize = 3.;\n"
    "\n"
    "            highp vec2 pos;\n"
    "#if defined(DEFORM)\n"
    "            highp float rotation = vRotation.x + vRotation.y * t * vData.y;\n"
    "            if (vRotation.z == 1.0){\n"
    "                highp vec2 curVel = vVec.zw * t * vData.y + vVec.xy;\n"
    "                rotation += atan(curVel.y, curVel.x);\n"
    "            }\n"
    "            highp vec2 trigCalcs = vec2(cos(rotation), sin(rotation));\n"
    "            highp vec4 deform = vDeformVec * currentSize * (vPosTex.zzww - 0.5);\n"
    "            highp vec4 rotatedDeform = deform.xxzz * trigCalcs.xyxy;\n"
    "            rotatedDeform = rotatedDeform + (deform.yyww * trigCalcs.yxyx * vec4(-1.,1.,-1.,1.));\n"
    "            /* The readable version:\n"
    "            highp vec2 xDeform = vDeformVec.xy * currentSize * (vTex.x-0.5);\n"
    "            highp vec2 yDeform = vDeformVec.zw * currentSize * (vTex.y-0.5);\n"
    "            highp vec2 xRotatedDeform;\n"
    "            xRotatedDeform.x = trigCalcs.x*xDeform.x - trigCalcs.y*xDeform.y;\n"
    "            xRotatedDeform.y = trigCalcs.y*xDeform.x + trigCalcs.x*xDeform.y;\n"
    "            highp vec2 yRotatedDeform;\n"
    "            yRotatedDeform.x = trigCalcs.x*yDeform.x - trigCalcs.y*yDeform.y;\n"
    "            yRotatedDeform.y = trigCalcs.y*yDeform.x + trigCalcs.x*yDeform.y;\n"
    "            */\n"
    "            pos = vPosTex.xy\n"
    "                  + rotatedDeform.xy\n"
    "                  + rotatedDeform.zw\n"
    "                  + vVec.xy * t * vData.y         // apply speed\n"
    "                  + 0.5 * vVec.zw * pow(t * vData.y, 2.); // apply acceleration\n"
    "#else\n"
    "            pos = vPos\n"
    "                  + vVec.xy * t * vData.y         // apply speed vector..\n"
    "                  + 0.5 * vVec.zw * pow(t * vData.y, 2.);\n"
    "            gl_PointSize = currentSize;\n"
    "#endif\n"
    "            gl_Position = qt_Matrix * vec4(pos.x, pos.y, 0, 1);\n"
    "\n"
    "#if defined(COLOR)\n"
    "            fColor = vColor * fade;\n"
    "#else\n"
    "            fFade = fade;\n"
    "#endif\n"
    "#if defined(TABLE)\n"
    "            tt.x = t;\n"
    "#endif\n"
    "        }\n"
    "    }\n"
    "}\n";

static const char fragmentShaderCode[] =
    "uniform sampler2D texture;\n"
    "uniform lowp float qt_Opacity;\n"
    "\n"
    "#if defined(SPRITE)\n"
    "varying highp vec4 fTexS;\n"
    "#elif defined(DEFORM)\n"
    "varying highp vec2 fTex;\n"
    "#endif\n"
    "#if defined(COLOR)\n"
    "varying lowp vec4 fColor;\n"
    "#else\n"
    "varying lowp float fFade;\n"
    "#endif\n"
    "#if defined(TABLE)\n"
    "varying lowp vec2 tt;\n"
    "uniform sampler2D colortable;\n"
    "#endif\n"
    "\n"
    "void main() {\n"
    "#if defined(SPRITE)\n"
    "    gl_FragColor = mix(texture2D(texture, fTexS.xy), texture2D(texture, fTexS.zw), tt.y)\n"
    "            * fColor\n"
    "            * texture2D(colortable, tt)\n"
    "            * qt_Opacity;\n"
    "#elif defined(TABLE)\n"
    "    gl_FragColor = texture2D(texture, fTex)\n"
    "            * fColor\n"
    "            * texture2D(colortable, tt)\n"
    "            * qt_Opacity;\n"
    "#elif defined(DEFORM)\n"
    "    gl_FragColor = (texture2D(texture, fTex)) * fColor * qt_Opacity;\n"
    "#elif defined(COLOR)\n"
    "    gl_FragColor = (texture2D(texture, gl_PointCoord)) * fColor * qt_Opacity;\n"
    "#else\n"
    "    gl_FragColor = texture2D(texture, gl_PointCoord) * (fFade * qt_Opacity);\n"
    "#endif\n"
    "}\n";

const qreal CONV = 0.017453292519943295;
class ImageMaterialData
{
    public:
    ImageMaterialData()
        : texture(0), colorTable(0)
    {}

    ~ImageMaterialData(){
        delete texture;
        delete colorTable;
    }

    QSGTexture *texture;
    QSGTexture *colorTable;
    float sizeTable[UNIFORM_ARRAY_SIZE];
    float opacityTable[UNIFORM_ARRAY_SIZE];

    qreal timestamp;
    qreal entry;
    QSizeF animSheetSize;
};

class TabledMaterialData : public ImageMaterialData {};
class TabledMaterial : public QSGSimpleMaterialShader<TabledMaterialData>
{
    QSG_DECLARE_SIMPLE_SHADER(TabledMaterial, TabledMaterialData)

public:
    TabledMaterial()
    {
        m_vertex_code = QByteArray(SHADER_DEFINES)
            + QByteArray("#define TABLE\n#define DEFORM\n#define COLOR\n")
            + vertexShaderCode;

        m_fragment_code = QByteArray(SHADER_DEFINES)
            + QByteArray("#define TABLE\n#define DEFORM\n#define COLOR\n")
            + fragmentShaderCode;

        Q_ASSERT(!m_vertex_code.isNull());
        Q_ASSERT(!m_fragment_code.isNull());
    }

    const char *vertexShader() const { return m_vertex_code.constData(); }
    const char *fragmentShader() const { return m_fragment_code.constData(); }

    QList<QByteArray> attributes() const {
        return QList<QByteArray>() << "vPosTex" << "vData" << "vVec"
            << "vColor" << "vDeformVec" << "vRotation";
    };

    void initialize() {
        QSGSimpleMaterialShader<TabledMaterialData>::initialize();
        program()->bind();
        program()->setUniformValue("texture", 0);
        program()->setUniformValue("colortable", 1);
        glFuncs = QOpenGLContext::currentContext()->functions();
        m_timestamp_id = program()->uniformLocation("timestamp");
        m_entry_id = program()->uniformLocation("entry");
        m_sizetable_id = program()->uniformLocation("sizetable");
        m_opacitytable_id = program()->uniformLocation("opacitytable");
    }

    void updateState(const TabledMaterialData* d, const TabledMaterialData*) {
        glFuncs->glActiveTexture(GL_TEXTURE1);
        d->colorTable->bind();

        glFuncs->glActiveTexture(GL_TEXTURE0);
        d->texture->bind();

        program()->setUniformValue(m_timestamp_id, (float) d->timestamp);
        program()->setUniformValue(m_entry_id, (float) d->entry);
        program()->setUniformValueArray(m_sizetable_id, (float*) d->sizeTable, UNIFORM_ARRAY_SIZE, 1);
        program()->setUniformValueArray(m_opacitytable_id, (float*) d->opacityTable, UNIFORM_ARRAY_SIZE, 1);
    }

    int m_entry_id;
    int m_timestamp_id;
    int m_sizetable_id;
    int m_opacitytable_id;
    QByteArray m_vertex_code;
    QByteArray m_fragment_code;
    QOpenGLFunctions* glFuncs;
};

class DeformableMaterialData : public ImageMaterialData {};
class DeformableMaterial : public QSGSimpleMaterialShader<DeformableMaterialData>
{
    QSG_DECLARE_SIMPLE_SHADER(DeformableMaterial, DeformableMaterialData)

public:
    DeformableMaterial()
    {
        m_vertex_code = QByteArray(SHADER_DEFINES)
            + QByteArray("#define DEFORM\n#define COLOR\n")
            + vertexShaderCode;

        m_fragment_code = QByteArray(SHADER_DEFINES)
            + QByteArray("#define DEFORM\n#define COLOR\n")
            + fragmentShaderCode;

        Q_ASSERT(!m_vertex_code.isNull());
        Q_ASSERT(!m_fragment_code.isNull());
    }

    const char *vertexShader() const { return m_vertex_code.constData(); }
    const char *fragmentShader() const { return m_fragment_code.constData(); }

    QList<QByteArray> attributes() const {
        return QList<QByteArray>() << "vPosTex" << "vData" << "vVec"
            << "vColor" << "vDeformVec" << "vRotation";
    };

    void initialize() {
        QSGSimpleMaterialShader<DeformableMaterialData>::initialize();
        program()->bind();
        program()->setUniformValue("texture", 0);
        glFuncs = QOpenGLContext::currentContext()->functions();
        m_timestamp_id = program()->uniformLocation("timestamp");
        m_entry_id = program()->uniformLocation("entry");
    }

    void updateState(const DeformableMaterialData* d, const DeformableMaterialData*) {
        glFuncs->glActiveTexture(GL_TEXTURE0);
        d->texture->bind();

        program()->setUniformValue(m_timestamp_id, (float) d->timestamp);
        program()->setUniformValue(m_entry_id, (float) d->entry);
    }

    int m_entry_id;
    int m_timestamp_id;
    QByteArray m_vertex_code;
    QByteArray m_fragment_code;
    QOpenGLFunctions* glFuncs;
};

class SpriteMaterialData : public ImageMaterialData {};
class SpriteMaterial : public QSGSimpleMaterialShader<SpriteMaterialData>
{
    QSG_DECLARE_SIMPLE_SHADER(SpriteMaterial, SpriteMaterialData)

public:
    SpriteMaterial()
    {
        m_vertex_code = QByteArray(SHADER_DEFINES)
            + QByteArray("#define SPRITE\n#define TABLE\n#define DEFORM\n#define COLOR\n")
            + vertexShaderCode;

        m_fragment_code = QByteArray(SHADER_DEFINES)
            + QByteArray("#define SPRITE\n#define TABLE\n#define DEFORM\n#define COLOR\n")
            + fragmentShaderCode;

        Q_ASSERT(!m_vertex_code.isNull());
        Q_ASSERT(!m_fragment_code.isNull());
    }

    const char *vertexShader() const { return m_vertex_code.constData(); }
    const char *fragmentShader() const { return m_fragment_code.constData(); }

    QList<QByteArray> attributes() const {
        return QList<QByteArray>() << "vPosTex" << "vData" << "vVec"
            << "vColor" << "vDeformVec" << "vRotation" << "vAnimData" << "vAnimPos";
    };

    void initialize() {
        QSGSimpleMaterialShader<SpriteMaterialData>::initialize();
        program()->bind();
        program()->setUniformValue("texture", 0);
        program()->setUniformValue("colortable", 1);
        glFuncs = QOpenGLContext::currentContext()->functions();
        m_timestamp_id = program()->uniformLocation("timestamp");
        m_animsize_id = program()->uniformLocation("animSheetSize");
        m_entry_id = program()->uniformLocation("entry");
        m_sizetable_id = program()->uniformLocation("sizetable");
        m_opacitytable_id = program()->uniformLocation("opacitytable");
    }

    void updateState(const SpriteMaterialData* d, const SpriteMaterialData*) {
        glFuncs->glActiveTexture(GL_TEXTURE1);
        d->colorTable->bind();

        // make sure we end by setting GL_TEXTURE0 as active texture
        glFuncs->glActiveTexture(GL_TEXTURE0);
        d->texture->bind();

        program()->setUniformValue(m_timestamp_id, (float) d->timestamp);
        program()->setUniformValue(m_animsize_id, d->animSheetSize);
        program()->setUniformValue(m_entry_id, (float) d->entry);
        program()->setUniformValueArray(m_sizetable_id, (float*) d->sizeTable, 64, 1);
        program()->setUniformValueArray(m_opacitytable_id, (float*) d->opacityTable, UNIFORM_ARRAY_SIZE, 1);
    }

    int m_timestamp_id;
    int m_animsize_id;
    int m_entry_id;
    int m_sizetable_id;
    int m_opacitytable_id;
    QByteArray m_vertex_code;
    QByteArray m_fragment_code;
    QOpenGLFunctions* glFuncs;
};

class ColoredMaterialData : public ImageMaterialData {};
class ColoredMaterial : public QSGSimpleMaterialShader<ColoredMaterialData>
{
    QSG_DECLARE_SIMPLE_SHADER(ColoredMaterial, ColoredMaterialData)

public:
    ColoredMaterial()
    {
        m_vertex_code = QByteArray(SHADER_DEFINES)
            + QByteArray("#define COLOR\n")
            + vertexShaderCode;

        m_fragment_code = QByteArray(SHADER_DEFINES)
            + QByteArray("#define COLOR\n")
            + fragmentShaderCode;

        Q_ASSERT(!m_vertex_code.isNull());
        Q_ASSERT(!m_fragment_code.isNull());
    }

    const char *vertexShader() const { return m_vertex_code.constData(); }
    const char *fragmentShader() const { return m_fragment_code.constData(); }

    void activate() {
        QSGSimpleMaterialShader<ColoredMaterialData>::activate();
#if !defined(QT_OPENGL_ES_2) && !defined(Q_OS_WIN)
        glEnable(GL_POINT_SPRITE);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
    }

    void deactivate() {
        QSGSimpleMaterialShader<ColoredMaterialData>::deactivate();
#if !defined(QT_OPENGL_ES_2) && !defined(Q_OS_WIN)
        glDisable(GL_POINT_SPRITE);
        glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
    }

    QList<QByteArray> attributes() const {
        return QList<QByteArray>() << "vPos" << "vData" << "vVec" << "vColor";
    }

    void initialize() {
        QSGSimpleMaterialShader<ColoredMaterialData>::initialize();
        program()->bind();
        program()->setUniformValue("texture", 0);
        glFuncs = QOpenGLContext::currentContext()->functions();
        m_timestamp_id = program()->uniformLocation("timestamp");
        m_entry_id = program()->uniformLocation("entry");
    }

    void updateState(const ColoredMaterialData* d, const ColoredMaterialData*) {
        glFuncs->glActiveTexture(GL_TEXTURE0);
        d->texture->bind();

        program()->setUniformValue(m_timestamp_id, (float) d->timestamp);
        program()->setUniformValue(m_entry_id, (float) d->entry);
    }

    int m_timestamp_id;
    int m_entry_id;
    QByteArray m_vertex_code;
    QByteArray m_fragment_code;
    QOpenGLFunctions* glFuncs;
};

class SimpleMaterialData : public ImageMaterialData {};
class SimpleMaterial : public QSGSimpleMaterialShader<SimpleMaterialData>
{
    QSG_DECLARE_SIMPLE_SHADER(SimpleMaterial, SimpleMaterialData)

public:
    SimpleMaterial()
    {
        m_vertex_code = QByteArray(SHADER_DEFINES)
            + vertexShaderCode;

        m_fragment_code = QByteArray(SHADER_DEFINES)
            + fragmentShaderCode;

        Q_ASSERT(!m_vertex_code.isNull());
        Q_ASSERT(!m_fragment_code.isNull());
    }

    const char *vertexShader() const { return m_vertex_code.constData(); }
    const char *fragmentShader() const { return m_fragment_code.constData(); }

    void activate() {
        QSGSimpleMaterialShader<SimpleMaterialData>::activate();
#if !defined(QT_OPENGL_ES_2) && !defined(Q_OS_WIN)
        glEnable(GL_POINT_SPRITE);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
    }

    void deactivate() {
        QSGSimpleMaterialShader<SimpleMaterialData>::deactivate();
#if !defined(QT_OPENGL_ES_2) && !defined(Q_OS_WIN)
        glDisable(GL_POINT_SPRITE);
        glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
    }

    QList<QByteArray> attributes() const {
        return QList<QByteArray>() << "vPos" << "vData" << "vVec";
    }

    void initialize() {
        QSGSimpleMaterialShader<SimpleMaterialData>::initialize();
        program()->bind();
        program()->setUniformValue("texture", 0);
        glFuncs = QOpenGLContext::currentContext()->functions();
        m_timestamp_id = program()->uniformLocation("timestamp");
        m_entry_id = program()->uniformLocation("entry");
    }

    void updateState(const SimpleMaterialData* d, const SimpleMaterialData*) {
        glFuncs->glActiveTexture(GL_TEXTURE0);
        d->texture->bind();

        program()->setUniformValue(m_timestamp_id, (float) d->timestamp);
        program()->setUniformValue(m_entry_id, (float) d->entry);
    }

    int m_timestamp_id;
    int m_entry_id;
    QByteArray m_vertex_code;
    QByteArray m_fragment_code;
    QOpenGLFunctions* glFuncs;
};

void fillUniformArrayFromImage(float* array, const QImage& img, int size)
{
    if (img.isNull()){
        for (int i=0; i<size; i++)
            array[i] = 1.0;
        return;
    }
    QImage scaled = img.scaled(size,1);
    for (int i=0; i<size; i++)
        array[i] = qAlpha(scaled.pixel(i,0))/255.0;
}

/*!
    \qmlclass ImageParticle QQuickImageParticle
    \inqmlmodule QtQuick.Particles 2
    \inherits ParticlePainter
    \brief The ImageParticle element visualizes logical particles using an image

    This element renders a logical particle as an image. The image can be
    \list
    \o colorized
    \o rotated
    \o deformed
    \o a sprite-based animation
    \endlist

    ImageParticles implictly share data on particles if multiple ImageParticles are painting
    the same logical particle group. This is broken down along the four capabilities listed
    above. So if one ImageParticle defines data for rendering the particles in one of those
    capabilities, and the other does not, then both will draw the particles the same in that
    aspect automatically. This is primarily useful when there is some random variation on
    the particle which is supposed to stay with it when switching painters. If both ImageParticles
    define how they should appear for that aspect, they diverge and each appears as it is defined.

    This sharing of data happens behind the scenes based off of whether properties were implicitly or explicitly
    set. One drawback of the current implementation is that it is only possible to reset the capabilities as a whole.
    So if you explicity set an attribute affecting color, such as redVariation, and then reset it (by setting redVariation
    to undefined), all color data will be reset and it will begin to have an implicit value of any shared color from
    other ImageParticles.
*/
/*!
    \qmlproperty url QtQuick.Particles2::ImageParticle::source

    The source image to be used.

    If the image is a sprite animation, use the sprite property instead.
*/
/*!
    \qmlproperty list<Sprite> QtQuick.Particles2::ImageParticle::sprites

    The sprite or sprites used to draw this particle.

    Note that the sprite image will be scaled to a square based on the size of
    the particle being rendered.
*/
/*!
    \qmlproperty url QtQuick.Particles2::ImageParticle::colorTable

    An image whose color will be used as a 1D texture to determine color over life. E.g. when
    the particle is halfway through its lifetime, it will have the color specified halfway
    across the image.

    This color is blended with the color property and the color of the source image.
*/
/*!
    \qmlproperty url QtQuick.Particles2::ImageParticle::sizeTable

    An image whose opacity will be used as a 1D texture to determine size over life.

    This property is expected to be removed shortly, in favor of custom easing curves to determine size over life.
*/
/*!
    \qmlproperty url QtQuick.Particles2::ImageParticle::opacityTable

    An image whose opacity will be used as a 1D texture to determine size over life.

    This property is expected to be removed shortly, in favor of custom easing curves to determine opacity over life.
*/
/*!
    \qmlproperty color QtQuick.Particles2::ImageParticle::color

    If a color is specified, the provided image will be colorized with it.

    Default is white (no change).
*/
/*!
    \qmlproperty real QtQuick.Particles2::ImageParticle::colorVariation

    This number represents the color variation applied to individual particles.
    Setting colorVariation is the same as setting redVariation, greenVariation,
    and blueVariation to the same number.

    Each channel can vary between particle by up to colorVariation from its usual color.

    Color is measured, per channel, from 0.0 to 1.0.

    Default is 0.0
*/
/*!
    \qmlproperty real QtQuick.Particles2::ImageParticle::redVariation
    The variation in the red color channel between particles.

    Color is measured, per channel, from 0.0 to 1.0.

    Default is 0.0
*/
/*!
    \qmlproperty real QtQuick.Particles2::ImageParticle::greenVariation
    The variation in the green color channel between particles.

    Color is measured, per channel, from 0.0 to 1.0.

    Default is 0.0
*/
/*!
    \qmlproperty real QtQuick.Particles2::ImageParticle::blueVariation
    The variation in the blue color channel between particles.

    Color is measured, per channel, from 0.0 to 1.0.

    Default is 0.0
*/
/*!
    \qmlproperty real QtQuick.Particles2::ImageParticle::alpha
    An alpha to be applied to the image. This value is multiplied by the value in
    the image, and the value in the color property.

    Particles have additive blending, so lower alpha on single particles leads
    to stronger effects when multiple particles overlap.

    Alpha is measured from 0.0 to 1.0.

    Default is 1.0
*/
/*!
    \qmlproperty real QtQuick.Particles2::ImageParticle::alphaVariation
    The variation in the alpha channel between particles.

    Alpha is measured from 0.0 to 1.0.

    Default is 0.0
*/
/*!
    \qmlproperty real QtQuick.Particles2::ImageParticle::rotation

    If set the image will be rotated by this many degrees before it is drawn.

    The particle coordinates are not transformed.
*/
/*!
    \qmlproperty real QtQuick.Particles2::ImageParticle::rotationVariation

    If set the rotation of individual particles will vary by up to this much
    between particles.

*/
/*!
    \qmlproperty real QtQuick.Particles2::ImageParticle::rotationSpeed

    If set particles will rotate at this speed in degrees/second.
*/
/*!
    \qmlproperty real QtQuick.Particles2::ImageParticle::rotationSpeedVariation

    If set the rotationSpeed of individual particles will vary by up to this much
    between particles.

*/
/*!
    \qmlproperty bool QtQuick.Particles2::ImageParticle::autoRotation

    If set to true then a rotation will be applied on top of the particles rotation, so
    that it faces the direction of travel. So to face away from the direction of travel,
    set autoRotation to true and rotation to 180.

    Default is false
*/
/*!
    \qmlproperty StochasticDirection QtQuick.Particles2::ImageParticle::xVector

    Allows you to deform the particle image when drawn. The rectangular image will
    be deformed so that the horizontal sides are in the shape of this vector instead
    of (1,0).
*/
/*!
    \qmlproperty StochasticDirection QtQuick.Particles2::ImageParticle::yVector

    Allows you to deform the particle image when drawn. The rectangular image will
    be deformed so that the vertical sides are in the shape of this vector instead
    of (0,1).
*/
/*!
    \qmlproperty EntryEffect QtQuick.Particles2::ImageParticle::entryEffect

    This property provides basic and cheap entrance and exit effects for the particles.
    For fine-grained control, see sizeTable and opacityTable.

    Acceptable values are
    \list
    \o None: Particles just appear and disappear.
    \o Fade: Particles fade in from 0 opacity at the start of their life, and fade out to 0 at the end.
    \o Scale: Particles scale in from 0 size at the start of their life, and scale back to 0 at the end.
    \endlist

    Default value is Fade.
*/
/*!
    \qmlproperty bool QtQuick.Particles2::ImageParticle::spritesInterpolate

    If set to true, sprite particles will interpolate between sprite frames each rendered frame, making
    the sprites look smoother.

    Default is true.
*/


QQuickImageParticle::QQuickImageParticle(QQuickItem* parent)
    : QQuickParticlePainter(parent)
    , m_color_variation(0.0)
    , m_rootNode(0)
    , m_material(0)
    , m_alphaVariation(0.0)
    , m_alpha(1.0)
    , m_redVariation(0.0)
    , m_greenVariation(0.0)
    , m_blueVariation(0.0)
    , m_rotation(0)
    , m_rotationVariation(0)
    , m_rotationSpeed(0)
    , m_rotationSpeedVariation(0)
    , m_autoRotation(false)
    , m_xVector(0)
    , m_yVector(0)
    , m_spriteEngine(0)
    , m_spritesInterpolate(true)
    , m_explicitColor(false)
    , m_explicitRotation(false)
    , m_explicitDeformation(false)
    , m_explicitAnimation(false)
    , m_bloat(false)
    , perfLevel(Unknown)
    , m_lastLevel(Unknown)
    , m_debugMode(false)
    , m_entryEffect(Fade)
{
    setFlag(ItemHasContents);
    m_debugMode = qmlParticlesDebug();
}

QQuickImageParticle::~QQuickImageParticle()
{
}

QDeclarativeListProperty<QQuickSprite> QQuickImageParticle::sprites()
{
    return QDeclarativeListProperty<QQuickSprite>(this, &m_sprites, spriteAppend, spriteCount, spriteAt, spriteClear);
}

void QQuickImageParticle::setImage(const QUrl &image)
{
    if (image == m_image_name)
        return;
    m_image_name = image;
    emit imageChanged();
    reset();
}


void QQuickImageParticle::setColortable(const QUrl &table)
{
    if (table == m_colortable_name)
        return;
    m_colortable_name = table;
    emit colortableChanged();
    reset();
}

void QQuickImageParticle::setSizetable(const QUrl &table)
{
    if (table == m_sizetable_name)
        return;
    m_sizetable_name = table;
    emit sizetableChanged();
    reset();
}

void QQuickImageParticle::setOpacitytable(const QUrl &table)
{
    if (table == m_opacitytable_name)
        return;
    m_opacitytable_name = table;
    emit opacitytableChanged();
    reset();
}

void QQuickImageParticle::setColor(const QColor &color)
{
    if (color == m_color)
        return;
    m_color = color;
    emit colorChanged();
    m_explicitColor = true;
    if (perfLevel < Colored)
        reset();
}

void QQuickImageParticle::setColorVariation(qreal var)
{
    if (var == m_color_variation)
        return;
    m_color_variation = var;
    emit colorVariationChanged();
    m_explicitColor = true;
    if (perfLevel < Colored)
        reset();
}

void QQuickImageParticle::setAlphaVariation(qreal arg)
{
    if (m_alphaVariation != arg) {
        m_alphaVariation = arg;
        emit alphaVariationChanged(arg);
    }
    m_explicitColor = true;
    if (perfLevel < Colored)
        reset();
}

void QQuickImageParticle::setAlpha(qreal arg)
{
    if (m_alpha != arg) {
        m_alpha = arg;
        emit alphaChanged(arg);
    }
    m_explicitColor = true;
    if (perfLevel < Colored)
        reset();
}

void QQuickImageParticle::setRedVariation(qreal arg)
{
    if (m_redVariation != arg) {
        m_redVariation = arg;
        emit redVariationChanged(arg);
    }
    m_explicitColor = true;
    if (perfLevel < Colored)
        reset();
}

void QQuickImageParticle::setGreenVariation(qreal arg)
{
    if (m_greenVariation != arg) {
        m_greenVariation = arg;
        emit greenVariationChanged(arg);
    }
    m_explicitColor = true;
    if (perfLevel < Colored)
        reset();
}

void QQuickImageParticle::setBlueVariation(qreal arg)
{
    if (m_blueVariation != arg) {
        m_blueVariation = arg;
        emit blueVariationChanged(arg);
    }
    m_explicitColor = true;
    if (perfLevel < Colored)
        reset();
}

void QQuickImageParticle::setRotation(qreal arg)
{
    if (m_rotation != arg) {
        m_rotation = arg;
        emit rotationChanged(arg);
    }
    m_explicitRotation = true;
    if (perfLevel < Deformable)
        reset();
}

void QQuickImageParticle::setRotationVariation(qreal arg)
{
    if (m_rotationVariation != arg) {
        m_rotationVariation = arg;
        emit rotationVariationChanged(arg);
    }
    m_explicitRotation = true;
    if (perfLevel < Deformable)
        reset();
}

void QQuickImageParticle::setRotationSpeed(qreal arg)
{
    if (m_rotationSpeed != arg) {
        m_rotationSpeed = arg;
        emit rotationSpeedChanged(arg);
    }
    m_explicitRotation = true;
    if (perfLevel < Deformable)
        reset();
}

void QQuickImageParticle::setRotationSpeedVariation(qreal arg)
{
    if (m_rotationSpeedVariation != arg) {
        m_rotationSpeedVariation = arg;
        emit rotationSpeedVariationChanged(arg);
    }
    m_explicitRotation = true;
    if (perfLevel < Deformable)
        reset();
}

void QQuickImageParticle::setAutoRotation(bool arg)
{
    if (m_autoRotation != arg) {
        m_autoRotation = arg;
        emit autoRotationChanged(arg);
    }
    m_explicitRotation = true;
    if (perfLevel < Deformable)
        reset();
}

void QQuickImageParticle::setXVector(QQuickDirection* arg)
{
    if (m_xVector != arg) {
        m_xVector = arg;
        emit xVectorChanged(arg);
    }
    m_explicitDeformation = true;
    if (perfLevel < Deformable)
        reset();
}

void QQuickImageParticle::setYVector(QQuickDirection* arg)
{
    if (m_yVector != arg) {
        m_yVector = arg;
        emit yVectorChanged(arg);
    }
    m_explicitDeformation = true;
    if (perfLevel < Deformable)
        reset();
}

void QQuickImageParticle::setSpritesInterpolate(bool arg)
{
    if (m_spritesInterpolate != arg) {
        m_spritesInterpolate = arg;
        emit spritesInterpolateChanged(arg);
    }
}

void QQuickImageParticle::setBloat(bool arg)
{
    if (m_bloat != arg) {
        m_bloat = arg;
        emit bloatChanged(arg);
    }
    if (perfLevel < 9999)
        reset();
}

void QQuickImageParticle::setEntryEffect(EntryEffect arg)
{
    if (m_entryEffect != arg) {
        m_entryEffect = arg;
        if (m_material)
            getState<ImageMaterialData>(m_material)->entry = (qreal) m_entryEffect;
        emit entryEffectChanged(arg);
    }
}

void QQuickImageParticle::resetColor()
{
    m_explicitColor = false;
    foreach (const QString &str, m_groups)
        foreach (QQuickParticleData* d, m_system->groupData[m_system->groupIds[str]]->data)
            if (d->colorOwner == this)
                d->colorOwner = 0;
    m_color = QColor();
    m_color_variation = 0.0f;
    m_redVariation = 0.0f;
    m_blueVariation = 0.0f;
    m_greenVariation = 0.0f;
    m_alpha = 1.0f;
    m_alphaVariation = 0.0f;
}

void QQuickImageParticle::resetRotation()
{
    m_explicitRotation = false;
    foreach (const QString &str, m_groups)
        foreach (QQuickParticleData* d, m_system->groupData[m_system->groupIds[str]]->data)
            if (d->rotationOwner == this)
                d->rotationOwner = 0;
    m_rotation = 0;
    m_rotationVariation = 0;
    m_rotationSpeed = 0;
    m_rotationSpeedVariation = 0;
    m_autoRotation = false;
}

void QQuickImageParticle::resetDeformation()
{
    m_explicitDeformation = false;
    foreach (const QString &str, m_groups)
        foreach (QQuickParticleData* d, m_system->groupData[m_system->groupIds[str]]->data)
            if (d->deformationOwner == this)
                d->deformationOwner = 0;
    if (m_xVector)
        delete m_xVector;
    if (m_yVector)
        delete m_yVector;
    m_xVector = 0;
    m_yVector = 0;
}

void QQuickImageParticle::reset()
{
    QQuickParticlePainter::reset();
    m_pleaseReset = true;
    update();
}

void QQuickImageParticle::createEngine()
{
    if (m_spriteEngine)
        delete m_spriteEngine;
    if (m_sprites.count()) {
        m_spriteEngine = new QQuickSpriteEngine(m_sprites, this);
        connect(m_spriteEngine, SIGNAL(stateChanged(int)),
                this, SLOT(spriteAdvance(int)));
        m_explicitAnimation = true;
    } else {
        m_spriteEngine = 0;
        m_explicitAnimation = false;
    }
    reset();
}

static QSGGeometry::Attribute SimpleParticle_Attributes[] = {
    QSGGeometry::Attribute::create(0, 2, GL_FLOAT, true),             // Position
    QSGGeometry::Attribute::create(1, 4, GL_FLOAT),             // Data
    QSGGeometry::Attribute::create(2, 4, GL_FLOAT)             // Vectors
};

static QSGGeometry::AttributeSet SimpleParticle_AttributeSet =
{
    3, // Attribute Count
    ( 2 + 4 + 4 ) * sizeof(float),
    SimpleParticle_Attributes
};

static QSGGeometry::Attribute ColoredParticle_Attributes[] = {
    QSGGeometry::Attribute::create(0, 2, GL_FLOAT, true),             // Position
    QSGGeometry::Attribute::create(1, 4, GL_FLOAT),             // Data
    QSGGeometry::Attribute::create(2, 4, GL_FLOAT),             // Vectors
    QSGGeometry::Attribute::create(3, 4, GL_UNSIGNED_BYTE),     // Colors
};

static QSGGeometry::AttributeSet ColoredParticle_AttributeSet =
{
    4, // Attribute Count
    ( 2 + 4 + 4 ) * sizeof(float) + 4 * sizeof(uchar),
    ColoredParticle_Attributes
};

static QSGGeometry::Attribute DeformableParticle_Attributes[] = {
    QSGGeometry::Attribute::create(0, 4, GL_FLOAT),             // Position & TexCoord
    QSGGeometry::Attribute::create(1, 4, GL_FLOAT),             // Data
    QSGGeometry::Attribute::create(2, 4, GL_FLOAT),             // Vectors
    QSGGeometry::Attribute::create(3, 4, GL_UNSIGNED_BYTE),     // Colors
    QSGGeometry::Attribute::create(4, 4, GL_FLOAT),             // DeformationVectors
    QSGGeometry::Attribute::create(5, 3, GL_FLOAT),             // Rotation
};

static QSGGeometry::AttributeSet DeformableParticle_AttributeSet =
{
    6, // Attribute Count
    (4 + 4 + 4 + 4 + 3) * sizeof(float) + 4 * sizeof(uchar),
    DeformableParticle_Attributes
};

static QSGGeometry::Attribute SpriteParticle_Attributes[] = {
    QSGGeometry::Attribute::create(0, 4, GL_FLOAT),       // Position & TexCoord
    QSGGeometry::Attribute::create(1, 4, GL_FLOAT),             // Data
    QSGGeometry::Attribute::create(2, 4, GL_FLOAT),             // Vectors
    QSGGeometry::Attribute::create(3, 4, GL_UNSIGNED_BYTE),     // Colors
    QSGGeometry::Attribute::create(4, 4, GL_FLOAT),             // DeformationVectors
    QSGGeometry::Attribute::create(5, 3, GL_FLOAT),             // Rotation
    QSGGeometry::Attribute::create(6, 4, GL_FLOAT),             // Anim Data
    QSGGeometry::Attribute::create(7, 4, GL_FLOAT)              // Anim Pos
};

static QSGGeometry::AttributeSet SpriteParticle_AttributeSet =
{
    8, // Attribute Count
    (4 + 4 + 4 + 4 + 4 + 4 + 3) * sizeof(float) + 4 * sizeof(uchar),
    SpriteParticle_Attributes
};

void QQuickImageParticle::clearShadows()
{
    foreach (const QVector<QQuickParticleData*> data, m_shadowData)
        qDeleteAll(data);
    m_shadowData.clear();
}

//Only call if you need to, may initialize the whole array first time
QQuickParticleData* QQuickImageParticle::getShadowDatum(QQuickParticleData* datum)
{
    QQuickParticleGroupData* gd = m_system->groupData[datum->group];
    if (!m_shadowData.contains(datum->group)) {
        QVector<QQuickParticleData*> data;
        for (int i=0; i<gd->size(); i++){
            QQuickParticleData* datum = new QQuickParticleData(m_system);
            *datum = *(gd->data[i]);
            data << datum;
        }
        m_shadowData.insert(datum->group, data);
    }
    //### If dynamic resize is added, remember to potentially resize the shadow data on out-of-bounds access request

    return m_shadowData[datum->group][datum->index];
}

QSGGeometryNode* QQuickImageParticle::buildParticleNodes()
{
#ifdef QT_OPENGL_ES_2
    if (m_count * 4 > 0xffff) {
        printf("ImageParticle: Too many particles - maximum 16,000 per ImageParticle.\n");//ES 2 vertex count limit is ushort
        return 0;
    }
#endif

    if (count() <= 0)
        return 0;

    if (m_sprites.count() || m_bloat) {
        perfLevel = Sprites;
    } else if (!m_colortable_name.isEmpty() || !m_sizetable_name.isEmpty()
               || !m_opacitytable_name.isEmpty()) {
        perfLevel = Tabled;
    } else if (m_autoRotation || m_rotation || m_rotationVariation
               || m_rotationSpeed || m_rotationSpeedVariation
               || m_xVector || m_yVector) {
        perfLevel = Deformable;
    } else if (m_alphaVariation || m_alpha != 1.0 || m_color.isValid() || m_color_variation
               || m_redVariation || m_blueVariation || m_greenVariation) {
        perfLevel = Colored;
    } else {
        perfLevel = Simple;
    }

    foreach (const QString &str, m_groups){//For sharing higher levels, need to have highest used so it renders
        int gIdx = m_system->groupIds[str];
        foreach (QQuickParticlePainter* p, m_system->groupData[gIdx]->painters){
            QQuickImageParticle* other = qobject_cast<QQuickImageParticle*>(p);
            if (other){
                if (other->perfLevel > perfLevel) {
                    if (other->perfLevel >= Tabled){//Deformable is the highest level needed for this, anything higher isn't shared (or requires your own sprite)
                        if (perfLevel < Deformable)
                            perfLevel = Deformable;
                    } else {
                        perfLevel = other->perfLevel;
                    }
                } else if (other->perfLevel < perfLevel) {
                    other->reset();
                }
            }
        }
    }

    if (perfLevel >= Colored  && !m_color.isValid())
        m_color = QColor(Qt::white);//Hidden default, but different from unset

    QImage image;
    if (perfLevel >= Sprites){
        if (!m_spriteEngine) {
            qWarning() << "ImageParticle: No sprite engine...";
            return 0;
        }
        image = m_spriteEngine->assembledImage();
        if (image.isNull())//Warning is printed in engine
            return 0;
    } else {
        image = QImage(m_image_name.toLocalFile());
        if (image.isNull()) {
            printf("ImageParticle: loading image failed '%s'\n", qPrintable(m_image_name.toLocalFile()));
            return 0;
        }
    }

    clearShadows();
    if (m_material)
        m_material = 0;

    //Setup material
    QImage colortable;
    QImage sizetable;
    QImage opacitytable;
    switch (perfLevel) {//Fallthrough intended
    case Sprites:
        m_material = SpriteMaterial::createMaterial();
        getState<ImageMaterialData>(m_material)->animSheetSize = QSizeF(image.size());
        m_spriteEngine->setCount(m_count);
    case Tabled:
        if (!m_material)
            m_material = TabledMaterial::createMaterial();
        colortable = QImage(m_colortable_name.toLocalFile());
        sizetable = QImage(m_sizetable_name.toLocalFile());
        opacitytable = QImage(m_opacitytable_name.toLocalFile());
        if (colortable.isNull()){
            colortable = QImage(1,1,QImage::Format_ARGB32);
            colortable.fill(Qt::white);
        }
        Q_ASSERT(!colortable.isNull());
        getState<ImageMaterialData>(m_material)->colorTable = QSGPlainTexture::fromImage(colortable);
        fillUniformArrayFromImage(getState<ImageMaterialData>(m_material)->sizeTable, sizetable, UNIFORM_ARRAY_SIZE);
        fillUniformArrayFromImage(getState<ImageMaterialData>(m_material)->opacityTable, opacitytable, UNIFORM_ARRAY_SIZE);
    case Deformable:
        if (!m_material)
            m_material = DeformableMaterial::createMaterial();
    case Colored:
        if (!m_material)
            m_material = ColoredMaterial::createMaterial();
    default://Also Simple
        if (!m_material)
            m_material = SimpleMaterial::createMaterial();
        getState<ImageMaterialData>(m_material)->texture = QSGPlainTexture::fromImage(image);
        getState<ImageMaterialData>(m_material)->texture->setFiltering(QSGTexture::Linear);
        getState<ImageMaterialData>(m_material)->entry = (qreal) m_entryEffect;
        m_material->setFlag(QSGMaterial::Blending);
    }

    foreach (const QString &str, m_groups){
        int gIdx = m_system->groupIds[str];
        int count = m_system->groupData[gIdx]->size();
        QSGGeometryNode* node = new QSGGeometryNode();
        node->setMaterial(m_material);
        node->markDirty(QSGNode::DirtyMaterial);

        m_nodes.insert(gIdx, node);
        m_idxStarts.insert(gIdx, m_lastIdxStart);
        m_startsIdx.append(qMakePair<int,int>(m_lastIdxStart, gIdx));
        m_lastIdxStart += count;

        //Create Particle Geometry
        int vCount = count * 4;
        int iCount = count * 6;

        QSGGeometry *g;
        if (perfLevel == Sprites)
            g = new QSGGeometry(SpriteParticle_AttributeSet, vCount, iCount);
        else if (perfLevel == Tabled)
            g = new QSGGeometry(DeformableParticle_AttributeSet, vCount, iCount);
        else if (perfLevel == Deformable)
            g = new QSGGeometry(DeformableParticle_AttributeSet, vCount, iCount);
        else if (perfLevel == Colored)
            g = new QSGGeometry(ColoredParticle_AttributeSet, count, 0);
        else //Simple
            g = new QSGGeometry(SimpleParticle_AttributeSet, count, 0);

        node->setGeometry(g);
        if (perfLevel <= Colored){
            g->setDrawingMode(GL_POINTS);
            if (m_debugMode){
                GLfloat pointSizeRange[2];
                glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, pointSizeRange);
                qDebug() << "Using point sprites, GL_ALIASED_POINT_SIZE_RANGE " <<pointSizeRange[0] << ":" << pointSizeRange[1];
            }
        }else
            g->setDrawingMode(GL_TRIANGLES);

        for (int p=0; p < count; ++p)
            commit(gIdx, p);//commit sets geometry for the node, has its own perfLevel switch

        if (perfLevel == Sprites)
            initTexCoords<SpriteVertex>((SpriteVertex*)g->vertexData(), vCount);
        else if (perfLevel == Tabled)
            initTexCoords<DeformableVertex>((DeformableVertex*)g->vertexData(), vCount);
        else if (perfLevel == Deformable)
            initTexCoords<DeformableVertex>((DeformableVertex*)g->vertexData(), vCount);

        if (perfLevel > Colored){
            quint16 *indices = g->indexDataAsUShort();
            for (int i=0; i < count; ++i) {
                int o = i * 4;
                indices[0] = o;
                indices[1] = o + 1;
                indices[2] = o + 2;
                indices[3] = o + 1;
                indices[4] = o + 3;
                indices[5] = o + 2;
                indices += 6;
            }
        }

    }

    foreach (QSGGeometryNode* node, m_nodes){
        if (node == *(m_nodes.begin()))
            node->setFlag(QSGGeometryNode::OwnsMaterial);//Root node owns the material for memory management purposes
        else
            (*(m_nodes.begin()))->appendChildNode(node);
    }

    return *(m_nodes.begin());
}

QSGNode *QQuickImageParticle::updatePaintNode(QSGNode *, UpdatePaintNodeData *)
{
    if (m_pleaseReset){
        m_lastLevel = perfLevel;

        delete m_rootNode;//Automatically deletes children, and SG manages material lifetime
        m_rootNode = 0;
        m_nodes.clear();

        m_idxStarts.clear();
        m_startsIdx.clear();
        m_lastIdxStart = 0;

        m_material = 0;

        m_pleaseReset = false;
    }

    if (m_system && m_system->isRunning() && !m_system->isPaused()){
        prepareNextFrame();
        if (m_rootNode) {
            update();
            foreach (QSGGeometryNode* node, m_nodes)
                node->markDirty(QSGNode::DirtyGeometry);
        }
    }

    return m_rootNode;
}

void QQuickImageParticle::prepareNextFrame()
{
    if (m_rootNode == 0){//TODO: Staggered loading (as emitted)
        m_rootNode = buildParticleNodes();
        if (m_rootNode == 0)
            return;
        if (m_debugMode) {
            qDebug() << "QQuickImageParticle Feature level: " << perfLevel;
            qDebug() << "QQuickImageParticle Nodes: ";
            int count = 0;
            foreach (int i, m_nodes.keys()) {
                qDebug() << "Group " << i << " (" << m_system->groupData[i]->size() << " particles)";
                count += m_system->groupData[i]->size();
            }
            qDebug() << "Total count: " << count;
        }
    }
    qint64 timeStamp = m_system->systemSync(this);

    qreal time = timeStamp / 1000.;

    switch (perfLevel){//Fall-through intended
    case Sprites:
        //Advance State
        m_spriteEngine->updateSprites(timeStamp);
    case Tabled:
    case Deformable:
    case Colored:
    case Simple:
    default: //Also Simple
        getState<ImageMaterialData>(m_material)->timestamp = time;
        break;
    }

    foreach (QSGGeometryNode* node, m_nodes)
        node->markDirty(QSGNode::DirtyMaterial);
}

void QQuickImageParticle::spriteAdvance(int spriteIdx)
{
    if (!m_startsIdx.count())//Probably overly defensive
        return;

    int gIdx = -1;
    int i;
    for (i = 0; i<m_startsIdx.count(); i++) {
        if (spriteIdx < m_startsIdx[i].first) {
            gIdx = m_startsIdx[i-1].second;
            break;
        }
    }
    if (gIdx == -1)
        gIdx = m_startsIdx[i-1].second;
    int pIdx = spriteIdx - m_startsIdx[i-1].first;

    QQuickParticleData* datum = m_system->groupData[gIdx]->data[pIdx];
    QQuickParticleData* d = (datum->animationOwner == this ? datum : getShadowDatum(datum));

    d->animIdx = m_spriteEngine->spriteState(spriteIdx);
    Vertices<SpriteVertex>* particles = (Vertices<SpriteVertex> *) m_nodes[gIdx]->geometry()->vertexData();
    Vertices<SpriteVertex> &p = particles[pIdx];
    d->animT = p.v1.animT = p.v2.animT = p.v3.animT = p.v4.animT = m_spriteEngine->spriteStart(spriteIdx)/1000.0;
    d->frameCount = p.v1.frameCount = p.v2.frameCount = p.v3.frameCount = p.v4.frameCount = m_spriteEngine->spriteFrames(spriteIdx);
    d->frameDuration = p.v1.frameDuration = p.v2.frameDuration = p.v3.frameDuration = p.v4.frameDuration = m_spriteEngine->spriteDuration(spriteIdx);
    d->animX = p.v1.animX = p.v2.animX = p.v3.animX = p.v4.animX = m_spriteEngine->spriteX(spriteIdx);
    d->animY = p.v1.animY = p.v2.animY = p.v3.animY = p.v4.animY = m_spriteEngine->spriteY(spriteIdx);
    d->animWidth = p.v1.animWidth = p.v2.animWidth = p.v3.animWidth = p.v4.animWidth = m_spriteEngine->spriteWidth(spriteIdx);
    d->animHeight = p.v1.animHeight = p.v2.animHeight = p.v3.animHeight = p.v4.animHeight = m_spriteEngine->spriteHeight(spriteIdx);
}

void QQuickImageParticle::reloadColor(const Color4ub &c, QQuickParticleData* d)
{
    d->color = c;
    //TODO: get index for reload - or make function take an index
}

void QQuickImageParticle::initialize(int gIdx, int pIdx)
{
    Color4ub color;
    QQuickParticleData* datum = m_system->groupData[gIdx]->data[pIdx];
    qreal redVariation = m_color_variation + m_redVariation;
    qreal greenVariation = m_color_variation + m_greenVariation;
    qreal blueVariation = m_color_variation + m_blueVariation;
    int spriteIdx = 0;
    if (m_spriteEngine) {
        spriteIdx = m_idxStarts[gIdx] + datum->index;
        if (spriteIdx >= m_spriteEngine->count())
            m_spriteEngine->setCount(spriteIdx+1);
    }

    float rotation;
    float rotationSpeed;
    float autoRotate;
    switch (perfLevel){//Fall-through is intended on all of them
        case Sprites:
            // Initial Sprite State
            if (m_explicitAnimation){
                if (!datum->animationOwner)
                    datum->animationOwner = this;
                QQuickParticleData* writeTo = (datum->animationOwner == this ? datum : getShadowDatum(datum));
                writeTo->animT = writeTo->t;
                //writeTo->animInterpolate = m_spritesInterpolate;
                if (m_spriteEngine){
                    m_spriteEngine->start(spriteIdx);
                    writeTo->frameCount = m_spriteEngine->spriteFrames(spriteIdx);
                    writeTo->frameDuration = m_spriteEngine->spriteDuration(spriteIdx);
                    writeTo->animX = m_spriteEngine->spriteX(spriteIdx);
                    writeTo->animY = m_spriteEngine->spriteY(spriteIdx);
                    writeTo->animWidth = m_spriteEngine->spriteWidth(spriteIdx);
                    writeTo->animHeight = m_spriteEngine->spriteHeight(spriteIdx);
                }else{
                    writeTo->frameCount = 1;
                    writeTo->frameDuration = 9999;
                    writeTo->animX = writeTo->animY = 0;
                    writeTo->animWidth = writeTo->animHeight = 1;
                }
            }
        case Tabled:
        case Deformable:
            //Initial Rotation
            if (m_explicitDeformation){
                if (!datum->deformationOwner)
                    datum->deformationOwner = this;
                if (m_xVector){
                    const QPointF &ret = m_xVector->sample(QPointF(datum->x, datum->y));
                    if (datum->deformationOwner == this) {
                        datum->xx = ret.x();
                        datum->xy = ret.y();
                    } else {
                        getShadowDatum(datum)->xx = ret.x();
                        getShadowDatum(datum)->xy = ret.y();
                    }
                }
                if (m_yVector){
                    const QPointF &ret = m_yVector->sample(QPointF(datum->x, datum->y));
                    if (datum->deformationOwner == this) {
                        datum->yx = ret.x();
                        datum->yy = ret.y();
                    } else {
                        getShadowDatum(datum)->yx = ret.x();
                        getShadowDatum(datum)->yy = ret.y();
                    }
                }
            }

            if (m_explicitRotation){
                if (!datum->rotationOwner)
                    datum->rotationOwner = this;
                rotation =
                        (m_rotation + (m_rotationVariation - 2*((qreal)rand()/RAND_MAX)*m_rotationVariation) ) * CONV;
                rotationSpeed =
                        (m_rotationSpeed + (m_rotationSpeedVariation - 2*((qreal)rand()/RAND_MAX)*m_rotationSpeedVariation) ) * CONV;
                autoRotate = m_autoRotation?1.0:0.0;
                if (datum->rotationOwner == this) {
                    datum->rotation = rotation;
                    datum->rotationSpeed = rotationSpeed;
                    datum->autoRotate = autoRotate;
                } else {
                    getShadowDatum(datum)->rotation = rotation;
                    getShadowDatum(datum)->rotationSpeed = rotationSpeed;
                    getShadowDatum(datum)->autoRotate = autoRotate;
                }
            }
        case Colored:
            //Color initialization
            // Particle color
            if (m_explicitColor) {
                if (!datum->colorOwner)
                    datum->colorOwner = this;
                color.r = m_color.red() * (1 - redVariation) + rand() % 256 * redVariation;
                color.g = m_color.green() * (1 - greenVariation) + rand() % 256 * greenVariation;
                color.b = m_color.blue() * (1 - blueVariation) + rand() % 256 * blueVariation;
                color.a = m_alpha * m_color.alpha() * (1 - m_alphaVariation) + rand() % 256 * m_alphaVariation;
                if (datum->colorOwner == this)
                    datum->color = color;
                else
                    getShadowDatum(datum)->color = color;
            }
        default:
            break;
    }
}

void QQuickImageParticle::commit(int gIdx, int pIdx)
{
    if (m_pleaseReset)
        return;
    QSGGeometryNode *node = m_nodes[gIdx];
    if (!node)
        return;
    QQuickParticleData* datum = m_system->groupData[gIdx]->data[pIdx];
    node->setFlag(QSGNode::OwnsGeometry, false);
    SpriteVertex *spriteVertices = (SpriteVertex *) node->geometry()->vertexData();
    DeformableVertex *deformableVertices = (DeformableVertex *) node->geometry()->vertexData();
    ColoredVertex *coloredVertices = (ColoredVertex *) node->geometry()->vertexData();
    SimpleVertex *simpleVertices = (SimpleVertex *) node->geometry()->vertexData();
    switch (perfLevel){//No automatic fall through intended on this one
    case Sprites:
        spriteVertices += pIdx*4;
        for (int i=0; i<4; i++){
            spriteVertices[i].x = datum->x  - m_systemOffset.x();
            spriteVertices[i].y = datum->y  - m_systemOffset.y();
            spriteVertices[i].t = datum->t;
            spriteVertices[i].lifeSpan = datum->lifeSpan;
            spriteVertices[i].size = datum->size;
            spriteVertices[i].endSize = datum->endSize;
            spriteVertices[i].vx = datum->vx;
            spriteVertices[i].vy = datum->vy;
            spriteVertices[i].ax = datum->ax;
            spriteVertices[i].ay = datum->ay;
            if (m_explicitDeformation && datum->deformationOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                spriteVertices[i].xx = shadow->xx;
                spriteVertices[i].xy = shadow->xy;
                spriteVertices[i].yx = shadow->yx;
                spriteVertices[i].yy = shadow->yy;
            } else {
                spriteVertices[i].xx = datum->xx;
                spriteVertices[i].xy = datum->xy;
                spriteVertices[i].yx = datum->yx;
                spriteVertices[i].yy = datum->yy;
            }
            if (m_explicitRotation && datum->rotationOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                spriteVertices[i].rotation = shadow->rotation;
                spriteVertices[i].rotationSpeed = shadow->rotationSpeed;
                spriteVertices[i].autoRotate = shadow->autoRotate;
            } else {
                spriteVertices[i].rotation = datum->rotation;
                spriteVertices[i].rotationSpeed = datum->rotationSpeed;
                spriteVertices[i].autoRotate = datum->autoRotate;
            }
            spriteVertices[i].animInterpolate = m_spritesInterpolate ? 1.0 : 0.0;//### Shadow? In particleData? Or uniform?
            if (m_explicitAnimation && datum->animationOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                spriteVertices[i].frameDuration = shadow->frameDuration;
                spriteVertices[i].frameCount = shadow->frameCount;
                spriteVertices[i].animT = shadow->animT;
                spriteVertices[i].animX = shadow->animX;
                spriteVertices[i].animY = shadow->animY;
                spriteVertices[i].animWidth = shadow->animWidth;
                spriteVertices[i].animHeight = shadow->animHeight;
            } else {
                spriteVertices[i].frameDuration = datum->frameDuration;
                spriteVertices[i].frameCount = datum->frameCount;
                spriteVertices[i].animT = datum->animT;
                spriteVertices[i].animX = datum->animX;
                spriteVertices[i].animY = datum->animY;
                spriteVertices[i].animWidth = datum->animWidth;
                spriteVertices[i].animHeight = datum->animHeight;
            }
            if (m_explicitColor && datum->colorOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                spriteVertices[i].color.r = shadow->color.r;
                spriteVertices[i].color.g = shadow->color.g;
                spriteVertices[i].color.b = shadow->color.b;
                spriteVertices[i].color.a = shadow->color.a;
            } else {
                spriteVertices[i].color.r = datum->color.r;
                spriteVertices[i].color.g = datum->color.g;
                spriteVertices[i].color.b = datum->color.b;
                spriteVertices[i].color.a = datum->color.a;
            }
        }
        break;
    case Tabled: //Fall through until it has its own vertex class
    case Deformable:
        deformableVertices += pIdx*4;
        for (int i=0; i<4; i++){
            deformableVertices[i].x = datum->x  - m_systemOffset.x();
            deformableVertices[i].y = datum->y  - m_systemOffset.y();
            deformableVertices[i].t = datum->t;
            deformableVertices[i].lifeSpan = datum->lifeSpan;
            deformableVertices[i].size = datum->size;
            deformableVertices[i].endSize = datum->endSize;
            deformableVertices[i].vx = datum->vx;
            deformableVertices[i].vy = datum->vy;
            deformableVertices[i].ax = datum->ax;
            deformableVertices[i].ay = datum->ay;
            if (m_explicitDeformation && datum->deformationOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                deformableVertices[i].xx = shadow->xx;
                deformableVertices[i].xy = shadow->xy;
                deformableVertices[i].yx = shadow->yx;
                deformableVertices[i].yy = shadow->yy;
            } else {
                deformableVertices[i].xx = datum->xx;
                deformableVertices[i].xy = datum->xy;
                deformableVertices[i].yx = datum->yx;
                deformableVertices[i].yy = datum->yy;
            }
            if (m_explicitRotation && datum->rotationOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                deformableVertices[i].rotation = shadow->rotation;
                deformableVertices[i].rotationSpeed = shadow->rotationSpeed;
                deformableVertices[i].autoRotate = shadow->autoRotate;
            } else {
                deformableVertices[i].rotation = datum->rotation;
                deformableVertices[i].rotationSpeed = datum->rotationSpeed;
                deformableVertices[i].autoRotate = datum->autoRotate;
            }
            if (m_explicitColor && datum->colorOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                deformableVertices[i].color.r = shadow->color.r;
                deformableVertices[i].color.g = shadow->color.g;
                deformableVertices[i].color.b = shadow->color.b;
                deformableVertices[i].color.a = shadow->color.a;
            } else {
                deformableVertices[i].color.r = datum->color.r;
                deformableVertices[i].color.g = datum->color.g;
                deformableVertices[i].color.b = datum->color.b;
                deformableVertices[i].color.a = datum->color.a;
            }
        }
        break;
    case Colored:
        coloredVertices += pIdx*1;
        for (int i=0; i<1; i++){
            coloredVertices[i].x = datum->x  - m_systemOffset.x();
            coloredVertices[i].y = datum->y  - m_systemOffset.y();
            coloredVertices[i].t = datum->t;
            coloredVertices[i].lifeSpan = datum->lifeSpan;
            coloredVertices[i].size = datum->size;
            coloredVertices[i].endSize = datum->endSize;
            coloredVertices[i].vx = datum->vx;
            coloredVertices[i].vy = datum->vy;
            coloredVertices[i].ax = datum->ax;
            coloredVertices[i].ay = datum->ay;
            if (m_explicitColor && datum->colorOwner != this) {
                QQuickParticleData* shadow = getShadowDatum(datum);
                coloredVertices[i].color.r = shadow->color.r;
                coloredVertices[i].color.g = shadow->color.g;
                coloredVertices[i].color.b = shadow->color.b;
                coloredVertices[i].color.a = shadow->color.a;
            } else {
                coloredVertices[i].color.r = datum->color.r;
                coloredVertices[i].color.g = datum->color.g;
                coloredVertices[i].color.b = datum->color.b;
                coloredVertices[i].color.a = datum->color.a;
            }
        }
        break;
    case Simple:
        simpleVertices += pIdx*1;
        for (int i=0; i<1; i++){
            simpleVertices[i].x = datum->x - m_systemOffset.x();
            simpleVertices[i].y = datum->y - m_systemOffset.y();
            simpleVertices[i].t = datum->t;
            simpleVertices[i].lifeSpan = datum->lifeSpan;
            simpleVertices[i].size = datum->size;
            simpleVertices[i].endSize = datum->endSize;
            simpleVertices[i].vx = datum->vx;
            simpleVertices[i].vy = datum->vy;
            simpleVertices[i].ax = datum->ax;
            simpleVertices[i].ay = datum->ay;
        }
        break;
    default:
        break;
    }

    node->setFlag(QSGNode::OwnsGeometry, true);
}



QT_END_NAMESPACE
