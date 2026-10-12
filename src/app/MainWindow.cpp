#include "app/MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>

#include "about/AboutDialog.h"
#include "clipboard/ClipboardMarkdown.h"
#include "editor/FindBar.h"
#include "files/TextFile.h"
#include "integration/DesktopIntegration.h"
#include "integration/IntegrationDialog.h"
#include "print/Printing.h"

namespace {

const char *const kFileFilter = "Text and Markdown (*.txt *.md *.markdown);;All files (*)";

QString canonicalOrAbsolute(const QString &path)
{
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? info.absoluteFilePath() : canonical;
}

// QMenu::addAction() with a shortcut changed signature in Qt 6.3; this works on every Qt 6.
template <typename Receiver, typename Slot>
QAction *addItem(QMenu *menu, const QString &text, const QKeySequence &shortcut, Receiver *receiver, Slot slot)
{
    QAction *action = menu->addAction(text);
    action->setShortcut(shortcut);
    QObject::connect(action, &QAction::triggered, receiver, slot);
    return action;
}

} // namespace

MainWindow::MainWindow() : m_tabs(new QTabWidget), m_findBar(new FindBar), m_lastDir(QDir::homePath())
{
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::updateForCurrentTab);

    m_findBar->hide();
    connect(m_findBar, &FindBar::findNext, this, [this] { runFind(false, false); });
    connect(m_findBar, &FindBar::findPrevious, this, [this] { runFind(true, false); });
    connect(m_findBar, &FindBar::searchEdited, this, [this] { runFind(false, true); });
    connect(m_findBar, &FindBar::closed, this, [this] {
        if (DocumentTab *tab = currentTab())
            tab->activeTextWidget()->setFocus();
    });

    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_tabs, 1);
    layout->addWidget(m_findBar);
    setCentralWidget(central);

    createMenus();
    setAcceptDrops(true);
    resize(1000, 700);
}

