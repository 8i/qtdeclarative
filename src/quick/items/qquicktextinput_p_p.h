// Commit: 47712d1f330e4b22ce6dd30e7557288ef7f7fca0
/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include <QtDeclarative/qdeclarative.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qpointer.h>
#include <QtCore/qbasictimer.h>
#include <QtGui/qclipboard.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpalette.h>
#include <QtGui/qtextlayout.h>
#include <QtGui/qstylehints.h>

#include "qplatformdefs.h"

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
        : color((QRgb)0)
        , styleColor((QRgb)0)
        , textNode(0)
        , m_maskData(0)
        , hscroll(0)
        , vscroll(0)
        , m_cursor(0)
        , m_preeditCursor(0)
        , m_cursorWidth(1)
        , m_blinkPeriod(0)
        , m_blinkTimer(0)
        , m_deleteAllTimer(0)
        , m_ascent(0)
        , m_maxLength(32767)
        , m_lastCursorPos(-1)
        , m_modifiedState(0)
        , m_undoState(0)
        , m_selstart(0)
        , m_selend(0)
        , style(QQuickText::Normal)
        , hAlign(QQuickTextInput::AlignLeft)
        , vAlign(QQuickTextInput::AlignTop)
        , wrapMode(QQuickTextInput::NoWrap)
        , mouseSelectionMode(QQuickTextInput::SelectCharacters)
        , inputMethodHints(Qt::ImhNone)
        , m_layoutDirection(Qt::LayoutDirectionAuto)
        , m_passwordCharacter(QLatin1Char('*'))
        , focused(false)
        , focusOnPress(true)
        , cursorVisible(false)
        , autoScroll(true)
        , selectByMouse(false)
        , canPaste(false)
        , canPasteValid(false)
        , canUndo(false)
        , canRedo(false)
        , hAlignImplicit(true)
        , selectPressed(false)
        , textLayoutDirty(true)
        , m_hideCursor(false)
        , m_separator(0)
        , m_readOnly(0)
        , m_echoMode(QQuickTextInput::Normal)
        , m_textDirty(0)
        , m_preeditDirty(0)
        , m_selDirty(0)
        , m_validInput(1)
        , m_acceptableInput(1)
        , m_blinkStatus(0)
        , m_passwordEchoEditing(false)
        , updateType(UpdatePaintNode)
    {
    }

    ~QQuickTextInputPrivate()
    {
    }

    void init();
    void startCreatingCursor();
    void updateHorizontalScroll();
    void updateVerticalScroll();
    bool determineHorizontalAlignment();
    bool setHAlign(QQuickTextInput::HAlignment, bool forceAlign = false);
    void mirrorChange();
    bool sendMouseEventToInputContext(QMouseEvent *event);
    void updateInputMethodHints();
    void hideCursor();
    void showCursor();

    struct MaskInputData {
        enum Casemode { NoCaseMode, Upper, Lower };
        QChar maskChar; // either the separator char or the inputmask
        bool separator;
        Casemode caseMode;
    };

    // undo/redo handling
    enum CommandType { Separator, Insert, Remove, Delete, RemoveSelection, DeleteSelection, SetSelection };
    struct Command {
        inline Command() {}
        inline Command(CommandType t, int p, QChar c, int ss, int se) : type(t),uc(c),pos(p),selStart(ss),selEnd(se) {}
        uint type : 4;
        QChar uc;
        int pos, selStart, selEnd;
    };

    enum DrawFlags {
        DrawText = 0x01,
        DrawSelections = 0x02,
        DrawCursor = 0x04,
        DrawAll = DrawText | DrawSelections | DrawCursor
    };

    QTextLayout m_textLayout;
    QString m_text;
    QString m_inputMask;
    QString m_cancelText;
    QString m_tentativeCommit;
    QPalette m_palette;
    QFont font;
    QFont sourceFont;
    QColor  color;
    QColor  selectionColor;
    QColor  selectedTextColor;
    QColor  styleColor;
    QPointer<QDeclarativeComponent> cursorComponent;
    QPointer<QQuickItem> cursorItem;
#ifndef QT_NO_VALIDATOR
    QPointer<QValidator> m_validator;
#endif
    QPointF pressPos;
    QQuickTextNode *textNode;
    MaskInputData *m_maskData;
    QElapsedTimer tripleClickTimer;
    QPoint tripleClickStartPoint;
    QList<int> m_transactions;
    QVector<Command> m_history;
    QRectF boundingRect;

    int lastSelectionStart;
    int lastSelectionEnd;
    int oldHeight;
    int oldWidth;
    int hscroll;
    int vscroll;
    int m_cursor;
    int m_preeditCursor;
    int m_cursorWidth;
    int m_blinkPeriod; // 0 for non-blinking cursor
    int m_blinkTimer;
    int m_deleteAllTimer;
