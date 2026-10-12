#include "markdown/MarkdownRender.h"

#include <QFontDatabase>
#include <QList>
#include <QPair>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextTable>

namespace Markdown {

namespace {

// Moves a colour towards white (dark backgrounds) or black (light backgrounds).
QColor shade(const QColor &background, qreal amount)
{
    const QColor target = background.lightness() < 128 ? QColor(Qt::white) : QColor(Qt::black);
    auto mix = [amount](float from, float to) { return from + (to - from) * float(amount); };
    return QColor::fromRgbF(mix(background.redF(), target.redF()),
                            mix(background.greenF(), target.greenF()),
                            mix(background.blueF(), target.blueF()));
}

bool isMonospace(const QTextCharFormat &format)
{
    static const QString monoFamily = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    const QFont font = format.font();
    return format.fontFixedPitch() || font.family() == monoFamily || font.families().contains(monoFamily);
}

bool isCodeBlock(const QTextBlock &block)
{
    const QTextBlockFormat format = block.blockFormat();
    if (format.hasProperty(QTextFormat::BlockCodeFence) || format.hasProperty(QTextFormat::BlockCodeLanguage))
        return true;
    // Indented code blocks carry no marker, only a monospace font.
    if (block.length() <= 1)
        return false;
    for (auto it = block.begin(); !it.atEnd(); ++it) {
        if (!isMonospace(it.fragment().charFormat()))
            return false;
    }
    return true;
}

void styleCode(QTextDocument *doc, const QColor &codeBackground)
{
    QTextCursor cursor(doc);
    QList<QPair<int, int>> inlineCode; // collected first: formatting splits fragments

    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        if (isCodeBlock(block)) {
            QTextBlockFormat format = block.blockFormat();
            format.setBackground(codeBackground);
            cursor.setPosition(block.position());
            cursor.setBlockFormat(format);
            continue;
        }
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (isMonospace(fragment.charFormat()))
                inlineCode.append({fragment.position(), fragment.length()});
        }
    }

    QTextCharFormat background;
    background.setBackground(codeBackground);
    for (const auto &range : inlineCode) {
        cursor.setPosition(range.first);
        cursor.setPosition(range.first + range.second, QTextCursor::KeepAnchor);
        cursor.mergeCharFormat(background);
    }
}

void styleTables(QTextFrame *frame, const QColor &border, const QColor &headerBackground)
{
    const QList<QTextFrame *> children = frame->childFrames();
    for (QTextFrame *child : children) {
        if (auto *table = qobject_cast<QTextTable *>(child)) {
            QTextTableFormat format = table->format();
            format.setBorder(1);
            format.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
            format.setBorderBrush(border);
            format.setBorderCollapse(true);
            format.setCellSpacing(0);
            format.setCellPadding(6);
            table->setFormat(format);

            // GitHub tables always have a header row.
            QTextCharFormat bold;
            bold.setFontWeight(QFont::Bold);
            for (int column = 0; column < table->columns(); ++column) {
                QTextTableCell cell = table->cellAt(0, column);
                QTextCharFormat cellFormat = cell.format();
                cellFormat.setBackground(headerBackground);
                cell.setFormat(cellFormat);
                QTextCursor cursor = cell.firstCursorPosition();
                cursor.setPosition(cell.lastCursorPosition().position(), QTextCursor::KeepAnchor);
                cursor.mergeCharFormat(bold);
            }
        }
        styleTables(child, border, headerBackground);
    }
}

} // namespace

void render(QTextDocument *doc, const QString &markdown, const QColor &background)
{
    doc->setMarkdown(markdown, QTextDocument::MarkdownDialectGitHub);
    doc->setDocumentMargin(16);
    styleCode(doc, shade(background, 0.07));
    styleTables(doc->rootFrame(), shade(background, 0.25), shade(background, 0.05));
}

} // namespace Markdown
