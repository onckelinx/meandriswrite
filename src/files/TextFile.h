#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

class QMimeData;

// How a text file is stored on disk, so it can be written back the same way.
struct TextFileFormat {
    bool crlf = false;    // file used "\r\n" line endings
    bool utf8Bom = false; // file started with a UTF-8 byte order mark
};

// Reading and writing text files. In memory, text always uses '\n' line endings.
namespace TextFile {

// Decodes UTF-8 (falling back to Latin-1 for files that are not valid UTF-8).
QString decode(const QByteArray &bytes, TextFileFormat *format);
QByteArray encode(const QString &text, const TextFileFormat &format);

bool read(const QString &path, QByteArray *bytes, QString *error);
// Writes atomically: the old file stays intact if anything goes wrong.
bool write(const QString &path, const QByteArray &bytes, QString *error);

bool isMarkdownPath(const QString &path);

// Local file paths in a drag-and-drop payload; empty if it holds anything else.
QStringList localFilePaths(const QMimeData *mime);

} // namespace TextFile
