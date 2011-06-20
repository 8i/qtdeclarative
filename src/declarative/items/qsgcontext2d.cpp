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

#include "qsgcontext2d_p.h"
#include "qsgcontext2d_p_p.h"
#include "private/qsgadaptationlayer_p.h"
#include "qsgcanvasitem_p.h"
#include <QtOpenGL/qglframebufferobject.h>
#include <QtCore/qdebug.h>
#include "private/qsgcontext_p.h"

#include <QtGui/qgraphicsitem.h>
#include <QtGui/qapplication.h>
#include <QtGui/qgraphicseffect.h>
#include <qdeclarativeinfo.h>
#include <QtCore/qmath.h>
#include "qdeclarativepixmapcache_p.h"
#include <QtScript/QScriptEngine>

QT_BEGIN_NAMESPACE

static const double Q_PI   = 3.14159265358979323846;   // pi
template <class T>
void memcpy_vector(QVector<T>* dst, const QVector<T>& src)
{
    int pos = dst->size();
    dst->resize(pos + src.size());
    memmove(dst->data() + pos, src.constData(), sizeof(T) * src.size());
}

template <class T>
void copy_vector(QVector<T>* dst, const QVector<T>& src)
{
    int pos = dst->size();
    dst->resize(pos + src.size());
    for (int i = 0; i < src.size(); i++) {
        (*dst)[pos + i] = src[i];
    }
}

// Note, this is exported but in a private header as qtopengl depends on it.
// But it really should be considered private API
void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed = 0);

#define DEGREES(t) ((t) * 180.0 / Q_PI)
#define qClamp(val, min, max) qMin(qMax(val, min), max)

static inline int extractInt(const char **name)
{
    int result = 0;
    bool negative = false;

    //eat leading whitespace
    while (isspace(*name[0]))
        ++*name;

    if (*name[0] == '-') {
        ++*name;
        negative = true;
    } /*else if (name[0] == '+')
        ++name;     //ignore*/

    //construct number
    while (isdigit(*name[0])) {
        result = result * 10 + (*name[0] - '0');
        ++*name;
    }
    if (negative)
        result = -result;

    //handle optional percentage
    if (*name[0] == '%')
        result *= qreal(255)/100;  //### floor or round?

    //eat trailing whitespace
    while (isspace(*name[0]))
        ++*name;

    return result;
}

static bool qt_get_rgb(const QString &string, QRgb *rgb)
{
    const char *name = string.toLatin1().constData();
    int len = qstrlen(name);

    if (len < 5)
        return false;

    bool handleAlpha = false;

    if (name[0] != 'r')
        return false;
    if (name[1] != 'g')
        return false;
    if (name[2] != 'b')
        return false;
    if (name[3] == 'a') {
        handleAlpha = true;
        if(name[3] != '(')
            return false;
    } else if (name[3] != '(')
        return false;

    name += 4;

    int r, g, b, a = 1;
    int result;

    //red
    result = extractInt(&name);
    if (name[0] == ',') {
        r = result;
        ++name;
    } else
        return false;

    //green
    result = extractInt(&name);
    if (name[0] == ',') {
        g = result;
        ++name;
    } else
        return false;

    char nextChar = handleAlpha ? ',' : ')';

    //blue
    result = extractInt(&name);
    if (name[0] == nextChar) {
        b = result;
        ++name;
    } else
        return false;

    //alpha
    if (handleAlpha) {
        result = extractInt(&name);
        if (name[0] == ')') {
            a = result * 255;   //map 0-1 to 0-255
            ++name;
        } else
            return false;
    }

    if (name[0] != '\0')
        return false;

    *rgb = qRgba(qClamp(r,0,255), qClamp(g,0,255), qClamp(b,0,255), qClamp(a,0,255));
    return true;
}

//### unify with qt_get_rgb?
static bool qt_get_hsl(const QString &string, QColor *color)
{
    const char *name = string.toLatin1().constData();
    int len = qstrlen(name);

    if (len < 5)
        return false;

    bool handleAlpha = false;

    if (name[0] != 'h')
        return false;
    if (name[1] != 's')
        return false;
    if (name[2] != 'l')
        return false;
    if (name[3] == 'a') {
        handleAlpha = true;
        if(name[3] != '(')
            return false;
    } else if (name[3] != '(')
        return false;

    name += 4;

    int h, s, l, a = 1;
    int result;

    //hue
    result = extractInt(&name);
    if (name[0] == ',') {
        h = result;
        ++name;
    } else
        return false;

    //saturation
    result = extractInt(&name);
    if (name[0] == ',') {
        s = result;
        ++name;
    } else
        return false;

    char nextChar = handleAlpha ? ',' : ')';

    //lightness
    result = extractInt(&name);
    if (name[0] == nextChar) {
        l = result;
        ++name;
    } else
        return false;

    //alpha
    if (handleAlpha) {
        result = extractInt(&name);
        if (name[0] == ')') {
            a = result * 255;   //map 0-1 to 0-255
            ++name;
        } else
            return false;
    }

    if (name[0] != '\0')
        return false;

    *color = QColor::fromHsl(qClamp(h,0,255), qClamp(s,0,255), qClamp(l,0,255), qClamp(a,0,255));
    return true;
}

//### optimize further
QColor colorFromString(const QString &name)
{
    if (name.startsWith(QLatin1String("rgb"))) {
        QRgb rgb;
        if (qt_get_rgb(name, &rgb))
            return QColor(rgb);
    } else if (name.startsWith(QLatin1String("hsl"))) {
        QColor color;
        if (qt_get_hsl(name, &color))
            return color;
    }

    return QColor(name);
}


static QPainter::CompositionMode compositeOperatorFromString(const QString &compositeOperator)
{
    if (compositeOperator == QLatin1String("source-over")) {
        return QPainter::CompositionMode_SourceOver;
    } else if (compositeOperator == QLatin1String("source-out")) {
        return QPainter::CompositionMode_SourceOut;
    } else if (compositeOperator == QLatin1String("source-in")) {
        return QPainter::CompositionMode_SourceIn;
    } else if (compositeOperator == QLatin1String("source-atop")) {
        return QPainter::CompositionMode_SourceAtop;
    } else if (compositeOperator == QLatin1String("destination-atop")) {
        return QPainter::CompositionMode_DestinationAtop;
    } else if (compositeOperator == QLatin1String("destination-in")) {
        return QPainter::CompositionMode_DestinationIn;
    } else if (compositeOperator == QLatin1String("destination-out")) {
        return QPainter::CompositionMode_DestinationOut;
    } else if (compositeOperator == QLatin1String("destination-over")) {
        return QPainter::CompositionMode_DestinationOver;
    } else if (compositeOperator == QLatin1String("darker")) {
        return QPainter::CompositionMode_SourceOver;
    } else if (compositeOperator == QLatin1String("lighter")) {
        return QPainter::CompositionMode_SourceOver;
    } else if (compositeOperator == QLatin1String("copy")) {
        return QPainter::CompositionMode_Source;
    } else if (compositeOperator == QLatin1String("xor")) {
        return QPainter::CompositionMode_Xor;
    }

    return QPainter::CompositionMode_SourceOver;
}

static QString compositeOperatorToString(QPainter::CompositionMode op)
{
    switch (op) {
    case QPainter::CompositionMode_SourceOver:
        return QLatin1String("source-over");
    case QPainter::CompositionMode_DestinationOver:
        return QLatin1String("destination-over");
    case QPainter::CompositionMode_Clear:
        return QLatin1String("clear");
    case QPainter::CompositionMode_Source:
        return QLatin1String("source");
    case QPainter::CompositionMode_Destination:
        return QLatin1String("destination");
    case QPainter::CompositionMode_SourceIn:
        return QLatin1String("source-in");
    case QPainter::CompositionMode_DestinationIn:
        return QLatin1String("destination-in");
    case QPainter::CompositionMode_SourceOut:
        return QLatin1String("source-out");
    case QPainter::CompositionMode_DestinationOut:
        return QLatin1String("destination-out");
    case QPainter::CompositionMode_SourceAtop:
        return QLatin1String("source-atop");
    case QPainter::CompositionMode_DestinationAtop:
        return QLatin1String("destination-atop");
    case QPainter::CompositionMode_Xor:
        return QLatin1String("xor");
    case QPainter::CompositionMode_Plus:
        return QLatin1String("plus");
    case QPainter::CompositionMode_Multiply:
        return QLatin1String("multiply");
    case QPainter::CompositionMode_Screen:
        return QLatin1String("screen");
    case QPainter::CompositionMode_Overlay:
        return QLatin1String("overlay");
    case QPainter::CompositionMode_Darken:
        return QLatin1String("darken");
    case QPainter::CompositionMode_Lighten:
        return QLatin1String("lighten");
    case QPainter::CompositionMode_ColorDodge:
        return QLatin1String("color-dodge");
    case QPainter::CompositionMode_ColorBurn:
        return QLatin1String("color-burn");
    case QPainter::CompositionMode_HardLight:
        return QLatin1String("hard-light");
    case QPainter::CompositionMode_SoftLight:
        return QLatin1String("soft-light");
    case QPainter::CompositionMode_Difference:
        return QLatin1String("difference");
    case QPainter::CompositionMode_Exclusion:
        return QLatin1String("exclusion");
    default:
        break;
    }
    return QString();
}


 #include <QScriptEngine>

//static QtScript functions
static QScriptValue ctx2d_sync(QScriptContext *c, QScriptEngine* e)
{
  QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
  ctx2d->sync();
  return e->nullValue();
}


// back-reference to the canvas, getter
static QScriptValue ctx2d_canvas(QScriptContext *c, QScriptEngine* e)
{
  QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
  return e->newQObject(ctx2d->canvas());
}

// state
static QScriptValue ctx2d_restore(QScriptContext *c, QScriptEngine *e)
{
  QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
  ctx2d->restore();
  return e->nullValue();
}

static QScriptValue ctx2d_save(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    ctx2d->save();
    return e->nullValue();
}