void MainWindow::createMenus()
{
    QMenu *file = menuBar()->addMenu(tr("&File"));
    addItem(file, tr("&New"), QKeySequence::New, this, &MainWindow::newDocument);
    addItem(file, tr("&Open..."), QKeySequence::Open, this, &MainWindow::openWithDialog);
    file->addSeparator();
    addItem(file, tr("&Save"), QKeySequence::Save, this, [this] {
        if (DocumentTab *tab = currentTab())
            save(tab);
    });
    addItem(file, tr("Save &As..."), QKeySequence::SaveAs, this, [this] {
        if (DocumentTab *tab = currentTab())
            saveAs(tab);
    });
    file->addSeparator();
    addItem(file, tr("&Print..."), QKeySequence::Print, this, [this] { print(false); });
    addItem(file, tr("Export as P&DF..."), QKeySequence(), this, [this] { print(true); });
    file->addSeparator();
    addItem(file, tr("&Close Tab"), QKeySequence::Close, this, [this] { closeTab(m_tabs->currentIndex()); });
    QAction *quit = addItem(file, tr("&Quit"), QKeySequence::Quit, this, &QWidget::close);
    quit->setMenuRole(QAction::QuitRole);

    QMenu *edit = menuBar()->addMenu(tr("&Edit"));
    addItem(edit, tr("&Undo"), QKeySequence::Undo, this, [this] { editAction("undo"); });
    addItem(edit, tr("&Redo"), QKeySequence::Redo, this, [this] { editAction("redo"); });
    edit->addSeparator();
    addItem(edit, tr("Cu&t"), QKeySequence::Cut, this, [this] { editAction("cut"); });
    addItem(edit, tr("&Copy"), QKeySequence::Copy, this, [this] { editAction("copy"); });
    addItem(edit, tr("&Paste"), QKeySequence::Paste, this, [this] { editAction("paste"); });
    addItem(edit, tr("Select &All"), QKeySequence::SelectAll, this, [this] { editAction("selectAll"); });
    edit->addSeparator();
    addItem(edit, tr("&Find..."), QKeySequence::Find, this, &MainWindow::showFindBar);
    addItem(edit, tr("Find &Next"), QKeySequence::FindNext, this, [this] { runFind(false, false); });
    addItem(edit, tr("Find Pre&vious"), QKeySequence::FindPrevious, this, [this] { runFind(true, false); });

    QMenu *view = menuBar()->addMenu(tr("&View"));
    m_markdownAction = addItem(view, tr("&Markdown"), QKeySequence(Qt::CTRL | Qt::Key_M), this, [this](bool on) {
        DocumentTab *tab = currentTab();
        if (!tab)
            return;
        tab->setMarkdown(on);
        if (on)
            tab->setViewMode(DocumentTab::ViewMode::Split);
    });
    m_markdownAction->setCheckable(true);
    view->addSeparator();
    m_viewGroup = new QActionGroup(this);
    auto addMode = [&](const QString &text, const QKeySequence &shortcut, DocumentTab::ViewMode mode) {
        QAction *action = addItem(view, text, shortcut, this, [this, mode] {
            if (DocumentTab *tab = currentTab())
                tab->setViewMode(mode);
        });
        action->setCheckable(true);
        m_viewGroup->addAction(action);
        return action;
    };
    m_sourceAction = addMode(tr("&Source"), QKeySequence(Qt::CTRL | Qt::Key_1), DocumentTab::ViewMode::Source);
    m_splitAction = addMode(tr("S&plit"), QKeySequence(Qt::CTRL | Qt::Key_2), DocumentTab::ViewMode::Split);
    m_previewAction = addMode(tr("P&review"), QKeySequence(Qt::CTRL | Qt::Key_3), DocumentTab::ViewMode::Preview);

    // The same three actions as buttons in the tab bar corner, so switching views is one click.
    auto *viewButtons = new QWidget;
    auto *buttonLayout = new QHBoxLayout(viewButtons);
    buttonLayout->setContentsMargins(0, 0, 4, 0);
    buttonLayout->setSpacing(0);
    for (QAction *action : {m_sourceAction, m_splitAction, m_previewAction}) {
        action->setToolTip(QStringLiteral("%1 (%2)").arg(action->iconText(),
                                                         action->shortcut().toString(QKeySequence::NativeText)));
        auto *button = new QToolButton;
        button->setDefaultAction(action);
        button->setAutoRaise(true);
        buttonLayout->addWidget(button);
    }
    m_tabs->setCornerWidget(viewButtons, Qt::TopRightCorner);
    view->addSeparator();
    QAction *wrap = addItem(view, tr("&Word Wrap"), QKeySequence(), this, [this](bool on) {
        m_wordWrap = on;
        for (int i = 0; i < m_tabs->count(); ++i)
            tabAt(i)->setWordWrap(on);
    });
    wrap->setCheckable(true);
    wrap->setChecked(m_wordWrap);

    QMenu *tools = menuBar()->addMenu(tr("&Tools"));
    addItem(tools, tr("Paste as &Markdown"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V), this,
            &MainWindow::pasteAsMarkdown);
    addItem(tools, tr("&New from Clipboard"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), this,
            &MainWindow::newFromClipboard);
    if (DesktopIntegration::installKind() != DesktopIntegration::InstallKind::None) {
        tools->addSeparator();
        addItem(tools, tr("&Desktop Integration..."), QKeySequence(), this,
                [this] { IntegrationDialog::showSettings(this); });
    }

    QMenu *help = menuBar()->addMenu(tr("&Help"));
    QAction *about = addItem(help, tr("&About Meandris Write"), QKeySequence(), this,
                             [this] { AboutDialog::show(this); });
    about->setMenuRole(QAction::AboutRole); // macOS: moves it to the app menu
}

DocumentTab *MainWindow::currentTab() const
{
    return qobject_cast<DocumentTab *>(m_tabs->currentWidget());
}

DocumentTab *MainWindow::tabAt(int index) const
{
    return qobject_cast<DocumentTab *>(m_tabs->widget(index));
}

void MainWindow::newDocument()
{
    addTab(new DocumentTab);
}

void MainWindow::openFile(const QString &path)
{
    const QString target = canonicalOrAbsolute(path);
    for (int i = 0; i < m_tabs->count(); ++i) {
        const QString open = tabAt(i)->filePath();
        if (!open.isEmpty() && canonicalOrAbsolute(open) == target) {
            m_tabs->setCurrentIndex(i);
            return;
        }
    }

    auto *tab = new DocumentTab;
    QString error;
    if (!QFileInfo(target).isFile() || !tab->load(target, &error)) {
        delete tab;
        QMessageBox::warning(this, tr("Open"),
                             tr("Could not open %1\n\n%2").arg(QDir::toNativeSeparators(target), error));
        return;
    }
    m_lastDir = QFileInfo(target).absolutePath();
    addTab(tab);
}

