/*
*
* Copyright (C) 2011 EgorZhuk
*
* Authors: Egor Zhuk <egor.zhuk@gmail.com>
*
* This file is part of CoolScroll plugin for QtCreator.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
* LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
* OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*/

#include "coolscrollbar.h"
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextLayout>
#include <QTextDocumentFragment>

#include <QtGui/QPainter>
#include <QApplication>
#include <QDebug>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/textdocumentlayout.h>

#include "coolscrollbarsettings.h"
#include <QTime>

namespace
{
    quint32 getLineNumberOfTextCursor(QTextCursor cursor)
    {
        cursor.movePosition(QTextCursor::StartOfLine);

        int lines = 1;
        while(cursor.positionInBlock()>0) {
            cursor.movePosition(QTextCursor::Up);
            lines++;
        }
        QTextBlock block = cursor.block().previous();

        while(block.isValid()) {
            lines += block.lineCount();
            block = block.previous();
        }
        return lines;
    }

    const qreal l_maxLineHeight = 2.0;
    const quint32 l_maxSymbolsPerLine = 100;
    const QString l_sampleString = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

}

CoolScrollBar::CoolScrollBar(TextEditor::TextEditorWidget *edit,
                             QSharedPointer<CoolScrollbarSettings>& settings) :
    m_parentEdit(edit),
    m_settings(settings),
    m_yAdditionalScale(1.0),
    m_highlightNextSelection(false),
    m_leftButtonPressed(false),
    m_renderData(nullptr)
{
}

CoolScrollBar::~CoolScrollBar()
{
    if (m_renderData)
        delete m_renderData;
}

void CoolScrollBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (!m_renderData) return;

    QPainter painter(this);

    QTime time = QTime::currentTime();
    // draw document picture
    painter.fillRect(rect(), Qt::white);
    drawDocumentPreview(painter, *m_parentEdit->textDocument());

    // draw selections
    painter.setBrush(settings().selectionHighlightColor);
    painter.setPen(Qt::NoPen);
    painter.drawRects(m_renderData->selectedAreas);

    // draw viewport rect
    qreal lineHeight = calculateLineHeight();
    QPointF rectPos(0, static_cast<qreal>(value() - 1) * lineHeight); // TODO: -1 is a magic number, check why drawContents is shifted
    QRectF rect(rectPos, QSizeF(settings().scrollBarWidth / getXScale(),
                                static_cast<qreal>(linesInViewportCount()) * lineHeight));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(settings().viewportColor));
    painter.drawRect(rect);

    painter.end();
    qDebug() << "render time = " << -QTime::currentTime().msecsTo(time);
}

int CoolScrollBar::unfoldedLinesCount() const
{
    Q_ASSERT(m_parentEdit);
    int res = 0;
    QTextBlock b = originalDocument().firstBlock();
    while(b != originalDocument().lastBlock())
    {
        if(b.isVisible())
        {
            res += b.lineCount();
        }
        b = b.next();
    }
    qDebug() << "UNFOLDED LINES = " << res;

    return res;
}

int CoolScrollBar::linesInViewportCount() const
{
    return pageStep();
}

QSize CoolScrollBar::sizeHint() const
{
    return QSize(settings().scrollBarWidth, 0);
}

QSize CoolScrollBar::minimumSizeHint() const
{
    return QSize(settings().scrollBarWidth, 0);
}

const QTextDocument& CoolScrollBar::originalDocument() const
{
    return *m_parentEdit->document();
}

void CoolScrollBar::documentContentChanged()
{
    update();
}

void CoolScrollBar::documentSelectionChanged()
{
    if(m_highlightNextSelection)
    {
        QString selectedStr = m_parentEdit->textCursor().selection().toPlainText();
        // clear previous highlight
        clearHighlight();
        m_stringToHighlight = selectedStr;
        // highlight new selection
        highlightEntryInDocument(m_stringToHighlight);

        update();
    }
}

void CoolScrollBar::documentSizeChanged(const QSizeF)
{
    qDebug() << __PRETTY_FUNCTION__;
    if (!m_renderData) return;

    if (hasHighlight())
    {
        m_renderData->selectedAreas.clear();
        highlightEntryInDocument(m_stringToHighlight);
        update();
    }
}

bool CoolScrollBar::eventFilter(QObject *obj, QEvent *e)
{
    if(obj == m_parentEdit->viewport())
    {
        m_highlightNextSelection = (e->type() == QEvent::MouseButtonDblClick);
    }
    return false;
}

void CoolScrollBar::drawDocumentPreview(QPainter &p, const TextEditor::TextDocument& document)
{
    if (!m_renderData) return;

    QTextBlock block = document.document()->firstBlock();
    qreal yPos = 0;
    qreal lineHeight = calculateLineHeight();
    qDebug() << "LH = " << lineHeight;
    m_renderData->font.setPointSizeF(lineHeight);
    QFontMetricsF fm(m_renderData->font);

    if (fm.width(l_sampleString) < width())
    {
        m_renderData->font.setStretch(100 * width() / fm.width(l_sampleString));
    }
    p.setFont(m_renderData->font);

    while (block.isValid())
    {
        if (block.isVisible())
        {
            p.drawText(QPointF(0.0, yPos), block.text());
            yPos += lineHeight;
        }
        block = block.next();
    }
}