// transformations
static QScriptValue ctx2d_rotate(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {
        ctx2d->rotate(c->argument(0).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_scale(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 2) {
        ctx2d->scale(c->argument(0).toNumber(), c->argument(1).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_setTransform(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 6) {
        ctx2d->setTransform(c->argument(0).toNumber()
                          , c->argument(1).toNumber()
                          , c->argument(2).toNumber()
                          , c->argument(3).toNumber()
                          , c->argument(4).toNumber()
                          , c->argument(5).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_transform(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 6) {
        ctx2d->transform(c->argument(0).toNumber()
                       , c->argument(1).toNumber()
                       , c->argument(2).toNumber()
                       , c->argument(3).toNumber()
                       , c->argument(4).toNumber()
                       , c->argument(5).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_translate(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 2) {
        ctx2d->translate(c->argument(0).toNumber()
                       , c->argument(1).toNumber());
    }
    return e->nullValue();
}

// compositing
// float getter/setter default 1.0
static QScriptValue ctx2d_globalAlpha(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {//setter
        ctx2d->setGlobalAlpha(c->argument(0).toNumber());
    }
    return e->toScriptValue(ctx2d->globalAlpha());
}

// string getter/setter default "source-over"
static QScriptValue ctx2d_globalCompositeOperation(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (ctx2d) {
        if (c->argumentCount() == 1) {//setter
            ctx2d->setGlobalCompositeOperation(c->argument(0).toString());
        }
        return e->toScriptValue(ctx2d->globalCompositeOperation());
    }
    return e->nullValue();
}

// colors and styles
// getter/setter
static QScriptValue ctx2d_fillStyle(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {//setter
        ctx2d->setFillStyle(c->argument(0).toVariant());
    }
    return e->toScriptValue(ctx2d->fillStyle());
}

// colors and styles
// getter/setter
static QScriptValue ctx2d_fillColor(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {//setter
        ctx2d->setFillColor(c->argument(0).toVariant().value<QColor>());
    }
    return e->toScriptValue(ctx2d->fillColor());
}


//getter/setter
static QScriptValue ctx2d_strokeStyle(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {//setter
        ctx2d->setStrokeStyle(c->argument(0).toVariant());
    }
    return e->toScriptValue(ctx2d->strokeStyle());
}

// colors and styles
// getter/setter
static QScriptValue ctx2d_strokeColor(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {//setter
        ctx2d->setStrokeColor(c->argument(0).toVariant().value<QColor>());
    }
    return e->toScriptValue(ctx2d->strokeColor());
}

static QScriptValue ctx2d_createLinearGradient(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 4) {
        QObject* gradient = ctx2d->createLinearGradient( c->argument(0).toNumber()
                                                        ,c->argument(1).toNumber()
                                                        ,c->argument(2).toNumber()
                                                        ,c->argument(3).toNumber());
        return e->toScriptValue(gradient);
    }
    return e->nullValue();
}
static QScriptValue ctx2d_createRadialGradient(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 6) {
        QObject* gradient = ctx2d->createRadialGradient( c->argument(0).toNumber()
                                                        ,c->argument(1).toNumber()
                                                        ,c->argument(2).toNumber()
                                                        ,c->argument(3).toNumber()
                                                        ,c->argument(4).toNumber()
                                                        ,c->argument(5).toNumber());
        return e->toScriptValue(gradient);
    }
    return e->nullValue();
}
static QScriptValue ctx2d_createPattern(QScriptContext *c, QScriptEngine *e)
{
    //TODO
    return e->nullValue();
}

// line styles
// string getter/setter
static QScriptValue ctx2d_lineCap(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {//setter
        ctx2d->setLineCap(c->argument(0).toString());
    }
    return e->toScriptValue(ctx2d->lineCap());
}

// string getter/setter
static QScriptValue ctx2d_lineJoin(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {//setter
        ctx2d->setLineJoin(c->argument(0).toString());
    }
    return e->toScriptValue(ctx2d->lineJoin());
}
// float getter/setter
static QScriptValue ctx2d_lineWidth(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {//setter
        ctx2d->setLineWidth(c->argument(0).toNumber());
    }
    return e->toScriptValue(ctx2d->lineWidth());
}
// float getter/setter
static QScriptValue ctx2d_miterLimit(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {//setter
        ctx2d->setMiterLimit(c->argument(0).toNumber());
    }
    return e->toScriptValue(ctx2d->miterLimit());
}

// shadows
// float getter/setter
static QScriptValue ctx2d_shadowBlur(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {//setter
        ctx2d->setShadowBlur(c->argument(0).toNumber());
    }
    return e->toScriptValue(ctx2d->shadowBlur());
}
static QScriptValue ctx2d_shadowColor(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {//setter
        ctx2d->setShadowColor(c->argument(0).toString());
    }
    return e->toScriptValue(ctx2d->shadowColor());
}
static QScriptValue ctx2d_shadowOffsetX(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {//setter
        ctx2d->setShadowOffsetX(c->argument(0).toNumber());
    }
    return e->toScriptValue(ctx2d->shadowOffsetX());
}

static QScriptValue ctx2d_shadowOffsetY(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 1) {//setter
        ctx2d->setShadowOffsetY(c->argument(0).toNumber());
    }
    return e->toScriptValue(ctx2d->shadowOffsetY());
}

//rects
static QScriptValue ctx2d_clearRect(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 4) {
        ctx2d->clearRect(c->argument(0).toNumber()
                        ,c->argument(1).toNumber()
                        ,c->argument(2).toNumber()
                        ,c->argument(3).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_fillRect(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 4) {
        ctx2d->fillRect(c->argument(0).toNumber()
                       ,c->argument(1).toNumber()
                       ,c->argument(2).toNumber()
                       ,c->argument(3).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_strokeRect(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 4) {
        ctx2d->strokeRect(c->argument(0).toNumber()
                         ,c->argument(1).toNumber()
                         ,c->argument(2).toNumber()
                         ,c->argument(3).toNumber());
    }
    return e->nullValue();
}

// Complex shapes (paths) API
static QScriptValue ctx2d_arc(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 6) {
        ctx2d->arc(c->argument(0).toNumber()
                  ,c->argument(1).toNumber()
                  ,c->argument(2).toNumber()
                  ,c->argument(3).toNumber()
                  ,c->argument(4).toNumber()
                  ,c->argument(5).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_arcTo(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 5) {
        ctx2d->arcTo(c->argument(0).toNumber()
                    ,c->argument(1).toNumber()
                    ,c->argument(2).toNumber()
                    ,c->argument(3).toNumber()
                    ,c->argument(4).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_beginPath(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    ctx2d->beginPath();
    return e->nullValue();
}
static QScriptValue ctx2d_bezierCurveTo(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 5) {
        ctx2d->bezierCurveTo(c->argument(0).toNumber()
                            ,c->argument(1).toNumber()
                            ,c->argument(2).toNumber()
                            ,c->argument(3).toNumber()
                            ,c->argument(4).toNumber()
                            ,c->argument(5).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_clip(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    ctx2d->clip();
    return e->nullValue();
}
static QScriptValue ctx2d_closePath(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    ctx2d->closePath();
    return e->nullValue();
}
static QScriptValue ctx2d_fill(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    ctx2d->fill();
    return e->nullValue();
}
static QScriptValue ctx2d_lineTo(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 2) {
        ctx2d->lineTo(c->argument(0).toNumber()
                     ,c->argument(1).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_moveTo(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 2) {
        ctx2d->moveTo(c->argument(0).toNumber()
                     ,c->argument(1).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_quadraticCurveTo(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 4) {
        ctx2d->quadraticCurveTo(c->argument(0).toNumber()
                               ,c->argument(1).toNumber()
                               ,c->argument(2).toNumber()
                               ,c->argument(3).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_rect(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 4) {
        ctx2d->rect(c->argument(0).toNumber()
                   ,c->argument(1).toNumber()
                   ,c->argument(2).toNumber()
                   ,c->argument(3).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_stroke(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    ctx2d->stroke();
    return e->nullValue();
}
static QScriptValue ctx2d_isPointInPath(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    bool pointInPath = false;
    if (c->argumentCount() == 2) {
        pointInPath = ctx2d->isPointInPath(c->argument(0).toNumber()
                                          ,c->argument(1).toNumber());
    }
    return e->toScriptValue(pointInPath);
}

// text
static QScriptValue ctx2d_font(QScriptContext *c, QScriptEngine *e)
{
}
static QScriptValue ctx2d_textAlign(QScriptContext *c, QScriptEngine *e)
{
}
static QScriptValue ctx2d_textBaseline(QScriptContext *c, QScriptEngine *e)
{
}
static QScriptValue ctx2d_fillText(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 3) {
        ctx2d->fillText(c->argument(0).toString()
                       ,c->argument(1).toNumber()
                       ,c->argument(2).toNumber());
    }
    return e->nullValue();
}
static QScriptValue ctx2d_strokeText(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 3) {
        ctx2d->strokeText(c->argument(0).toString()
                         ,c->argument(1).toNumber()
                         ,c->argument(2).toNumber());
    }
    return e->nullValue();
}

// drawing images
static QScriptValue ctx2d_drawImage(QScriptContext *c, QScriptEngine *e)
{
    QSGContext2D* ctx2d = qscriptvalue_cast<QSGContext2D*>(c->thisObject().data());
    if (c->argumentCount() == 3) {
        ctx2d->drawImage(c->argument(0).toString()
                        ,c->argument(1).toNumber()
                        ,c->argument(2).toNumber());
    } else if (c->argumentCount() == 5) {
        ctx2d->drawImage(c->argument(0).toString()
                        ,c->argument(1).toNumber()
                        ,c->argument(2).toNumber()
                        ,c->argument(3).toNumber()
                        ,c->argument(4).toNumber());
    } else if (c->argumentCount() == 9) {
        ctx2d->drawImage(c->argument(0).toString()
                        ,c->argument(1).toNumber()
                        ,c->argument(2).toNumber()
                        ,c->argument(3).toNumber()
                        ,c->argument(4).toNumber()
                        ,c->argument(5).toNumber()
                        ,c->argument(6).toNumber()
                        ,c->argument(7).toNumber()
                        ,c->argument(8).toNumber());
    }
    return e->nullValue();
}

// pixel manipulation
static QScriptValue ctx2d_createImageData(QScriptContext *c, QScriptEngine *e)
{
    //#TODO
}
static QScriptValue ctx2d_getImageData(QScriptContext *c, QScriptEngine *e)
{
    //#TODO
}
static QScriptValue ctx2d_putImageData(QScriptContext *c, QScriptEngine *e)
{
    //#TODO
}

//Image Data Interface
static QScriptValue ctx2d_imageData_data(QScriptContext *c, QScriptEngine *e)
{
    //#TODO
}
static QScriptValue ctx2d_imageData_height(QScriptContext *c, QScriptEngine *e)
{
    //#TODO
}
static QScriptValue ctx2d_imageData_width(QScriptContext *c, QScriptEngine *e)
{
    //#TODO
}

//CanvasPixelArray interface
static QScriptValue ctx2d_pixelArray_length(QScriptContext *c, QScriptEngine *e)
{
    //#TODO
}
//getter/setter by index how to?
static QScriptValue ctx2d_pixelArray(QScriptContext *c, QScriptEngine *e)
{
    //#TODO
}

//CanvasGradient interface
static QScriptValue ctx2d_gradient_addColorStop(QScriptContext *c, QScriptEngine *e)
{
    //#TODO

}

//TextMetrics
static QScriptValue ctx2d_textMetrics_width(QScriptContext *c, QScriptEngine *e)
{
    //#TODO

}


bool QSGContext2DPrivate::hasShadow() const
{
    return state.shadowColor.isValid()
        && state.shadowColor.alpha()
        && (state.shadowBlur || state.shadowOffsetX || state.shadowOffsetY);
}

void QSGContext2DPrivate::clearShadow()
{
    state.shadowOffsetX = 0;
    state.shadowOffsetY = 0;
    state.shadowBlur = 0;
    state.shadowColor = QColor();
}

QImage QSGContext2DPrivate::makeShadowImage(const QPixmap& pix)
{
    QImage shadowImg(pix.width() + state.shadowBlur * 2 + qAbs(state.shadowOffsetX),
                     pix.height() + state.shadowBlur *2 + qAbs(state.shadowOffsetY),
                     QImage::Format_ARGB32);
    shadowImg.fill(0);
    QPainter tmpPainter(&shadowImg);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
    qreal shadowX = state.shadowOffsetX > 0? state.shadowOffsetX : 0;
    qreal shadowY = state.shadowOffsetY > 0? state.shadowOffsetY : 0;

    tmpPainter.drawPixmap(shadowX, shadowY, pix);
    tmpPainter.end();

    // blur the alpha channel
    if (state.shadowBlur > 0) {
        QImage blurred(shadowImg.size(), QImage::Format_ARGB32);
        blurred.fill(0);
        QPainter blurPainter(&blurred);
        qt_blurImage(&blurPainter, shadowImg, state.shadowBlur, false, true);
        blurPainter.end();
        shadowImg = blurred;
    }

    // blacken the image with shadow color...
    tmpPainter.begin(&shadowImg);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(shadowImg.rect(), state.shadowColor);
    tmpPainter.end();
    return shadowImg;
}

void QSGContext2DPrivate::fillRectShadow(QPainter* p, QRectF shadowRect)
{
    QRectF r = shadowRect;
    r.moveTo(0, 0);

    QImage shadowImage(r.size().width() + 1, r.size().height() + 1, QImage::Format_ARGB32);
    QPainter tp;
    tp.begin(&shadowImage);
    tp.fillRect(r, p->brush());
    tp.end();
    shadowImage = makeShadowImage(QPixmap::fromImage(shadowImage));

    qreal dx = shadowRect.left() + (state.shadowOffsetX < 0? state.shadowOffsetX:0);
    qreal dy = shadowRect.top() + (state.shadowOffsetY < 0? state.shadowOffsetY:0);

    p->drawImage(dx, dy, shadowImage);
    p->fillRect(shadowRect, p->brush());
}

void QSGContext2DPrivate::fillShadowPath(QPainter* p, const QPainterPath& path)
{
    QRectF r = path.boundingRect();
    QImage img(r.size().width() + r.left() + 1,
               r.size().height() + r.top() + 1,
               QImage::Format_ARGB32);
    img.fill(0);
    QPainter tp(&img);
    tp.fillPath(path.translated(0, 0), p->brush());
    tp.end();

    QImage shadowImage = makeShadowImage(QPixmap::fromImage(img));
    qreal dx = r.left() + (state.shadowOffsetX < 0? state.shadowOffsetX:0);
    qreal dy = r.top() + (state.shadowOffsetY < 0? state.shadowOffsetY:0);

    p->drawImage(dx, dy, shadowImage);
    p->fillPath(path, p->brush());
}

void QSGContext2DPrivate::strokeShadowPath(QPainter* p, const QPainterPath& path)
{
    QRectF r = path.boundingRect();
    QImage img(r.size().width() + r.left() + 1,
               r.size().height() + r.top() + 1,
               QImage::Format_ARGB32);
    img.fill(0);
    QPainter tp(&img);
    tp.strokePath(path, p->pen());
    tp.end();

    QImage shadowImage = makeShadowImage(QPixmap::fromImage(img));
    qreal dx = r.left() + (state.shadowOffsetX < 0? state.shadowOffsetX:0);
    qreal dy = r.top() + (state.shadowOffsetY < 0? state.shadowOffsetY:0);
    p->drawImage(dx, dy, shadowImage);
    p->strokePath(path, p->pen());
}

void QSGContext2DPrivate::clear()
{
    clearRect(0, 0, size.width(), size.height());
}

void QSGContext2DPrivate::reset()
{
    stateStack.clear();
    state.matrix = QMatrix();
    state.clipPath = QPainterPath();
    state.strokeStyle = Qt::black;
    state.fillStyle = Qt::black;
    state.globalAlpha = 1.0;
    state.lineWidth = 1;
    state.lineCap = Qt::FlatCap;
    state.lineJoin = Qt::MiterJoin;
    state.miterLimit = 10;
    state.shadowOffsetX = 0;
    state.shadowOffsetY = 0;
    state.shadowBlur = 0;
    state.shadowColor = qRgba(0, 0, 0, 0);
    state.globalCompositeOperation = QPainter::CompositionMode_SourceOver;
    state.font = QFont();
    state.textAlign = QSGContext2D::Start;
    state.textBaseline = QSGContext2D::Alphabetic;
    clear();
}


void QSGContext2DPrivate::updateMatrix(const QMatrix& m)
{
    commands.push_back(QSGContext2D::UpdateMatrix);
    matrixes.push_back(m);
}




void QSGContext2DPrivate::save()
{
    stateStack.push(state);
}

void QSGContext2DPrivate::restore()
{
    if (!stateStack.isEmpty()) {
        bool update = false;
        QSGContext2D::State s = stateStack.pop();
        if (state.matrix != s.matrix) {
            updateMatrix(s.matrix);
            update = true;
        }

        if (s.pen != state.pen) {
            commands.push_back(QSGContext2D::UpdatePen);
            pens.push_back(s.pen);
            update = true;
        }

        if (s.globalAlpha != state.globalAlpha) {
            commands.push_back(QSGContext2D::GlobalAlpha);
            reals.push_back(s.globalAlpha);
            update = true;
        }

        if (s.globalCompositeOperation != state.globalCompositeOperation) {
            commands.push_back(QSGContext2D::GlobalCompositeOperation);
            ints.push_back(s.globalCompositeOperation);
            update = true;
        }

        if (s.font != state.font) {
            commands.push_back(QSGContext2D::Font);
            fonts.push_back(s.font);
            update = true;
        }

        if (s.fillStyle != state.fillStyle) {
            commands.push_back(QSGContext2D::FillStyle);
            brushes.push_back(s.fillStyle);
            update = true;
        }

        if (s.clipPath != state.clipPath) {
            //commands.push_back(QSGContext2D::ClipPath);
            update = true;
        }

        if (s.textAlign != state.textAlign) {
            commands.push_back(QSGContext2D::TextAlign);
            update = true;
        }

        if (s.textBaseline != state.textBaseline) {
            commands.push_back(QSGContext2D::TextBaseline);
            update = true;
        }

        if (s.shadowBlur != state.shadowBlur
         || s.shadowColor != state.shadowColor
         || s.shadowOffsetX != state.shadowOffsetX
         || s.shadowOffsetY != state.shadowOffsetY) {
            update = true;
        }

        if (update)
            state = s;
    }
}

void QSGContext2DPrivate::scale(qreal x, qreal y)
{
    state.matrix.scale(x, y);
    updateMatrix(state.matrix);
}

void QSGContext2DPrivate::rotate(qreal angle)
{
    state.matrix.rotate(DEGREES(angle));
    updateMatrix(state.matrix);
}


void QSGContext2DPrivate::translate(qreal x, qreal y)
{
    state.matrix.translate(x, y);
    updateMatrix(state.matrix);
}

void QSGContext2DPrivate::transform(
                                    qreal m11, qreal m12,
                                    qreal m21, qreal m22,
                                    qreal dx, qreal dy)
{
    QMatrix matrix(m11, m12, m21, m22, dx, dy);
    state.matrix *= matrix;
    updateMatrix(state.matrix);
}

void QSGContext2DPrivate::setTransform(
                                       qreal m11, qreal m12,
                                       qreal m21, qreal m22,
                                       qreal dx, qreal dy)
{
   QMatrix matrix(m11, m12, m21, m22, dx, dy);
   state.matrix = matrix;
   updateMatrix(state.matrix);
}

void QSGContext2DPrivate::clearRect(qreal x, qreal y,
                                    qreal w, qreal h)
{
    commands.push_back(QSGContext2D::ClearRect);
    reals.push_back(x);
    reals.push_back(y);
    reals.push_back(w);
    reals.push_back(h);
}

void QSGContext2DPrivate::fillRect(qreal x, qreal y,
                                   qreal w, qreal h)
{
    commands.push_back(QSGContext2D::FillRect);
    reals.push_back(x);
    reals.push_back(y);
    reals.push_back(w);
    reals.push_back(h);
}

void QSGContext2DPrivate::strokeRect(qreal x, qreal y,
                                     qreal w, qreal h)
{
    QPainterPath path;
    path.addRect(x, y, w, h);
    commands.push_back(QSGContext2D::Stroke);
    pathes.push_back(path);
}

void QSGContext2DPrivate::beginPath()
{
    path = QPainterPath();
}

void QSGContext2DPrivate::closePath()
{
    path.closeSubpath();
}

void QSGContext2DPrivate::moveTo( qreal x, qreal y)
{
    path.moveTo(state.matrix.map(QPointF(x, y)));
}

void QSGContext2DPrivate::lineTo( qreal x, qreal y)
{
    path.lineTo(state.matrix.map(QPointF(x, y)));
}

void QSGContext2DPrivate::quadraticCurveTo(qreal cpx, qreal cpy,
                                           qreal x, qreal y)
{
    path.quadTo(state.matrix.map(QPointF(cpx, cpy)),
                      state.matrix.map(QPointF(x, y)));
}

void QSGContext2DPrivate::bezierCurveTo(qreal cp1x, qreal cp1y,
                                        qreal cp2x, qreal cp2y,
                                        qreal x, qreal y)
{
    path.cubicTo(state.matrix.map(QPointF(cp1x, cp1y)),
                       state.matrix.map(QPointF(cp2x, cp2y)),
                       state.matrix.map(QPointF(x, y)));
}

void QSGContext2DPrivate::arcTo(qreal x1, qreal y1,
                                qreal x2, qreal y2,
                                qreal radius)
{
    QPointF st  = state.matrix.map(QPoint(x1, y1));
    QPointF end = state.matrix.map(QPoint(x2, y2));

    path.arcTo(st.x(), st.y(),
                     end.x()-st.x(), end.y()-st.y(),
                     radius, 90);
}

void QSGContext2DPrivate::rect(qreal x, qreal y,
                               qreal w, qreal h)
{
    QPainterPath path;
    path.addRect(QRectF(x, y, w, h));
    path.addPath(state.matrix.map(path));
}

void QSGContext2DPrivate::arc(qreal xc,
                              qreal yc,
                              qreal radius,
                              qreal sar,
                              qreal ear,
                              bool antiClockWise)
{
    QPainterPath path;

    //### HACK

    // In Qt we don't switch the coordinate system for degrees
    // and still use the 0,0 as bottom left for degrees so we need
    // to switch
    sar = -sar;
    ear = -ear;
    antiClockWise = !antiClockWise;
    //end hack

    float sa = DEGREES(sar);
    float ea = DEGREES(ear);

    double span = 0;

    double xs     = xc - radius;
    double ys     = yc - radius;
    double width  = radius*2;
    double height = radius*2;

    if (!antiClockWise && (ea < sa)) {
        span += 360;
    } else if (antiClockWise && (sa < ea)) {
        span -= 360;
    }

    //### this is also due to switched coordinate system
    // we would end up with a 0 span instead of 360
    if (!(qFuzzyCompare(span + (ea - sa) + 1, 1) &&
          qFuzzyCompare(qAbs(span), 360))) {
        span   += ea - sa;
    }

    path.moveTo(QPointF(xc + radius  * qCos(sar),
                        yc - radius  * qSin(sar)));

    path.arcTo(xs, ys, width, height, sa, span);

    path.addPath(state.matrix.map(path));
}

void QSGContext2DPrivate::fill()
{
    commands.push_back(QSGContext2D::Fill);
    pathes.push_back(path);
}

void QSGContext2DPrivate::stroke()
{
    commands.push_back(QSGContext2D::Stroke);
    pathes.push_back(path);
//        painter->setMatrix(state.matrix, false);
//        QPainterPath tmp = state.matrix.inverted().map(path); //why?
//        painter->strokePath(tmp, painter->pen());
}

void QSGContext2DPrivate::clip()
{
    state.clipPath = path;
    pathes.push_back(state.clipPath);
    commands.push_back(QSGContext2D::Clip);
}

void QSGContext2DPrivate::setGlobalAlpha( qreal alpha)
{
    state.globalAlpha = alpha;
    commands.push_back(QSGContext2D::GlobalAlpha);
    reals.push_back(state.globalAlpha);
}

void QSGContext2DPrivate::setGlobalCompositeOperation( const QString &op)
{
    state.globalCompositeOperation = compositeOperatorFromString(op);
    commands.push_back(QSGContext2D::GlobalCompositeOperation);
    ints.push_back(state.globalCompositeOperation);
}

void QSGContext2DPrivate::setStrokeStyle( const QVariant &style)
{
    QSGCanvasGradient *gradient= qobject_cast<QSGCanvasGradient*>(style.value<QObject*>());
    QBrush b;
    if (gradient) {
        b = gradient->value();
    } else {
        b =  colorFromString(style.toString());
    }

    if (state.strokeStyle != b) {
        state.strokeStyle = b;
        state.pen.setBrush(state.strokeStyle);
        commands.push_back(QSGContext2D::UpdatePen);
        pens.push_back(state.pen);
    }
}
void QSGContext2DPrivate::setStrokeColor(const QColor& color)
{
    if (state.strokeStyle != color) {
        state.strokeStyle = color;
        commands.push_back(QSGContext2D::UpdatePen);
        QPen pen;
        pen.setBrush(state.strokeStyle);
        pens.push_back(pen);
    }
}

void QSGContext2DPrivate::setFillColor(const QColor& color)
{
    if (state.fillStyle != color) {
        state.fillStyle = color;
        commands.push_back(QSGContext2D::UpdateBrush);
        brushes.push_back(state.fillStyle);
    }
}

void QSGContext2DPrivate::setFillStyle( const QVariant &style)
{
    QSGCanvasGradient *gradient= qobject_cast<QSGCanvasGradient*>(style.value<QObject*>());
    QBrush b;
    if (gradient) {
        b = gradient->value();
    } else {
        b =  colorFromString(style.toString());
    }

    if (state.fillStyle != b) {
        state.fillStyle = b;
        commands.push_back(QSGContext2D::UpdateBrush);
        brushes.push_back(b);
    }
}


void QSGContext2DPrivate::setLineWidth( qreal w)
{
    if (state.lineWidth != w) {
        state.pen.setWidthF(w);
        state.lineWidth = w;
        commands.push_back(QSGContext2D::UpdatePen);
        pens.push_back(state.pen);
    }
}

void QSGContext2DPrivate::setLineCap( const QString& cap)
{
    Qt::PenCapStyle style;
    if (cap == QLatin1String("round"))
        style = Qt::RoundCap;
    else if (cap == QLatin1String("square"))
        style = Qt::SquareCap;
    else //if (capString == "butt")
        style = Qt::FlatCap;


    if (state.lineCap != style) {
        state.pen.setCapStyle(style);
        state.lineCap = style;
        commands.push_back(QSGContext2D::UpdatePen);
        pens.push_back(state.pen);
    }
}

void QSGContext2DPrivate::setLineJoin( const QString& join)
{
    Qt::PenJoinStyle style;
    if (join == QLatin1String("round"))
        style = Qt::RoundJoin;
    else if (join == QLatin1String("bevel"))
        style = Qt::BevelJoin;
    else //if (joinString == "miter")
        style = Qt::MiterJoin;
    if (state.lineJoin != style) {
        state.lineJoin = style;
        state.pen.setJoinStyle(style);
        commands.push_back(QSGContext2D::UpdatePen);
        pens.push_back(state.pen);
    }
}

void QSGContext2DPrivate::setMiterLimit( qreal limit)
{
    if (state.miterLimit != limit) {
        state.pen.setMiterLimit(limit);
        state.miterLimit = limit;
        commands.push_back(QSGContext2D::UpdatePen);
        pens.push_back(state.pen);
    }
}

void QSGContext2DPrivate::setShadowOffsetX( qreal x)
{
    if (state.shadowOffsetX != x) {
        state.shadowOffsetX = x;
        commands.push_back(QSGContext2D::ShadowOffsetX);
        reals.push_back(x);
    }
}

void QSGContext2DPrivate::setShadowOffsetY( qreal y)
{
    if (state.shadowOffsetY != y) {
        state.shadowOffsetY = y;
        commands.push_back(QSGContext2D::ShadowOffsetY);
        reals.push_back(y);
    }
}

void QSGContext2DPrivate::setShadowBlur( qreal b)
{
    if (state.shadowBlur != b) {
        state.shadowBlur = b;
        commands.push_back(QSGContext2D::ShadowBlur);
        reals.push_back(b);
    }
}

void QSGContext2DPrivate::setShadowColor( const QString&  color)
{
    QColor c = colorFromString(color);
    if (state.shadowColor != c) {
        state.shadowColor = c;
        commands.push_back(QSGContext2D::ShadowColor);
        colors.push_back(c);
    }
}

void QSGContext2DPrivate::setFont( const QString&  fontString)
{
   QFont font;
    // ### this is simplified and incomplete
   // ### TODO:get code from Qt webkit
    QStringList tokens = fontString.split(QLatin1String(" "));
    foreach (const QString &token, tokens) {
        if (token == QLatin1String("italic"))
            font.setItalic(true);
        else if (token == QLatin1String("bold"))
            font.setBold(true);
        else if (token.endsWith(QLatin1String("px"))) {
            QString number = token;
            number.remove(QLatin1String("px"));
            font.setPointSizeF(number.trimmed().toFloat());
        } else
            font.setFamily(token);
    }

    if (state.font != font) {
        state.font = font;
        commands.push_back(QSGContext2D::Font);
        fonts.push_back(font);
    }
}

void QSGContext2DPrivate::setTextBaseline( const QString&  baseline)
{
    QSGContext2D::TextBaseLineType tbl;
     if (baseline==QLatin1String("alphabetic"))
        tbl = QSGContext2D::Alphabetic;
    else if (baseline == QLatin1String("hanging"))
        tbl = QSGContext2D::Hanging;
    else if (baseline == QLatin1String("top"))
        tbl = QSGContext2D::Top;
    else if (baseline == QLatin1String("bottom"))
        tbl = QSGContext2D::Bottom;
    else if (baseline == QLatin1String("middle"))
        tbl = QSGContext2D::Middle;
    else {
        tbl = QSGContext2D::Alphabetic;
        Q_Q(QSGContext2D);
        qmlInfo(q) << "QSGContext2D: invalid baseline:" << baseline;
    }
    if (state.textBaseline != tbl) {
        state.textBaseline = tbl;
        commands.push_back(QSGContext2D::TextBaseline);
        ints.push_back(tbl);
    }
}

void QSGContext2DPrivate::setTextAlign(const QString&  align)
{
    QSGContext2D::TextAlignType ta;
     if (align==QLatin1String("start"))
        ta = QSGContext2D::Start;
    else if (align == QLatin1String("end"))
        ta = QSGContext2D::End;
    else if (align == QLatin1String("left"))
        ta = QSGContext2D::Left;
    else if (align == QLatin1String("right"))
        ta = QSGContext2D::Right;
    else if (align == QLatin1String("center"))
        ta = QSGContext2D::Center;
    else {
        ta = QSGContext2D::Start;
        Q_Q(QSGContext2D);
        qmlInfo(q)  << "QSGContext2D: invalid text align:" << align;
    }
     if (state.textAlign != ta) {
         state.textAlign = ta;
         commands.push_back(QSGContext2D::TextAlign);
         ints.push_back(ta);
     }
}

void QSGContext2DPrivate::fillText(const QString&  text, qreal x, qreal y)
{
    commands.push_back(QSGContext2D::FillText);
    strings.push_back(text);
    reals.push_back(x);
    reals.push_back(y);
    ints.push_back(state.textAlign);
    ints.push_back(state.textBaseline);
}


void QSGContext2DPrivate::strokeText( const QString&  text, qreal x, qreal y)
{
    commands.push_back(QSGContext2D::StrokeText);
    strings.push_back(text);
    reals.push_back(x);
    reals.push_back(y);
    ints.push_back(state.textAlign);
    ints.push_back(state.textBaseline);
}

void QSGContext2DPrivate::drawImage(const QString& url, qreal dx, qreal dy)
{
    commands.push_back(QSGContext2D::DrawImage1);
    strings.push_back(url);
    reals.push_back(dx);
    reals.push_back(dy);
}

void QSGContext2DPrivate::drawImage(const QString& url, qreal dx, qreal dy, qreal dw, qreal dh)
{
    commands.push_back(QSGContext2D::DrawImage2);
    strings.push_back(url);
    reals.push_back(dx);
    reals.push_back(dy);
    reals.push_back(dw);
    reals.push_back(dh);
}

void QSGContext2DPrivate::drawImage(const QString& url, qreal sx, qreal sy, qreal sw, qreal sh, qreal dx, qreal dy, qreal dw, qreal dh)
{
    commands.push_back(QSGContext2D::DrawImage3);
    strings.push_back(url);
    reals.push_back(sx);
    reals.push_back(sy);
    reals.push_back(sw);
    reals.push_back(sh);
    reals.push_back(dx);
    reals.push_back(dy);
    reals.push_back(dw);
    reals.push_back(dh);
}

QList<int> QSGContext2DPrivate::getImageData(qreal sx, qreal sy, qreal sw, qreal sh)
{
    Q_Q(QSGContext2D);
    waitingForPainting = true;
    commands.push_back(QSGContext2D::GetImageData);
    reals.push_back(sx);
    reals.push_back(sy);
    reals.push_back(sw);
    reals.push_back(sh);
    q->sync();
   return imageData;
}

void QSGContext2DPrivate::putImageData(const QVariantList& imageData, qreal dx, qreal dy, qreal w, qreal h)
{
    QImage image = cachedImage.copy(dx, dy, w, h);
    uchar* data = image.bits();
    int i = 0;
    while(i< imageData.size() && i < image.byteCount()) {
        //the stored order in QImage:BGRA
        //the stored order in Canvas:RGBA
        *(data+i)   = imageData[i+2].toInt();//B
        *(data+i+1) = imageData[i+1].toInt();//G
        *(data+i+2) = imageData[i].toInt();//R
        *(data+i+3) = imageData[i+3].toInt();//A
        i+=4;
    }
    commands.push_back(QSGContext2D::PutImageData);
    images.push_back(image);
    reals.push_back(dx);
    reals.push_back(dy);
}

void QSGContext2D::save()
{
    Q_D(QSGContext2D);
    d->save();
}


void QSGContext2D::restore()
{
    Q_D(QSGContext2D);
    d->restore();
}


void QSGContext2D::scale(qreal x, qreal y)
{
    Q_D(QSGContext2D);
    d->scale(x, y);
}


void QSGContext2D::rotate(qreal angle)
{
    Q_D(QSGContext2D);
    d->rotate(angle);
}


void QSGContext2D::translate(qreal x, qreal y)
{
    Q_D(QSGContext2D);
    d->translate(x, y);
}

void QSGContext2D::transform(qreal m11, qreal m12, qreal m21, qreal m22,
                          qreal dx, qreal dy)
{
    Q_D(QSGContext2D);
    d->transform(m11, m12, m21, m22, dx, dy);
}


void QSGContext2D::setTransform(qreal m11, qreal m12, qreal m21, qreal m22,
                             qreal dx, qreal dy)
{
    Q_D(QSGContext2D);
    d->setTransform(m11, m12, m21, m22, dx, dy);
}

QString QSGContext2D::globalCompositeOperation() const
{
    Q_D(const QSGContext2D);
    return compositeOperatorToString(d->state.globalCompositeOperation);
}

void QSGContext2D::setGlobalCompositeOperation(const QString &op)
{
    Q_D(QSGContext2D);
    d->setGlobalCompositeOperation(op);
}

QVariant QSGContext2D::strokeStyle() const
{
    Q_D(const QSGContext2D);
    return d->state.strokeStyle;
}

void QSGContext2D::setStrokeStyle(const QVariant &style)
{
    Q_D(QSGContext2D);
    d->setStrokeStyle(style);
}

QVariant QSGContext2D::fillStyle() const
{
    Q_D(const QSGContext2D);
    return d->state.fillStyle;
}

QColor QSGContext2D::strokeColor() const
{
    Q_D(const QSGContext2D);
    return d->state.strokeStyle.color();
}

QColor QSGContext2D::fillColor() const
{
    Q_D(const QSGContext2D);
    return d->state.fillStyle.color();
}

void QSGContext2D::setFillStyle(const QVariant &style)
{
    Q_D(QSGContext2D);
    d->setFillStyle(style);
}
void QSGContext2D::setStrokeColor(const QColor& color)
{
    Q_D(QSGContext2D);
    d->setStrokeColor(color);
}

void QSGContext2D::setFillColor(const QColor& color)
{
    Q_D(QSGContext2D);
    d->setFillColor(color);
}

qreal QSGContext2D::globalAlpha() const
{
    Q_D(const QSGContext2D);
    return d->state.globalAlpha;
}

void QSGContext2D::setGlobalAlpha(qreal alpha)
{
    Q_D(QSGContext2D);
    d->setGlobalAlpha(alpha);
}

QSGImage *QSGContext2D::createImage(const QString &url)
{
    Q_D(QSGContext2D);
//### cache image
    QSGImage* img = new QSGImage(d->canvas);
    img->setSource(QUrl(url));
    return img;
}

QSGCanvasGradient *QSGContext2D::createLinearGradient(qreal x0, qreal y0,
                                                qreal x1, qreal y1)
{
    QLinearGradient g(x0, y0, x1, y1);
    return new QSGCanvasGradient(g);
}


QSGCanvasGradient *QSGContext2D::createRadialGradient(qreal x0, qreal y0,
                                                qreal r0, qreal x1,
                                                qreal y1, qreal r1)
{
    QRadialGradient g(QPointF(x1, y1), r0+r1, QPointF(x0, y0));
    return new QSGCanvasGradient(g);
}

qreal QSGContext2D::lineWidth() const
{
    Q_D(const QSGContext2D);
    return d->state.lineWidth;
}

void QSGContext2D::setLineWidth(qreal w)
{
    Q_D(QSGContext2D);
    d->setLineWidth(w);
}

QString QSGContext2D::lineCap() const
{
    Q_D(const QSGContext2D);
    switch(d->state.lineCap) {
    case Qt::RoundCap:
        return QLatin1String("round");
    case Qt::FlatCap:
        return QLatin1String("butt");
    case Qt::SquareCap:
        return QLatin1String("square");
    default:
        break;
    }
    return QLatin1String("");
}

void QSGContext2D::setLineCap(const QString &capString)
{
    Q_D(QSGContext2D);
    d->setLineCap(capString);
}

QString QSGContext2D::lineJoin() const
{
    Q_D(const QSGContext2D);
    switch (d->state.lineJoin) {
    case Qt::RoundJoin:
        return QLatin1String("round");
    case Qt::BevelJoin:
        return QLatin1String("bevel");
    case Qt::MiterJoin:
        return QLatin1String("miter");
    default:
        break;
    }
    return QLatin1String("");
}

void QSGContext2D::setLineJoin(const QString &joinString)
{
    Q_D(QSGContext2D);
    d->setLineJoin(joinString);
}

qreal QSGContext2D::miterLimit() const
{
    Q_D(const QSGContext2D);
    return d->state.miterLimit;
}

void QSGContext2D::setMiterLimit(qreal m)
{
    Q_D(QSGContext2D);
    d->setMiterLimit(m);
}

void QSGContext2D::setShadowOffsetX(qreal x)
{
    Q_D(QSGContext2D);
    d->setShadowOffsetX(x);
}

void QSGContext2D::setShadowOffsetY(qreal y)
{
    Q_D(QSGContext2D);
    d->setShadowOffsetY(y);
}

void QSGContext2D::setShadowBlur(qreal b)
{
    Q_D(QSGContext2D);
    d->setShadowBlur(b);
}

void QSGContext2D::setShadowColor(const QString &str)
{
    Q_D(QSGContext2D);
    d->setShadowColor(str);
}

QString QSGContext2D::textBaseline() const
{
    Q_D(const QSGContext2D);
    switch(d->state.textBaseline) {
    case QSGContext2D::Alphabetic:
        return QLatin1String("alphabetic");
    case QSGContext2D::Hanging:
        return QLatin1String("hanging");
    case QSGContext2D::Top:
        return QLatin1String("top");
    case QSGContext2D::Bottom:
        return QLatin1String("bottom");
    case QSGContext2D::Middle:
        return QLatin1String("middle");
    default:
        break;
    }
    return QLatin1String("alphabetic");
}

void QSGContext2D::setTextBaseline(const QString &baseline)
{
    Q_D(QSGContext2D);
    d->setTextBaseline(baseline);
}

QString QSGContext2D::textAlign() const
{
    Q_D(const QSGContext2D);
    switch(d->state.textAlign) {
    case QSGContext2D::Start:
        return QLatin1String("start");
   case QSGContext2D::End:
        return QLatin1String("end");
   case QSGContext2D::Left:
        return QLatin1String("left");
   case QSGContext2D::Right:
        return QLatin1String("right");
   case QSGContext2D::Center:
        return QLatin1String("center");
    default:
        break;
    }
    return QLatin1String("start");
}

void QSGContext2D::setTextAlign(const QString &align)
{
    Q_D(QSGContext2D);
    d->setTextAlign(align);

    d->commands.push_back(QSGContext2D::TextAlign);
    d->ints.push_back(d->state.textAlign);
}

void QSGContext2D::setFont(const QString &fontString)
{
    Q_D(QSGContext2D);
    d->setFont(fontString);
}

QString QSGContext2D::font() const
{
    //### TODO
    Q_D(const QSGContext2D);
    return d->state.font.toString();
}

qreal QSGContext2D::shadowOffsetX() const
{
    Q_D(const QSGContext2D);
    return d->state.shadowOffsetX;
}

qreal QSGContext2D::shadowOffsetY() const
{
    Q_D(const QSGContext2D);
    return d->state.shadowOffsetY;
}


qreal QSGContext2D::shadowBlur() const
{
    Q_D(const QSGContext2D);
    return d->state.shadowBlur;
}


QString QSGContext2D::shadowColor() const
{
    Q_D(const QSGContext2D);
    return d->state.shadowColor.name();
}


void QSGContext2D::clearRect(qreal x, qreal y, qreal w, qreal h)
{
    Q_D(QSGContext2D);
    d->clearRect(x, y, w, h);
}

void QSGContext2D::fillRect(qreal x, qreal y, qreal w, qreal h)
{
    Q_D(QSGContext2D);
    d->fillRect(x, y, w, h);
}

int QSGContext2DPrivate::baseLineOffset(QSGContext2D::TextBaseLineType value, const QFontMetrics &metrics)
{
    int offset = 0;
    switch (value) {
    case QSGContext2D::Top:
        break;
    case QSGContext2D::Alphabetic:
    case QSGContext2D::Middle:
    case QSGContext2D::Hanging:
        offset = metrics.ascent();
        break;
    case QSGContext2D::Bottom:
        offset = metrics.height();
       break;
    }
    return offset;
}

int QSGContext2DPrivate::textAlignOffset(QSGContext2D::TextAlignType value, const QFontMetrics &metrics, const QString &text)
{
    int offset = 0;
    if (value == QSGContext2D::Start)
        value = qApp->layoutDirection() == Qt::LeftToRight ? QSGContext2D::Left : QSGContext2D::Right;
    else if (value == QSGContext2D::End)
        value = qApp->layoutDirection() == Qt::LeftToRight ? QSGContext2D::Right: QSGContext2D::Left;
    switch (value) {
    case QSGContext2D::Center:
        offset = metrics.width(text)/2;
        break;
    case QSGContext2D::Right:
        offset = metrics.width(text);
    case QSGContext2D::Left:
    default:
        break;
    }
    return offset;
}

void QSGContext2D::fillText(const QString &text, qreal x, qreal y)
{
    Q_D(QSGContext2D);
    d->fillText(text, x, y);
}

void QSGContext2D::strokeText(const QString &text, qreal x, qreal y)
{
    Q_D(QSGContext2D);
    d->strokeText(text, x, y);
}

void QSGContext2D::strokeRect(qreal x, qreal y, qreal w, qreal h)
{
    Q_D(QSGContext2D);
    d->strokeRect(x, y, w, h);
}

void QSGContext2D::beginPath()
{
    Q_D(QSGContext2D);
    d->beginPath();
}


void QSGContext2D::closePath()
{
    Q_D(QSGContext2D);
    d->closePath();
}


void QSGContext2D::moveTo(qreal x, qreal y)
{
    Q_D(QSGContext2D);
    d->moveTo(x, y);
}


void QSGContext2D::lineTo(qreal x, qreal y)
{
    Q_D(QSGContext2D);
    d->lineTo(x, y);
}


void QSGContext2D::quadraticCurveTo(qreal cpx, qreal cpy, qreal x, qreal y)
{
    Q_D(QSGContext2D);
    d->quadraticCurveTo(cpx, cpy, x, y);
}


void QSGContext2D::bezierCurveTo(qreal cp1x, qreal cp1y,
                              qreal cp2x, qreal cp2y, qreal x, qreal y)
{
    Q_D(QSGContext2D);
    d->bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y);
}


void QSGContext2D::arcTo(qreal x1, qreal y1, qreal x2, qreal y2, qreal radius)
{
    Q_D(QSGContext2D);
    d->arcTo(x1, y1, x2, y2, radius);
}


void QSGContext2D::rect(qreal x, qreal y, qreal w, qreal h)
{
    Q_D(QSGContext2D);
    d->rect(x, y, w, h);
}

void QSGContext2D::arc(qreal xc, qreal yc, qreal radius,
                    qreal sar, qreal ear,
                    bool anticlockwise)
{
    Q_D(QSGContext2D);
    d->arc(xc, yc, radius, sar, ear, anticlockwise);
}


void QSGContext2D::fill()
{
    Q_D(QSGContext2D);
    d->fill();
}


void QSGContext2D::stroke()
{
    Q_D(QSGContext2D);
    d->stroke();
}


void QSGContext2D::clip()
{
    Q_D(QSGContext2D);
    d->clip();
}


bool QSGContext2D::isPointInPath(qreal x, qreal y) const
{
    Q_D(const QSGContext2D);
    return d->path.contains(QPointF(x, y));
}


QList<int> QSGContext2D::getImageData(qreal sx, qreal sy, qreal sw, qreal sh)
{
    Q_D(QSGContext2D);
    return d->getImageData(sx, sy, sw, sh);
}

void QSGContext2D::putImageData(const QVariant& imageData, qreal x, qreal y, qreal w, qreal h)
{
    Q_D(QSGContext2D);
    return d->putImageData(imageData.toList(), x, y, w, h);
}

QSGContext2D::QSGContext2D(QObject *parent)
    : QObject(*(new QSGContext2DPrivate()), parent)
{
    Q_D(QSGContext2D);
    d->canvas = qobject_cast<QSGCanvasItem*>(parent);
}

QSGContext2D::QSGContext2D(QSGContext2D *orig, QSGContext2DWorkerAgent* agentData)
    : QObject(*(new QSGContext2DPrivate()), 0)
{
    Q_D(QSGContext2D);
    d->agent = 0;
    d->agentData = agentData;
    if (d->agentData) {
        d->agentData->orig = orig;
    }
    d->canvas = qobject_cast<QSGCanvasItem*>(orig);
}

QSGCanvasItem*  QSGContext2D::canvas() const
{
    Q_D(const QSGContext2D);
    return d->canvas;
}
QSGContext2D::~QSGContext2D()
{
    Q_D(QSGContext2D);
    if (d->agent) {
        d->agentData->syncDone.wakeAll();
        d->agent->release();
    }
}

bool QSGContext2D::isDirty() const
{
    Q_D(const QSGContext2D);
    return !d->commands.isEmpty();
}

QScriptValue QSGContext2D::scriptValue() const
{
    Q_D(const QSGContext2D);
    return d->scriptValue;
}

void QSGContext2D::setScriptEngine(QScriptEngine *eng)
{
    Q_D(QSGContext2D);
    if (d->scriptEngine != eng) {
        d->scriptEngine = eng;
        d->scriptValue = eng->newObject();
        d->scriptValue.setData(eng->toScriptValue(this));
        d->scriptValue.setProperty(QLatin1String("sync"), eng->newFunction(ctx2d_sync));
        d->scriptValue.setProperty(QLatin1String("canvas"), eng->newFunction(ctx2d_canvas),QScriptValue::PropertyGetter);
        d->scriptValue.setProperty(QLatin1String("restore"), eng->newFunction(ctx2d_restore));
        d->scriptValue.setProperty(QLatin1String("save"), eng->newFunction(ctx2d_save));
        d->scriptValue.setProperty(QLatin1String("rotate"), eng->newFunction(ctx2d_rotate, 1));
        d->scriptValue.setProperty(QLatin1String("scale"), eng->newFunction(ctx2d_scale, 2));
        d->scriptValue.setProperty(QLatin1String("setTransform"), eng->newFunction(ctx2d_setTransform, 6));
        d->scriptValue.setProperty(QLatin1String("transform"), eng->newFunction(ctx2d_transform, 6));
        d->scriptValue.setProperty(QLatin1String("translate"), eng->newFunction(ctx2d_translate, 2));
        d->scriptValue.setProperty(QLatin1String("globalAlpha"), eng->newFunction(ctx2d_globalAlpha), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("globalCompositeOperation"), eng->newFunction(ctx2d_globalCompositeOperation), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("fillStyle"), eng->newFunction(ctx2d_fillStyle), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("strokeStyle"), eng->newFunction(ctx2d_strokeStyle),  QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("fillColor"), eng->newFunction(ctx2d_fillColor), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("strokeColor"), eng->newFunction(ctx2d_strokeColor),  QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("createLinearGradient"), eng->newFunction(ctx2d_createLinearGradient, 4));
        d->scriptValue.setProperty(QLatin1String("createRadialGradient"), eng->newFunction(ctx2d_createRadialGradient, 6));
        d->scriptValue.setProperty(QLatin1String("createPattern"), eng->newFunction(ctx2d_createPattern, 2));
        d->scriptValue.setProperty(QLatin1String("lineCap"), eng->newFunction(ctx2d_lineCap), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("lineJoin"), eng->newFunction(ctx2d_lineJoin), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("lineWidth"), eng->newFunction(ctx2d_lineWidth), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("miterLimit"), eng->newFunction(ctx2d_miterLimit), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("shadowBlur"), eng->newFunction(ctx2d_shadowBlur), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("shadowColor"), eng->newFunction(ctx2d_shadowColor), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("shadowOffsetX"), eng->newFunction(ctx2d_shadowOffsetX), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("shadowOffsetY"), eng->newFunction(ctx2d_shadowOffsetY), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("clearRect"), eng->newFunction(ctx2d_clearRect, 4));
        d->scriptValue.setProperty(QLatin1String("fillRect"), eng->newFunction(ctx2d_fillRect, 4));
        d->scriptValue.setProperty(QLatin1String("strokeRect"), eng->newFunction(ctx2d_strokeRect, 4));
        d->scriptValue.setProperty(QLatin1String("arc"), eng->newFunction(ctx2d_arc, 6));
        d->scriptValue.setProperty(QLatin1String("arcTo"), eng->newFunction(ctx2d_arcTo, 5));
        d->scriptValue.setProperty(QLatin1String("beginPath"), eng->newFunction(ctx2d_beginPath));
        d->scriptValue.setProperty(QLatin1String("bezierCurveTo"), eng->newFunction(ctx2d_bezierCurveTo, 6));
        d->scriptValue.setProperty(QLatin1String("clip"), eng->newFunction(ctx2d_clip));
        d->scriptValue.setProperty(QLatin1String("closePath"), eng->newFunction(ctx2d_closePath));
        d->scriptValue.setProperty(QLatin1String("fill"), eng->newFunction(ctx2d_fill));
        d->scriptValue.setProperty(QLatin1String("lineTo"), eng->newFunction(ctx2d_lineTo, 2));
        d->scriptValue.setProperty(QLatin1String("moveTo"), eng->newFunction(ctx2d_moveTo, 2));
        d->scriptValue.setProperty(QLatin1String("quadraticCurveTo"), eng->newFunction(ctx2d_quadraticCurveTo, 4));
        d->scriptValue.setProperty(QLatin1String("rect"), eng->newFunction(ctx2d_rect, 4));
        d->scriptValue.setProperty(QLatin1String("stroke"), eng->newFunction(ctx2d_stroke));
        d->scriptValue.setProperty(QLatin1String("isPointInPath"), eng->newFunction(ctx2d_isPointInPath, 2));
        d->scriptValue.setProperty(QLatin1String("font"), eng->newFunction(ctx2d_font), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("textAlign"), eng->newFunction(ctx2d_textAlign), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("textBaseline"), eng->newFunction(ctx2d_textBaseline), QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
        d->scriptValue.setProperty(QLatin1String("fillText"), eng->newFunction(ctx2d_fillText, 4));
        //d->scriptValue.setProperty(QLatin1String("measureText"), eng->newFunction(ctx2d_measureText, 1));
        d->scriptValue.setProperty(QLatin1String("strokeText"), eng->newFunction(ctx2d_strokeText, 4));
        d->scriptValue.setProperty(QLatin1String("drawImage"), eng->newFunction(ctx2d_drawImage, 9));
        d->scriptValue.setProperty(QLatin1String("createImageData"), eng->newFunction(ctx2d_createImageData, 2));
        d->scriptValue.setProperty(QLatin1String("getImageData"), eng->newFunction(ctx2d_getImageData, 4));
        d->scriptValue.setProperty(QLatin1String("putImageData"), eng->newFunction(ctx2d_putImageData, 7));
    }
}

QScriptEngine *QSGContext2D::scriptEngine() const
{
    Q_D(const QSGContext2D);
    return d->scriptEngine;
}

void QSGContext2D::addref()
{
    Q_D(QSGContext2D);
    Q_ASSERT(d->agentData);
    d->agentData->ref.ref();
}

void QSGContext2D::release()
{
    Q_D(QSGContext2D);
    Q_ASSERT(d->agentData);
    if (!d->agentData->ref.deref()) {
        deleteLater();
    }
}


bool QSGContext2D::inWorkerThread() const
{
    Q_D(const QSGContext2D);
    return d->agentData != 0;
}
const QString& QSGContext2D::agentScript() const
{
    static QString script;
    if (script.isEmpty()) {
        script = QString::fromLatin1(
        "function CanvasImageData(w, h, d) {"
        "     this.width = w;"
        "     this.height = h;"
        "     this.data = d;"
        "}"
        "function Context2DAgent(_ctx2d) {"
        "  this._ctx = _ctx2d;"
        "  this._fillColor = '#000000';"
        "  this._fillStyle = '#000000';"
        "  this._strokeColor = '#000000';"
        "  this._strokeStyle = '#000000';"
        "  this._globalCompositeOperation = \"source-over\";"
        "  this._commands = [];"
        "  this.createImageData = function() {"
        "    var d = null;"
        "    if (arguments.length == 1 && arguments[0] instanceof CanvasImageData) {"
        "      d = new CanvasImageData(arguments[0].width,"
        "                              arguments[0].height,"
        "                              new Array(arguments[0].width * arguments[0].height * 4));"
        "    } else if (arguments.length == 2) {"
        "      d = new CanvasImageData(arguments[0], arguments[1], new Array(arguments[0] * arguments[1] * 4));"
        "    }"
        "    if (d)"
        "      for (var i=0; i<d.data.length; i++)"
        "        d.data[i] = 255;"
        "    return d;"
        "  };"
        "  this.getImageData = function(sx, sy, sw, sh) {"
        "    var imageData = new CanvasImageData(sw, sh, this._ctx.getImageData(sx, sy, sw, sh));"
        "    return imageData;"
        "  };"
        "  this.sync = function() {"
        "    this._ctx.processCommands(this._commands);"
        "    this._commands.length = 0;"
        "  };");

        script.append(QString::fromLatin1(
                        "this.save = function() {"
                        "  this._commands.push([%1]);"
                        "};").arg(Save));

        script.append(QString::fromLatin1(
                        "this.restore = function() {"
                        "  this._commands.push([%1]);"
                        "};").arg(Restore));

        script.append(QString::fromLatin1(
                        "this.scale = function(x, y) {"
                        "  this._commands.push([%1, x, y]);"
                        "};").arg(Scale));

        script.append(QString::fromLatin1(
                        "this.createImage = function(url) {"
                          "  return this._ctx.createImage(url);"
                        "};"));

        script.append(QString::fromLatin1(
                        "this.rotate = function(x) {"
                        "  this._commands.push([%1, x]);"
                        "};").arg(Rotate));

        script.append(QString::fromLatin1(
                        "this.translate = function(x, y) {"
                        "  this._commands.push([%1, x, y]);"
                        "};").arg(Translate));

        script.append(QString::fromLatin1(
                        "this.transform = function(a1, a2, a3, a4, a5, a6) {"
                        "  this._commands.push([%1, a1, a2, a3, a4, a5, a6]);"
                        "};").arg(Transform));

        script.append(QString::fromLatin1(
                        "this.setTransform = function(a1, a2, a3, a4, a5, a6) {"
                        "  this._commands.push([%1, a1, a2, a3, a4, a5, a6]);"
                        "};").arg(SetTransform));

        script.append(QString::fromLatin1(
                        "this.clearRect = function(x, y, w, h) {"
                        "  this._commands.push([%1, x, y, w, h]);"
                        "};").arg(ClearRect));

        script.append(QString::fromLatin1(
                        "this.fillRect = function(x, y, w, h) {"
                        "  this._commands.push([%1, x, y, w, h]);"
                        "};").arg(FillRect));

        script.append(QString::fromLatin1(
                        "this.strokeRect = function(x, y, w, h) {"
                        "  this._commands.push([%1, x, y, w, h]);"
                        "};").arg(StrokeRect));

        script.append(QString::fromLatin1(
                        "this.beginPath = function() {"
                        "  this._commands.push([%1]);"
                        "};").arg(BeginPath));

        script.append(QString::fromLatin1(
                        "this.closePath = function() {"
                        "  this._commands.push([%1]);"
                        "};").arg(ClosePath));

        script.append(QString::fromLatin1(
                        "this.moveTo = function(x, y) {"
                        "  this._commands.push([%1, x, y]);"
                        "};").arg(MoveTo));

        script.append(QString::fromLatin1(
                        "this.lineTo = function(x, y) {"
                        "  this._commands.push([%1, x, y]);"
                        "};").arg(LineTo));

        script.append(QString::fromLatin1(
                        "this.quadraticCurveTo = function(a1, a2, a3, a4) {"
                        "  this._commands.push([%1, a1, a2, a3, a4]);"
                        "};").arg(QuadraticCurveTo));

        script.append(QString::fromLatin1(
                        "this.bezierCurveTo = function(a1, a2, a3, a4, a5, a6) {"
                        "  this._commands.push([%1, a1, a2, a3, a4, a5, a6]);"
                        "};").arg(BezierCurveTo));

        script.append(QString::fromLatin1(
                        "this.arcTo = function(x1, y1, x2, y2, radius) {"
                        "  this._commands.push([%1, x1, y1, x2, y2, radius]);"
                        "};").arg(ArcTo));

        script.append(QString::fromLatin1(
                        "this.rect = function(x, y, w, h) {"
                        "  this._commands.push([%1, x, y, w, h]);"
                        "};").arg(Rect));

        script.append(QString::fromLatin1(
                        "this.rect = function(x, y, radius, startAngle, endAngle, anticlockwise) {"
                        "  this._commands.push([%1, x, y, radius, startAngle, endAngle, anticlockwise]);"
                        "};").arg(Arc));

        script.append(QString::fromLatin1(
                        "this.fill = function() {"
                        "  this._commands.push([%1]);"
                        "};").arg(Fill));

        script.append(QString::fromLatin1(
                        "this.stroke = function() {"
                        "  this._commands.push([%1]);"
                        "};").arg(Stroke));

        script.append(QString::fromLatin1(
                        "this.clip = function() {"
                        "  this._commands.push([%1]);"
                        "};").arg(Clip));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"globalAlpha\", function() {"
        "    return this._globalAlpha;"
        "  });"
        "  this.__defineSetter__(\"globalAlpha\", function(v) {"
        "    this._globalAlpha = v;"
        "    this._commands.push([%1, v]);"
        "  });").arg(GlobalAlpha));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"globalCompositeOperation\", function() {"
        "    return this._globalCompositeOperation;"
        "  });"
        "  this.__defineSetter__(\"globalCompositeOperation\", function(v) {"
        "    this._globalCompositeOperation = v;"
        "    this._commands.push([%1, v]);"
        "  });").arg(GlobalCompositeOperation));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"strokeStyle\", function() {return this._strokeStyle; });"
        "  this.__defineSetter__(\"strokeStyle\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._strokeStyle = v;"
        "  });").arg(StrokeStyle));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"fillStyle\", function() {return this._fillStyle; });"
        "  this.__defineSetter__(\"fillStyle\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._fillStyle = v;"
        "  });").arg(FillStyle));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"strokeColor\", function() {return this._strokeColor; });"
        "  this.__defineSetter__(\"strokeColor\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._strokeColor = v;"
        "  });").arg(StrokeColor));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"fillColor\", function() {return this._fillColor; });"
        "  this.__defineSetter__(\"fillColor\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._fillColor = v;"
        "  });").arg(FillColor));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"lineWidth\", function() {return this._lineWidth; });"
        "  this.__defineSetter__(\"lineWidth\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._lineWidth = v;"
        "  });").arg(LineWidth));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"lineCap\", function() {return this._lineCap; });"
        "  this.__defineSetter__(\"lineCap\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._lineCap = v;"
        "  });").arg(LineCap));


        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"lineJoin\", function() {return this._lineJoin; });"
        "  this.__defineSetter__(\"lineJoin\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._lineJoin = v;"
        "  });").arg(LineJoin));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"miterLimit\", function() {return this._miterLimit; });"
        "  this.__defineSetter__(\"miterLimit\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._miterLimit = v;"
        "  });").arg(MiterLimit));


        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"shadowOffsetX\", function() {return this._shadowOffsetX; });"
        "  this.__defineSetter__(\"shadowOffsetX\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._shadowOffsetX = v;"
        "  });").arg(ShadowOffsetX));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"shadowOffsetY\", function() {return this._shadowOffsetY; });"
        "  this.__defineSetter__(\"shadowOffsetY\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._shadowOffsetY = v;"
        "  });").arg(ShadowOffsetY));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"shadowBlur\", function() {return this._shadowBlur; });"
        "  this.__defineSetter__(\"shadowBlur\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._shadowBlur = v;"
        "  });").arg(ShadowBlur));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"shadowColor\", function() {return this._shadowColor; });"
        "  this.__defineSetter__(\"shadowColor\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._shadowColor = v;"
        "  });").arg(ShadowColor));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"font\", function() {return this._font; });"
        "  this.__defineSetter__(\"font\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._font = v;"
        "  });").arg(Font));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"textBaseline\", function() {return this._textBaseline; });"
        "  this.__defineSetter__(\"textBaseline\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._textBaseline = v;"
        "  });").arg(TextBaseline));

        script.append(QString::fromLatin1(
        "  this.__defineGetter__(\"textAlign\", function() {return this._textAlign; });"
        "  this.__defineSetter__(\"textAlign\", function(v) {"
        "    this._commands.push([%1, v]);"
        "    this._textAlign = v;"
        "  });").arg(TextAlign));

        script.append(QString::fromLatin1(
                        "this.fillText = function(text, x, y) {"
                        "  this._commands.push([%1, text, x, y]);"
                        "};").arg(FillText));

        script.append(QString::fromLatin1(
                        "this.strokeText = function(text, x, y) {"
                        "  this._commands.push([%1, text, x, y]);"
                        "};").arg(StrokeText));

        script.append(QString::fromLatin1(
                        "this.drawImage = function() {"
                          "  if (arguments.length == 3) {"
                          "     this._commands.push([%1, arguments[0], arguments[1], arguments[2]]);"
                          "  } else if (arguments.length == 5) {"
                          "     this._commands.push([%2, arguments[0], arguments[1], arguments[2], arguments[3], arguments[4]]);"
                          "  } else if (arguments.length == 9) {"
                          "     this._commands.push([%3, arguments[0], arguments[1], arguments[2], arguments[3], arguments[4], arguments[5], arguments[6], arguments[7], arguments[8]]);}"
                        "};").arg(DrawImage1).arg(DrawImage2).arg(DrawImage3));

        script.append(QString::fromLatin1(
                        "this.putImageData = function() {"
                          "  var dx = arguments[1];"
                          "  var dy = arguments[2];"
                          "  if (arguments.length == 3) {"
                          "     this._commands.push([%1, arguments[0].data, dx, dy, arguments[0].width, arguments[0].height]);"
                          "  } else if (arguments.length == 7) {"
                          "     var dirtyX = arguments[3];"
                          "     var dirtyY = arguments[4];"
                          "     var dirtyWidth = arguments[5];"
                          "     var dirtyHeight = arguments[6];"
                          "     var width = arguments[0].width;"
                          "     var height = arguments[0].heigh;"
                          "     var filteredData = arguments[0].data.filter(function(element, index, array){"
                          "                                            var x=index/width;"
                          "                                            var y=index%width-1;"
                          "                                            return x >= dirtyX && x < dirtyX+dirtyWidth"
                          "                                                && y >= dirtyY && y < dirtyY+dirtyHeight;"
                          "                                           });"
                          "     this._commands.push([%2, filteredData, dx, dy, dirtyWidth, dirtyHeight]);"
                          "  }"
                        "};").arg(PutImageData).arg(PutImageData));
        script.append(QString::fromLatin1("}"));
    }
    return script;
}

