/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include "qquickspriteimage_p.h"
#include "qquicksprite_p.h"
#include "qquickspriteengine_p.h"
#include <QtQuick/private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>
#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgengine.h>
#include <QtQuick/qsgtexturematerial.h>
#include <QtQuick/qsgtexture.h>
#include <QtQuick/qquickcanvas.h>
#include <QFile>
#include <cmath>
#include <qmath.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

static const char vertexShaderCode[] =
    "attribute highp vec2 vTex;\n"
    "uniform highp vec3 animData;// w,h(premultiplied of anim), interpolation progress\n"
    "uniform highp vec4 animPos;//x,y, x,y (two frames for interpolation)\n"
    "uniform highp vec2 size;//w,h of element\n"
    "\n"
    "uniform highp mat4 qt_Matrix;\n"
    "\n"
    "varying highp vec4 fTexS;\n"
    "varying lowp float progress;\n"
    "\n"
    "\n"
    "void main() {\n"
    "    progress = animData.z;\n"
    "    //Calculate frame location in texture\n"
    "    fTexS.xy = animPos.xy + vTex.xy * animData.xy;\n"
    "    //Next frame is also passed, for interpolation\n"
    "    fTexS.zw = animPos.zw + vTex.xy * animData.xy;\n"
    "\n"
    "    gl_Position = qt_Matrix * vec4(size.x * vTex.x, size.y * vTex.y, 0, 1);\n"
    "}\n";

static const char fragmentShaderCode[] =
    "uniform sampler2D texture;\n"
    "uniform lowp float qt_Opacity;\n"
    "\n"
    "varying highp vec4 fTexS;\n"
    "varying lowp float progress;\n"
    "\n"
    "void main() {\n"
    "    gl_FragColor = mix(texture2D(texture, fTexS.xy), texture2D(texture, fTexS.zw), progress) * qt_Opacity;\n"
    "}\n";

class QQuickSpriteMaterial : public QSGMaterial
{
public:
    QQuickSpriteMaterial();
    virtual ~QQuickSpriteMaterial();
    virtual QSGMaterialType *type() const { static QSGMaterialType type; return &type; }
    virtual QSGMaterialShader *createShader() const;
    virtual int compare(const QSGMaterial *other) const
    {
        return this - static_cast<const QQuickSpriteMaterial *>(other);
    }

    QSGTexture *texture;

    float animT;
    float animX1;
    float animY1;
    float animX2;
    float animY2;
    float animW;
    float animH;
    float elementWidth;
    float elementHeight;
};

QQuickSpriteMaterial::QQuickSpriteMaterial()
    : animT(0.0f)
    , animX1(0.0f)
    , animY1(0.0f)
    , animX2(0.0f)
    , animY2(0.0f)
    , animW(1.0f)
    , animH(1.0f)
    , elementWidth(1.0f)
    , elementHeight(1.0f)
{
    setFlag(Blending, true);
}

QQuickSpriteMaterial::~QQuickSpriteMaterial()
{
    delete texture;
}

class SpriteMaterialData : public QSGMaterialShader
{
public:
    SpriteMaterialData(const char * /* vertexFile */ = 0, const char * /* fragmentFile */ = 0)
    {
    }

    void deactivate() {
        QSGMaterialShader::deactivate();

        for (int i=0; i<8; ++i) {
            program()->setAttributeArray(i, GL_FLOAT, chunkOfBytes, 1, 0);
        }
    }

    virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *)
    {
        QQuickSpriteMaterial *m = static_cast<QQuickSpriteMaterial *>(newEffect);
        m->texture->bind();

        program()->setUniformValue(m_opacity_id, state.opacity());
        program()->setUniformValue(m_animData_id, m->animW, m->animH, m->animT);
        program()->setUniformValue(m_animPos_id, m->animX1, m->animY1, m->animX2, m->animY2);
        program()->setUniformValue(m_size_id, m->elementWidth, m->elementHeight);

        if (state.isMatrixDirty())
            program()->setUniformValue(m_matrix_id, state.combinedMatrix());
    }

    virtual void initialize() {
        m_matrix_id = program()->uniformLocation("qt_Matrix");
        m_opacity_id = program()->uniformLocation("qt_Opacity");
        m_animData_id = program()->uniformLocation("animData");
        m_animPos_id = program()->uniformLocation("animPos");
        m_size_id = program()->uniformLocation("size");
    }

    virtual const char *vertexShader() const { return vertexShaderCode; }
    virtual const char *fragmentShader() const { return fragmentShaderCode; }

    virtual char const *const *attributeNames() const {
        static const char *attr[] = {
           "vTex",
            0
        };
        return attr;
    }

    int m_matrix_id;
    int m_opacity_id;
    int m_animData_id;
    int m_animPos_id;
    int m_size_id;

    static float chunkOfBytes[1024];
};

