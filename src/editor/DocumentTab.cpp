#include "editor/DocumentTab.h"

#include <QCryptographicHash>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>

#include "markdown/MarkdownPreview.h"

namespace {

QByteArray hashOf(const QByteArray &bytes)
{
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha1);
}

template <typename TextWidget>
bool findIn(TextWidget *widget, const QString &needle, bool backward, bool restartAtSelection)
{
    const QTextCursor original = widget->textCursor();
    QTextDocument::FindFlags flags;
    if (backward)
        flags |= QTextDocument::FindBackward;

    if (restartAtSelection) {
        QTextCursor start = original;
        start.setPosition(original.selectionStart());
        widget->setTextCursor(start);
    }
    if (widget->find(needle, flags))
        return true;

    QTextCursor wrapped = original;
    wrapped.movePosition(backward ? QTextCursor::End : QTextCursor::Start);
    widget->setTextCursor(wrapped);
    if (widget->find(needle, flags))
        return true;

    widget->setTextCursor(original);
    return false;
}

} // namespace

DocumentTab::DocumentTab(QWidget *parent)
    : QWidget(parent)
    , m_editor(new QPlainTextEdit)
    , m_preview(new MarkdownPreview)
    , m_splitter(new QSplitter(Qt::Horizontal))
    , m_renderTimer(new QTimer(this))
    , m_diskCheckTimer(new QTimer(this))
    , m_watcher(new QFileSystemWatcher(this))
{
    const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_editor->setFont(mono);
    m_editor->setTabStopDistance(QFontMetricsF(mono).horizontalAdvance(QLatin1Char(' ')) * 4);
    m_editor->setFrameShape(QFrame::NoFrame);
    m_preview->setFrameShape(QFrame::NoFrame);

    m_splitter->addWidget(m_editor);
    m_splitter->addWidget(m_preview);
    m_splitter->setChildrenCollapsible(false);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter);

    m_renderTimer->setSingleShot(true);
    connect(m_renderTimer, &QTimer::timeout, this, &DocumentTab::renderPreview);
    connect(m_editor->document(), &QTextDocument::contentsChanged, this, &DocumentTab::schedulePreview);
    connect(m_editor->document(), &QTextDocument::modificationChanged, this, &DocumentTab::modificationChanged);
    connect(m_preview, &MarkdownPreview::openFileRequested, this, &DocumentTab::openFileRequested);

    QScrollBar *editorBar = m_editor->verticalScrollBar();
    QScrollBar *previewBar = m_preview->verticalScrollBar();
    connect(editorBar, &QScrollBar::valueChanged, this, [=] { syncScroll(editorBar, previewBar); });
    connect(previewBar, &QScrollBar::valueChanged, this, [=] { syncScroll(previewBar, editorBar); });

    // Tools often write a file in several steps; wait briefly before looking at it.
    m_diskCheckTimer->setSingleShot(true);
    m_diskCheckTimer->setInterval(150);
    connect(m_diskCheckTimer, &QTimer::timeout, this, &DocumentTab::checkDisk);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, m_diskCheckTimer, qOverload<>(&QTimer::start));
    // Files saved by replacing them drop out of the watcher; the folder tells us when they return.
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] {
        if (!m_watcher->files().contains(m_path))
            m_diskCheckTimer->start();
    });

    m_editor->viewport()->installEventFilter(this);
    m_preview->viewport()->installEventFilter(this);
    updatePanes();
}

bool DocumentTab::load(const QString &path, QString *error)
{
    QByteArray bytes;
    if (!TextFile::read(path, &bytes, error))
        return false;

    m_editor->setPlainText(TextFile::decode(bytes, &m_format));
    m_diskHash = hashOf(bytes);
    setPath(path);
    setMarkdown(TextFile::isMarkdownPath(path));
    setViewMode(m_markdown ? ViewMode::Preview : ViewMode::Source);
    return true;
}

bool DocumentTab::save(const QString &path, QString *error)
{
    const QByteArray bytes = TextFile::encode(text(), m_format);
    if (!TextFile::write(path, bytes, error))
        return false;

    // Only a new name decides Markdown mode, so a .txt switched to Markdown stays that way on save.
    const bool renamed = QFileInfo(path).absoluteFilePath() != m_path;
    m_diskHash = hashOf(bytes);
    setPath(path);
    m_editor->document()->setModified(false);
    if (renamed && TextFile::isMarkdownPath(path) != m_markdown)
        setMarkdown(!m_markdown);
    return true;
}

void DocumentTab::setInitialText(const QString &text, bool markdown)
{
    m_editor->setPlainText(text);
    m_editor->document()->setModified(!text.isEmpty());
    setMarkdown(markdown);
    setViewMode(markdown ? ViewMode::Preview : ViewMode::Source);
}

QString DocumentTab::displayName() const
{
    return m_path.isEmpty() ? tr("Untitled") : QFileInfo(m_path).fileName();
}

QString DocumentTab::text() const
{
    QString text = m_editor->document()->toRawText();
    for (QChar &c : text) {
        if (c == QChar::ParagraphSeparator || c == QChar::LineSeparator)
            c = QLatin1Char('\n');
    }
    return text;
}

bool DocumentTab::isModified() const
{
    return m_editor->document()->isModified();
}

bool DocumentTab::isBlankUntitled() const
{
    return m_path.isEmpty() && !isModified() && m_editor->document()->isEmpty();
}

void DocumentTab::setViewMode(ViewMode mode)
{
    if (!m_markdown)
        mode = ViewMode::Source;
    const bool enteringSplit = mode == ViewMode::Split && m_mode != ViewMode::Split;
    m_mode = mode;
    updatePanes();

    if (enteringSplit) {
        m_splitter->setSizes({1000, 1000});
        syncScroll(m_editor->verticalScrollBar(), m_preview->verticalScrollBar());
    }
    activeTextWidget()->setFocus();
    emit viewModeChanged();
}

