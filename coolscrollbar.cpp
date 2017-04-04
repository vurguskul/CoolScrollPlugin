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
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>

#include "coolscrollbarsettings.h"

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

    QPainter p(this);

    // draw document picture
    p.drawPixmap(0, 0, width(), height(), m_renderData->contentPixmap);
    p.scale(getXScale(), getYScale());

    // draw selections
    p.setBrush(settings().selectionHighlightColor);
    p.setPen(Qt::NoPen);
    p.drawRects(m_renderData->selectedAreas);

    // draw viewport rect
    qreal lineHeight = calculateLineHeight();
    QPointF rectPos(0, value() * lineHeight);
    QRectF rect(rectPos, QSizeF(settings().scrollBarWidth / getXScale(),
                                linesInViewportCount() * lineHeight));

    p.setPen(Qt::NoPen);
    p.setBrush(QBrush(settings().viewportColor));
    p.drawRect(rect);

    p.end();
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
    return res;
}

int CoolScrollBar::linesInViewportCount() const
{
    return (2 * originalDocument().lineCount() - unfoldedLinesCount() - maximum());
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
    internalDocument()->setPlainText(originalDocument().toPlainText());

// TODO: make optional
//    QTextBlock origBlock = originalDocument().firstBlock();
//    QTextBlock internBlock = internalDocument().firstBlock();
//    while(origBlock.isValid() && internBlock.isValid())
//    {
//        internBlock.layout()->setAdditionalFormats(origBlock.layout()->additionalFormats());
//        origBlock = origBlock.next();
//        internBlock = internBlock.next();
//    }
 //   updatePicture();
   // update();
}

void CoolScrollBar::documentSelectionChanged()
{
    if(m_highlightNextSelection)
    {
        QString selectedStr = m_parentEdit->textCursor().selection().toPlainText();
        if(selectedStr != m_stringToHighlight)
        {
            // clear previous highlight
            clearHighlight();
            m_stringToHighlight = selectedStr;
            // highlight new selection
            highlightEntryInDocument(m_stringToHighlight);

            recreatePixmap();
            update();
        }
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

qreal CoolScrollBar::getXScale() const
{
    return settings().xDefaultScale;
}

qreal CoolScrollBar::getYScale() const
{
    return settings().yDefaultScale * m_yAdditionalScale;
}

QTextDocument *CoolScrollBar::internalDocument()
{
    if (m_renderData && m_renderData->currentDocumentCopy)
        return m_renderData->currentDocumentCopy;
    else
        return nullptr;
}

const QTextDocument *CoolScrollBar::internalDocument() const
{
    if (m_renderData && m_renderData->currentDocumentCopy)
        return m_renderData->currentDocumentCopy;
    else
        return nullptr;
}

qreal CoolScrollBar::calculateLineHeight() const
{
    QFontMetrics fm(settings().m_font);
    return qreal(fm.height());
}

void CoolScrollBar::highlightEntryInDocument(const QString& stringToHighlight)
{
    if(stringToHighlight.isEmpty() || !m_renderData)
    {
        return;
    }
        // TODO: optimize using QVector::reserve()
    m_renderData->selectedAreas.clear();
    QTextCursor cur_cursor(internalDocument());
    int numIter = 0;
    qreal yScale = getYScale();
    while(true)
    {
        cur_cursor = internalDocument()->find(stringToHighlight, cur_cursor);
        if(!cur_cursor.isNull())
        {
            const QTextLayout* layout = cur_cursor.block().layout();
            QTextLine line = layout->lineForTextPosition(cur_cursor.positionInBlock());

            QRectF selectionRect = line.naturalTextRect();
            // calculate bounding rect for selected word
            int blockPos = cur_cursor.block().position();

            qreal leftPercent = qreal(cur_cursor.selectionStart() - blockPos) / line.textLength();
            qreal rightPercent = qreal(cur_cursor.selectionEnd() - blockPos) / line.textLength();

            selectionRect.setLeft(line.naturalTextWidth() * leftPercent);
            selectionRect.setRight(line.naturalTextWidth() * rightPercent);

            selectionRect.translate(layout->position());

            // apply minimum selection height for good visibility on large files
            if((selectionRect.height() * yScale) < settings().m_minSelectionHeight)
            {
                selectionRect.setHeight(settings().m_minSelectionHeight / yScale);
            }

            m_renderData->selectedAreas.push_back(selectionRect);
        }
        else
        {
            break;
        }
        // prevents UI freezing
        ++numIter;
        if(numIter % 15 == 0)
        {
            qApp->processEvents();
        }
    }
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

void CoolScrollBar::recreatePixmap()
{
    if (!m_renderData) return;

    int lineHeight = calculateLineHeight();

    qreal documentHeight = qreal(lineHeight * internalDocument()->lineCount());
    documentHeight *= settings().yDefaultScale;

    if(documentHeight > size().height())
    {
        m_yAdditionalScale = size().height() / documentHeight;
    }

    QPixmap& pixmap = m_renderData->contentPixmap;
    pixmap = QPixmap(width() / getXScale(), height() / getYScale());
    pixmap.fill(Qt::white);
    QPainter pic(&pixmap);
    internalDocument()->drawContents(&pic);
    pic.end();

    // scale pixmap with bilinear filtering
    pixmap = pixmap.scaled(width(), height(), Qt::IgnoreAspectRatio,
                                       Qt::SmoothTransformation);
}

void CoolScrollBar::resizeEvent(QResizeEvent *)
{
    recreatePixmap();
}

int CoolScrollBar::posToScrollValue(qreal pos) const
{
    if (!m_renderData) return 0;

    qreal documentHeight = internalDocument()->lineCount() * calculateLineHeight() * getYScale();
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

    internalDocument()->setDefaultFont(settings().m_font);
    internalDocument()->setDefaultTextOption(settings().m_textOption);

    recreatePixmap();
}


void CoolScrollBar::activate()
{
    if (m_renderData != nullptr)
    {
        qDebug() << "Already active";
        return;
    }

    m_renderData = new CoolScrallBarRenderData();
    m_renderData->currentDocumentCopy = originalDocument().clone();

    applySettings();
    update();

    m_parentEdit->viewport()->installEventFilter(this);
    connect(m_parentEdit, SIGNAL(textChanged()), SLOT(documentContentChanged()));
    connect(m_parentEdit, SIGNAL(selectionChanged()), SLOT(documentSelectionChanged()));
}

void CoolScrollBar::deactivate()
{
    if (m_renderData)
        delete m_renderData;

    m_renderData = nullptr;

    m_parentEdit->viewport()->removeEventFilter(this);
    disconnect(m_parentEdit, 0, this, 0);
}