void MainWindow::openFiles(const QStringList &paths)
{
    for (const QString &path : paths)
        openFile(path);
}

void MainWindow::addTab(DocumentTab *tab)
{
    DocumentTab *blank = currentTab();
    if (blank && !blank->isBlankUntitled())
        blank = nullptr;

    tab->setWordWrap(m_wordWrap);
    connect(tab, &DocumentTab::modificationChanged, this, [this, tab] { updateTabTitle(tab); });
    connect(tab, &DocumentTab::viewModeChanged, this, [this, tab] {
        if (tab == currentTab())
            updateForCurrentTab();
    });
    connect(tab, &DocumentTab::openFileRequested, this, &MainWindow::openFile);
    connect(tab, &DocumentTab::filesDropped, this, &MainWindow::openFiles);
    connect(tab, &DocumentTab::statusMessage, this, [this](const QString &message) {
        statusBar()->showMessage(message, 5000);
    });

    m_tabs->setCurrentIndex(m_tabs->addTab(tab, tab->displayName()));
    updateTabTitle(tab);

    if (blank) {
        m_tabs->removeTab(m_tabs->indexOf(blank));
        blank->deleteLater();
    }
    tab->activeTextWidget()->setFocus();
}

bool MainWindow::closeTab(int index)
{
    DocumentTab *tab = tabAt(index);
    if (!tab)
        return false;
    m_tabs->setCurrentIndex(index);
    if (!maybeSave(tab))
        return false;
    m_tabs->removeTab(index);
    tab->deleteLater();
    if (m_tabs->count() == 0)
        newDocument();
    return true;
}

void MainWindow::openWithDialog()
{
    const QStringList paths = QFileDialog::getOpenFileNames(this, tr("Open"), m_lastDir, tr(kFileFilter));
    openFiles(paths);
}

bool MainWindow::save(DocumentTab *tab)
{
    if (tab->filePath().isEmpty())
        return saveAs(tab);
    QString error;
    if (!tab->save(tab->filePath(), &error)) {
        QMessageBox::critical(this, tr("Save"), tr("Could not save %1\n\n%2").arg(tab->displayName(), error));
        return false;
    }
    updateTabTitle(tab);
    return true;
}

bool MainWindow::saveAs(DocumentTab *tab)
{
    QString suggested = tab->filePath();
    if (suggested.isEmpty()) {
        const QString name = tab->isMarkdown() ? QStringLiteral("Untitled.md") : QStringLiteral("Untitled.txt");
        suggested = QDir(m_lastDir).filePath(name);
    }
    const QString path = QFileDialog::getSaveFileName(this, tr("Save As"), suggested, tr(kFileFilter));
    if (path.isEmpty())
        return false;

    QString error;
    if (!tab->save(path, &error)) {
        QMessageBox::critical(this, tr("Save As"),
                              tr("Could not save %1\n\n%2").arg(QDir::toNativeSeparators(path), error));
        return false;
    }
    m_lastDir = QFileInfo(path).absolutePath();
    updateTabTitle(tab);
    updateForCurrentTab();
    return true;
}

bool MainWindow::maybeSave(DocumentTab *tab)
{
    if (!tab->isModified())
        return true;
    const auto answer = QMessageBox::warning(this, tr("Unsaved changes"),
                                             tr("Save changes to %1 before closing?").arg(tab->displayName()),
                                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                             QMessageBox::Save);
    if (answer == QMessageBox::Save)
        return save(tab);
    return answer == QMessageBox::Discard;
}

void MainWindow::editAction(const char *slot)
{
    // Menu clicks go to whichever text field has focus (the find bar too), else to the document.
    QWidget *target = QApplication::focusWidget();
    if (!qobject_cast<QLineEdit *>(target) && !qobject_cast<QPlainTextEdit *>(target)
        && !qobject_cast<QTextEdit *>(target)) {
        DocumentTab *tab = currentTab();
        target = tab ? tab->activeTextWidget() : nullptr;
    }
    if (target)
        QMetaObject::invokeMethod(target, slot);
}