#ifdef QT_GUI_PASSWORD_ECHO_DELAY
    QBasicTimer m_passwordEchoTimer;
#endif
    int m_ascent;
    int m_maxLength;
    int m_lastCursorPos;
    int m_modifiedState;
    int m_undoState;
    int m_selstart;
    int m_selend;

    QQuickText::TextStyle style;
    QQuickTextInput::HAlignment hAlign;
    QQuickTextInput::VAlignment vAlign;
    QQuickTextInput::WrapMode wrapMode;
    QQuickTextInput::SelectionMode mouseSelectionMode;
    Qt::InputMethodHints inputMethodHints;
    Qt::LayoutDirection m_layoutDirection;

    QChar m_blank;
    QChar m_passwordCharacter;

    bool focused:1;
    bool focusOnPress:1;
    bool cursorVisible:1;
    bool autoScroll:1;
    bool selectByMouse:1;
    bool canPaste:1;
    bool canPasteValid:1;
    bool canUndo:1;
    bool canRedo:1;
    bool hAlignImplicit:1;
    bool selectPressed:1;
    bool textLayoutDirty:1;

    uint m_hideCursor : 1; // used to hide the m_cursor inside preedit areas
    uint m_separator : 1;
    uint m_readOnly : 1;
    uint m_echoMode : 2;
    uint m_textDirty : 1;
    uint m_preeditDirty : 1;
    uint m_selDirty : 1;
    uint m_validInput : 1;
    uint m_acceptableInput : 1;
    uint m_blinkStatus : 1;
    uint m_passwordEchoEditing;

    enum UpdateType {
        UpdateNone,
        UpdateOnlyPreprocess,
        UpdatePaintNode
    };
    UpdateType updateType;

    static inline QQuickTextInputPrivate *get(QQuickTextInput *t) {
        return t->d_func();
    }
    bool hasPendingTripleClick() const {
        return !tripleClickTimer.hasExpired(qApp->styleHints()->mouseDoubleClickInterval());
    }


    int nextMaskBlank(int pos)
    {
        int c = findInMask(pos, true, false);
        m_separator |= (c != pos);
        return (c != -1 ?  c : m_maxLength);
    }

    int prevMaskBlank(int pos)
    {
        int c = findInMask(pos, false, false);
        m_separator |= (c != pos);
        return (c != -1 ? c : 0);
    }

    bool isUndoAvailable() const { return !m_readOnly && m_undoState; }
    bool isRedoAvailable() const { return !m_readOnly && m_undoState < (int)m_history.size(); }
    void clearUndo() { m_history.clear(); m_modifiedState = m_undoState = 0; }

    bool isModified() const { return m_modifiedState != m_undoState; }
    void setModified(bool modified) { m_modifiedState = modified ? -1 : m_undoState; }

    bool allSelected() const { return !m_text.isEmpty() && m_selstart == 0 && m_selend == (int)m_text.length(); }
    bool hasSelectedText() const { return !m_text.isEmpty() && m_selend > m_selstart; }

    void setSelection(int start, int length);

    inline QString selectedText() const { return hasSelectedText() ? m_text.mid(m_selstart, m_selend - m_selstart) : QString(); }
    QString textBeforeSelection() const { return hasSelectedText() ? m_text.left(m_selstart) : QString(); }
    QString textAfterSelection() const { return hasSelectedText() ? m_text.mid(m_selend) : QString(); }

    int selectionStart() const { return hasSelectedText() ? m_selstart : -1; }
    int selectionEnd() const { return hasSelectedText() ? m_selend : -1; }

    int positionAt(int x, int y, QTextLine::CursorPosition position) const;
    int positionAt(const QPointF &point, QTextLine::CursorPosition position = QTextLine::CursorBetweenCharacters) const {
        return positionAt(point.x(), point.y(), position);
    }

    void removeSelection()
    {
        int priorState = m_undoState;
        removeSelectedText();
        finishChange(priorState);
    }

    int start() const { return 0; }
    int end() const { return m_text.length(); }

    QString realText() const;

#ifndef QT_NO_CLIPBOARD
    void copy(QClipboard::Mode mode = QClipboard::Clipboard) const;
    void paste(QClipboard::Mode mode = QClipboard::Clipboard);