void DocumentTab::setWordWrap(bool on)
{
    m_editor->setLineWrapMode(on ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

void DocumentTab::insertText(const QString &text)
{
    if (m_markdown && m_mode == ViewMode::Preview)
        setViewMode(ViewMode::Split);
    m_editor->insertPlainText(text);
    m_editor->setFocus();
}

bool DocumentTab::find(const QString &needle, bool backward, bool restartAtSelection)
{
    if (needle.isEmpty())
        return true;
    if (m_markdown && m_mode == ViewMode::Preview)
        return findIn(static_cast<QTextEdit *>(m_preview), needle, backward, restartAtSelection);
    return findIn(m_editor, needle, backward, restartAtSelection);
}

QWidget *DocumentTab::activeTextWidget() const
{
    if (m_markdown && m_mode == ViewMode::Preview)
        return m_preview;
    return m_editor;
}

bool DocumentTab::eventFilter(QObject *watched, QEvent *event)
{
    // Dropping files onto the editor opens them instead of inserting their paths.
    const QEvent::Type type = event->type();
    if (type == QEvent::DragEnter || type == QEvent::DragMove || type == QEvent::Drop) {
        auto *drop = static_cast<QDropEvent *>(event);
        const QStringList paths = TextFile::localFilePaths(drop->mimeData());
        if (!paths.isEmpty()) {
            drop->acceptProposedAction();
            if (type == QEvent::Drop)
                emit filesDropped(paths);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void DocumentTab::setPath(const QString &path)
{
    m_path = QFileInfo(path).absoluteFilePath();
    m_preview->setBaseDirectory(QFileInfo(m_path).absolutePath());
    watchFile();
}

void DocumentTab::setMarkdown(bool markdown)
{
    m_markdown = markdown;
    if (!markdown)
        m_mode = ViewMode::Source;
    m_previewStale = true;
    updatePanes();
    emit viewModeChanged();
}

void DocumentTab::updatePanes()
{
    const bool showPreview = m_markdown && m_mode != ViewMode::Source;
    const bool showEditor = !m_markdown || m_mode != ViewMode::Preview;
    if (showPreview && m_previewStale)
        renderPreview();
    m_editor->setVisible(showEditor);
    m_preview->setVisible(showPreview);
}

void DocumentTab::schedulePreview()
{
    m_previewStale = true;
    if (m_markdown && m_mode != ViewMode::Source)
        m_renderTimer->start(m_renderDelayMs);
}

void DocumentTab::renderPreview()
{
    if (!m_markdown)
        return;
    QElapsedTimer timer;
    timer.start();

    m_syncingScroll = true; // the re-render must not scroll the editor
    m_preview->render(text());
    m_syncingScroll = false;
    m_previewStale = false;

    if (m_mode == ViewMode::Split)
        syncScroll(m_editor->verticalScrollBar(), m_preview->verticalScrollBar());

    // Keep typing smooth in big documents: wait longer between renders when rendering is slow.
    m_renderDelayMs = qBound(200, int(timer.elapsed()) * 3, 1500);
}

void DocumentTab::syncScroll(QScrollBar *from, QScrollBar *to)
{
    if (m_syncingScroll || !m_markdown || m_mode != ViewMode::Split)
        return;
    m_syncingScroll = true;
    const qreal fraction = from->maximum() > 0 ? qreal(from->value()) / from->maximum() : 0.0;
    to->setValue(qRound(fraction * to->maximum()));
    m_syncingScroll = false;
}

void DocumentTab::watchFile()
{
    const QStringList watched = m_watcher->files() + m_watcher->directories();
    if (!watched.isEmpty())
        m_watcher->removePaths(watched);
    if (m_path.isEmpty())
        return;
    m_watcher->addPath(QFileInfo(m_path).absolutePath());
    if (QFileInfo::exists(m_path))
        m_watcher->addPath(m_path);
}

void DocumentTab::checkDisk()
{
    if (m_path.isEmpty() || m_askingReload || !QFileInfo::exists(m_path))
        return;
    if (!m_watcher->files().contains(m_path))
        m_watcher->addPath(m_path);

    QByteArray bytes;
    if (!TextFile::read(m_path, &bytes, nullptr))
        return;
    const QByteArray hash = hashOf(bytes);
    if (hash == m_diskHash)
        return; // our own save, or nothing really changed
    m_diskHash = hash;

    if (isModified()) {
        m_askingReload = true;
        const auto answer = QMessageBox::question(
            this, tr("File changed on disk"),
            tr("%1 was changed by another program.\n\nReload it and lose your unsaved changes?").arg(displayName()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        m_askingReload = false;
        if (answer != QMessageBox::Yes)
            return;
        // It may have changed again while the question was open.
        if (!TextFile::read(m_path, &bytes, nullptr))
            return;
        m_diskHash = hashOf(bytes);
    }

    replaceTextKeepingCursor(TextFile::decode(bytes, &m_format));
    emit statusMessage(tr("Reloaded %1 (changed on disk)").arg(displayName()));
}

void DocumentTab::replaceTextKeepingCursor(const QString &text)
{
    const int position = m_editor->textCursor().position();
    const int scroll = m_editor->verticalScrollBar()->value();

    // An edit block rather than setPlainText(), so the reload can be undone.
    QTextCursor cursor(m_editor->document());
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(text);
    cursor.endEditBlock();

    cursor.setPosition(qMin(position, m_editor->document()->characterCount() - 1));
    m_editor->setTextCursor(cursor);
    m_editor->verticalScrollBar()->setValue(scroll);
    m_editor->document()->setModified(false);
}
