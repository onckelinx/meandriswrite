#pragma once

#include <QByteArray>
#include <QFileSystemWatcher>
#include <QWidget>

#include "files/TextFile.h"

class MarkdownPreview;
class QPlainTextEdit;
class QScrollBar;
class QSplitter;
class QTimer;

// One open document: the source editor, the Markdown preview and the file they belong to.
class DocumentTab : public QWidget {
    Q_OBJECT

public:
    enum class ViewMode { Source, Split, Preview };

    explicit DocumentTab(QWidget *parent = nullptr);

    bool load(const QString &path, QString *error);
    bool save(const QString &path, QString *error);
    // Starts an unsaved document with the given contents.
    void setInitialText(const QString &text, bool markdown);

    QString filePath() const { return m_path; }
    QString displayName() const;
    // The editor text. Use this rather than toPlainText(), which turns non-breaking spaces into spaces.
    QString text() const;
    bool isModified() const;
    // An untitled tab nobody has typed in; opening a file replaces it.
    bool isBlankUntitled() const;

    bool isMarkdown() const { return m_markdown; }
    // Markdown mode enables the Split and Preview views. It follows the file extension on
    // load and Save As, and can be switched on for any document.
    void setMarkdown(bool markdown);
    ViewMode viewMode() const { return m_mode; }
    void setViewMode(ViewMode mode);

    void setWordWrap(bool on);
    void insertText(const QString &text);
    // Searches the visible pane (the preview in Preview mode, otherwise the source), wrapping around.
    // restartAtSelection searches again from the start of the current match (find-as-you-type).
    bool find(const QString &needle, bool backward, bool restartAtSelection);
    // The editor, or the preview in Preview mode: the target for copy, select all and find.
    QWidget *activeTextWidget() const;

signals:
    void modificationChanged(bool modified);
    void viewModeChanged();
    void openFileRequested(const QString &path);
    void filesDropped(const QStringList &paths);
    void statusMessage(const QString &message);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setPath(const QString &path);
    void updatePanes();
    void schedulePreview();
    void renderPreview();
    void syncScroll(QScrollBar *from, QScrollBar *to);
    void watchFile();
    void checkDisk();
    void replaceTextKeepingCursor(const QString &text);

    QPlainTextEdit *m_editor;
    MarkdownPreview *m_preview;
    QSplitter *m_splitter;
    QTimer *m_renderTimer;
    QTimer *m_diskCheckTimer;
    QFileSystemWatcher *m_watcher;

    QString m_path;
    TextFileFormat m_format;
    QByteArray m_diskHash; // file contents as last loaded or saved, to ignore our own writes
    bool m_markdown = false;
    ViewMode m_mode = ViewMode::Source;
    bool m_previewStale = true;
    bool m_syncingScroll = false;
    bool m_askingReload = false;
    int m_renderDelayMs = 200;
};
