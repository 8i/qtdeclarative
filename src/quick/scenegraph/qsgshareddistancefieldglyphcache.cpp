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

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#if defined(QSGSHAREDDISTANCEFIELDGLYPHCACHE_DEBUG)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#include "qsgshareddistancefieldglyphcache_p.h"

#include <QtCore/qhash.h>
#include <QtCore/qthread.h>
#include <QtGui/qplatformsharedgraphicscache_qpa.h>

#include <QtQuick/qquickcanvas.h>

#include <QtOpenGL/qglframebufferobject.h>

// #define QSGSHAREDDISTANCEFIELDGLYPHCACHE_DEBUG

Q_DECLARE_METATYPE(QVector<quint32>)
Q_DECLARE_METATYPE(QVector<QImage>)

QT_BEGIN_NAMESPACE

QSGSharedDistanceFieldGlyphCache::QSGSharedDistanceFieldGlyphCache(const QByteArray &cacheId,
                                                                   QPlatformSharedGraphicsCache *sharedGraphicsCache,
                                                                   QSGDistanceFieldGlyphCacheManager *man,
                                                                   QOpenGLContext *c,
                                                                   const QRawFont &font)
    : QSGDistanceFieldGlyphCache(man, c, font)
    , m_cacheId(cacheId)
    , m_sharedGraphicsCache(sharedGraphicsCache)
{
#if defined(QSGSHAREDDISTANCEFIELDGLYPHCACHE_DEBUG)
    qDebug("QSGSharedDistanceFieldGlyphCache with id %s created in thread %p",
           cacheId.constData(), QThread::currentThreadId());
#endif

    Q_ASSERT(sizeof(glyph_t) == sizeof(quint32));
    Q_ASSERT(sharedGraphicsCache != 0);

    qRegisterMetaType<QVector<quint32> >();
    qRegisterMetaType<QVector<QImage> >();

    connect(sharedGraphicsCache, SIGNAL(itemsMissing(QByteArray,QVector<quint32>)),
            this, SLOT(reportItemsMissing(QByteArray,QVector<quint32>)),
            Qt::DirectConnection);
    connect(sharedGraphicsCache, SIGNAL(itemsAvailable(QByteArray,void*,QSize,QVector<quint32>,QVector<QPoint>)),
            this, SLOT(reportItemsAvailable(QByteArray,void*,QSize,QVector<quint32>,QVector<QPoint>)),
            Qt::DirectConnection);
    connect(sharedGraphicsCache, SIGNAL(itemsUpdated(QByteArray,void*,QSize,QVector<quint32>,QVector<QPoint>)),
            this, SLOT(reportItemsAvailable(QByteArray,void*,QSize,QVector<quint32>,QVector<QPoint>)),
            Qt::DirectConnection);
    connect(sharedGraphicsCache, SIGNAL(itemsInvalidated(QByteArray,QVector<quint32>)),
            this, SLOT(reportItemsInvalidated(QByteArray,QVector<quint32>)),
            Qt::DirectConnection);
}

QSGSharedDistanceFieldGlyphCache::~QSGSharedDistanceFieldGlyphCache()
{
    {
        QHash<glyph_t, void *>::const_iterator it = m_bufferForGlyph.constBegin();
        while (it != m_bufferForGlyph.constEnd()) {
            m_sharedGraphicsCache->dereferenceBuffer(it.value());
            ++it;
        }
    }

    {
        QHash<quint32, PendingGlyph>::const_iterator it = m_pendingReadyGlyphs.constBegin();
        while (it != m_pendingReadyGlyphs.constEnd()) {
            m_sharedGraphicsCache->dereferenceBuffer(it.value().buffer);
            ++it;
        }
    }
}

void QSGSharedDistanceFieldGlyphCache::requestGlyphs(const QSet<glyph_t> &glyphs)
{
    QMutexLocker locker(&m_pendingGlyphsMutex);

#if defined(QSGSHAREDDISTANCEFIELDGLYPHCACHE_DEBUG)
    qDebug("QSGSharedDistanceFieldGlyphCache::requestGlyphs() called for %s (%d glyphs)",
           m_cacheId.constData(), glyphs.size());
#endif

    m_requestedGlyphsThatHaveNotBeenReturned.unite(glyphs);

    QVector<quint32> glyphsVector;
    glyphsVector.reserve(glyphs.size());

    QSet<glyph_t>::const_iterator it;
    for (it = glyphs.constBegin(); it != glyphs.constEnd(); ++it) {
        Q_ASSERT(!m_bufferForGlyph.contains(*it));
        glyphsVector.append(*it);
    }

    // Invoke method on queued connection to make sure it's called asynchronously on the
    // correct thread (requestGlyphs() is called from the rendering thread.)
    QMetaObject::invokeMethod(m_sharedGraphicsCache, "requestItems", Qt::QueuedConnection,
                              Q_ARG(QByteArray, m_cacheId),
                              Q_ARG(QVector<quint32>, glyphsVector));
}

