#include "clipboard/ClipboardMarkdown.h"

#include <QFont>
#include <QFontDatabase>
#include <QList>
#include <QMimeData>
#include <QRegularExpression>
#include <QStringList>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

namespace ClipboardMarkdown {

namespace {

QString normalizeLineEndings(QString text)
{
    text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return text;
}

// The HTML importer turns each line of a <pre> into a non-wrapping block, and gives
// <code>/<pre> text the system's fixed-width font family (just "monospace" without a
// desktop). Tag both so the Markdown writer emits fenced code blocks and `inline code`,
// whether or not fontconfig reports that font as fixed-pitch.
void markCode(QTextDocument *doc)
{
    QStringList monoFamilies = QFontDatabase::systemFont(QFontDatabase::FixedFont).families();
    monoFamilies << QStringLiteral("monospace");
    const auto isMono = [&monoFamilies](const QTextCharFormat &format) {
        const QStringList families = format.fontFamilies().toStringList();
        for (const QString &family : families) {
            if (monoFamilies.contains(family, Qt::CaseInsensitive))
                return true;
        }
        return false;
    };

    QTextCursor cursor(doc);
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat blockFormat = block.blockFormat();
        if (blockFormat.nonBreakableLines() && !blockFormat.hasProperty(QTextFormat::BlockCodeFence)) {
            blockFormat.setProperty(QTextFormat::BlockCodeFence, QStringLiteral("`"));
            // Without a language property the writer puts a blank line after every code line.
            blockFormat.setProperty(QTextFormat::BlockCodeLanguage, QString());
            cursor.setPosition(block.position());
            cursor.setBlockFormat(blockFormat);
        }

        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            QTextCharFormat charFormat = fragment.charFormat();
            if (charFormat.fontFixedPitch() || !isMono(charFormat))
                continue;
            charFormat.setFontFixedPitch(true);
            cursor.setPosition(fragment.position());
            cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
            cursor.setCharFormat(charFormat);
        }
    }
}

// The writer starts the next paragraph right after a closing fence; add a blank line.
QString blankLineAfterCodeBlocks(const QString &markdown)
{
    QStringList lines = markdown.split(QLatin1Char('\n'));
    bool inFence = false;
    for (int i = 0; i < lines.size(); ++i) {
        if (!lines[i].trimmed().startsWith(QLatin1String("```")))
            continue;
        inFence = !inFence;
        if (!inFence && i + 1 < lines.size() && !lines[i + 1].trimmed().isEmpty())
            lines.insert(i + 1, QString());
    }
    return lines.join(QLatin1Char('\n'));
}

} // namespace

bool looksLikeMarkdown(const QString &text)
{
    const auto multiline = QRegularExpression::MultilineOption;

    // One of these is enough: headings, code fences, table separator rows.
    static const QRegularExpression strong(
        QStringLiteral(R"(^#{1,6}[ \t]+\S|^ {0,3}(```|~~~)|^[ \t]*\|?[ \t]*:?-{3,}:?[ \t]*\|)"), multiline);
    if (strong.match(text).hasMatch())
        return true;

    // These also appear in ordinary text, so require two different kinds.
    static const QList<QRegularExpression> weak = {
        QRegularExpression(QStringLiteral(R"(^[ \t]*[-*+][ \t]+\S)"), multiline),     // bullet list
        QRegularExpression(QStringLiteral(R"(^[ \t]*\d+[.)][ \t]+\S)"), multiline),   // numbered list
        QRegularExpression(QStringLiteral(R"(^[ \t]*>[ \t]?\S)"), multiline),         // quote
        QRegularExpression(QStringLiteral(R"(\*\*[^*\n]+\*\*|__[^_\n]+__)")),         // bold
        QRegularExpression(QStringLiteral(R"(\[[^\]\n]+\]\([^)\s]+\))")),             // link
        QRegularExpression(QStringLiteral(R"(`[^`\n]+`)")),                           // inline code
    };
    int hits = 0;
    for (const QRegularExpression &re : weak) {
        if (re.match(text).hasMatch() && ++hits >= 2)
            return true;
    }
    return false;
}

QString htmlToMarkdown(const QString &html)
{
    QTextDocument doc;
    doc.setHtml(html);
    markCode(&doc);
    QString markdown = blankLineAfterCodeBlocks(doc.toMarkdown(QTextDocument::MarkdownDialectGitHub));

    static const QRegularExpression trailingSpaces(QStringLiteral(R"([ \t]+\n)"));
    static const QRegularExpression extraBlankLines(QStringLiteral(R"(\n{3,})"));
    markdown.replace(trailingSpaces, QStringLiteral("\n"));
    markdown.replace(extraBlankLines, QStringLiteral("\n\n"));
    markdown = markdown.trimmed();
    return markdown.isEmpty() ? markdown : markdown + QLatin1Char('\n');
}

QString fromMimeData(const QMimeData *mime)
{
    if (!mime)
        return {};

    if (mime->hasFormat(QStringLiteral("text/markdown"))) {
        const QString markdown = QString::fromUtf8(mime->data(QStringLiteral("text/markdown")));
        if (!markdown.trimmed().isEmpty())
            return normalizeLineEndings(markdown);
    }

    const QString plain = normalizeLineEndings(mime->text());
    if (looksLikeMarkdown(plain))
        return plain;

    if (mime->hasHtml()) {
        const QString markdown = htmlToMarkdown(mime->html());
        if (!markdown.trimmed().isEmpty())
            return markdown;
    }
    return plain;
}

} // namespace ClipboardMarkdown
