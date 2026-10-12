#include "markdown/MarkdownPreview.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QScrollBar>

#include "files/TextFile.h"
#include "markdown/MarkdownRender.h"

MarkdownPreview::MarkdownPreview(QWidget *parent) : QTextBrowser(parent)
{
    setOpenLinks(false);
    setOpenExternalLinks(false);
    connect(this, &QTextBrowser::anchorClicked, this, &MarkdownPreview::onAnchorClicked);
}

void MarkdownPreview::setBaseDirectory(const QString &dir)
{
    m_baseDir = dir;
    document()->setBaseUrl(dir.isEmpty() ? QUrl() : QUrl::fromLocalFile(dir + QLatin1Char('/')));
    setSearchPaths(dir.isEmpty() ? QStringList() : QStringList{dir});
}

void MarkdownPreview::render(const QString &markdown)
{
    QScrollBar *bar = verticalScrollBar();
    const int scroll = bar->value();
    Markdown::render(document(), markdown, palette().color(QPalette::Base));
    bar->setValue(scroll);
}

void MarkdownPreview::onAnchorClicked(const QUrl &url)
{
    if (url.scheme().isEmpty() && url.path().isEmpty() && url.hasFragment()) {
        scrollToAnchor(url.fragment());
        return;
    }

    QString localPath;
    if (url.isLocalFile())
        localPath = url.toLocalFile();
    else if (url.isRelative() && !m_baseDir.isEmpty())
        localPath = QDir(m_baseDir).absoluteFilePath(url.path(QUrl::FullyDecoded));

    if (localPath.isEmpty()) {
        QDesktopServices::openUrl(url);
        return;
    }

    const QFileInfo info(localPath);
    const bool editable = TextFile::isMarkdownPath(localPath) || info.suffix().toLower() == QLatin1String("txt");
    if (editable && info.isFile())
        emit openFileRequested(info.absoluteFilePath());
    else
        QDesktopServices::openUrl(QUrl::fromLocalFile(localPath));
}