void QSGSharedDistanceFieldGlyphCache::waitForGlyphs()
{
    {
        QMutexLocker locker(&m_pendingGlyphsMutex);
        while (!m_requestedGlyphsThatHaveNotBeenReturned.isEmpty())
            m_pendingGlyphsCondition.wait(&m_pendingGlyphsMutex);
    }
}

void QSGSharedDistanceFieldGlyphCache::storeGlyphs(const QHash<glyph_t, QImage> &glyphs)
{
    {
        QMutexLocker locker(&m_pendingGlyphsMutex);
#if defined(QSGSHAREDDISTANCEFIELDGLYPHCACHE_DEBUG)
        qDebug("QSGSharedDistanceFieldGlyphCache::storeGlyphs() called for %s (%d glyphs)",
               m_cacheId.constData(), glyphs.size());
#endif

        int glyphCount = glyphs.size();
        QVector<quint32> glyphIds(glyphCount);
        QVector<QImage> images(glyphCount);
        QHash<glyph_t, QImage>::const_iterator it = glyphs.constBegin();
        int i=0;
        while (it != glyphs.constEnd()) {
            m_requestedGlyphsThatHaveNotBeenReturned.insert(it.key());
            glyphIds[i] = it.key();
            images[i] = it.value();

            ++it; ++i;
        }

        QMetaObject::invokeMethod(m_sharedGraphicsCache, "insertItems", Qt::QueuedConnection,
                                  Q_ARG(QByteArray, m_cacheId),
                                  Q_ARG(QVector<quint32>, glyphIds),
                                  Q_ARG(QVector<QImage>, images));
    }

    processPendingGlyphs();
}

void QSGSharedDistanceFieldGlyphCache::referenceGlyphs(const QSet<glyph_t> &glyphs)
{
    Q_UNUSED(glyphs);

    // Intentionally empty. Not required in this implementation, since the glyphs are reference
    // counted outside and releaseGlyphs() will only be called when there are no more references.
}

void QSGSharedDistanceFieldGlyphCache::releaseGlyphs(const QSet<glyph_t> &glyphs)
{
#if defined(QSGSHAREDDISTANCEFIELDGLYPHCACHE_DEBUG)
    qDebug("QSGSharedDistanceFieldGlyphCache::releaseGlyphs() called for %s (%d glyphs)",
           m_cacheId.constData(), glyphs.size());
#endif

    QVector<quint32> glyphsVector;
    glyphsVector.reserve(glyphs.size());

    QSet<glyph_t>::const_iterator glyphsIt;
    for (glyphsIt = glyphs.constBegin(); glyphsIt != glyphs.constEnd(); ++glyphsIt) {
        QHash<glyph_t, void *>::iterator bufferIt = m_bufferForGlyph.find(*glyphsIt);
        if (bufferIt != m_bufferForGlyph.end()) {
            void *buffer = bufferIt.value();
            removeGlyph(*glyphsIt);
            m_bufferForGlyph.erase(bufferIt);
            Q_ASSERT(!m_bufferForGlyph.contains(*glyphsIt));

            if (!m_sharedGraphicsCache->dereferenceBuffer(buffer)) {
#if !defined(QT_NO_DEBUG)
                bufferIt = m_bufferForGlyph.begin();
                while (bufferIt != m_bufferForGlyph.end()) {
                    Q_ASSERT(bufferIt.value() != buffer);
                    ++bufferIt;
                }
#endif
            }
        }

        glyphsVector.append(*glyphsIt);
    }

    QMetaObject::invokeMethod(m_sharedGraphicsCache, "releaseItems", Qt::QueuedConnection,
                              Q_ARG(QByteArray, m_cacheId),
                              Q_ARG(QVector<quint32>, glyphsVector));
}

