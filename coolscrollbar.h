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

#ifndef COOLSCROLLAREA_H
#define COOLSCROLLAREA_H

#include <QScrollBar>
#include <QtGui/QPixmap>
#include <QTextCharFormat>
#include <QTextDocument>

#include <experimental/optional>

namespace TextEditor
{
    class TextEditorWidget;
}

class CoolScrollbarSettings;
class QTextDocument;

class CoolScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    CoolScrollBar(TextEditor::TextEditorWidget* edit,
                  QSharedPointer<CoolScrollbarSettings>& settings);

    ~CoolScrollBar();

    void applySettings();

    void activate();
    void deactivate();

protected:

    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

    inline const CoolScrollbarSettings& settings() const { return *m_settings; }

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    int unfoldedLinesCount() const;
    int linesInViewportCount() const;
    qreal calculateLineHeight() const;

    qreal getXScale() const;
    qreal getYScale() const;

    // original document access
    const QTextDocument& originalDocument() const;

    // access to a copy of original document
    QTextDocument* internalDocument();
    const QTextDocument* internalDocument() const;

    bool eventFilter(QObject *obj, QEvent *e);

protected slots:

    void documentContentChanged();
    void documentSelectionChanged();

private:

    struct CoolScrallBarRenderData
    {
        CoolScrallBarRenderData() : currentDocumentCopy(nullptr) {}
        ~CoolScrallBarRenderData()
        {
            if (currentDocumentCopy)
                delete currentDocumentCopy;
            selectedAreas.clear();
        }
        QPixmap         contentPixmap;
        QVector<QRectF> selectedAreas;
        QTextDocument*  currentDocumentCopy;

    };


    int posToScrollValue(qreal pos) const;

    void clearHighlight();
    void highlightEntryInDocument(const QString& stringToHighlight);
    void recreatePixmap();

    TextEditor::TextEditorWidget* m_parentEdit;
    const QSharedPointer<CoolScrollbarSettings> m_settings;

    qreal m_yAdditionalScale; // this paramter is <1.0 if file is to large

    QString m_stringToHighlight;

    bool m_highlightNextSelection;
    bool m_leftButtonPressed;

    CoolScrallBarRenderData* m_renderData;

};

#endif // COOLSCROLLAREA_H