void MainWindow::showFindBar()
{
    QString selected;
    if (auto *editor = qobject_cast<QPlainTextEdit *>(QApplication::focusWidget()))
        selected = editor->textCursor().selectedText();
    else if (auto *browser = qobject_cast<QTextEdit *>(QApplication::focusWidget()))
        selected = browser->textCursor().selectedText();
    if (selected.contains(QChar::ParagraphSeparator))
        selected.clear();
    m_findBar->activate(selected);
}

void MainWindow::runFind(bool backward, bool restartAtSelection)
{
    DocumentTab *tab = currentTab();
    const QString needle = m_findBar->text();
    if (needle.isEmpty()) {
        m_findBar->setNotFound(false);
        if (!m_findBar->isVisible()) // Find Next before anything was searched
            m_findBar->activate(QString());
        return;
    }
    if (tab)
        m_findBar->setNotFound(!tab->find(needle, backward, restartAtSelection));
}

void MainWindow::pasteAsMarkdown()
{
    DocumentTab *tab = currentTab();
    const QString markdown = ClipboardMarkdown::fromMimeData(QApplication::clipboard()->mimeData());
    if (!tab || markdown.isEmpty()) {
        statusBar()->showMessage(tr("The clipboard has no text"), 5000);
        return;
    }
    tab->insertText(markdown);
    // Show the rendered result next to the pasted text.
    if (!tab->isMarkdown())
        tab->setMarkdown(true);
    if (tab->viewMode() == DocumentTab::ViewMode::Source)
        tab->setViewMode(DocumentTab::ViewMode::Split);
}

void MainWindow::newFromClipboard()
{
    const QString markdown = ClipboardMarkdown::fromMimeData(QApplication::clipboard()->mimeData());
    if (markdown.isEmpty()) {
        statusBar()->showMessage(tr("The clipboard has no text"), 5000);
        return;
    }
    auto *tab = new DocumentTab;
    tab->setInitialText(markdown, true);
    addTab(tab);
}

void MainWindow::print(bool toPdf)
{
    DocumentTab *tab = currentTab();
    if (!tab)
        return;
    Printing::Document document;
    document.text = tab->text();
    document.markdown = tab->isMarkdown();
    document.filePath = tab->filePath();
    document.title = QFileInfo(tab->displayName()).completeBaseName();
    if (toPdf)
        Printing::exportPdf(this, document);
    else
        Printing::print(this, document);
}

void MainWindow::updateTabTitle(DocumentTab *tab)
{
    const int index = m_tabs->indexOf(tab);
    if (index < 0)
        return;
    m_tabs->setTabText(index, tab->displayName() + (tab->isModified() ? QStringLiteral(" •") : QString()));
    m_tabs->setTabToolTip(index, QDir::toNativeSeparators(tab->filePath()));
    if (tab == currentTab())
        updateForCurrentTab();
}

void MainWindow::updateForCurrentTab()
{
    DocumentTab *tab = currentTab();
    if (!tab)
        return;

    setWindowTitle(tab->displayName() + QStringLiteral("[*] — Meandris Write"));
    setWindowModified(tab->isModified());
    setWindowFilePath(tab->filePath());

    m_markdownAction->setChecked(tab->isMarkdown());
    m_viewGroup->setEnabled(tab->isMarkdown());
    switch (tab->viewMode()) {
    case DocumentTab::ViewMode::Source: m_sourceAction->setChecked(true); break;
    case DocumentTab::ViewMode::Split: m_splitAction->setChecked(true); break;
    case DocumentTab::ViewMode::Preview: m_previewAction->setChecked(true); break;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        DocumentTab *tab = tabAt(i);
        if (!tab->isModified())
            continue;
        m_tabs->setCurrentIndex(i);
        if (!maybeSave(tab)) {
            event->ignore();
            return;
        }
    }
    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (!TextFile::localFilePaths(event->mimeData()).isEmpty())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QStringList paths = TextFile::localFilePaths(event->mimeData());
    if (paths.isEmpty())
        return;
    event->acceptProposedAction();
    openFiles(paths);
}