void QSGSharedDistanceFieldGlyphCache::registerOwnerElement(QQuickItem *ownerElement)
{
    bool ok = connect(this, SIGNAL(glyphsPending()), ownerElement, SLOT(triggerPreprocess()));
    Q_ASSERT_X(ok, Q_FUNC_INFO, "QML element that owns a glyph node must have triggerPreprocess() slot");
    Q_UNUSED(ok);
}

void QSGSharedDistanceFieldGlyphCache::unregisterOwnerElement(QQuickItem *ownerElement)
{
    disconnect(this, SIGNAL(glyphsPending()), ownerElement, SLOT(triggerPreprocess()));
}

#if defined(QSGSHAREDDISTANCEFIELDGLYPHCACHE_DEBUG_)
#  include <QtOpenGL/private/qglextensions_p.h>

void QSGSharedDistanceFieldGlyphCache::saveTexture(GLuint textureId, int width, int height)
{
    GLuint fboId;
    glGenFramebuffers(1, &fboId);

    GLuint tmpTexture = 0;
    glGenTextures(1, &tmpTexture);
    glBindTexture(GL_TEXTURE_2D, tmpTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER_EXT, fboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
                           tmpTexture, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    GLfloat textureCoordinateArray[8];
    textureCoordinateArray[0] = 0.0f;
    textureCoordinateArray[1] = 0.0f;
    textureCoordinateArray[2] = 1.0f;
    textureCoordinateArray[3] = 0.0f;
    textureCoordinateArray[4] = 1.0f;
    textureCoordinateArray[5] = 1.0f;
    textureCoordinateArray[6] = 0.0f;
    textureCoordinateArray[7] = 1.0f;

    GLfloat vertexCoordinateArray[8];
    vertexCoordinateArray[0] = -1.0f;
    vertexCoordinateArray[1] = -1.0f;
    vertexCoordinateArray[2] =  1.0f;
    vertexCoordinateArray[3] = -1.0f;
    vertexCoordinateArray[4] =  1.0f;
    vertexCoordinateArray[5] =  1.0f;
    vertexCoordinateArray[6] = -1.0f;
    vertexCoordinateArray[7] =  1.0f;

    glViewport(0, 0, width, height);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertexCoordinateArray);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, textureCoordinateArray);

    {
        static const char *vertexShaderSource =
                "attribute highp   vec4      vertexCoordsArray; \n"
                "attribute highp   vec2      textureCoordArray; \n"
                "varying   highp   vec2      textureCoords;     \n"
                "void main(void) \n"
                "{ \n"
                "    gl_Position = vertexCoordsArray;   \n"
                "    textureCoords = textureCoordArray; \n"
                "} \n";

        static const char *fragmentShaderSource =
                "varying   highp   vec2      textureCoords; \n"
                "uniform   sampler2D         texture;       \n"
                "void main() \n"
                "{ \n"
                "    gl_FragColor = texture2D(texture, textureCoords); \n"
                "} \n";

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

        if (vertexShader == 0 || fragmentShader == 0) {
            GLenum error = glGetError();
            qWarning("SharedGraphicsCacheServer::setupShaderPrograms: Failed to create shaders. (GL error: %x)",
                     error);
            return;
        }

        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(vertexShader);

        GLint len = 1;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &len);

        char infoLog[2048];
        glGetShaderInfoLog(vertexShader, 2048, NULL, infoLog);
        if (qstrlen(infoLog) > 0) {
            qWarning("SharedGraphicsCacheServer::setupShaderPrograms, problems compiling vertex shader:\n %s",
                     infoLog);
            //return;
        }

        glCompileShader(fragmentShader);
        glGetShaderInfoLog(fragmentShader, 2048, NULL, infoLog);
        if (qstrlen(infoLog) > 0) {
            qWarning("SharedGraphicsCacheServer::setupShaderPrograms, problems compiling fragent shader:\n %s",
                     infoLog);
            //return;
        }

        GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);

        glBindAttribLocation(shaderProgram, 0, "vertexCoordsArray");
        glBindAttribLocation(shaderProgram, 1, "textureCoordArray");

        glLinkProgram(shaderProgram);
        glGetProgramInfoLog(shaderProgram, 2048, NULL, infoLog);
        if (qstrlen(infoLog) > 0) {
            qWarning("SharedGraphicsCacheServer::setupShaderPrograms, problems linking shaders:\n %s",
                     infoLog);
            //return;
        }

        glUseProgram(shaderProgram);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        int textureUniformLocation = glGetUniformLocation(shaderProgram, "texture");
        glUniform1i(textureUniformLocation, 0);
    }

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            qWarning("SharedGraphicsCacheServer::readBackBuffer: glDrawArrays reported error 0x%x",
                     error);
        }
    }

    uchar *data = new uchar[width * height * 4];

    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

    QImage image(width, height, QImage::Format_ARGB32);
    quint32 *dest = reinterpret_cast<quint32 *>(image.bits());
    for (int i=0; i<width*height; ++i)
        dest[i] = qRgba(0xff, 0xff, 0xff, data[i]);

    QByteArray fileName = m_cacheId + " " + QByteArray::number(textureId);
    fileName = fileName.replace('/', '_').replace(' ', '_') + ".png";
    image.save(QString::fromLocal8Bit(fileName));

    {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            qWarning("SharedGraphicsCacheServer::readBackBuffer: glReadPixels reported error 0x%x",
                     error);
        }
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glDeleteFramebuffers(1, &fboId);
    glDeleteTextures(1, &tmpTexture);

    delete[] data;
}
#endif

