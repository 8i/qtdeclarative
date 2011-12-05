// Commit: 47712d1f330e4b22ce6dd30e7557288ef7f7fca0
/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QQUICKTEXTINPUT_P_P_H
#define QQUICKTEXTINPUT_P_P_H

#include "qquicktextinput_p.h"
#include "qquicktext_p.h"
#include "qquickimplicitsizeitem_p_p.h"

#include <private/qlinecontrol_p.h>

#include <QtDeclarative/qdeclarative.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qpointer.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

QT_BEGIN_NAMESPACE

class QQuickTextNode;

class Q_AUTOTEST_EXPORT QQuickTextInputPrivate : public QQuickImplicitSizeItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickTextInput)
public:
    QQuickTextInputPrivate()
                 : control(new QLineControl(QString()))
                 , color((QRgb)0)
                 , style(QQuickText::Normal)
                 , styleColor((QRgb)0)
                 , hAlign(QQuickTextInput::AlignLeft)
                 , mouseSelectionMode(QQuickTextInput::SelectCharacters)
                 , inputMethodHints(Qt::ImhNone)
                 , textNode(0)
                 , hscroll(0)
                 , oldScroll(0)
                 , oldValidity(false)
                 , focused(false)
                 , focusOnPress(true)
                 , cursorVisible(false)
                 , autoScroll(true)
                 , selectByMouse(false)
                 , canPaste(false)
                 , hAlignImplicit(true)
                 , selectPressed(false)
                 , textLayoutDirty(true)
    {
    }

    ~QQuickTextInputPrivate()
    {
    }

    int xToPos(int x, QTextLine::CursorPosition betweenOrOn = QTextLine::CursorBetweenCharacters) const
    {
        Q_Q(const QQuickTextInput);
        QRect cr = q->boundingRect().toRect();
        x-= cr.x() - hscroll;
        return control->xToPos(x, betweenOrOn);
    }

    void init();
    void startCreatingCursor();
    void updateHorizontalScroll();
    bool determineHorizontalAlignment();
    bool setHAlign(QQuickTextInput::HAlignment, bool forceAlign = false);
    void mirrorChange();
    int calculateTextWidth();
    bool sendMouseEventToInputContext(QMouseEvent *event);
    void updateInputMethodHints();
    void hideCursor();
    void showCursor();

    QLineControl* control;

    QFont font;
    QFont sourceFont;
    QColor  color;
    QColor  selectionColor;
    QColor  selectedTextColor;
    QQuickText::TextStyle style;
    QColor  styleColor;
    QQuickTextInput::HAlignment hAlign;
    QQuickTextInput::SelectionMode mouseSelectionMode;
    Qt::InputMethodHints inputMethodHints;
    QPointer<QDeclarativeComponent> cursorComponent;
    QPointer<QQuickItem> cursorItem;
    QPointF pressPos;
    QQuickTextNode *textNode;
    QElapsedTimer tripleClickTimer;
    QPoint tripleClickStartPoint;

    int lastSelectionStart;
    int lastSelectionEnd;
    int oldHeight;
    int oldWidth;
    int hscroll;
    int oldScroll;

    bool oldValidity:1;
    bool focused:1;
    bool focusOnPress:1;
    bool cursorVisible:1;
    bool autoScroll:1;
    bool selectByMouse:1;
    bool canPaste:1;
    bool hAlignImplicit:1;
    bool selectPressed:1;
    bool textLayoutDirty:1;

    static inline QQuickTextInputPrivate *get(QQuickTextInput *t) {
        return t->d_func();
    }
    bool hasPendingTripleClick() const {
        return !tripleClickTimer.hasExpired(qApp->styleHints()->mouseDoubleClickInterval());
    }
};

QT_END_NAMESPACE

#endif // QQUICKTEXTINPUT_P_P_H