float SpriteMaterialData::chunkOfBytes[1024];

QSGMaterialShader *QQuickSpriteMaterial::createShader() const
{
    return new SpriteMaterialData;
}

struct SpriteVertex {
    float tx;
    float ty;
};

struct SpriteVertices {
    SpriteVertex v1;
    SpriteVertex v2;
    SpriteVertex v3;
    SpriteVertex v4;
};

/*!
    \qmlclass SpriteImage QQuickSpriteImage
    \inqmlmodule QtQuick 2
    \inherits Item
    \brief The SpriteImage element draws a sprite animation

*/
/*!
    \qmlproperty bool QtQuick2::SpriteImage::running

    Whether the sprite is animating or not.

    Default is true
*/
/*!
    \qmlproperty bool QtQuick2::SpriteImage::interpolate

    If true, interpolation will occur between sprite frames to make the
    animation appear smoother.

    Default is true.
*/
/*!
    \qmlproperty string QtQuick2::SpriteImage::goalSprite

    The name of the Sprite which is currently animating.
*/
/*!
    \qmlproperty string QtQuick2::SpriteImage::goalSprite

    The name of the Sprite which the animation should move to.

    Sprite states have defined durations and transitions between them, setting goalState
    will cause it to disregard any path weightings (including 0) and head down the path
    which will reach the goalState quickest (fewest animations). It will pass through
    intermediate states on that path, and animate them for their duration.

    If it is possible to return to the goalState from the starting point of the goalState
    it will continue to do so until goalState is set to "" or an unreachable state.
*/
/*! \qmlmethod void QtQuick2::SpriteImage::jumpTo(string sprite)

    This function causes the sprite to jump to the specified state immediately, intermediate
    states are not played.
*/
/*!
    \qmlproperty list<Sprite> QtQuick2::SpriteImage::sprites

    The sprite or sprites to draw. Sprites will be scaled to the size of this element.
*/

//TODO: Implicitly size element to size of first sprite?
QQuickSpriteImage::QQuickSpriteImage(QQuickItem *parent) :
    QQuickItem(parent)
    , m_node(0)
    , m_material(0)
    , m_spriteEngine(0)
    , m_curFrame(0)
    , m_pleaseReset(false)
    , m_running(true)
    , m_interpolate(true)
    , m_curStateIdx(0)
{
    setFlag(ItemHasContents);
    connect(this, SIGNAL(runningChanged(bool)),
            this, SLOT(update()));
}

void QQuickSpriteImage::jumpTo(const QString &sprite)
{
    if (!m_spriteEngine)
        return;
    m_spriteEngine->setGoal(m_spriteEngine->stateIndex(sprite), 0, true);
}

void QQuickSpriteImage::setGoalSprite(const QString &sprite)
{
    if (m_goalState != sprite){
        m_goalState = sprite;
        emit goalSpriteChanged(sprite);
        m_spriteEngine->setGoal(m_spriteEngine->stateIndex(sprite));
    }
}

QDeclarativeListProperty<QQuickSprite> QQuickSpriteImage::sprites()
{
    return QDeclarativeListProperty<QQuickSprite>(this, &m_sprites, spriteAppend, spriteCount, spriteAt, spriteClear);
}

void QQuickSpriteImage::createEngine()
{
    //TODO: delay until component complete
    if (m_spriteEngine)
        delete m_spriteEngine;
    if (m_sprites.count())
        m_spriteEngine = new QQuickSpriteEngine(m_sprites, this);
    else
        m_spriteEngine = 0;
    reset();
}

static QSGGeometry::Attribute SpriteImage_Attributes[] = {
    QSGGeometry::Attribute::create(0, 2, GL_FLOAT),         // tex
};

static QSGGeometry::AttributeSet SpriteImage_AttributeSet =
{
    1, // Attribute Count
    2 * sizeof(float),
    SpriteImage_Attributes
};

