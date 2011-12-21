/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qquicktextcontrol_p.h"
#include "qquicktextcontrol_p_p.h"

#ifndef QT_NO_TEXTCONTROL

#include <qfont.h>
#include <qpainter.h>
#include <qevent.h>
#include <qdebug.h>
#include <qdrag.h>
#include <qclipboard.h>
#include <qtimer.h>
#include <qinputpanel.h>
#include "private/qtextdocumentlayout_p.h"
#include "private/qabstracttextdocumentlayout_p.h"
#include "qtextdocument.h"
#include "private/qtextdocument_p.h"
#include "qtextlist.h"
#include "qtextdocumentwriter.h"
#include "private/qtextcursor_p.h"
#include "qpagedpaintdevice.h"
#include "private/qpagedpaintdevice_p.h"

#include <qtextformat.h>
#include <qdatetime.h>
#include <qbuffer.h>
#include <qguiapplication.h>
#include <limits.h>
#include <qtexttable.h>
#include <qvariant.h>
#include <qurl.h>
#include <qstylehints.h>

// ### these should come from QStyleHints
const int textCursorWidth = 1;
const bool fullWidthSelection = true;

QT_BEGIN_NAMESPACE

#ifndef QT_NO_CONTEXTMENU
#endif

// could go into QTextCursor...
static QTextLine currentTextLine(const QTextCursor &cursor)
{
    const QTextBlock block = cursor.block();
    if (!block.isValid())
        return QTextLine();

    const QTextLayout *layout = block.layout();
    if (!layout)
        return QTextLine();

    const int relativePos = cursor.position() - block.position();
    return layout->lineForTextPosition(relativePos);
}

QQuickTextControlPrivate::QQuickTextControlPrivate()
    : doc(0), cursorOn(false), cursorIsFocusIndicator(false),
      interactionFlags(Qt::TextEditorInteraction),
      dragEnabled(true),
#ifndef QT_NO_DRAGANDDROP
      mousePressed(false), mightStartDrag(false),
#endif
      lastSelectionState(false), ignoreAutomaticScrollbarAdjustement(false),
      overwriteMode(false),
      acceptRichText(true),
      preeditCursor(0), hideCursor(false),
      hasFocus(false),
#ifdef QT_KEYPAD_NAVIGATION
      hasEditFocus(false),
#endif
      isEnabled(true),
      hadSelectionOnMousePress(false),
      ignoreUnusedNavigationEvents(false),
      openExternalLinks(false),
      wordSelectionEnabled(false)
{}

bool QQuickTextControlPrivate::cursorMoveKeyEvent(QKeyEvent *e)
{
#ifdef QT_NO_SHORTCUT
    Q_UNUSED(e);
#endif

    Q_Q(QQuickTextControl);
    if (cursor.isNull())
        return false;

    const QTextCursor oldSelection = cursor;
    const int oldCursorPos = cursor.position();

    QTextCursor::MoveMode mode = QTextCursor::MoveAnchor;
    QTextCursor::MoveOperation op = QTextCursor::NoMove;

    if (false) {
    }
#ifndef QT_NO_SHORTCUT
    if (e == QKeySequence::MoveToNextChar) {
            op = QTextCursor::Right;
    }
    else if (e == QKeySequence::MoveToPreviousChar) {
            op = QTextCursor::Left;
    }
    else if (e == QKeySequence::SelectNextChar) {
           op = QTextCursor::Right;
           mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectPreviousChar) {
            op = QTextCursor::Left;
            mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectNextWord) {
            op = QTextCursor::WordRight;
            mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectPreviousWord) {
            op = QTextCursor::WordLeft;
            mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectStartOfLine) {
            op = QTextCursor::StartOfLine;
            mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectEndOfLine) {
            op = QTextCursor::EndOfLine;
            mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectStartOfBlock) {
            op = QTextCursor::StartOfBlock;
            mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectEndOfBlock) {
            op = QTextCursor::EndOfBlock;
            mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectStartOfDocument) {
            op = QTextCursor::Start;
            mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectEndOfDocument) {
            op = QTextCursor::End;
            mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectPreviousLine) {
            op = QTextCursor::Up;
            mode = QTextCursor::KeepAnchor;
    }
    else if (e == QKeySequence::SelectNextLine) {
            op = QTextCursor::Down;
            mode = QTextCursor::KeepAnchor;
            {
                QTextBlock block = cursor.block();
                QTextLine line = currentTextLine(cursor);
                if (!block.next().isValid()
                    && line.isValid()
                    && line.lineNumber() == block.layout()->lineCount() - 1)
                    op = QTextCursor::End;
            }
    }
    else if (e == QKeySequence::MoveToNextWord) {
            op = QTextCursor::WordRight;
    }
    else if (e == QKeySequence::MoveToPreviousWord) {
            op = QTextCursor::WordLeft;
    }
    else if (e == QKeySequence::MoveToEndOfBlock) {
            op = QTextCursor::EndOfBlock;
    }
    else if (e == QKeySequence::MoveToStartOfBlock) {
            op = QTextCursor::StartOfBlock;
    }
    else if (e == QKeySequence::MoveToNextLine) {
            op = QTextCursor::Down;
    }
    else if (e == QKeySequence::MoveToPreviousLine) {
            op = QTextCursor::Up;
    }
    else if (e == QKeySequence::MoveToStartOfLine) {
            op = QTextCursor::StartOfLine;
    }
    else if (e == QKeySequence::MoveToEndOfLine) {
            op = QTextCursor::EndOfLine;
    }
    else if (e == QKeySequence::MoveToStartOfDocument) {
            op = QTextCursor::Start;
    }
    else if (e == QKeySequence::MoveToEndOfDocument) {
            op = QTextCursor::End;
    }
#endif // QT_NO_SHORTCUT
    else {
        return false;
    }

// Except for pageup and pagedown, Mac OS X has very different behavior, we don't do it all, but
// here's the breakdown:
// Shift still works as an anchor, but only one of the other keys can be down Ctrl (Command),
// Alt (Option), or Meta (Control).
// Command/Control + Left/Right -- Move to left or right of the line
//                 + Up/Down -- Move to top bottom of the file. (Control doesn't move the cursor)
// Option + Left/Right -- Move one word Left/right.
//        + Up/Down  -- Begin/End of Paragraph.
// Home/End Top/Bottom of file. (usually don't move the cursor, but will select)

    bool visualNavigation = cursor.visualNavigation();
    cursor.setVisualNavigation(true);
    const bool moved = cursor.movePosition(op, mode);
    cursor.setVisualNavigation(visualNavigation);
    q->ensureCursorVisible();

    bool ignoreNavigationEvents = ignoreUnusedNavigationEvents;
    bool isNavigationEvent = e->key() == Qt::Key_Up || e->key() == Qt::Key_Down;

#ifdef QT_KEYPAD_NAVIGATION
    ignoreNavigationEvents = ignoreNavigationEvents || QGuiApplication::keypadNavigationEnabled();
    isNavigationEvent = isNavigationEvent ||
                        (QGuiApplication::navigationMode() == Qt::NavigationModeKeypadDirectional
                         && (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right));
#else
    isNavigationEvent = isNavigationEvent || e->key() == Qt::Key_Left || e->key() == Qt::Key_Right;
#endif

    if (moved) {
        if (cursor.position() != oldCursorPos)
            emit q->cursorPositionChanged();
        emit q->microFocusChanged();
    } else if (ignoreNavigationEvents && isNavigationEvent && oldSelection.anchor() == cursor.anchor()) {
        return false;
    }

    selectionChanged(/*forceEmitSelectionChanged =*/(mode == QTextCursor::KeepAnchor));

    repaintOldAndNewSelection(oldSelection);

    return true;
}

void QQuickTextControlPrivate::updateCurrentCharFormat()
{
    Q_Q(QQuickTextControl);

    QTextCharFormat fmt = cursor.charFormat();
    if (fmt == lastCharFormat)
        return;
    lastCharFormat = fmt;

    emit q->currentCharFormatChanged(fmt);
    emit q->microFocusChanged();
}

void QQuickTextControlPrivate::indent()
{
    QTextBlockFormat blockFmt = cursor.blockFormat();

    QTextList *list = cursor.currentList();
    if (!list) {
        QTextBlockFormat modifier;
        modifier.setIndent(blockFmt.indent() + 1);
        cursor.mergeBlockFormat(modifier);
    } else {
        QTextListFormat format = list->format();
        format.setIndent(format.indent() + 1);

        if (list->itemNumber(cursor.block()) == 1)
            list->setFormat(format);
        else
            cursor.createList(format);
    }
}

void QQuickTextControlPrivate::outdent()
{
    QTextBlockFormat blockFmt = cursor.blockFormat();

    QTextList *list = cursor.currentList();

    if (!list) {
        QTextBlockFormat modifier;
        modifier.setIndent(blockFmt.indent() - 1);
        cursor.mergeBlockFormat(modifier);
    } else {
        QTextListFormat listFmt = list->format();
        listFmt.setIndent(listFmt.indent() - 1);
        list->setFormat(listFmt);
    }
}

void QQuickTextControlPrivate::gotoNextTableCell()
{
    QTextTable *table = cursor.currentTable();
    QTextTableCell cell = table->cellAt(cursor);

    int newColumn = cell.column() + cell.columnSpan();
    int newRow = cell.row();

    if (newColumn >= table->columns()) {
        newColumn = 0;
        ++newRow;
        if (newRow >= table->rows())
            table->insertRows(table->rows(), 1);
    }

    cell = table->cellAt(newRow, newColumn);
    cursor = cell.firstCursorPosition();
}

void QQuickTextControlPrivate::gotoPreviousTableCell()
{
    QTextTable *table = cursor.currentTable();
    QTextTableCell cell = table->cellAt(cursor);

    int newColumn = cell.column() - 1;
    int newRow = cell.row();

    if (newColumn < 0) {
        newColumn = table->columns() - 1;
        --newRow;
        if (newRow < 0)
            return;
    }

    cell = table->cellAt(newRow, newColumn);
    cursor = cell.firstCursorPosition();
}

void QQuickTextControlPrivate::createAutoBulletList()
{
    cursor.beginEditBlock();

    QTextBlockFormat blockFmt = cursor.blockFormat();

    QTextListFormat listFmt;
    listFmt.setStyle(QTextListFormat::ListDisc);
    listFmt.setIndent(blockFmt.indent() + 1);

    blockFmt.setIndent(0);
    cursor.setBlockFormat(blockFmt);

    cursor.createList(listFmt);

    cursor.endEditBlock();
}

void QQuickTextControlPrivate::init(Qt::TextFormat format, const QString &text, QTextDocument *document)
{
    Q_Q(QQuickTextControl);
    setContent(format, text, document);

    doc->setUndoRedoEnabled(interactionFlags & Qt::TextEditable);
    q->setCursorWidth(-1);

    QObject::connect(q, SIGNAL(updateCursorRequest(QRectF)), q, SIGNAL(updateRequest(QRectF)));
}