QSGContext2D *QSGContext2D::agent()
{
    Q_D(QSGContext2D);

    if (d->agent)
        return d->agent;

    d->agent = new QSGContext2D(this, new QSGContext2DWorkerAgent);
    connect(this, SIGNAL(painted()), d->agent, SIGNAL(painted()));
    d->agent->setSize(size());
    return d->agent;

}
void QSGContext2D::processCommands(const QScriptValue& commands)
{
#ifdef QSGCANVASITEM_DEBUG
    QElapsedTimer t;
    t.start();
#endif
    int ii = 0;
    if (commands.isArray()) {
        QScriptValue cmd = commands.property(ii);
        while(cmd.isValid()) {
            processCommand(cmd);
            ii++;
            cmd = commands.property(ii);
        }
    }

#ifdef QSGCANVASITEM_DEBUG
    qDebug() << "processed" << ii << "commands in " << t.nsecsElapsed() << "nsecs";
#endif
    sync();
}

void QSGContext2D::sync()
{
    Q_D(QSGContext2D);

#ifdef QSGCANVASITEM_DEBUG
    QElapsedTimer t;
    t.start();
#endif
    if (d->agentData) {
        if (d->agentData->ref == 1) return;

        Sync *s = new Sync;
        s->data = d->agentData;

        d->agentData->mutex.lock();
        QCoreApplication::postEvent(this, s);
        d->agentData->syncDone.wait(&d->agentData->mutex);
        d->agentData->mutex.unlock();
    } else {
        //qmlInfo(this) << "Context2D sync() can only be called from a WorkerScript;";
        emit changed();
    }

#ifdef QSGCANVASITEM_DEBUG
    qDebug() << "syncing time:" << t.nsecsElapsed();
#endif
}