QSGGeometryNode* QQuickSpriteImage::buildNode()
{
    if (!m_spriteEngine) {
        qWarning() << "SpriteImage: No sprite engine...";
        return 0;
    }

    m_material = new QQuickSpriteMaterial();

    QImage image = m_spriteEngine->assembledImage();
    if (image.isNull())
        return 0;
    m_sheetSize = QSizeF(image.size());
    m_material->texture = canvas()->createTextureFromImage(image);
    m_material->texture->setFiltering(QSGTexture::Linear);
    m_spriteEngine->start(0);
    m_material->animT = 0;
    m_material->animX1 = m_spriteEngine->spriteX() / m_sheetSize.width();
    m_material->animY1 = m_spriteEngine->spriteY() / m_sheetSize.height();
    m_material->animX2 = m_material->animX1;
    m_material->animY2 = m_material->animY1;
    m_material->animW = m_spriteEngine->spriteWidth() / m_sheetSize.width();
    m_material->animH = m_spriteEngine->spriteHeight() / m_sheetSize.height();
    m_material->elementWidth = width();
    m_material->elementHeight = height();
    m_curState = m_spriteEngine->state(m_spriteEngine->curState())->name();
    emit currentSpriteChanged(m_curState);

    int vCount = 4;
    int iCount = 6;
    QSGGeometry *g = new QSGGeometry(SpriteImage_AttributeSet, vCount, iCount);
    g->setDrawingMode(GL_TRIANGLES);

    SpriteVertices *p = (SpriteVertices *) g->vertexData();

    p->v1.tx = 0;
    p->v1.ty = 0;

    p->v2.tx = 1.0;
    p->v2.ty = 0;

    p->v3.tx = 0;
    p->v3.ty = 1.0;

    p->v4.tx = 1.0;
    p->v4.ty = 1.0;

    quint16 *indices = g->indexDataAsUShort();
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 1;
    indices[4] = 3;
    indices[5] = 2;


    m_timestamp.start();
    m_node = new QSGGeometryNode();
    m_node->setGeometry(g);
    m_node->setMaterial(m_material);
    m_node->setFlag(QSGGeometryNode::OwnsMaterial);
    return m_node;
}

void QQuickSpriteImage::reset()
{
    m_pleaseReset = true;
}

QSGNode *QQuickSpriteImage::updatePaintNode(QSGNode *, UpdatePaintNodeData *)
{
    if (m_pleaseReset) {
        delete m_node;
        delete m_material;

        m_node = 0;
        m_material = 0;
        m_pleaseReset = false;
    }

    prepareNextFrame();

    if (m_running) {
        update();
        if (m_node)
            m_node->markDirty(QSGNode::DirtyMaterial);
    }

    return m_node;
}

void QQuickSpriteImage::prepareNextFrame()
{
    if (m_node == 0)
        m_node = buildNode();
    if (m_node == 0) //error creating node
        return;

    uint timeInt = m_timestamp.elapsed();
    qreal time =  timeInt / 1000.;
    m_material->elementHeight = height();
    m_material->elementWidth = width();

    //Advance State
    m_spriteEngine->updateSprites(timeInt);
    if (m_curStateIdx != m_spriteEngine->curState()) {
        m_curStateIdx = m_spriteEngine->curState();
        m_curState = m_spriteEngine->state(m_spriteEngine->curState())->name();
        emit currentSpriteChanged(m_curState);
        m_curFrame= -1;
    }

    //Advance Sprite
    qreal animT = m_spriteEngine->spriteStart()/1000.0;
    qreal frameCount = m_spriteEngine->spriteFrames();
    qreal frameDuration = m_spriteEngine->spriteDuration()/frameCount;
    double frameAt;
    qreal progress;
    if (frameDuration > 0) {
        qreal frame = (time - animT)/(frameDuration / 1000.0);
        frame = qBound(qreal(0.0), frame, frameCount - qreal(1.0));//Stop at count-1 frames until we have between anim interpolation
        progress = modf(frame,&frameAt);
    } else {
        m_curFrame++;
        if (m_curFrame >= frameCount){
            m_curFrame = 0;
            m_spriteEngine->advance();
        }
        frameAt = m_curFrame;
        progress = 0;
    }
    qreal y = m_spriteEngine->spriteY() / m_sheetSize.height();
    qreal w = m_spriteEngine->spriteWidth() / m_sheetSize.width();
    qreal h = m_spriteEngine->spriteHeight() / m_sheetSize.height();
    qreal x1 = m_spriteEngine->spriteX() / m_sheetSize.width();
    x1 += frameAt * w;
    qreal x2 = x1;
    if (frameAt < (frameCount-1))
        x2 += w;

    m_material->animX1 = x1;
    m_material->animY1 = y;
    m_material->animX2 = x2;
    m_material->animY2 = y;
    m_material->animW = w;
    m_material->animH = h;
    m_material->animT = m_interpolate ? progress : 0.0;
}

QT_END_NAMESPACE