void QQuickTextControlPrivate::setContent(Qt::TextFormat format, const QString &text, QTextDocument *document)
{
    Q_Q(QQuickTextControl);

    // for use when called from setPlainText. we may want to re-use the currently
    // set char format then.
    const QTextCharFormat charFormatForInsertion = cursor.charFormat();

    bool clearDocument = true;
    if (!doc) {
        if (document) {
            doc = document;
            clearDocument = false;
        } else {
            palette = QGuiApplication::palette();
            doc = new QTextDocument(q);
        }
        _q_documentLayoutChanged();
        cursor = QTextCursor(doc);

// ####        doc->documentLayout()->setPaintDevice(viewport);

        QObject::connect(doc, SIGNAL(contentsChanged()), q, SLOT(_q_updateCurrentCharFormatAndSelection()));
        QObject::connect(doc, SIGNAL(cursorPositionChanged(QTextCursor)), q, SLOT(_q_emitCursorPosChanged(QTextCursor)));
        QObject::connect(doc, SIGNAL(documentLayoutChanged()), q, SLOT(_q_documentLayoutChanged()));

        // convenience signal forwards
        QObject::connect(doc, SIGNAL(undoAvailable(bool)), q, SIGNAL(undoAvailable(bool)));
        QObject::connect(doc, SIGNAL(redoAvailable(bool)), q, SIGNAL(redoAvailable(bool)));
        QObject::connect(doc, SIGNAL(modificationChanged(bool)), q, SIGNAL(modificationChanged(bool)));
        QObject::connect(doc, SIGNAL(blockCountChanged(int)), q, SIGNAL(blockCountChanged(int)));
    }

    bool previousUndoRedoState = doc->isUndoRedoEnabled();
    if (!document)
        doc->setUndoRedoEnabled(false);

    //Saving the index save some time.
    static int contentsChangedIndex = QTextDocument::staticMetaObject.indexOfSignal("contentsChanged()");
    static int textChangedIndex = QQuickTextControl::staticMetaObject.indexOfSignal("textChanged()");
    // avoid multiple textChanged() signals being emitted
    QMetaObject::disconnect(doc, contentsChangedIndex, q, textChangedIndex);

    if (!text.isEmpty()) {
        // clear 'our' cursor for insertion to prevent
        // the emission of the cursorPositionChanged() signal.
        // instead we emit it only once at the end instead of
        // at the end of the document after loading and when
        // positioning the cursor again to the start of the
        // document.
        cursor = QTextCursor();
        if (format == Qt::PlainText) {
            QTextCursor formatCursor(doc);
            // put the setPlainText and the setCharFormat into one edit block,
            // so that the syntax highlight triggers only /once/ for the entire
            // document, not twice.
            formatCursor.beginEditBlock();
            doc->setPlainText(text);
            doc->setUndoRedoEnabled(false);
            formatCursor.select(QTextCursor::Document);
            formatCursor.setCharFormat(charFormatForInsertion);
            formatCursor.endEditBlock();
        } else {
#ifndef QT_NO_TEXTHTMLPARSER
            doc->setHtml(text);
#else
            doc->setPlainText(text);
#endif
            doc->setUndoRedoEnabled(false);
        }
        cursor = QTextCursor(doc);
    } else if (clearDocument) {
        doc->clear();
    }
    cursor.setCharFormat(charFormatForInsertion);

    QMetaObject::connect(doc, contentsChangedIndex, q, textChangedIndex);
    emit q->textChanged();
    if (!document)
        doc->setUndoRedoEnabled(previousUndoRedoState);
    _q_updateCurrentCharFormatAndSelection();
    if (!document)
        doc->setModified(false);

    q->ensureCursorVisible();
    emit q->cursorPositionChanged();
}

void QQuickTextControlPrivate::startDrag()
{
#ifndef QT_NO_DRAGANDDROP
    Q_Q(QQuickTextControl);
    mousePressed = false;
    if (!contextObject)
        return;
    QMimeData *data = q->createMimeDataFromSelection();

    QDrag *drag = new QDrag(contextObject);
    drag->setMimeData(data);

    Qt::DropActions actions = Qt::CopyAction;
    Qt::DropAction action;
    if (interactionFlags & Qt::TextEditable) {
        actions |= Qt::MoveAction;
        action = drag->exec(actions, Qt::MoveAction);
    } else {
        action = drag->exec(actions, Qt::CopyAction);
    }

    if (action == Qt::MoveAction && drag->target() != contextObject)
        cursor.removeSelectedText();
#endif
}

void QQuickTextControlPrivate::setCursorPosition(const QPointF &pos)
{
    Q_Q(QQuickTextControl);
    const int cursorPos = q->hitTest(pos, Qt::FuzzyHit);
    if (cursorPos == -1)
        return;
    cursor.setPosition(cursorPos);
}

void QQuickTextControlPrivate::setCursorPosition(int pos, QTextCursor::MoveMode mode)
{
    cursor.setPosition(pos, mode);

    if (mode != QTextCursor::KeepAnchor) {
        selectedWordOnDoubleClick = QTextCursor();
        selectedBlockOnTrippleClick = QTextCursor();
    }
}

void QQuickTextControlPrivate::repaintCursor()
{
    Q_Q(QQuickTextControl);
    emit q->updateCursorRequest(cursorRectPlusUnicodeDirectionMarkers(cursor));
}

void QQuickTextControlPrivate::repaintOldAndNewSelection(const QTextCursor &oldSelection)
{
    Q_Q(QQuickTextControl);
    if (cursor.hasSelection()
        && oldSelection.hasSelection()
        && cursor.currentFrame() == oldSelection.currentFrame()
        && !cursor.hasComplexSelection()
        && !oldSelection.hasComplexSelection()
        && cursor.anchor() == oldSelection.anchor()
        ) {
        QTextCursor differenceSelection(doc);
        differenceSelection.setPosition(oldSelection.position());
        differenceSelection.setPosition(cursor.position(), QTextCursor::KeepAnchor);
        emit q->updateRequest(q->selectionRect(differenceSelection));
    } else {
        if (!oldSelection.hasSelection() && !cursor.hasSelection()) {
            if (!oldSelection.isNull())
                emit q->updateCursorRequest(q->selectionRect(oldSelection) | cursorRectPlusUnicodeDirectionMarkers(oldSelection));
            emit q->updateCursorRequest(q->selectionRect() | cursorRectPlusUnicodeDirectionMarkers(cursor));

        } else {
            if (!oldSelection.isNull())
                emit q->updateRequest(q->selectionRect(oldSelection) | cursorRectPlusUnicodeDirectionMarkers(oldSelection));
            emit q->updateRequest(q->selectionRect() | cursorRectPlusUnicodeDirectionMarkers(cursor));
        }
    }
}

void QQuickTextControlPrivate::selectionChanged(bool forceEmitSelectionChanged /*=false*/)
{
    Q_Q(QQuickTextControl);
    if (forceEmitSelectionChanged)
        emit q->selectionChanged();

    bool current = cursor.hasSelection();
    if (current == lastSelectionState)
        return;

    lastSelectionState = current;
    emit q->copyAvailable(current);
    if (!forceEmitSelectionChanged)
        emit q->selectionChanged();
    emit q->microFocusChanged();
}

void QQuickTextControlPrivate::_q_updateCurrentCharFormatAndSelection()
{
    updateCurrentCharFormat();
    selectionChanged();
}

#ifndef QT_NO_CLIPBOARD
void QQuickTextControlPrivate::setClipboardSelection()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!cursor.hasSelection() || !clipboard->supportsSelection())
        return;
    Q_Q(QQuickTextControl);
    QMimeData *data = q->createMimeDataFromSelection();
    clipboard->setMimeData(data, QClipboard::Selection);
}
#endif

void QQuickTextControlPrivate::_q_emitCursorPosChanged(const QTextCursor &someCursor)
{
    Q_Q(QQuickTextControl);
    if (someCursor.isCopyOf(cursor)) {
        emit q->cursorPositionChanged();
        emit q->microFocusChanged();
    }
}

void QQuickTextControlPrivate::_q_documentLayoutChanged()
{
    Q_Q(QQuickTextControl);
    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    QObject::connect(layout, SIGNAL(update(QRectF)), q, SIGNAL(updateRequest(QRectF)));
    QObject::connect(layout, SIGNAL(updateBlock(QTextBlock)), q, SLOT(_q_updateBlock(QTextBlock)));
    QObject::connect(layout, SIGNAL(documentSizeChanged(QSizeF)), q, SIGNAL(documentSizeChanged(QSizeF)));

}

void QQuickTextControlPrivate::setBlinkingCursorEnabled(bool enable)
{
    Q_Q(QQuickTextControl);

    if (enable && qApp->styleHints()->cursorFlashTime() > 0)
        cursorBlinkTimer.start(qApp->styleHints()->cursorFlashTime() / 2, q);
    else
        cursorBlinkTimer.stop();

    cursorOn = enable;

    repaintCursor();
}

void QQuickTextControlPrivate::extendWordwiseSelection(int suggestedNewPosition, qreal mouseXPosition)
{
    Q_Q(QQuickTextControl);

    // if inside the initial selected word keep that
    if (suggestedNewPosition >= selectedWordOnDoubleClick.selectionStart()
        && suggestedNewPosition <= selectedWordOnDoubleClick.selectionEnd()) {
        q->setTextCursor(selectedWordOnDoubleClick);
        return;
    }

    QTextCursor curs = selectedWordOnDoubleClick;
    curs.setPosition(suggestedNewPosition, QTextCursor::KeepAnchor);

    if (!curs.movePosition(QTextCursor::StartOfWord))
        return;
    const int wordStartPos = curs.position();

    const int blockPos = curs.block().position();
    const QPointF blockCoordinates = q->blockBoundingRect(curs.block()).topLeft();

    QTextLine line = currentTextLine(curs);
    if (!line.isValid())
        return;

    const qreal wordStartX = line.cursorToX(curs.position() - blockPos) + blockCoordinates.x();

    if (!curs.movePosition(QTextCursor::EndOfWord))
        return;
    const int wordEndPos = curs.position();

    const QTextLine otherLine = currentTextLine(curs);
    if (otherLine.textStart() != line.textStart()
        || wordEndPos == wordStartPos)
        return;

    const qreal wordEndX = line.cursorToX(curs.position() - blockPos) + blockCoordinates.x();

    if (!wordSelectionEnabled && (mouseXPosition < wordStartX || mouseXPosition > wordEndX))
        return;

    if (wordSelectionEnabled) {
        if (suggestedNewPosition < selectedWordOnDoubleClick.position()) {
            cursor.setPosition(selectedWordOnDoubleClick.selectionEnd());
            setCursorPosition(wordStartPos, QTextCursor::KeepAnchor);
        } else {
            cursor.setPosition(selectedWordOnDoubleClick.selectionStart());
            setCursorPosition(wordEndPos, QTextCursor::KeepAnchor);
        }
    } else {
        // keep the already selected word even when moving to the left
        // (#39164)
        if (suggestedNewPosition < selectedWordOnDoubleClick.position())
            cursor.setPosition(selectedWordOnDoubleClick.selectionEnd());
        else
            cursor.setPosition(selectedWordOnDoubleClick.selectionStart());

        const qreal differenceToStart = mouseXPosition - wordStartX;
        const qreal differenceToEnd = wordEndX - mouseXPosition;

        if (differenceToStart < differenceToEnd)
            setCursorPosition(wordStartPos, QTextCursor::KeepAnchor);
        else
            setCursorPosition(wordEndPos, QTextCursor::KeepAnchor);
    }

    if (interactionFlags & Qt::TextSelectableByMouse) {
#ifndef QT_NO_CLIPBOARD
        setClipboardSelection();
#endif
        selectionChanged(true);
    }
}