bool QSGContext2D::event(QEvent *e)
{
    Q_D(QSGContext2D);
    if (e->type() == QEvent::User && d->agentData) {
        QMutexLocker locker(&d->agentData->mutex);
        Sync *s = static_cast<Sync *>(e);

        QSGContext2DPrivate* origin_d = static_cast<QSGContext2DPrivate*>(s->data->orig->d_func());

        //quick copy
        memcpy_vector<PaintCommand>(&origin_d->commands, d->commands);
        memcpy_vector<int>(&origin_d->ints, d->ints);
        memcpy_vector<qreal>(&origin_d->reals, d->reals);
        memcpy_vector<QColor>(&origin_d->colors, d->colors);
        memcpy_vector<QMatrix>(&origin_d->matrixes, d->matrixes);
        memcpy_vector<QSize>(&origin_d->sizes, d->sizes);

        //slow copy
        copy_vector<QString>(&origin_d->strings, d->strings);
        copy_vector<QVariant>(&origin_d->variants, d->variants);
        copy_vector<QPen>(&origin_d->pens, d->pens);
        copy_vector<QBrush>(&origin_d->brushes, d->brushes);
        copy_vector<QPainterPath>(&origin_d->pathes, d->pathes);
        copy_vector<QFont>(&origin_d->fonts, d->fonts);
        copy_vector<QImage>(&origin_d->images, d->images);
        origin_d->state = d->state;
        d->clearCommands();

        if (d->waitingForPainting) {
            d->imageData.clear();
            origin_d->imageData.clear();
            emit s->data->orig->changed();
            while(origin_d->imageData.isEmpty()) {
                QCoreApplication::processEvents();
            }
            d->imageData = origin_d->imageData;
            d->waitingForPainting = false;
            qDebug() << "imageData size:" << d->imageData.size();
        } else {
            emit s->data->orig->changed();
        }

        d->agentData->syncDone.wakeAll();
        return true;
    }
    return QObject::event(e);
}