qreal CoolScrollBar::getXScale() const
{
    return settings().xDefaultScale;
}

qreal CoolScrollBar::calculateLineHeight() const
{
    qreal lineHeight = static_cast<float>(height()) / unfoldedLinesCount();
    if (lineHeight > l_maxLineHeight)
    {
        lineHeight = l_maxLineHeight;
    }
    return lineHeight;
}

void CoolScrollBar::highlightEntryInDocument(const QString& stringToHighlight)
{
    if(stringToHighlight.isEmpty() || !m_renderData)
    {
        return;
    }
    m_renderData->selectedAreas.clear();
    m_renderData->selectedAreas.reserve(50);
    auto document = m_parentEdit->document();
    QTextCursor cur_cursor(document);
    int numIter = 0;
    while(true)
    {
        if (cur_cursor.isNull())
            break;

        cur_cursor = document->find(stringToHighlight, cur_cursor);
        if(!cur_cursor.isNull())
        {
            QRectF selectionRect;
            // calculate bounding rect for selected word
            int blockPos = cur_cursor.block().position();

            auto lineNumber = getLineNumberOfTextCursor(cur_cursor);

            QFontMetricsF fm(m_renderData->font);
            qreal left = fm.width(cur_cursor.block().text().mid(0, cur_cursor.selectionStart() - blockPos));
            if (left > settings().scrollBarWidth)
            {
                left = settings().scrollBarWidth - fm.width(m_stringToHighlight);
            }
            selectionRect.setLeft(left);
            selectionRect.setWidth(fm.width(m_stringToHighlight));

            auto lineHeight = calculateLineHeight();
            selectionRect.setTop(lineHeight * (lineNumber - 2));
            // apply minimum selection height for good visibility in large files
            if(lineHeight < settings().m_minSelectionHeight)
            {
                lineHeight = settings().m_minSelectionHeight;
            }
            selectionRect.setHeight(lineHeight);


            qDebug() << "rect = " << selectionRect;
            m_renderData->selectedAreas.push_back(selectionRect);
        }
        // prevents UI freezing
        ++numIter;
        if(numIter % 15 == 0)
        {
            qApp->processEvents();
        }
    }
    qDebug() << "m_renderData->selectedAreas.size() = " << m_renderData->selectedAreas.size();
}

void CoolScrollBar::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        setValue(posToScrollValue(event->pos().y()));
        m_leftButtonPressed = true;
    }
    else if(event->button() == Qt::RightButton)
    {
        clearHighlight();
        update();
    }
}

void CoolScrollBar::contextMenuEvent(QContextMenuEvent *event)
{
    if(!settings().disableContextMenu)
    {
        QScrollBar::contextMenuEvent(event);
    }
}

void CoolScrollBar::mouseMoveEvent(QMouseEvent *event)
{
    if(m_leftButtonPressed)
    {
        setValue(posToScrollValue(event->pos().y()));
    }
}

bool CoolScrollBar::hasHighlight() const
{
    return m_renderData && !m_renderData->selectedAreas.empty();
}

void CoolScrollBar::resizeEvent(QResizeEvent *)
{
}

int CoolScrollBar::posToScrollValue(qreal pos) const
{
    if (!m_renderData) return 0;

    qreal documentHeight = calculateLineHeight() * unfoldedLinesCount();//internalDocument()->lineCount() * calculateLineHeight() * getYScale();
    int value = int(pos * (maximum() + linesInViewportCount()) / documentHeight);

    // set center of a viewport to position of click
    value -= linesInViewportCount() / 2;
    if(value > maximum())
    {
        value = maximum();
    }
    else if (value < minimum())
    {
        value = minimum();
    }
    return value;
}

void CoolScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        m_leftButtonPressed = false;
    }
}


void CoolScrollBar::clearHighlight()
{
    if (!m_renderData) return;

    m_renderData->selectedAreas.clear();
    m_stringToHighlight.clear();
}

void CoolScrollBar::applySettings()
{
    if (!m_renderData) return;

    resize(settings().scrollBarWidth, height());
    updateGeometry();
}


void CoolScrollBar::activate()
{
    if (m_renderData != nullptr)
    {
        qDebug() << "Already active";
        return;
    }

    m_renderData = new CoolScrallBarRenderData();

    applySettings();
    update();

    m_parentEdit->viewport()->installEventFilter(this);
    connect(m_parentEdit, SIGNAL(textChanged()),      SLOT(documentContentChanged()));
    connect(m_parentEdit, SIGNAL(selectionChanged()), SLOT(documentSelectionChanged()));
    connect(m_parentEdit->document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
                                                  this, &CoolScrollBar::documentSizeChanged);
}

void CoolScrollBar::deactivate()
{
    if (m_renderData)
        delete m_renderData;

    m_renderData = nullptr;

    m_parentEdit->viewport()->removeEventFilter(this);
    disconnect(m_parentEdit, 0, this, 0);
    disconnect(m_parentEdit->document()->documentLayout(), 0, this, 0);
}

CoolScrollBar::CoolScrallBarRenderData::CoolScrallBarRenderData()
{
    font.setPointSizeF(l_maxLineHeight);
    font.setStyleHint(QFont::Monospace);
}
