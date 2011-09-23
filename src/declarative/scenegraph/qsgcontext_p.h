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

#ifndef QSGCONTEXT_H
#define QSGCONTEXT_H

#include <QImage>
#include <QObject>
#include "private/qabstractanimation2_p.h"

#include "qsgnode.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGContextPrivate;
class QSGRectangleNode;
class QSGImageNode;
class QSGGlyphNode;
class QSGRenderer;

class QSGTexture;
class QSGMaterial;
class QSGMaterialShader;
class QSGEngine;

class QOpenGLContext;
class QOpenGLFramebufferObject;

class Q_DECLARATIVE_EXPORT QSGContext : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSGContext)

public:
    explicit QSGContext(QObject *parent = 0);
    ~QSGContext();

    virtual void initialize(QOpenGLContext *context);

    QSGRenderer *renderer() const;

    void setRootNode(QSGRootNode *node);
    QSGRootNode *rootNode() const;

    QSGEngine *engine() const;
    QOpenGLContext *glContext() const;

    bool isReady() const;

    QSGMaterialShader *prepareMaterial(QSGMaterial *material);

    virtual void renderNextFrame(QOpenGLFramebufferObject *fbo = 0);

    virtual QSGRectangleNode *createRectangleNode();
    virtual QSGImageNode *createImageNode();
    virtual QSGGlyphNode *createGlyphNode();
    virtual QSGRenderer *createRenderer();

    virtual bool canDecodeImageToTexture() const;
    virtual QSGTexture *decodeImageToTexture(QIODevice *dev,
                                                     QSize *size,
                                                     const QSize &requestSize);
    virtual QSGTexture *createTexture(const QImage &image = QImage()) const;
    virtual QSize minimumFBOSize() const;

    static QSGContext *createDefaultContext();

    void scheduleTextureForCleanup(QSGTexture *texture);
    void cleanupTextures();

    void setFlashModeEnabled(bool enabled);
    bool isFlashModeEnabled() const;

    void setRenderAlpha(qreal renderAlpha);
    qreal renderAlpha() const;

    void setDistanceFieldEnabled(bool enabled);
    bool isDistanceFieldEnabled() const;

    virtual QAnimationDriver2 *createAnimationDriver(QObject *parent);

signals:
    void ready();
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSGCONTEXT_H