void QSGContext2D::processCommand(const QScriptValue& cmd)
{
    int action = cmd.property(0).toInt32();
    switch (action) {
    case QSGContext2D::Save:
        save();
        break;
    case QSGContext2D::Restore:
        restore();
        break;
    case QSGContext2D::Scale:
        scale(cmd.property(1).toNumber(), cmd.property(2).toNumber());
        break;
    case QSGContext2D::Rotate:
        rotate(cmd.property(1).toNumber());
        break;
    case QSGContext2D::Translate:
        translate(cmd.property(1).toNumber(), cmd.property(2).toNumber());
        break;
    case QSGContext2D::Transform:
        transform(cmd.property(1).toNumber(),
                  cmd.property(2).toNumber(),
                  cmd.property(3).toNumber(),
                  cmd.property(4).toNumber(),
                  cmd.property(5).toNumber(),
                  cmd.property(6).toNumber());
        break;
    case QSGContext2D::SetTransform:
        setTransform(cmd.property(1).toNumber(),
                  cmd.property(2).toNumber(),
                  cmd.property(3).toNumber(),
                  cmd.property(4).toNumber(),
                  cmd.property(5).toNumber(),
                  cmd.property(6).toNumber());
        break;
    case QSGContext2D::ClearRect:
        clearRect(cmd.property(1).toNumber(),
                  cmd.property(2).toNumber(),
                  cmd.property(3).toNumber(),
                  cmd.property(4).toNumber());
        break;
    case QSGContext2D::FillRect:
        fillRect(cmd.property(1).toNumber(),
                 cmd.property(2).toNumber(),
                 cmd.property(3).toNumber(),
                 cmd.property(4).toNumber());
        break;
    case QSGContext2D::StrokeRect:
        strokeRect(cmd.property(1).toNumber(),
                   cmd.property(2).toNumber(),
                   cmd.property(3).toNumber(),
                   cmd.property(4).toNumber());
        break;
    case QSGContext2D::BeginPath:
        beginPath();
        break;
    case QSGContext2D::ClosePath:
        closePath();
        break;
    case QSGContext2D::MoveTo:
        moveTo(cmd.property(1).toNumber(),
               cmd.property(2).toNumber());
        break;
    case QSGContext2D::LineTo:
        lineTo(cmd.property(1).toNumber(),
               cmd.property(2).toNumber());
        break;
    case QSGContext2D::QuadraticCurveTo:
        quadraticCurveTo(cmd.property(1).toNumber(),
                         cmd.property(2).toNumber(),
                         cmd.property(3).toNumber(),
                         cmd.property(4).toNumber());
        break;
    case QSGContext2D::BezierCurveTo:
        bezierCurveTo(cmd.property(1).toNumber(),
                      cmd.property(2).toNumber(),
                      cmd.property(3).toNumber(),
                      cmd.property(4).toNumber(),
                      cmd.property(5).toNumber(),
                      cmd.property(6).toNumber());
        break;
    case QSGContext2D::ArcTo:
        arcTo(cmd.property(1).toNumber(),
              cmd.property(2).toNumber(),
              cmd.property(3).toNumber(),
              cmd.property(4).toNumber(),
              cmd.property(5).toNumber());
        break;
    case QSGContext2D::Rect:
        rect(cmd.property(1).toNumber(),
             cmd.property(2).toNumber(),
             cmd.property(3).toNumber(),
             cmd.property(4).toNumber());
        break;
    case QSGContext2D::Arc:
        arc(cmd.property(1).toNumber(),
            cmd.property(2).toNumber(),
            cmd.property(3).toNumber(),
            cmd.property(4).toNumber(),
            cmd.property(5).toNumber(),
            cmd.property(6).toBool());
        break;
    case QSGContext2D::Fill:
        fill();
        break;
    case QSGContext2D::Stroke:
        stroke();
        break;
    case QSGContext2D::Clip:
        clip();
        break;
    case QSGContext2D::GlobalAlpha:
        setGlobalAlpha(cmd.property(1).toNumber());
        break;
    case QSGContext2D::GlobalCompositeOperation:
        setGlobalCompositeOperation(cmd.property(1).toString());
        break;
    case QSGContext2D::StrokeStyle:
        setStrokeStyle(cmd.property(1).toVariant());
        break;
    case QSGContext2D::FillStyle:
        setFillStyle(cmd.property(1).toVariant());
        break;
    case QSGContext2D::FillColor:
        setFillColor(cmd.property(1).toVariant().value<QColor>());
        break;
    case QSGContext2D::StrokeColor:
        setStrokeColor(cmd.property(1).toVariant().value<QColor>());
        break;
    case QSGContext2D::LineWidth:
        setLineWidth(cmd.property(1).toNumber());
        break;
    case QSGContext2D::LineCap:
        setLineCap(cmd.property(1).toString());
        break;
    case QSGContext2D::LineJoin:
        setLineJoin(cmd.property(1).toString());
        break;
    case QSGContext2D::MiterLimit:
        setMiterLimit(cmd.property(1).toNumber());
        break;
    case QSGContext2D::ShadowOffsetX:
        setShadowOffsetX(cmd.property(1).toNumber());
        break;
    case QSGContext2D::ShadowOffsetY:
        setShadowOffsetY(cmd.property(1).toNumber());
        break;
    case QSGContext2D::ShadowBlur:
        setShadowBlur(cmd.property(1).toNumber());
        break;
    case QSGContext2D::ShadowColor:
        setShadowColor(cmd.property(1).toString());
        break;
    case QSGContext2D::Font:
        setFont(cmd.property(1).toString());
        break;
    case QSGContext2D::TextBaseline:
        setTextBaseline(cmd.property(1).toString());
        break;
    case QSGContext2D::TextAlign:
        setTextAlign(cmd.property(1).toString());
        break;
    case QSGContext2D::FillText:
        fillText(cmd.property(1).toString(), cmd.property(2).toNumber(), cmd.property(3).toNumber());
        break;
    case QSGContext2D::StrokeText:
        strokeText(cmd.property(1).toString(), cmd.property(2).toNumber(), cmd.property(3).toNumber());
        break;
    case QSGContext2D::DrawImage1:
    {
        drawImage(cmd.property(1).toString(),
                  cmd.property(2).toNumber(),
                  cmd.property(3).toNumber());
        break;
    }
    case QSGContext2D::DrawImage2:
        drawImage(cmd.property(1).toString(),
                  cmd.property(2).toNumber(),
                  cmd.property(3).toNumber(),
                  cmd.property(4).toNumber(),
                  cmd.property(5).toNumber());
        break;
    case QSGContext2D::DrawImage3:
            drawImage(cmd.property(1).toString(),
                      cmd.property(2).toNumber(),
                      cmd.property(3).toNumber(),
                      cmd.property(4).toNumber(),
                      cmd.property(5).toNumber(),
                      cmd.property(6).toNumber(),
                      cmd.property(7).toNumber(),
                      cmd.property(8).toNumber(),
                      cmd.property(9).toNumber());
            break;
    case QSGContext2D::PutImageData:
            putImageData(cmd.property(1).toVariant(),
                         cmd.property(2).toNumber(),
                         cmd.property(3).toNumber(),
                         cmd.property(4).toNumber(),
                         cmd.property(5).toNumber());
            break;
    default:
        break;
    }
}

