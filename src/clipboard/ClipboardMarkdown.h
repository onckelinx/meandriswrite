#pragma once

#include <QString>

class QMimeData;

// Turns clipboard contents (typically copied from an LLM web page) into Markdown.
namespace ClipboardMarkdown {

// Rules, in order:
//  1. text/markdown, or plain text that already looks like Markdown, is used as is
//     (LLM "copy" buttons put Markdown in the plain text).
//  2. Otherwise HTML is converted to Markdown.
//  3. Otherwise the plain text is kept unchanged.
QString fromMimeData(const QMimeData *mime);

bool looksLikeMarkdown(const QString &text);
QString htmlToMarkdown(const QString &html);

} // namespace ClipboardMarkdown
