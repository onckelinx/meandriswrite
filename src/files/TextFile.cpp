#include "files/TextFile.h"

#include <QFile>
#include <QFileInfo>
#include <QMimeData>
#include <QSaveFile>
#include <QStringDecoder>
#include <QUrl>

namespace TextFile {

namespace {
const QByteArray kUtf8Bom("\xEF\xBB\xBF");
}

QString decode(const QByteArray &bytes, TextFileFormat *format)
{
    TextFileFormat fmt;
    QByteArray data = bytes;
    if (data.startsWith(kUtf8Bom)) {
        fmt.utf8Bom = true;
        data.remove(0, kUtf8Bom.size());
    }
    fmt.crlf = data.contains("\r\n");

    QStringDecoder decoder(QStringDecoder::Utf8);
    QString text = decoder(data);
    if (decoder.hasError())
        text = QString::fromLatin1(data); // best effort; saved back as UTF-8

    text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    if (format)
        *format = fmt;
    return text;
}

QByteArray encode(const QString &text, const TextFileFormat &format)
{
    QString out = text;
    if (format.crlf)
        out.replace(QLatin1Char('\n'), QLatin1String("\r\n"));
    QByteArray bytes = out.toUtf8();
    if (format.utf8Bom)
        bytes.prepend(kUtf8Bom);
    return bytes;
}

bool read(const QString &path, QByteArray *bytes, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    *bytes = file.readAll();
    return true;
}

bool write(const QString &path, const QByteArray &bytes, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

bool isMarkdownPath(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == QLatin1String("md") || suffix == QLatin1String("markdown")
        || suffix == QLatin1String("mdown") || suffix == QLatin1String("mkd");
}

QStringList localFilePaths(const QMimeData *mime)
{
    QStringList paths;
    if (!mime || !mime->hasUrls())
        return paths;
    for (const QUrl &url : mime->urls()) {
        if (!url.isLocalFile())
            return {};
        paths << url.toLocalFile();
    }
    return paths;
}

} // namespace TextFile
