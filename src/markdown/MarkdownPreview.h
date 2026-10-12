#pragma once

#include <QTextBrowser>

// Read-only rendered view of a Markdown document.
class MarkdownPreview : public QTextBrowser {
    Q_OBJECT

public:
    explicit MarkdownPreview(QWidget *parent = nullptr);

    // Folder used to resolve relative image paths and links.
    void setBaseDirectory(const QString &dir);
    // Re-renders, keeping the scroll position.
    void render(const QString &markdown);

signals:
    // A link to a local text or Markdown file was clicked.
    void openFileRequested(const QString &path);

private:
    void onAnchorClicked(const QUrl &url);

    QString m_baseDir;
};