void QSGContext2D::paint(QPainter* p)
{
    Q_D(QSGContext2D);

    QTransform transform = p->worldTransform();
    if (!d->commands.isEmpty()) {
        int matrix_idx, real_idx, int_idx, variant_idx, string_idx,color_idx,cmd_idx,
            pen_idx, brush_idx, font_idx, path_idx, image_idx, size_idx;

        matrix_idx = real_idx = int_idx = variant_idx = string_idx =color_idx = cmd_idx
         = pen_idx = brush_idx = font_idx = path_idx = image_idx = size_idx = 0;

        foreach(PaintCommand cmd, d->commands) {
            switch (cmd) {
            case UpdateMatrix:
            {
//                qDebug() << "update matrix from " << d->state.matrix << " to " << d->matrixes[matrix_idx];
                //p->setWorldTransform(transform * QTransform(d->matrixes[matrix_idx++]), false);
                //p->setMatrix(d->matrixes[matrix_idx++]);
                d->state.matrix = d->matrixes[matrix_idx++];
                break;
            }
            case ClearRect:
            {
                qreal x = d->reals[real_idx++];
                qreal y = d->reals[real_idx++];
                qreal w = d->reals[real_idx++];
                qreal h = d->reals[real_idx++];
                p->eraseRect(QRectF(x, y, w, h));
                break;
            }
            case FillRect:
            {
                qreal x = d->reals[real_idx++];
                qreal y = d->reals[real_idx++];
                qreal w = d->reals[real_idx++];
                qreal h = d->reals[real_idx++];
//                qDebug() << "fillRect(" << x << y << w << h << ")";
                if (d->hasShadow())
                    d->fillRectShadow(p, QRectF(x, y, w, h));
                else
                    p->fillRect(QRectF(x, y, w, h), p->brush());
                break;
            }
            case ShadowColor:
            {
                QColor c = d->colors[color_idx++];
                d->state.shadowColor = c;
                break;
            }
            case ShadowBlur:
            {
                qreal blur = d->reals[real_idx++];
                d->state.shadowBlur = blur;
                break;
            }
            case ShadowOffsetX:
            {
                qreal x = d->reals[real_idx++];
                d->state.shadowOffsetX = x;
                break;
            }
            case ShadowOffsetY:
            {
                qreal y = d->reals[real_idx++];
                d->state.shadowOffsetY = y;
                break;
            }
            case Fill:
            {
                QPainterPath path = d->pathes[path_idx++];
                if (d->hasShadow())
                    d->fillShadowPath(p,path);
                else
                    p->fillPath(path, p->brush());
                break;
            }
            case Stroke:
            {
                p->setMatrix(d->state.matrix);
                QPainterPath path = d->state.matrix.inverted().map(d->pathes[path_idx++]);
                if (d->hasShadow())
                    d->strokeShadowPath(p,path);
                else
                    p->strokePath(path, p->pen());
                break;
            }
            case Clip:
            {
                QPainterPath clipPath = d->pathes[path_idx++];
                p->setClipPath(clipPath);
                p->setClipping(true);
                break;
            }
            case UpdateBrush:
            {
                p->setBrush(d->brushes[brush_idx++]);
                break;
            }
            case UpdatePen:
            {
                p->setPen(d->pens[pen_idx++]);
                break;
            }
            case GlobalAlpha:
            {
                p->setOpacity(d->reals[real_idx++]);
                break;
            }
            case GlobalCompositeOperation:
            {
                p->setCompositionMode(static_cast<QPainter::CompositionMode>(d->ints[int_idx++]));
                break;
            }
            case Font:
            {
                p->setFont(d->fonts[font_idx++]);
                break;
            }
            case StrokeText:
            {
                QString text = d->strings[string_idx++];
                qreal x = d->reals[real_idx++];
                qreal y = d->reals[real_idx++];
                int align = d->ints[int_idx++];
                int baseline = d->ints[int_idx++];

                QPen oldPen = p->pen();
                p->setPen(QPen(p->brush(),0));
                //p->setMatrix(state.matrix, false); // always set?

                QPainterPath textPath;
                QFont oldFont = p->font();
                QFont font = p->font();
                font.setStyleStrategy(QFont::ForceOutline);
                p->setFont(font);
                const QFontMetrics &metrics = p->fontMetrics();
                int yoffset = d->baseLineOffset(static_cast<QSGContext2D::TextBaseLineType>(baseline), metrics);
                int xoffset = d->textAlignOffset(static_cast<QSGContext2D::TextAlignType>(align), metrics, text);
                textPath.addText(x - xoffset, y - yoffset+metrics.ascent(), font, text);
                if (d->hasShadow())
                    d->strokeShadowPath(p,textPath);

                p->strokePath(textPath, QPen(p->brush(), p->pen().widthF()));

                //reset old font
                p->setFont(oldFont);
                p->setPen(oldPen);
                break;
            }
            case FillText:
            {
                QString text = d->strings[string_idx++];
                qreal x = d->reals[real_idx++];
                qreal y = d->reals[real_idx++];
                int align = d->ints[int_idx++];
                int baseline = d->ints[int_idx++];

                QFont oldFont = p->font();
                QPen oldPen = p->pen();
                p->setPen(QPen(p->brush(), p->pen().widthF()));
                //p->setMatrix(state.matrix, false);
                //QFont font = p->font();
                QFont font = d->state.font;
                font.setBold(true);

                p->setFont(font);
                int yoffset = d->baseLineOffset(static_cast<QSGContext2D::TextBaseLineType>(baseline), p->fontMetrics());
                int xoffset = d->textAlignOffset(static_cast<QSGContext2D::TextAlignType>(align), p->fontMetrics(), text);
                QTextOption opt; // Adjust baseLine etc
                if (d->hasShadow()) {
                    const QFontMetrics &metrics = p->fontMetrics();
                    QPainterPath textPath;
                    textPath.addText(x - xoffset, y - yoffset+metrics.ascent(), font, text);
                    d->fillShadowPath(p,textPath);
                }
                //p->drawText(QRectF(x - xoffset, y - yoffset, QWIDGETSIZE_MAX, p->fontMetrics().height()), text, opt);
                p->setFont(oldFont);
                p->setPen(oldPen);
                break;
            }
            case DrawImage1:
            {
                QUrl url(d->strings[string_idx++]);
                qreal x = d->reals[real_idx++];
                qreal y = d->reals[real_idx++];
                QDeclarativePixmap px(qmlEngine(d->canvas), url);
                qDebug() << "draw image:" << url << px.pixmap().size();
                if (px.isReady()) {
                    QPixmap pixmap = px.pixmap();
                    if (d->hasShadow()) {
                        QImage shadow = d->makeShadowImage(pixmap);
                        qreal dx = x + (d->state.shadowOffsetX < 0? d->state.shadowOffsetX:0);
                        qreal dy = y + (d->state.shadowOffsetY < 0? d->state.shadowOffsetY:0);
                        p->drawImage(QPointF(dx, dy), shadow);
                    }
                    p->drawPixmap(QPointF(x, y), pixmap);
                }
                break;
            }
            case DrawImage2:
            {
                qreal dx = d->reals[real_idx++];
                qreal dy = d->reals[real_idx++];
                qreal dw = d->reals[real_idx++];
                qreal dh = d->reals[real_idx++];
                QUrl url(d->strings[string_idx++]);
                QDeclarativePixmap px(qmlEngine(d->canvas), url);
                if (px.isReady()) {
                    QPixmap pixmap = px.pixmap().scaled(dw, dh);
                    if (d->hasShadow()) {
                        QImage shadow = d->makeShadowImage(pixmap);
                        qreal shadow_dx = dx + (d->state.shadowOffsetX < 0? d->state.shadowOffsetX:0);
                        qreal shadow_dy = dy + (d->state.shadowOffsetY < 0? d->state.shadowOffsetY:0);
                        p->drawImage(QPointF(shadow_dx, shadow_dy), shadow);
                    }
                    p->drawPixmap(QPointF(dx, dy), pixmap);
                }
                break;
            }
            case DrawImage3:
            {
                qreal sx = d->reals[real_idx++];
                qreal sy = d->reals[real_idx++];
                qreal sw = d->reals[real_idx++];
                qreal sh = d->reals[real_idx++];
                qreal dx = d->reals[real_idx++];
                qreal dy = d->reals[real_idx++];
                qreal dw = d->reals[real_idx++];
                qreal dh = d->reals[real_idx++];
                QUrl url(d->strings[string_idx++]);
                QDeclarativePixmap px(qmlEngine(d->canvas), url);
                if (px.isReady()) {
                    QPixmap pixmap = px.pixmap().copy(sx, sy, sw, sh).scaled(dw, dh);
                    if (d->hasShadow()) {
                        QImage shadow = d->makeShadowImage(pixmap);
                        qreal shadow_dx = dx + (d->state.shadowOffsetX < 0? d->state.shadowOffsetX:0);
                        qreal shadow_dy = dy + (d->state.shadowOffsetY < 0? d->state.shadowOffsetY:0);
                        p->drawImage(QPointF(shadow_dx, shadow_dy), shadow);
                    }
                    p->drawPixmap(QPointF(dx, dy), pixmap);
                }
                break;
            }
            case GetImageData:
            {
                qreal sx = d->reals[real_idx++];
                qreal sy = d->reals[real_idx++];
                qreal sw = d->reals[real_idx++];
                qreal sh = d->reals[real_idx++];
                QImage img = toImage().copy(sx, sy, sw, sh);
                const uchar* data = img.constBits();
                int i = 0;

                while(i< img.byteCount()) {
                    //the stored order in QImage:BGRA
                    d->imageData << *(data+i+2);//R
                    d->imageData << *(data+i+1);//G
                    d->imageData << *(data+i);//B
                    d->imageData << *(data+i+3);//A
                    i+=4;
                }
                break;
            }
            case PutImageData:
            {
                QImage image = d->images[image_idx++];
                qreal x = d->reals[real_idx++];
                qreal y = d->reals[real_idx++];
                p->drawImage(QPointF(x, y), image);
                break;
            }
            default:
                break;
            }
        }
        d->clearCommands();
    }
}