#endif

    void commitPreedit();

    Qt::CursorMoveStyle cursorMoveStyle() const { return m_textLayout.cursorMoveStyle(); }
    void setCursorMoveStyle(Qt::CursorMoveStyle style) { m_textLayout.setCursorMoveStyle(style); }

    void moveCursor(int pos, bool mark = false);
    void cursorForward(bool mark, int steps)
    {
        int c = m_cursor;
        if (steps > 0) {
            while (steps--)
                c = cursorMoveStyle() == Qt::VisualMoveStyle ? m_textLayout.rightCursorPosition(c)
                                                             : m_textLayout.nextCursorPosition(c);
        } else if (steps < 0) {
            while (steps++)
                c = cursorMoveStyle() == Qt::VisualMoveStyle ? m_textLayout.leftCursorPosition(c)
                                                             : m_textLayout.previousCursorPosition(c);
        }
        moveCursor(c, mark);
    }

    void cursorWordForward(bool mark) { moveCursor(m_textLayout.nextCursorPosition(m_cursor, QTextLayout::SkipWords), mark); }
    void cursorWordBackward(bool mark) { moveCursor(m_textLayout.previousCursorPosition(m_cursor, QTextLayout::SkipWords), mark); }

    void home(bool mark) { moveCursor(0, mark); }
    void end(bool mark) { moveCursor(q_func()->text().length(), mark); }

    void backspace();
    void del();
    void deselect() { internalDeselect(); finishChange(); }
    void selectAll() { m_selstart = m_selend = m_cursor = 0; moveCursor(m_text.length(), true); }

    void insert(const QString &);
    void clear();
    void selectWordAtPos(int);

    void setCursorPosition(int pos) { if (pos <= m_text.length()) moveCursor(qMax(0, pos)); }

    bool fixup();

    QString inputMask() const { return m_maskData ? m_inputMask + QLatin1Char(';') + m_blank : QString(); }
    void setInputMask(const QString &mask)
    {
        parseInputMask(mask);
        if (m_maskData)
            moveCursor(nextMaskBlank(0));
    }

    // input methods
#ifndef QT_NO_IM
    bool composeMode() const { return !m_textLayout.preeditAreaText().isEmpty(); }
#endif

    QString preeditAreaText() const { return m_textLayout.preeditAreaText(); }

    void updatePasswordEchoEditing(bool editing);

    void cancelPasswordEchoTimer() {
#ifdef QT_GUI_PASSWORD_ECHO_DELAY
        m_passwordEchoTimer.stop();
#endif
    }

    Qt::LayoutDirection layoutDirection() const {
        if (m_layoutDirection == Qt::LayoutDirectionAuto) {
            if (m_text.isEmpty())
                return qApp->inputPanel()->inputDirection();
            return m_text.isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight;
        }
        return m_layoutDirection;
    }
    void setLayoutDirection(Qt::LayoutDirection direction)
    {
        if (direction != m_layoutDirection) {
            m_layoutDirection = direction;
            updateDisplayText();
        }
    }

    void processInputMethodEvent(QInputMethodEvent *event);
    void processKeyEvent(QKeyEvent* ev);

    void setCursorBlinkPeriod(int msec);
    void resetCursorBlinkTimer();

    void updateLayout();

private:
    void init(const QString &txt);
    void removeSelectedText();
    void internalSetText(const QString &txt, int pos = -1, bool edited = true);
    void updateDisplayText(bool forceUpdate = false);

    void internalInsert(const QString &s);
    void internalDelete(bool wasBackspace = false);
    void internalRemove(int pos);

    inline void internalDeselect()
    {
        m_selDirty |= (m_selend > m_selstart);
        m_selstart = m_selend = 0;
    }

    void internalUndo(int until = -1);
    void internalRedo();
    void emitUndoRedoChanged();

    bool emitCursorPositionChanged();

    bool finishChange(int validateFromState = -1, bool update = false, bool edited = true);

    void addCommand(const Command& cmd);

    inline void separate() { m_separator = true; }

    enum ValidatorState {
#ifndef QT_NO_VALIDATOR
        InvalidInput        = QValidator::Invalid,
        IntermediateInput   = QValidator::Intermediate,
        AcceptableInput     = QValidator::Acceptable
#else
        Invalid,
        Intermediate,
        Acceptable
#endif
    };

    // masking
    void parseInputMask(const QString &maskFields);
    bool isValidInput(QChar key, QChar mask) const;
    ValidatorState hasAcceptableInput(const QString &text) const;
    void checkIsValid();
    QString maskString(uint pos, const QString &str, bool clear = false) const;
    QString clearString(uint pos, uint len) const;
    QString stripString(const QString &str) const;
    int findInMask(int pos, bool forward, bool findSeparator, QChar searchChar = QChar()) const;
};

QT_END_NAMESPACE

#endif // QQUICKTEXTINPUT_P_P_H