namespace {
    struct TextureContent {
        QSize size;
        QVector<glyph_t> glyphs;
    };
}

void QSGSharedDistanceFieldGlyphCache::processPendingGlyphs()
{
    Q_ASSERT(QThread::currentThread() == thread());

    waitForGlyphs();

    {
        QMutexLocker locker(&m_pendingGlyphsMutex);
        if (m_pendingMissingGlyphs.isEmpty()
            && m_pendingReadyGlyphs.isEmpty()
            && m_pendingInvalidatedGlyphs.isEmpty()) {
            return;
        }

        {
            QVector<glyph_t> pendingMissingGlyphs;
            pendingMissingGlyphs.reserve(m_pendingMissingGlyphs.size());

            QSet<glyph_t>::const_iterator it = m_pendingMissingGlyphs.constBegin();
            while (it != m_pendingMissingGlyphs.constEnd()) {
                pendingMissingGlyphs.append(*it);
                ++it;
            }

            markGlyphsToRender(pendingMissingGlyphs);
        }

        {
            QVector<glyph_t> filteredPendingInvalidatedGlyphs;
            filteredPendingInvalidatedGlyphs.reserve(m_pendingInvalidatedGlyphs.size());

            QSet<glyph_t>::const_iterator it = m_pendingInvalidatedGlyphs.constBegin();
            while (it != m_pendingInvalidatedGlyphs.constEnd()) {
                bool rerequestGlyph = false;

                // The glyph was invalidated right after being posted as ready, we throw away
                // the ready glyph and rerequest it to be certain
                QHash<quint32, PendingGlyph>::iterator pendingGlyphIt = m_pendingReadyGlyphs.find(*it);
                if (pendingGlyphIt != m_pendingReadyGlyphs.end()) {
                    m_sharedGraphicsCache->dereferenceBuffer(pendingGlyphIt.value().buffer);
                    pendingGlyphIt = m_pendingReadyGlyphs.erase(pendingGlyphIt);
                    rerequestGlyph = true;
                }

                void *bufferId = m_bufferForGlyph.value(*it, 0);
                if (bufferId != 0) {
                    m_sharedGraphicsCache->dereferenceBuffer(bufferId);
                    m_bufferForGlyph.remove(*it);
                    rerequestGlyph = true;
                }

                if (rerequestGlyph)
                    filteredPendingInvalidatedGlyphs.append(*it);

                ++it;
            }

            // If this cache is still using the glyphs, reset the texture held by them, and mark them
            // to be rendered again since they are still needed.
            if (!filteredPendingInvalidatedGlyphs.isEmpty()) {
                setGlyphsTexture(filteredPendingInvalidatedGlyphs, Texture());
                markGlyphsToRender(filteredPendingInvalidatedGlyphs);
            }
        }

        {
            QList<GlyphPosition> glyphPositions;

            QHash<void *, TextureContent> textureContentForBuffer;
            {
                QHash<quint32, PendingGlyph>::iterator it = m_pendingReadyGlyphs.begin();
                while (it != m_pendingReadyGlyphs.end()) {
                    void *currentGlyphBuffer = m_bufferForGlyph.value(it.key(), 0);
                    if (currentGlyphBuffer != 0) {
                        if (!m_sharedGraphicsCache->dereferenceBuffer(currentGlyphBuffer)) {
                            Q_ASSERT(!textureContentForBuffer.contains(currentGlyphBuffer));
                        }
                    }

                    PendingGlyph &pendingGlyph  = it.value();

                    // We don't ref or deref the buffer here, since it was already referenced when
                    // added to the pending ready glyphs
                    m_bufferForGlyph[it.key()] = pendingGlyph.buffer;

                    textureContentForBuffer[pendingGlyph.buffer].size = pendingGlyph.bufferSize;
                    textureContentForBuffer[pendingGlyph.buffer].glyphs.append(it.key());

                    GlyphPosition glyphPosition;
                    glyphPosition.glyph = it.key();
                    glyphPosition.position = pendingGlyph.position;

                    glyphPositions.append(glyphPosition);

                    ++it;
                }
            }

            setGlyphsPosition(glyphPositions);

            {
                QHash<void *, TextureContent>::const_iterator it = textureContentForBuffer.constBegin();
                while (it != textureContentForBuffer.constEnd()) {
                    Texture texture;
                    texture.textureId = m_sharedGraphicsCache->textureIdForBuffer(it.key());
                    texture.size = it.value().size;

#if defined(QSGSHAREDDISTANCEFIELDGLYPHCACHE_DEBUG_)
                    saveTexture(texture.textureId, texture.size.width(), texture.size.height());
#endif
                    setGlyphsTexture(it.value().glyphs, texture);

                    ++it;
                }
            }
        }

        m_pendingMissingGlyphs.clear();
        m_pendingInvalidatedGlyphs.clear();
        m_pendingReadyGlyphs.clear();
    }
}