QPaintDevice* QSGContext2D::paintDevice()
{
    Q_D(QSGContext2D);
    return &d->cachedImage;
}
const QImage& QSGContext2D::toImage() const
{
    Q_D(const QSGContext2D);
    return d->cachedImage;
}
bool QSGContext2D::requireCachedImage() const
{
    Q_D(const QSGContext2D);
    return d->waitingForPainting;
}
void QSGContext2D::setCachedImage(const QImage& image)
{
    Q_D(QSGContext2D);
#ifndef QSGCANVASITEM_PAINTING_ON_IMAGE
    if (d->waitingForPainting) {
        d->cachedImage = image;
        d->waitingForPainting = false;
    }
#endif
    if (inWorkerThread()) {
        d->agent->setCachedImage(image);
    }
}

void QSGContext2D::clear()
{
    Q_D(QSGContext2D);
    d->clear();
}

void QSGContext2D::reset()
{
    Q_D(QSGContext2D);
    d->reset();
}

void QSGContext2D::drawImage(const QString& imgUrl, qreal dx, qreal dy)
{
    Q_D(QSGContext2D);
    if (!imgUrl.isEmpty())
        d->drawImage(imgUrl, dx, dy);
}

void QSGContext2D::drawImage(const QString& imgUrl, qreal sx, qreal sy, qreal sw, qreal sh, qreal dx, qreal dy, qreal dw, qreal dh)
{
    Q_D(QSGContext2D);
    if (!imgUrl.isEmpty())
        d->drawImage(imgUrl, sx, sy, sw, sh, dx, dy, dw, dh);
}

void QSGContext2D::drawImage(const QString& imgUrl, qreal dx, qreal dy,
                             qreal dw, qreal dh)
{
    Q_D(QSGContext2D);
    if (!imgUrl.isEmpty())
        d->drawImage(imgUrl, dx, dy, dw, dh);
}

void QSGContext2D::setSize(int width, int height)
{
    QSize size(width, height);
    setSize(size);
}

void QSGContext2D::setSize(const QSize &size)
{
    Q_D(QSGContext2D);

    if (d->size == size)
        return;
    d->setSize(size);
    emit changed();
}

QSize QSGContext2D::size() const
{
    Q_D(const QSGContext2D);
    return d->size;
}

QMatrix QSGContext2D::worldMatrix() const
{
    Q_D(const QSGContext2D);
    return d->state.matrix;
}

QT_END_NAMESPACE