void QQuickTextControlPrivate::extendBlockwiseSelection(int suggestedNewPosition)
{
    Q_Q(QQuickTextControl);

    // if inside the initial selected line keep that
    if (suggestedNewPosition >= selectedBlockOnTrippleClick.selectionStart()
        && suggestedNewPosition <= selectedBlockOnTrippleClick.selectionEnd()) {
        q->setTextCursor(selectedBlockOnTrippleClick);
        return;
    }

    if (suggestedNewPosition < selectedBlockOnTrippleClick.position()) {
        cursor.setPosition(selectedBlockOnTrippleClick.selectionEnd());
        cursor.setPosition(suggestedNewPosition, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    } else {
        cursor.setPosition(selectedBlockOnTrippleClick.selectionStart());
        cursor.setPosition(suggestedNewPosition, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    }

    if (interactionFlags & Qt::TextSelectableByMouse) {
#ifndef QT_NO_CLIPBOARD
        setClipboardSelection();
#endif
        selectionChanged(true);
    }
}

void QQuickTextControlPrivate::_q_deleteSelected()
{
    if (!(interactionFlags & Qt::TextEditable) || !cursor.hasSelection())
        return;
    cursor.removeSelectedText();
}

void QQuickTextControl::undo()
{
    Q_D(QQuickTextControl);
    d->repaintSelection();
    const int oldCursorPos = d->cursor.position();
    d->doc->undo(&d->cursor);
    if (d->cursor.position() != oldCursorPos)
        emit cursorPositionChanged();
    emit microFocusChanged();
    ensureCursorVisible();
}

void QQuickTextControl::redo()
{
    Q_D(QQuickTextControl);
    d->repaintSelection();
    const int oldCursorPos = d->cursor.position();
    d->doc->redo(&d->cursor);
        if (d->cursor.position() != oldCursorPos)
        emit cursorPositionChanged();
    emit microFocusChanged();
    ensureCursorVisible();
}

QQuickTextControl::QQuickTextControl(QObject *parent)
    : QObject(*new QQuickTextControlPrivate, parent)
{
    Q_D(QQuickTextControl);
    d->init();
}

QQuickTextControl::QQuickTextControl(const QString &text, QObject *parent)
    : QObject(*new QQuickTextControlPrivate, parent)
{
    Q_D(QQuickTextControl);
    d->init(Qt::RichText, text);
}

QQuickTextControl::QQuickTextControl(QTextDocument *doc, QObject *parent)
    : QObject(*new QQuickTextControlPrivate, parent)
{
    Q_D(QQuickTextControl);
    d->init(Qt::RichText, QString(), doc);
}

QQuickTextControl::~QQuickTextControl()
{
}

void QQuickTextControl::setView(QObject *view)
{
    Q_D(QQuickTextControl);
    d->contextObject = view;
}

QObject *QQuickTextControl::view() const
{
    Q_D(const QQuickTextControl);
    return d->contextObject;
}

void QQuickTextControl::setDocument(QTextDocument *document)
{
    Q_D(QQuickTextControl);
    if (d->doc == document)
        return;

    d->doc->disconnect(this);
    d->doc->documentLayout()->disconnect(this);
    d->doc->documentLayout()->setPaintDevice(0);

    if (d->doc->parent() == this)
        delete d->doc;

    d->doc = 0;
    d->setContent(Qt::RichText, QString(), document);
}

QTextDocument *QQuickTextControl::document() const
{
    Q_D(const QQuickTextControl);
    return d->doc;
}

void QQuickTextControl::setTextCursor(const QTextCursor &cursor)
{
    Q_D(QQuickTextControl);
    d->cursorIsFocusIndicator = false;
    const bool posChanged = cursor.position() != d->cursor.position();
    const QTextCursor oldSelection = d->cursor;
    d->cursor = cursor;
    d->cursorOn = d->hasFocus && (d->interactionFlags & Qt::TextEditable);
    d->_q_updateCurrentCharFormatAndSelection();
    ensureCursorVisible();
    d->repaintOldAndNewSelection(oldSelection);
    if (posChanged)
        emit cursorPositionChanged();
}

QTextCursor QQuickTextControl::textCursor() const
{
    Q_D(const QQuickTextControl);
    return d->cursor;
}

#ifndef QT_NO_CLIPBOARD

void QQuickTextControl::cut()
{
    Q_D(QQuickTextControl);
    if (!(d->interactionFlags & Qt::TextEditable) || !d->cursor.hasSelection())
        return;
    copy();
    d->cursor.removeSelectedText();
}

void QQuickTextControl::copy()
{
    Q_D(QQuickTextControl);
    if (!d->cursor.hasSelection())
        return;
    QMimeData *data = createMimeDataFromSelection();
    QGuiApplication::clipboard()->setMimeData(data);
}

void QQuickTextControl::paste(QClipboard::Mode mode)
{
    const QMimeData *md = QGuiApplication::clipboard()->mimeData(mode);
    if (md)
        insertFromMimeData(md);
}
#endif

void QQuickTextControl::clear()
{
    Q_D(QQuickTextControl);
    // clears and sets empty content
    d->extraSelections.clear();
    d->setContent();
}


void QQuickTextControl::selectAll()
{
    Q_D(QQuickTextControl);
    const int selectionLength = qAbs(d->cursor.position() - d->cursor.anchor());
    d->cursor.select(QTextCursor::Document);
    d->selectionChanged(selectionLength != qAbs(d->cursor.position() - d->cursor.anchor()));
    d->cursorIsFocusIndicator = false;
    emit updateRequest();
}

void QQuickTextControl::processEvent(QEvent *e, const QPointF &coordinateOffset)
{
    QMatrix m;
    m.translate(coordinateOffset.x(), coordinateOffset.y());
    processEvent(e, m);
}

void QQuickTextControl::processEvent(QEvent *e, const QMatrix &matrix)
{
    Q_D(QQuickTextControl);
    if (d->interactionFlags == Qt::NoTextInteraction) {
        e->ignore();
        return;
    }

    switch (e->type()) {
        case QEvent::KeyPress:
            d->keyPressEvent(static_cast<QKeyEvent *>(e));
            break;
        case QEvent::MouseButtonPress: {
            QMouseEvent *ev = static_cast<QMouseEvent *>(e);
            d->mousePressEvent(ev, ev->button(), matrix.map(ev->localPos()), ev->modifiers(),
                               ev->buttons(), ev->screenPos().toPoint());
            break; }
        case QEvent::MouseMove: {
            QMouseEvent *ev = static_cast<QMouseEvent *>(e);
            d->mouseMoveEvent(ev, ev->button(), matrix.map(ev->localPos()), ev->modifiers(),
                              ev->buttons(), ev->screenPos().toPoint());
            break; }
        case QEvent::MouseButtonRelease: {
            QMouseEvent *ev = static_cast<QMouseEvent *>(e);
            d->mouseReleaseEvent(ev, ev->button(), matrix.map(ev->localPos()), ev->modifiers(),
                                 ev->buttons(), ev->screenPos().toPoint());
            break; }
        case QEvent::MouseButtonDblClick: {
            QMouseEvent *ev = static_cast<QMouseEvent *>(e);
            d->mouseDoubleClickEvent(ev, ev->button(), matrix.map(ev->localPos()), ev->modifiers(),
                                     ev->buttons(), ev->screenPos().toPoint());
            break; }
        case QEvent::InputMethod:
            d->inputMethodEvent(static_cast<QInputMethodEvent *>(e));
            break;
        case QEvent::FocusIn:
        case QEvent::FocusOut:
            d->focusEvent(static_cast<QFocusEvent *>(e));
            break;

        case QEvent::EnabledChange:
            d->isEnabled = e->isAccepted();
            break;

#ifndef QT_NO_DRAGANDDROP
        case QEvent::DragEnter: {
            QDragEnterEvent *ev = static_cast<QDragEnterEvent *>(e);
            if (d->dragEnterEvent(e, ev->mimeData()))
                ev->acceptProposedAction();
            break;
        }
        case QEvent::DragLeave:
            d->dragLeaveEvent();
            break;
        case QEvent::DragMove: {
            QDragMoveEvent *ev = static_cast<QDragMoveEvent *>(e);
            if (d->dragMoveEvent(e, ev->mimeData(), matrix.map(ev->pos())))
                ev->acceptProposedAction();
            break;
        }
        case QEvent::Drop: {
            QDropEvent *ev = static_cast<QDropEvent *>(e);
            if (d->dropEvent(ev->mimeData(), matrix.map(ev->pos()), ev->dropAction(), ev->source()))
                ev->acceptProposedAction();
            break;
        }
#endif

#ifdef QT_KEYPAD_NAVIGATION
        case QEvent::EnterEditFocus:
        case QEvent::LeaveEditFocus:
            if (QGuiApplication::keypadNavigationEnabled())
                d->editFocusEvent(e);
            break;
#endif
        case QEvent::ShortcutOverride:
            if (d->interactionFlags & Qt::TextEditable) {
                QKeyEvent* ke = static_cast<QKeyEvent *>(e);
                if (ke->modifiers() == Qt::NoModifier
                    || ke->modifiers() == Qt::ShiftModifier
                    || ke->modifiers() == Qt::KeypadModifier) {
                    if (ke->key() < Qt::Key_Escape) {
                        ke->accept();
                    } else {
                        switch (ke->key()) {
                            case Qt::Key_Return:
                            case Qt::Key_Enter:
                            case Qt::Key_Delete:
                            case Qt::Key_Home:
                            case Qt::Key_End:
                            case Qt::Key_Backspace:
                            case Qt::Key_Left:
                            case Qt::Key_Right:
                            case Qt::Key_Up:
                            case Qt::Key_Down:
                            case Qt::Key_Tab:
                            ke->accept();
                        default:
                            break;
                        }
                    }
#ifndef QT_NO_SHORTCUT
                } else if (ke == QKeySequence::Copy
                           || ke == QKeySequence::Paste
                           || ke == QKeySequence::Cut
                           || ke == QKeySequence::Redo
                           || ke == QKeySequence::Undo
                           || ke == QKeySequence::MoveToNextWord
                           || ke == QKeySequence::MoveToPreviousWord
                           || ke == QKeySequence::MoveToStartOfDocument
                           || ke == QKeySequence::MoveToEndOfDocument
                           || ke == QKeySequence::SelectNextWord
                           || ke == QKeySequence::SelectPreviousWord
                           || ke == QKeySequence::SelectStartOfLine
                           || ke == QKeySequence::SelectEndOfLine
                           || ke == QKeySequence::SelectStartOfBlock
                           || ke == QKeySequence::SelectEndOfBlock
                           || ke == QKeySequence::SelectStartOfDocument
                           || ke == QKeySequence::SelectEndOfDocument
                           || ke == QKeySequence::SelectAll
                          ) {
                    ke->accept();
#endif
                }
            }
            break;
        default:
            break;
    }
}

bool QQuickTextControl::event(QEvent *e)
{
    return QObject::event(e);
}

void QQuickTextControl::timerEvent(QTimerEvent *e)
{
    Q_D(QQuickTextControl);
    if (e->timerId() == d->cursorBlinkTimer.timerId()) {
        d->cursorOn = !d->cursorOn;

        // ###
//        if (d->cursor.hasSelection())
//            d->cursorOn &= (QGuiApplication::style()->styleHint(QStyle::SH_BlinkCursorWhenTextSelected)
//                            != 0);

        d->repaintCursor();
    } else if (e->timerId() == d->trippleClickTimer.timerId()) {
        d->trippleClickTimer.stop();
    }
}

void QQuickTextControl::setPlainText(const QString &text)
{
    Q_D(QQuickTextControl);
    d->setContent(Qt::PlainText, text);
}

void QQuickTextControl::setHtml(const QString &text)
{
    Q_D(QQuickTextControl);
    d->setContent(Qt::RichText, text);
}

void QQuickTextControlPrivate::keyPressEvent(QKeyEvent *e)
{
    Q_Q(QQuickTextControl);
#ifndef QT_NO_SHORTCUT
    if (e == QKeySequence::SelectAll) {
            e->accept();
            q->selectAll();
            return;
    }
#ifndef QT_NO_CLIPBOARD
    else if (e == QKeySequence::Copy) {
            e->accept();
            q->copy();
            return;
    }
#endif
#endif // QT_NO_SHORTCUT

    if (interactionFlags & Qt::TextSelectableByKeyboard
        && cursorMoveKeyEvent(e))
        goto accept;

    if (interactionFlags & Qt::LinksAccessibleByKeyboard) {
        if ((e->key() == Qt::Key_Return
             || e->key() == Qt::Key_Enter
#ifdef QT_KEYPAD_NAVIGATION
             || e->key() == Qt::Key_Select
#endif
             )
            && cursor.hasSelection()) {

            e->accept();
            activateLinkUnderCursor();
            return;
        }
    }

    if (!(interactionFlags & Qt::TextEditable)) {
        e->ignore();
        return;
    }

    if (e->key() == Qt::Key_Direction_L || e->key() == Qt::Key_Direction_R) {
        QTextBlockFormat fmt;
        fmt.setLayoutDirection((e->key() == Qt::Key_Direction_L) ? Qt::LeftToRight : Qt::RightToLeft);
        cursor.mergeBlockFormat(fmt);
        goto accept;
    }

    // schedule a repaint of the region of the cursor, as when we move it we
    // want to make sure the old cursor disappears (not noticeable when moving
    // only a few pixels but noticeable when jumping between cells in tables for
    // example)
    repaintSelection();

    if (e->key() == Qt::Key_Backspace && !(e->modifiers() & ~Qt::ShiftModifier)) {
        QTextBlockFormat blockFmt = cursor.blockFormat();
        QTextList *list = cursor.currentList();
        if (list && cursor.atBlockStart() && !cursor.hasSelection()) {
            list->remove(cursor.block());
        } else if (cursor.atBlockStart() && blockFmt.indent() > 0) {
            blockFmt.setIndent(blockFmt.indent() - 1);
            cursor.setBlockFormat(blockFmt);
        } else {
            QTextCursor localCursor = cursor;
            localCursor.deletePreviousChar();
        }
        goto accept;
    }
#ifndef QT_NO_SHORTCUT
      else if (e == QKeySequence::InsertParagraphSeparator) {
        cursor.insertBlock();
        e->accept();
        goto accept;
    } else if (e == QKeySequence::InsertLineSeparator) {
        cursor.insertText(QString(QChar::LineSeparator));
        e->accept();
        goto accept;
    }
#endif
    if (false) {
    }
#ifndef QT_NO_SHORTCUT
    else if (e == QKeySequence::Undo) {
            q->undo();
    }
    else if (e == QKeySequence::Redo) {
           q->redo();
    }
#ifndef QT_NO_CLIPBOARD
    else if (e == QKeySequence::Cut) {
           q->cut();
    }
    else if (e == QKeySequence::Paste) {
        QClipboard::Mode mode = QClipboard::Clipboard;
        q->paste(mode);
    }
#endif
    else if (e == QKeySequence::Delete) {
        QTextCursor localCursor = cursor;
        localCursor.deleteChar();
    }
    else if (e == QKeySequence::DeleteEndOfWord) {
        if (!cursor.hasSelection())
            cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }
    else if (e == QKeySequence::DeleteStartOfWord) {
        if (!cursor.hasSelection())
            cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }
    else if (e == QKeySequence::DeleteEndOfLine) {
        QTextBlock block = cursor.block();
        if (cursor.position() == block.position() + block.length() - 2)
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
        else
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }
#endif // QT_NO_SHORTCUT
    else {
        goto process;
    }
    goto accept;

process:
    {
        QString text = e->text();
        if (!text.isEmpty() && (text.at(0).isPrint() || text.at(0) == QLatin1Char('\t'))) {
            if (overwriteMode
                // no need to call deleteChar() if we have a selection, insertText
                // does it already
                && !cursor.hasSelection()
                && !cursor.atBlockEnd())
                cursor.deleteChar();

            cursor.insertText(text);
            selectionChanged();
        } else {
            e->ignore();
            return;
        }
    }

 accept:

    e->accept();
    cursorOn = true;

    q->ensureCursorVisible();

    updateCurrentCharFormat();
}

QVariant QQuickTextControl::loadResource(int type, const QUrl &name)
{
#if 1
    Q_UNUSED(type);
    Q_UNUSED(name);
#else
    if (QTextEdit *textEdit = qobject_cast<QTextEdit *>(parent())) {
        QUrl resolvedName = textEdit->d_func()->resolveUrl(name);
        return textEdit->loadResource(type, resolvedName);
    }
#endif
    return QVariant();
}

void QQuickTextControlPrivate::_q_updateBlock(const QTextBlock &block)
{
    Q_Q(QQuickTextControl);
    QRectF br = q->blockBoundingRect(block);
    br.setRight(qreal(INT_MAX)); // the block might have shrunk
    emit q->updateRequest(br);
}

QRectF QQuickTextControlPrivate::rectForPosition(int position) const
{
    Q_Q(const QQuickTextControl);
    const QTextBlock block = doc->findBlock(position);
    if (!block.isValid())
        return QRectF();
    const QAbstractTextDocumentLayout *docLayout = doc->documentLayout();
    const QTextLayout *layout = block.layout();
    const QPointF layoutPos = q->blockBoundingRect(block).topLeft();
    int relativePos = position - block.position();
    if (preeditCursor != 0) {
        int preeditPos = layout->preeditAreaPosition();
        if (relativePos == preeditPos)
            relativePos += preeditCursor;
        else if (relativePos > preeditPos)
            relativePos += layout->preeditAreaText().length();
    }
    QTextLine line = layout->lineForTextPosition(relativePos);

    int cursorWidth;
    {
        bool ok = false;
#ifndef QT_NO_PROPERTIES
        cursorWidth = docLayout->property("cursorWidth").toInt(&ok);
#endif
        if (!ok)
            cursorWidth = 1;
    }

    QRectF r;

    if (line.isValid()) {
        qreal x = line.cursorToX(relativePos);
        qreal w = 0;
        if (overwriteMode) {
            if (relativePos < line.textLength() - line.textStart())
                w = line.cursorToX(relativePos + 1) - x;
            else
                w = QFontMetrics(block.layout()->font()).width(QLatin1Char(' ')); // in sync with QTextLine::draw()
        }
        r = QRectF(layoutPos.x() + x, layoutPos.y() + line.y(),
                   cursorWidth + w, line.height());
    } else {
        r = QRectF(layoutPos.x(), layoutPos.y(), cursorWidth, 10); // #### correct height
    }

    return r;
}

static inline bool firstFramePosLessThanCursorPos(QTextFrame *frame, int position)
{
    return frame->firstPosition() < position;
}

static inline bool cursorPosLessThanLastFramePos(int position, QTextFrame *frame)
{
    return position < frame->lastPosition();
}

static QRectF boundingRectOfFloatsInSelection(const QTextCursor &cursor)
{
    QRectF r;
    QTextFrame *frame = cursor.currentFrame();
    const QList<QTextFrame *> children = frame->childFrames();

    const QList<QTextFrame *>::ConstIterator firstFrame = qLowerBound(children.constBegin(), children.constEnd(),
                                                                      cursor.selectionStart(), firstFramePosLessThanCursorPos);
    const QList<QTextFrame *>::ConstIterator lastFrame = qUpperBound(children.constBegin(), children.constEnd(),
                                                                     cursor.selectionEnd(), cursorPosLessThanLastFramePos);
    for (QList<QTextFrame *>::ConstIterator it = firstFrame; it != lastFrame; ++it) {
        if ((*it)->frameFormat().position() != QTextFrameFormat::InFlow)
            r |= frame->document()->documentLayout()->frameBoundingRect(*it);
    }
    return r;
}

QRectF QQuickTextControl::selectionRect(const QTextCursor &cursor) const
{
    Q_D(const QQuickTextControl);

    QRectF r = d->rectForPosition(cursor.selectionStart());

    if (cursor.hasComplexSelection() && cursor.currentTable()) {
        QTextTable *table = cursor.currentTable();

        r = d->doc->documentLayout()->frameBoundingRect(table);
        /*
        int firstRow, numRows, firstColumn, numColumns;
        cursor.selectedTableCells(&firstRow, &numRows, &firstColumn, &numColumns);

        const QTextTableCell firstCell = table->cellAt(firstRow, firstColumn);
        const QTextTableCell lastCell = table->cellAt(firstRow + numRows - 1, firstColumn + numColumns - 1);

        const QAbstractTextDocumentLayout * const layout = doc->documentLayout();

        QRectF tableSelRect = layout->blockBoundingRect(firstCell.firstCursorPosition().block());

        for (int col = firstColumn; col < firstColumn + numColumns; ++col) {
            const QTextTableCell cell = table->cellAt(firstRow, col);
            const qreal y = layout->blockBoundingRect(cell.firstCursorPosition().block()).top();

            tableSelRect.setTop(qMin(tableSelRect.top(), y));
        }

        for (int row = firstRow; row < firstRow + numRows; ++row) {
            const QTextTableCell cell = table->cellAt(row, firstColumn);
            const qreal x = layout->blockBoundingRect(cell.firstCursorPosition().block()).left();

            tableSelRect.setLeft(qMin(tableSelRect.left(), x));
        }

        for (int col = firstColumn; col < firstColumn + numColumns; ++col) {
            const QTextTableCell cell = table->cellAt(firstRow + numRows - 1, col);
            const qreal y = layout->blockBoundingRect(cell.lastCursorPosition().block()).bottom();

            tableSelRect.setBottom(qMax(tableSelRect.bottom(), y));
        }

        for (int row = firstRow; row < firstRow + numRows; ++row) {
            const QTextTableCell cell = table->cellAt(row, firstColumn + numColumns - 1);
            const qreal x = layout->blockBoundingRect(cell.lastCursorPosition().block()).right();

            tableSelRect.setRight(qMax(tableSelRect.right(), x));
        }

        r = tableSelRect.toRect();
        */
    } else if (cursor.hasSelection()) {
        const int position = cursor.selectionStart();
        const int anchor = cursor.selectionEnd();
        const QTextBlock posBlock = d->doc->findBlock(position);
        const QTextBlock anchorBlock = d->doc->findBlock(anchor);
        if (posBlock == anchorBlock && posBlock.isValid() && posBlock.layout()->lineCount()) {
            const QTextLine posLine = posBlock.layout()->lineForTextPosition(position - posBlock.position());
            const QTextLine anchorLine = anchorBlock.layout()->lineForTextPosition(anchor - anchorBlock.position());

            const int firstLine = qMin(posLine.lineNumber(), anchorLine.lineNumber());
            const int lastLine = qMax(posLine.lineNumber(), anchorLine.lineNumber());
            const QTextLayout *layout = posBlock.layout();
            r = QRectF();
            for (int i = firstLine; i <= lastLine; ++i) {
                r |= layout->lineAt(i).rect();
                r |= layout->lineAt(i).naturalTextRect(); // might be bigger in the case of wrap not enabled
            }
            r.translate(blockBoundingRect(posBlock).topLeft());
        } else {
            QRectF anchorRect = d->rectForPosition(cursor.selectionEnd());
            r |= anchorRect;
            r |= boundingRectOfFloatsInSelection(cursor);
            QRectF frameRect(d->doc->documentLayout()->frameBoundingRect(cursor.currentFrame()));
            r.setLeft(frameRect.left());
            r.setRight(frameRect.right());
        }
        if (r.isValid())
            r.adjust(-1, -1, 1, 1);
    }

    return r;
}

QRectF QQuickTextControl::selectionRect() const
{
    Q_D(const QQuickTextControl);
    return selectionRect(d->cursor);
}

void QQuickTextControlPrivate::mousePressEvent(QEvent *e, Qt::MouseButton button, const QPointF &pos, Qt::KeyboardModifiers modifiers,
                                          Qt::MouseButtons buttons, const QPoint &globalPos)
{
    Q_Q(QQuickTextControl);

    if (sendMouseEventToInputContext(
            e, QEvent::MouseButtonPress, button, pos, modifiers, buttons, globalPos)) {
        return;
    }

    if (interactionFlags & Qt::LinksAccessibleByMouse) {
        anchorOnMousePress = q->anchorAt(pos);

        if (cursorIsFocusIndicator) {
            cursorIsFocusIndicator = false;
            repaintSelection();
            cursor.clearSelection();
        }
    }
    if (!(button & Qt::LeftButton) ||
        !((interactionFlags & Qt::TextSelectableByMouse) || (interactionFlags & Qt::TextEditable))) {
            e->ignore();
            return;
    }

    cursorIsFocusIndicator = false;
    const QTextCursor oldSelection = cursor;
    const int oldCursorPos = cursor.position();

    mousePressed = (interactionFlags & Qt::TextSelectableByMouse);
#ifndef QT_NO_DRAGANDDROP
    mightStartDrag = false;
#endif

    if (trippleClickTimer.isActive()
        && ((pos - trippleClickPoint).toPoint().manhattanLength() < qApp->styleHints()->startDragDistance())) {

        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        selectedBlockOnTrippleClick = cursor;

        anchorOnMousePress = QString();

        trippleClickTimer.stop();
    } else {
        int cursorPos = q->hitTest(pos, Qt::FuzzyHit);
        if (cursorPos == -1) {
            e->ignore();
            return;
        }

        if (modifiers == Qt::ShiftModifier && (interactionFlags & Qt::TextSelectableByMouse)) {
            if (wordSelectionEnabled && !selectedWordOnDoubleClick.hasSelection()) {
                selectedWordOnDoubleClick = cursor;
                selectedWordOnDoubleClick.select(QTextCursor::WordUnderCursor);
            }

            if (selectedBlockOnTrippleClick.hasSelection())
                extendBlockwiseSelection(cursorPos);
            else if (selectedWordOnDoubleClick.hasSelection())
                extendWordwiseSelection(cursorPos, pos.x());
            else if (!wordSelectionEnabled)
                setCursorPosition(cursorPos, QTextCursor::KeepAnchor);
        } else {

            if (dragEnabled
                && cursor.hasSelection()
                && !cursorIsFocusIndicator
                && cursorPos >= cursor.selectionStart()
                && cursorPos <= cursor.selectionEnd()
                && q->hitTest(pos, Qt::ExactHit) != -1) {
#ifndef QT_NO_DRAGANDDROP
                mightStartDrag = true;
                dragStartPos = pos.toPoint();
#endif
                return;
            }

            setCursorPosition(cursorPos);
        }
    }

    if (interactionFlags & Qt::TextEditable) {
        q->ensureCursorVisible();
        if (cursor.position() != oldCursorPos)
            emit q->cursorPositionChanged();
        _q_updateCurrentCharFormatAndSelection();
    } else {
        if (cursor.position() != oldCursorPos) {
            emit q->cursorPositionChanged();
            emit q->microFocusChanged();
        }
        selectionChanged();
    }
    repaintOldAndNewSelection(oldSelection);
    hadSelectionOnMousePress = cursor.hasSelection();
}

void QQuickTextControlPrivate::mouseMoveEvent(QEvent *e, Qt::MouseButton button, const QPointF &mousePos, Qt::KeyboardModifiers modifiers,
                                         Qt::MouseButtons buttons, const QPoint &globalPos)
{
    Q_Q(QQuickTextControl);

    if (sendMouseEventToInputContext(
            e, QEvent::MouseMove, button, mousePos, modifiers, buttons, globalPos)) {
        return;
    }

    if (interactionFlags & Qt::LinksAccessibleByMouse) {
        QString anchor = q->anchorAt(mousePos);
        if (anchor != highlightedAnchor) {
            highlightedAnchor = anchor;
            emit q->linkHovered(anchor);
        }
    }

    if (!(buttons & Qt::LeftButton))
        return;

    const bool editable = interactionFlags & Qt::TextEditable;

    if (!(mousePressed
          || editable
          || mightStartDrag
          || selectedWordOnDoubleClick.hasSelection()
          || selectedBlockOnTrippleClick.hasSelection()))
        return;

    const QTextCursor oldSelection = cursor;
    const int oldCursorPos = cursor.position();

    if (mightStartDrag) {
        if ((mousePos.toPoint() - dragStartPos).manhattanLength() > qApp->styleHints()->startDragDistance())
            startDrag();
        return;
    }

    if (!mousePressed)
        return;

    const qreal mouseX = qreal(mousePos.x());

    int newCursorPos = q->hitTest(mousePos, Qt::FuzzyHit);
    if (newCursorPos == -1)
        return;

    if (wordSelectionEnabled && !selectedWordOnDoubleClick.hasSelection()) {
        selectedWordOnDoubleClick = cursor;
        selectedWordOnDoubleClick.select(QTextCursor::WordUnderCursor);
    }

    if (selectedBlockOnTrippleClick.hasSelection())
        extendBlockwiseSelection(newCursorPos);
    else if (selectedWordOnDoubleClick.hasSelection())
        extendWordwiseSelection(newCursorPos, mouseX);
    else
        setCursorPosition(newCursorPos, QTextCursor::KeepAnchor);

    if (interactionFlags & Qt::TextEditable) {
        // don't call ensureVisible for the visible cursor to avoid jumping
        // scrollbars. the autoscrolling ensures smooth scrolling if necessary.
        //q->ensureCursorVisible();
        if (cursor.position() != oldCursorPos)
            emit q->cursorPositionChanged();
        _q_updateCurrentCharFormatAndSelection();
        if (qGuiApp)
            qGuiApp->inputPanel()->update(Qt::ImQueryInput);
    } else {
        //emit q->visibilityRequest(QRectF(mousePos, QSizeF(1, 1)));
        if (cursor.position() != oldCursorPos) {
            emit q->cursorPositionChanged();
            emit q->microFocusChanged();
        }
    }
    selectionChanged(true);
    repaintOldAndNewSelection(oldSelection);
}

void QQuickTextControlPrivate::mouseReleaseEvent(QEvent *e, Qt::MouseButton button, const QPointF &pos, Qt::KeyboardModifiers modifiers,
                                            Qt::MouseButtons buttons, const QPoint &globalPos)
{
    Q_Q(QQuickTextControl);

    if (sendMouseEventToInputContext(
            e, QEvent::MouseButtonRelease, button, pos, modifiers, buttons, globalPos)) {
        return;
    }

    const QTextCursor oldSelection = cursor;
    const int oldCursorPos = cursor.position();

#ifndef QT_NO_DRAGANDDROP
    if (mightStartDrag && (button & Qt::LeftButton)) {
        mousePressed = false;
        setCursorPosition(pos);
        cursor.clearSelection();
        selectionChanged();
    }
#endif
    if (mousePressed) {
        mousePressed = false;
#ifndef QT_NO_CLIPBOARD
        setClipboardSelection();
        selectionChanged(true);
    } else if (button == Qt::MidButton
               && (interactionFlags & Qt::TextEditable)
               && QGuiApplication::clipboard()->supportsSelection()) {
        setCursorPosition(pos);
        const QMimeData *md = QGuiApplication::clipboard()->mimeData(QClipboard::Selection);
        if (md)
            q->insertFromMimeData(md);
#endif
    }

    repaintOldAndNewSelection(oldSelection);

    if (cursor.position() != oldCursorPos) {
        emit q->cursorPositionChanged();
        emit q->microFocusChanged();
    }

    if (interactionFlags & Qt::LinksAccessibleByMouse) {
        if (!(button & Qt::LeftButton))
            return;

        const QString anchor = q->anchorAt(pos);

        if (anchor.isEmpty())
            return;

        if (!cursor.hasSelection()
            || (anchor == anchorOnMousePress && hadSelectionOnMousePress)) {

            const int anchorPos = q->hitTest(pos, Qt::ExactHit);
            if (anchorPos != -1) {
                cursor.setPosition(anchorPos);

                QString anchor = anchorOnMousePress;
                anchorOnMousePress = QString();
                activateLinkUnderCursor(anchor);
            }
        }
    }
}

void QQuickTextControlPrivate::mouseDoubleClickEvent(QEvent *e, Qt::MouseButton button, const QPointF &pos, Qt::KeyboardModifiers modifiers,
                                                Qt::MouseButtons buttons, const QPoint &globalPos)
{
    Q_Q(QQuickTextControl);

    if (sendMouseEventToInputContext(
            e, QEvent::MouseButtonDblClick, button, pos, modifiers, buttons, globalPos)) {
        return;
    }

    if (button != Qt::LeftButton
        || !(interactionFlags & Qt::TextSelectableByMouse)) {
        e->ignore();
        return;
    }

#ifndef QT_NO_DRAGANDDROP
    mightStartDrag = false;
#endif
    const QTextCursor oldSelection = cursor;
    setCursorPosition(pos);
    QTextLine line = currentTextLine(cursor);
    bool doEmit = false;
    if (line.isValid() && line.textLength()) {
        cursor.select(QTextCursor::WordUnderCursor);
        doEmit = true;
    }
    repaintOldAndNewSelection(oldSelection);

    cursorIsFocusIndicator = false;
    selectedWordOnDoubleClick = cursor;

    trippleClickPoint = pos;
    trippleClickTimer.start(qApp->styleHints()->mouseDoubleClickInterval(), q);
    if (doEmit) {
        selectionChanged();
#ifndef QT_NO_CLIPBOARD
        setClipboardSelection();
#endif
        emit q->cursorPositionChanged();
    }
}

bool QQuickTextControlPrivate::sendMouseEventToInputContext(
        QEvent *e, QEvent::Type eventType, Qt::MouseButton button, const QPointF &pos,
        Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons, const QPoint &globalPos)
{
#if 0 // ### !defined(QT_NO_IM)
    Q_Q(QQuickTextControl);

    QTextLayout *layout = cursor.block().layout();
    if (contextObject && layout && !layout->preeditAreaText().isEmpty()) {
        QInputContext *ctx = inputContext();
        int cursorPos = q->hitTest(pos, Qt::FuzzyHit) - cursor.position();

        if (cursorPos < 0 || cursorPos > layout->preeditAreaText().length()) {
            cursorPos = -1;
            // don't send move events outside the preedit area
            if (eventType == QEvent::MouseMove)
                return true;
        }
        if (ctx) {
            QMouseEvent ev(eventType, contextObject->mapFromGlobal(globalPos), globalPos,
                           button, buttons, modifiers);
            ctx->mouseHandler(cursorPos, &ev);
            e->setAccepted(ev.isAccepted());
        }
        if (!layout->preeditAreaText().isEmpty())
            return true;
    }
#else
    Q_UNUSED(e);
    Q_UNUSED(eventType);
    Q_UNUSED(button);
    Q_UNUSED(pos);
    Q_UNUSED(modifiers);
    Q_UNUSED(buttons);
    Q_UNUSED(globalPos);
#endif
    return false;
}

bool QQuickTextControlPrivate::dragEnterEvent(QEvent *e, const QMimeData *mimeData)
{
    Q_Q(QQuickTextControl);
    if (!(interactionFlags & Qt::TextEditable) || !q->canInsertFromMimeData(mimeData)) {
        e->ignore();
        return false;
    }

    dndFeedbackCursor = QTextCursor();

    return true; // accept proposed action
}

void QQuickTextControlPrivate::dragLeaveEvent()
{
    Q_Q(QQuickTextControl);

    const QRectF crect = q->cursorRect(dndFeedbackCursor);
    dndFeedbackCursor = QTextCursor();

    if (crect.isValid())
        emit q->updateRequest(crect);
}

bool QQuickTextControlPrivate::dragMoveEvent(QEvent *e, const QMimeData *mimeData, const QPointF &pos)
{
    Q_Q(QQuickTextControl);
    if (!(interactionFlags & Qt::TextEditable) || !q->canInsertFromMimeData(mimeData)) {
        e->ignore();
        return false;
    }

    const int cursorPos = q->hitTest(pos, Qt::FuzzyHit);
    if (cursorPos != -1) {
        QRectF crect = q->cursorRect(dndFeedbackCursor);
        if (crect.isValid())
            emit q->updateRequest(crect);

        dndFeedbackCursor = cursor;
        dndFeedbackCursor.setPosition(cursorPos);

        crect = q->cursorRect(dndFeedbackCursor);
        emit q->updateRequest(crect);
    }

    return true; // accept proposed action
}

bool QQuickTextControlPrivate::dropEvent(const QMimeData *mimeData, const QPointF &pos, Qt::DropAction dropAction, QObject *source)
{
    Q_Q(QQuickTextControl);
    dndFeedbackCursor = QTextCursor();

    if (!(interactionFlags & Qt::TextEditable) || !q->canInsertFromMimeData(mimeData))
        return false;

    repaintSelection();

    QTextCursor insertionCursor = q->cursorForPosition(pos);
    insertionCursor.beginEditBlock();

    if (dropAction == Qt::MoveAction && source == contextObject)
        cursor.removeSelectedText();

    cursor = insertionCursor;
    q->insertFromMimeData(mimeData);
    insertionCursor.endEditBlock();
    q->ensureCursorVisible();
    return true; // accept proposed action
}

void QQuickTextControlPrivate::inputMethodEvent(QInputMethodEvent *e)
{
    Q_Q(QQuickTextControl);
    if (!(interactionFlags & Qt::TextEditable) || cursor.isNull()) {
        e->ignore();
        return;
    }
    bool isGettingInput = !e->commitString().isEmpty()
            || e->preeditString() != cursor.block().layout()->preeditAreaText()
            || e->replacementLength() > 0;
    bool forceSelectionChanged = false;

    cursor.beginEditBlock();
    if (isGettingInput) {
        cursor.removeSelectedText();
    }

    // insert commit string
    if (!e->commitString().isEmpty() || e->replacementLength()) {
        QTextCursor c = cursor;
        c.setPosition(c.position() + e->replacementStart());
        c.setPosition(c.position() + e->replacementLength(), QTextCursor::KeepAnchor);
        c.insertText(e->commitString());
    }

    for (int i = 0; i < e->attributes().size(); ++i) {
        const QInputMethodEvent::Attribute &a = e->attributes().at(i);
        if (a.type == QInputMethodEvent::Selection) {
            QTextCursor oldCursor = cursor;
            int blockStart = a.start + cursor.block().position();
            cursor.setPosition(blockStart, QTextCursor::MoveAnchor);
            cursor.setPosition(blockStart + a.length, QTextCursor::KeepAnchor);
            q->ensureCursorVisible();
            repaintOldAndNewSelection(oldCursor);
            forceSelectionChanged = true;
        }
    }

    QTextBlock block = cursor.block();
    QTextLayout *layout = block.layout();
    if (isGettingInput)
        layout->setPreeditArea(cursor.position() - block.position(), e->preeditString());
    QList<QTextLayout::FormatRange> overrides;
    const int oldPreeditCursor = preeditCursor;
    preeditCursor = e->preeditString().length();
    hideCursor = false;
    for (int i = 0; i < e->attributes().size(); ++i) {
        const QInputMethodEvent::Attribute &a = e->attributes().at(i);
        if (a.type == QInputMethodEvent::Cursor) {
            preeditCursor = a.start;
            hideCursor = !a.length;
        } else if (a.type == QInputMethodEvent::TextFormat) {
            QTextCharFormat f = qvariant_cast<QTextFormat>(a.value).toCharFormat();
            if (f.isValid()) {
                QTextLayout::FormatRange o;
                o.start = a.start + cursor.position() - block.position();
                o.length = a.length;
                o.format = f;
                overrides.append(o);
            }
        }
    }
    layout->setAdditionalFormats(overrides);
    cursor.endEditBlock();
    QTextCursorPrivate *cursor_d = QTextCursorPrivate::getPrivate(&cursor);
    if (cursor_d)
        cursor_d->setX();
    if (oldPreeditCursor != preeditCursor)
        emit q->microFocusChanged();
    selectionChanged(forceSelectionChanged);
}

QVariant QQuickTextControl::inputMethodQuery(Qt::InputMethodQuery property) const
{
    Q_D(const QQuickTextControl);
    QTextBlock block = d->cursor.block();
    switch (property) {
    case Qt::ImCursorRectangle:
        return cursorRect();
    case Qt::ImFont:
        return QVariant(d->cursor.charFormat().font());
    case Qt::ImCursorPosition:
        return QVariant(d->cursor.position() - block.position());
    case Qt::ImSurroundingText:
        return QVariant(block.text());
    case Qt::ImCurrentSelection:
        return QVariant(d->cursor.selectedText());
    case Qt::ImMaximumTextLength:
        return QVariant(); // No limit.
    case Qt::ImAnchorPosition:
        return QVariant(d->cursor.anchor() - block.position());
    default:
        return QVariant();
    }
}

void QQuickTextControl::setFocus(bool focus, Qt::FocusReason reason)
{
    QFocusEvent ev(focus ? QEvent::FocusIn : QEvent::FocusOut,
                   reason);
    processEvent(&ev);
}

void QQuickTextControlPrivate::focusEvent(QFocusEvent *e)
{
    Q_Q(QQuickTextControl);
    emit q->updateRequest(q->selectionRect());
    if (e->gotFocus()) {
#ifdef QT_KEYPAD_NAVIGATION
        if (!QGuiApplication::keypadNavigationEnabled() || (hasEditFocus && (e->reason() == Qt::PopupFocusReason
            ))) {
#endif
            setBlinkingCursorEnabled(interactionFlags & (Qt::TextEditable | Qt::TextSelectableByKeyboard));
#ifdef QT_KEYPAD_NAVIGATION
        }
#endif
    } else {
        setBlinkingCursorEnabled(false);

        if (cursorIsFocusIndicator
            && e->reason() != Qt::ActiveWindowFocusReason
            && e->reason() != Qt::PopupFocusReason
            && cursor.hasSelection()) {
            cursor.clearSelection();
        }
    }
    hasFocus = e->gotFocus();
}

QString QQuickTextControlPrivate::anchorForCursor(const QTextCursor &anchorCursor) const
{
    if (anchorCursor.hasSelection()) {
        QTextCursor cursor = anchorCursor;
        if (cursor.selectionStart() != cursor.position())
            cursor.setPosition(cursor.selectionStart());
        cursor.movePosition(QTextCursor::NextCharacter);
        QTextCharFormat fmt = cursor.charFormat();
        if (fmt.isAnchor() && fmt.hasProperty(QTextFormat::AnchorHref))
            return fmt.stringProperty(QTextFormat::AnchorHref);
    }
    return QString();
}

#ifdef QT_KEYPAD_NAVIGATION
void QQuickTextControlPrivate::editFocusEvent(QEvent *e)
{
    Q_Q(QQuickTextControl);

    if (QGuiApplication::keypadNavigationEnabled()) {
        if (e->type() == QEvent::EnterEditFocus && interactionFlags & Qt::TextEditable) {
            const QTextCursor oldSelection = cursor;
            const int oldCursorPos = cursor.position();
            const bool moved = cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
            q->ensureCursorVisible();
            if (moved) {
                if (cursor.position() != oldCursorPos)
                    emit q->cursorPositionChanged();
                emit q->microFocusChanged();
            }
            selectionChanged();
            repaintOldAndNewSelection(oldSelection);

            setBlinkingCursorEnabled(true);
        } else
            setBlinkingCursorEnabled(false);
    }

    hasEditFocus = e->type() == QEvent::EnterEditFocus ? true : false;
}
#endif

QTextCursor QQuickTextControl::cursorForPosition(const QPointF &pos) const
{
    Q_D(const QQuickTextControl);
    int cursorPos = hitTest(pos, Qt::FuzzyHit);
    if (cursorPos == -1)
        cursorPos = 0;
    QTextCursor c(d->doc);
    c.setPosition(cursorPos);
    return c;
}

QRectF QQuickTextControl::cursorRect(const QTextCursor &cursor) const
{
    Q_D(const QQuickTextControl);
    if (cursor.isNull())
        return QRectF();

    return d->rectForPosition(cursor.position());
}

QRectF QQuickTextControl::cursorRect() const
{
    Q_D(const QQuickTextControl);
    return cursorRect(d->cursor);
}

QRectF QQuickTextControlPrivate::cursorRectPlusUnicodeDirectionMarkers(const QTextCursor &cursor) const
{
    if (cursor.isNull())
        return QRectF();

    return rectForPosition(cursor.position()).adjusted(-4, 0, 4, 0);
}

QString QQuickTextControl::anchorAt(const QPointF &pos) const
{
    Q_D(const QQuickTextControl);
    return d->doc->documentLayout()->anchorAt(pos);
}

QString QQuickTextControl::anchorAtCursor() const
{
    Q_D(const QQuickTextControl);

    return d->anchorForCursor(d->cursor);
}

bool QQuickTextControl::overwriteMode() const
{
    Q_D(const QQuickTextControl);
    return d->overwriteMode;
}

void QQuickTextControl::setOverwriteMode(bool overwrite)
{
    Q_D(QQuickTextControl);
    d->overwriteMode = overwrite;
}

int QQuickTextControl::cursorWidth() const
{
#ifndef QT_NO_PROPERTIES
    Q_D(const QQuickTextControl);
    return d->doc->documentLayout()->property("cursorWidth").toInt();
#else
    return 1;
#endif
}

void QQuickTextControl::setCursorWidth(int width)
{
    Q_D(QQuickTextControl);
#ifdef QT_NO_PROPERTIES
    Q_UNUSED(width);
#else
    if (width == -1)
        width = textCursorWidth;
    d->doc->documentLayout()->setProperty("cursorWidth", width);
#endif
    d->repaintCursor();
}

bool QQuickTextControl::acceptRichText() const
{
    Q_D(const QQuickTextControl);
    return d->acceptRichText;
}

void QQuickTextControl::setAcceptRichText(bool accept)
{
    Q_D(QQuickTextControl);
    d->acceptRichText = accept;
}

void QQuickTextControl::setExtraSelections(const QVector<QAbstractTextDocumentLayout::Selection> &selections)
{
    Q_D(QQuickTextControl);

    QHash<int, int> hash;
    for (int i = 0; i < d->extraSelections.count(); ++i) {
        const QAbstractTextDocumentLayout::Selection &esel = d->extraSelections.at(i);
        hash.insertMulti(esel.cursor.anchor(), i);
    }

    for (int i = 0; i < selections.count(); ++i) {
        const QAbstractTextDocumentLayout::Selection &sel = selections.at(i);
        QHash<int, int>::iterator it = hash.find(sel.cursor.anchor());
        if (it != hash.end()) {
            const QAbstractTextDocumentLayout::Selection &esel = d->extraSelections.at(it.value());
            if (esel.cursor.position() == sel.cursor.position()
                && esel.format == sel.format) {
                hash.erase(it);
                continue;
            }
        }
        QRectF r = selectionRect(sel.cursor);
        if (sel.format.boolProperty(QTextFormat::FullWidthSelection)) {
            r.setLeft(0);
            r.setWidth(qreal(INT_MAX));
        }
        emit updateRequest(r);
    }

    for (QHash<int, int>::iterator it = hash.begin(); it != hash.end(); ++it) {
        const QAbstractTextDocumentLayout::Selection &esel = d->extraSelections.at(it.value());
        QRectF r = selectionRect(esel.cursor);
        if (esel.format.boolProperty(QTextFormat::FullWidthSelection)) {
            r.setLeft(0);
            r.setWidth(qreal(INT_MAX));
        }
        emit updateRequest(r);
    }

    d->extraSelections = selections;
}

QVector<QAbstractTextDocumentLayout::Selection> QQuickTextControl::extraSelections() const
{
    Q_D(const QQuickTextControl);
    return d->extraSelections;
}

void QQuickTextControl::setTextWidth(qreal width)
{
    Q_D(QQuickTextControl);
    d->doc->setTextWidth(width);
}

qreal QQuickTextControl::textWidth() const
{
    Q_D(const QQuickTextControl);
    return d->doc->textWidth();
}

QSizeF QQuickTextControl::size() const
{
    Q_D(const QQuickTextControl);
    return d->doc->size();
}

void QQuickTextControl::setOpenExternalLinks(bool open)
{
    Q_D(QQuickTextControl);
    d->openExternalLinks = open;
}

bool QQuickTextControl::openExternalLinks() const
{
    Q_D(const QQuickTextControl);
    return d->openExternalLinks;
}

bool QQuickTextControl::ignoreUnusedNavigationEvents() const
{
    Q_D(const QQuickTextControl);
    return d->ignoreUnusedNavigationEvents;
}

void QQuickTextControl::setIgnoreUnusedNavigationEvents(bool ignore)
{
    Q_D(QQuickTextControl);
    d->ignoreUnusedNavigationEvents = ignore;
}

void QQuickTextControl::moveCursor(QTextCursor::MoveOperation op, QTextCursor::MoveMode mode)
{
    Q_D(QQuickTextControl);
    const QTextCursor oldSelection = d->cursor;
    const bool moved = d->cursor.movePosition(op, mode);
    d->_q_updateCurrentCharFormatAndSelection();
    ensureCursorVisible();
    d->repaintOldAndNewSelection(oldSelection);
    if (moved)
        emit cursorPositionChanged();
}

bool QQuickTextControl::canPaste() const
{
#ifndef QT_NO_CLIPBOARD
    Q_D(const QQuickTextControl);
    if (d->interactionFlags & Qt::TextEditable) {
        const QMimeData *md = QGuiApplication::clipboard()->mimeData();
        return md && canInsertFromMimeData(md);
    }
#endif
    return false;
}

void QQuickTextControl::setCursorIsFocusIndicator(bool b)
{
    Q_D(QQuickTextControl);
    d->cursorIsFocusIndicator = b;
    d->repaintCursor();
}

bool QQuickTextControl::cursorIsFocusIndicator() const
{
    Q_D(const QQuickTextControl);
    return d->cursorIsFocusIndicator;
}


void QQuickTextControl::setDragEnabled(bool enabled)
{
    Q_D(QQuickTextControl);
    d->dragEnabled = enabled;
}

bool QQuickTextControl::isDragEnabled() const
{
    Q_D(const QQuickTextControl);
    return d->dragEnabled;
}

void QQuickTextControl::setWordSelectionEnabled(bool enabled)
{
    Q_D(QQuickTextControl);
    d->wordSelectionEnabled = enabled;
}

bool QQuickTextControl::isWordSelectionEnabled() const
{
    Q_D(const QQuickTextControl);
    return d->wordSelectionEnabled;
}

QMimeData *QQuickTextControl::createMimeDataFromSelection() const
{
    Q_D(const QQuickTextControl);
    const QTextDocumentFragment fragment(d->cursor);
    return new QQuickTextEditMimeData(fragment);
}

bool QQuickTextControl::canInsertFromMimeData(const QMimeData *source) const
{
    Q_D(const QQuickTextControl);
    if (d->acceptRichText)
        return (source->hasText() && !source->text().isEmpty())
            || source->hasHtml()
            || source->hasFormat(QLatin1String("application/x-qrichtext"))
            || source->hasFormat(QLatin1String("application/x-qt-richtext"));
    else
        return source->hasText() && !source->text().isEmpty();
}

void QQuickTextControl::insertFromMimeData(const QMimeData *source)
{
    Q_D(QQuickTextControl);
    if (!(d->interactionFlags & Qt::TextEditable) || !source)
        return;

    bool hasData = false;
    QTextDocumentFragment fragment;
#ifndef QT_NO_TEXTHTMLPARSER
    if (source->hasFormat(QLatin1String("application/x-qrichtext")) && d->acceptRichText) {
        // x-qrichtext is always UTF-8 (taken from Qt3 since we don't use it anymore).
        QString richtext = QString::fromUtf8(source->data(QLatin1String("application/x-qrichtext")));
        richtext.prepend(QLatin1String("<meta name=\"qrichtext\" content=\"1\" />"));
        fragment = QTextDocumentFragment::fromHtml(richtext, d->doc);
        hasData = true;
    } else if (source->hasHtml() && d->acceptRichText) {
        fragment = QTextDocumentFragment::fromHtml(source->html(), d->doc);
        hasData = true;
    } else {
        QString text = source->text();
        if (!text.isNull()) {
            fragment = QTextDocumentFragment::fromPlainText(text);
            hasData = true;
        }
    }
#else
    fragment = QTextDocumentFragment::fromPlainText(source->text());
#endif // QT_NO_TEXTHTMLPARSER

    if (hasData)
        d->cursor.insertFragment(fragment);
    ensureCursorVisible();
}

bool QQuickTextControl::findNextPrevAnchor(const QTextCursor &startCursor, bool next, QTextCursor &newAnchor)
{
    Q_D(QQuickTextControl);

    int anchorStart = -1;
    QString anchorHref;
    int anchorEnd = -1;

    if (next) {
        const int startPos = startCursor.selectionEnd();

        QTextBlock block = d->doc->findBlock(startPos);
        QTextBlock::Iterator it = block.begin();

        while (!it.atEnd() && it.fragment().position() < startPos)
            ++it;

        while (block.isValid()) {
            anchorStart = -1;

            // find next anchor
            for (; !it.atEnd(); ++it) {
                const QTextFragment fragment = it.fragment();
                const QTextCharFormat fmt = fragment.charFormat();

                if (fmt.isAnchor() && fmt.hasProperty(QTextFormat::AnchorHref)) {
                    anchorStart = fragment.position();
                    anchorHref = fmt.anchorHref();
                    break;
                }
            }

            if (anchorStart != -1) {
                anchorEnd = -1;

                // find next non-anchor fragment
                for (; !it.atEnd(); ++it) {
                    const QTextFragment fragment = it.fragment();
                    const QTextCharFormat fmt = fragment.charFormat();

                    if (!fmt.isAnchor() || fmt.anchorHref() != anchorHref) {
                        anchorEnd = fragment.position();
                        break;
                    }
                }

                if (anchorEnd == -1)
                    anchorEnd = block.position() + block.length() - 1;

                // make found selection
                break;
            }

            block = block.next();
            it = block.begin();
        }
    } else {
        int startPos = startCursor.selectionStart();
        if (startPos > 0)
            --startPos;

        QTextBlock block = d->doc->findBlock(startPos);
        QTextBlock::Iterator blockStart = block.begin();
        QTextBlock::Iterator it = block.end();

        if (startPos == block.position()) {
            it = block.begin();
        } else {
            do {
                if (it == blockStart) {
                    it = QTextBlock::Iterator();
                    block = QTextBlock();
                } else {
                    --it;
                }
            } while (!it.atEnd() && it.fragment().position() + it.fragment().length() - 1 > startPos);
        }

        while (block.isValid()) {
            anchorStart = -1;

            if (!it.atEnd()) {
                do {
                    const QTextFragment fragment = it.fragment();
                    const QTextCharFormat fmt = fragment.charFormat();

                    if (fmt.isAnchor() && fmt.hasProperty(QTextFormat::AnchorHref)) {
                        anchorStart = fragment.position() + fragment.length();
                        anchorHref = fmt.anchorHref();
                        break;
                    }

                    if (it == blockStart)
                        it = QTextBlock::Iterator();
                    else
                        --it;
                } while (!it.atEnd());
            }

            if (anchorStart != -1 && !it.atEnd()) {
                anchorEnd = -1;

                do {
                    const QTextFragment fragment = it.fragment();
                    const QTextCharFormat fmt = fragment.charFormat();

                    if (!fmt.isAnchor() || fmt.anchorHref() != anchorHref) {
                        anchorEnd = fragment.position() + fragment.length();
                        break;
                    }

                    if (it == blockStart)
                        it = QTextBlock::Iterator();
                    else
                        --it;
                } while (!it.atEnd());

                if (anchorEnd == -1)
                    anchorEnd = qMax(0, block.position());

                break;
            }

            block = block.previous();
            it = block.end();
            if (it != block.begin())
                --it;
            blockStart = block.begin();
        }

    }

    if (anchorStart != -1 && anchorEnd != -1) {
        newAnchor = d->cursor;
        newAnchor.setPosition(anchorStart);
        newAnchor.setPosition(anchorEnd, QTextCursor::KeepAnchor);
        return true;
    }

    return false;
}

void QQuickTextControlPrivate::activateLinkUnderCursor(QString href)
{
    QTextCursor oldCursor = cursor;

    if (href.isEmpty()) {
        QTextCursor tmp = cursor;
        if (tmp.selectionStart() != tmp.position())
            tmp.setPosition(tmp.selectionStart());
        tmp.movePosition(QTextCursor::NextCharacter);
        href = tmp.charFormat().anchorHref();
    }
    if (href.isEmpty())
        return;

    if (!cursor.hasSelection()) {
        QTextBlock block = cursor.block();
        const int cursorPos = cursor.position();

        QTextBlock::Iterator it = block.begin();
        QTextBlock::Iterator linkFragment;

        for (; !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();
            const int fragmentPos = fragment.position();
            if (fragmentPos <= cursorPos &&
                fragmentPos + fragment.length() > cursorPos) {
                linkFragment = it;
                break;
            }
        }

        if (!linkFragment.atEnd()) {
            it = linkFragment;
            cursor.setPosition(it.fragment().position());
            if (it != block.begin()) {
                do {
                    --it;
                    QTextFragment fragment = it.fragment();
                    if (fragment.charFormat().anchorHref() != href)
                        break;
                    cursor.setPosition(fragment.position());
                } while (it != block.begin());
            }

            for (it = linkFragment; !it.atEnd(); ++it) {
                QTextFragment fragment = it.fragment();
                if (fragment.charFormat().anchorHref() != href)
                    break;
                cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
            }
        }
    }

    if (hasFocus) {
        cursorIsFocusIndicator = true;
    } else {
        cursorIsFocusIndicator = false;
        cursor.clearSelection();
    }
    repaintOldAndNewSelection(oldCursor);

#if 0 // ###ndef QT_NO_DESKTOPSERVICES
    if (openExternalLinks)
        QDesktopServices::openUrl(href);
    else
#endif
        emit q_func()->linkActivated(href);
}

bool QQuickTextControl::setFocusToNextOrPreviousAnchor(bool next)
{
    Q_D(QQuickTextControl);

    if (!(d->interactionFlags & Qt::LinksAccessibleByKeyboard))
        return false;

    QRectF crect = selectionRect();
    emit updateRequest(crect);

    // If we don't have a current anchor, we start from the start/end
    if (!d->cursor.hasSelection()) {
        d->cursor = QTextCursor(d->doc);
        if (next)
            d->cursor.movePosition(QTextCursor::Start);
        else
            d->cursor.movePosition(QTextCursor::End);
    }

    QTextCursor newAnchor;
    if (findNextPrevAnchor(d->cursor, next, newAnchor)) {
        d->cursor = newAnchor;
        d->cursorIsFocusIndicator = true;
    } else {
        d->cursor.clearSelection();
    }

    if (d->cursor.hasSelection()) {
        crect = selectionRect();
        emit updateRequest(crect);
        emit visibilityRequest(crect);
        return true;
    } else {
        return false;
    }
}

bool QQuickTextControl::setFocusToAnchor(const QTextCursor &newCursor)
{
    Q_D(QQuickTextControl);

    if (!(d->interactionFlags & Qt::LinksAccessibleByKeyboard))
        return false;

    // Verify that this is an anchor.
    const QString anchorHref = d->anchorForCursor(newCursor);
    if (anchorHref.isEmpty())
        return false;

    // and process it
    QRectF crect = selectionRect();
    emit updateRequest(crect);

    d->cursor.setPosition(newCursor.selectionStart());
    d->cursor.setPosition(newCursor.selectionEnd(), QTextCursor::KeepAnchor);
    d->cursorIsFocusIndicator = true;

    crect = selectionRect();
    emit updateRequest(crect);
    emit visibilityRequest(crect);
    return true;
}

void QQuickTextControl::setTextInteractionFlags(Qt::TextInteractionFlags flags)
{
    Q_D(QQuickTextControl);
    if (flags == d->interactionFlags)
        return;
    d->interactionFlags = flags;

    if (d->hasFocus)
        d->setBlinkingCursorEnabled(flags & (Qt::TextEditable | Qt::TextSelectableByKeyboard));
}

Qt::TextInteractionFlags QQuickTextControl::textInteractionFlags() const
{
    Q_D(const QQuickTextControl);
    return d->interactionFlags;
}

void QQuickTextControl::mergeCurrentCharFormat(const QTextCharFormat &modifier)
{
    Q_D(QQuickTextControl);
    d->cursor.mergeCharFormat(modifier);
    d->updateCurrentCharFormat();
}

void QQuickTextControl::setCurrentCharFormat(const QTextCharFormat &format)
{
    Q_D(QQuickTextControl);
    d->cursor.setCharFormat(format);
    d->updateCurrentCharFormat();
}

QTextCharFormat QQuickTextControl::currentCharFormat() const
{
    Q_D(const QQuickTextControl);
    return d->cursor.charFormat();
}

void QQuickTextControl::insertPlainText(const QString &text)
{
    Q_D(QQuickTextControl);
    d->cursor.insertText(text);
}

#ifndef QT_NO_TEXTHTMLPARSER
void QQuickTextControl::insertHtml(const QString &text)
{
    Q_D(QQuickTextControl);
    d->cursor.insertHtml(text);
}
#endif // QT_NO_TEXTHTMLPARSER

QPointF QQuickTextControl::anchorPosition(const QString &name) const
{
    Q_D(const QQuickTextControl);
    if (name.isEmpty())
        return QPointF();

    QRectF r;
    for (QTextBlock block = d->doc->begin(); block.isValid(); block = block.next()) {
        QTextCharFormat format = block.charFormat();
        if (format.isAnchor() && format.anchorNames().contains(name)) {
            r = d->rectForPosition(block.position());
            break;
        }

        for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();
            format = fragment.charFormat();
            if (format.isAnchor() && format.anchorNames().contains(name)) {
                r = d->rectForPosition(fragment.position());
                block = QTextBlock();
                break;
            }
        }
    }
    if (!r.isValid())
        return QPointF();
    return QPointF(0, r.top());
}

void QQuickTextControl::adjustSize()
{
    Q_D(QQuickTextControl);
    d->doc->adjustSize();
}

bool QQuickTextControl::find(const QString &exp, QTextDocument::FindFlags options)
{
    Q_D(QQuickTextControl);
    QTextCursor search = d->doc->find(exp, d->cursor, options);
    if (search.isNull())
        return false;

    setTextCursor(search);
    return true;
}



void QQuickTextControlPrivate::append(const QString &text, Qt::TextFormat format)
{
    QTextCursor tmp(doc);
    tmp.beginEditBlock();
    tmp.movePosition(QTextCursor::End);

    if (!doc->isEmpty())
        tmp.insertBlock(cursor.blockFormat(), cursor.charFormat());
    else
        tmp.setCharFormat(cursor.charFormat());

    // preserve the char format
    QTextCharFormat oldCharFormat = cursor.charFormat();

#ifndef QT_NO_TEXTHTMLPARSER
    if (format == Qt::RichText || (format == Qt::AutoText && Qt::mightBeRichText(text))) {
        tmp.insertHtml(text);
    } else {
        tmp.insertText(text);
    }
#else
    tmp.insertText(text);
#endif // QT_NO_TEXTHTMLPARSER
    if (!cursor.hasSelection())
        cursor.setCharFormat(oldCharFormat);

    tmp.endEditBlock();
}

void QQuickTextControl::append(const QString &text)
{
    Q_D(QQuickTextControl);
    d->append(text, Qt::AutoText);
}

void QQuickTextControl::appendHtml(const QString &html)
{
    Q_D(QQuickTextControl);
    d->append(html, Qt::RichText);
}

void QQuickTextControl::appendPlainText(const QString &text)
{
    Q_D(QQuickTextControl);
    d->append(text, Qt::PlainText);
}


void QQuickTextControl::ensureCursorVisible()
{
    Q_D(QQuickTextControl);
    QRectF crect = d->rectForPosition(d->cursor.position()).adjusted(-5, 0, 5, 0);
    emit visibilityRequest(crect);
    emit microFocusChanged();
}

QPalette QQuickTextControl::palette() const
{
    Q_D(const QQuickTextControl);
    return d->palette;
}

void QQuickTextControl::setPalette(const QPalette &pal)
{
    Q_D(QQuickTextControl);
    d->palette = pal;
}

bool QQuickTextControl::cursorOn() const
{
    Q_D(const QQuickTextControl);
    return d->cursorOn;
}

QAbstractTextDocumentLayout::PaintContext QQuickTextControl::getPaintContext() const
{
    Q_D(const QQuickTextControl);

    QAbstractTextDocumentLayout::PaintContext ctx;

    ctx.selections = d->extraSelections;
    ctx.palette = d->palette;
    if (d->cursorOn && d->isEnabled) {
        if (d->hideCursor)
            ctx.cursorPosition = -1;
        else if (d->preeditCursor != 0)
            ctx.cursorPosition = - (d->preeditCursor + 2);
        else
            ctx.cursorPosition = d->cursor.position();
    }

    if (!d->dndFeedbackCursor.isNull())
        ctx.cursorPosition = d->dndFeedbackCursor.position();
#ifdef QT_KEYPAD_NAVIGATION
    if (!QGuiApplication::keypadNavigationEnabled() || d->hasEditFocus)
#endif
    if (d->cursor.hasSelection()) {
        QAbstractTextDocumentLayout::Selection selection;
        selection.cursor = d->cursor;
        if (0 && d->cursorIsFocusIndicator) {
#if 0
            // ###
            QStyleOption opt;
            opt.palette = ctx.palette;
            QStyleHintReturnVariant ret;
            QStyle *style = QGuiApplication::style();
            if (widget)
                style = widget->style();
            style->styleHint(QStyle::SH_TextControl_FocusIndicatorTextCharFormat, &opt, widget, &ret);
            selection.format = qvariant_cast<QTextFormat>(ret.variant).toCharFormat();
#endif
        } else {
            QPalette::ColorGroup cg = d->hasFocus ? QPalette::Active : QPalette::Inactive;
            selection.format.setBackground(ctx.palette.brush(cg, QPalette::Highlight));
            selection.format.setForeground(ctx.palette.brush(cg, QPalette::HighlightedText));
            if (fullWidthSelection)
                selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        }
        ctx.selections.append(selection);
    }

    return ctx;
}

void QQuickTextControl::drawContents(QPainter *p, const QRectF &rect)
{
    Q_D(QQuickTextControl);
    p->save();
    QAbstractTextDocumentLayout::PaintContext ctx = getPaintContext();
    if (rect.isValid())
        p->setClipRect(rect, Qt::IntersectClip);
    ctx.clip = rect;

    d->doc->documentLayout()->draw(p, ctx);
    p->restore();
}

void QQuickTextControlPrivate::_q_copyLink()
{
#ifndef QT_NO_CLIPBOARD
    QMimeData *md = new QMimeData;
    md->setText(linkToCopy);
    QGuiApplication::clipboard()->setMimeData(md);
#endif
}

QInputContext *QQuickTextControlPrivate::inputContext()
{
#if 0
    // ###
    QInputContext *ctx = contextObject->inputContext();
    if (!ctx && contextObject->parentWidget())
        ctx = contextObject->parentWidget()->inputContext();
    return ctx;
#else
    return 0;
#endif
}

int QQuickTextControl::hitTest(const QPointF &point, Qt::HitTestAccuracy accuracy) const
{
    Q_D(const QQuickTextControl);
    return d->doc->documentLayout()->hitTest(point, accuracy);
}

QRectF QQuickTextControl::blockBoundingRect(const QTextBlock &block) const
{
    Q_D(const QQuickTextControl);
    return d->doc->documentLayout()->blockBoundingRect(block);
}



QStringList QQuickTextEditMimeData::formats() const
{
    if (!fragment.isEmpty())
        return QStringList() << QString::fromLatin1("text/plain") << QString::fromLatin1("text/html")
#ifndef QT_NO_TEXTODFWRITER
            << QString::fromLatin1("application/vnd.oasis.opendocument.text")
#endif
        ;
    else
        return QMimeData::formats();
}

QVariant QQuickTextEditMimeData::retrieveData(const QString &mimeType, QVariant::Type type) const
{
    if (!fragment.isEmpty())
        setup();
    return QMimeData::retrieveData(mimeType, type);
}

void QQuickTextEditMimeData::setup() const
{
    QQuickTextEditMimeData *that = const_cast<QQuickTextEditMimeData *>(this);
#ifndef QT_NO_TEXTHTMLPARSER
    that->setData(QLatin1String("text/html"), fragment.toHtml("utf-8").toUtf8());
#endif
#ifndef QT_NO_TEXTODFWRITER
    {
        QBuffer buffer;
        QTextDocumentWriter writer(&buffer, "ODF");
        writer.write(fragment);
        buffer.close();
        that->setData(QLatin1String("application/vnd.oasis.opendocument.text"), buffer.data());
    }
#endif
    that->setText(fragment.toPlainText());
    fragment = QTextDocumentFragment();
}


QT_END_NAMESPACE

#include "moc_qquicktextcontrol_p.cpp"

#endif // QT_NO_TEXTCONTROL