void QSGSharedDistanceFieldGlyphCache::reportItemsAvailable(const QByteArray &cacheId,
                                                        void *bufferId, const QSize &bufferSize,
                                                        const QVector<quint32> &itemIds,
                                                        const QVector<QPoint> &positions)
{
    {
        QMutexLocker locker(&m_pendingGlyphsMutex);
        if (m_cacheId != cacheId)
            return;

        Q_ASSERT(itemIds.size() == positions.size());

#if defined(QSGSHAREDDISTANCEFIELDGLYPHCACHE_DEBUG)
        qDebug("QSGSharedDistanceFieldGlyphCache::reportItemsAvailable() called for %s (%d glyphs, bufferSize: %dx%d)",
               cacheId.constData(), itemIds.size(), bufferSize.width(), bufferSize.height());
#endif

        for (int i=0; i<itemIds.size(); ++i) {
            PendingGlyph &pendingGlyph = m_pendingReadyGlyphs[itemIds.at(i)];
            void *oldBuffer = pendingGlyph.buffer;
            Q_ASSERT(bufferSize.height() >= pendingGlyph.bufferSize.height());

            pendingGlyph.buffer = bufferId;
            pendingGlyph.position = positions.at(i);
            pendingGlyph.bufferSize = bufferSize;

            m_sharedGraphicsCache->referenceBuffer(bufferId);
            if (oldBuffer != 0)
                m_sharedGraphicsCache->dereferenceBuffer(oldBuffer);

            m_requestedGlyphsThatHaveNotBeenReturned.remove(itemIds.at(i));
        }
    }

    m_pendingGlyphsCondition.wakeAll();
    emit glyphsPending();
}

void QSGSharedDistanceFieldGlyphCache::reportItemsInvalidated(const QByteArray &cacheId,
                                                              const QVector<quint32> &itemIds)
{
    {
        QMutexLocker locker(&m_pendingGlyphsMutex);
        if (m_cacheId != cacheId)
            return;

        for (int i=0; i<itemIds.size(); ++i)
            m_pendingInvalidatedGlyphs.insert(itemIds.at(i));
    }

    emit glyphsPending();
}


void QSGSharedDistanceFieldGlyphCache::reportItemsMissing(const QByteArray &cacheId,
                                                          const QVector<quint32> &itemIds)
{
    {
        QMutexLocker locker(&m_pendingGlyphsMutex);
        if (m_cacheId != cacheId)
            return;

#if defined(QSGSHAREDDISTANCEFIELDGLYPHCACHE_DEBUG)
        qDebug("QSGSharedDistanceFieldGlyphCache::reportItemsMissing() called for %s (%d glyphs)",
               cacheId.constData(), itemIds.size());
#endif

        for (int i=0; i<itemIds.size(); ++i) {
            m_pendingMissingGlyphs.insert(itemIds.at(i));
            m_requestedGlyphsThatHaveNotBeenReturned.remove(itemIds.at(i));
        }
    }

    m_pendingGlyphsCondition.wakeAll();
    emit glyphsPending();
}

QT_END_NAMESPACE
