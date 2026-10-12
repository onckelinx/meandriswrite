#pragma once

#include <QMainWindow>

#include "editor/DocumentTab.h"

class FindBar;
class QAction;
class QActionGroup;
class QTabWidget;

// The window: menus, tabs and the find bar. Features live in their own folders;
// this class only wires them together.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();

    void newDocument();
    void openFile(const QString &path);
    void openFiles(const QStringList &paths);

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void createMenus();
    DocumentTab *currentTab() const;
    DocumentTab *tabAt(int index) const;
    // Adds the tab and closes the current one if it is a blank untitled tab.
    void addTab(DocumentTab *tab);
    bool closeTab(int index);

    void openWithDialog();
    bool save(DocumentTab *tab);
    bool saveAs(DocumentTab *tab);
    bool maybeSave(DocumentTab *tab);

    void editAction(const char *slot);
    void showFindBar();
    void runFind(bool backward, bool restartAtSelection);
    void pasteAsMarkdown();
    void newFromClipboard();
    void print(bool toPdf);

    void updateTabTitle(DocumentTab *tab);
    void updateForCurrentTab();

    QTabWidget *m_tabs;
    FindBar *m_findBar;
    QAction *m_markdownAction = nullptr;
    QActionGroup *m_viewGroup = nullptr;
    QAction *m_sourceAction = nullptr;
    QAction *m_splitAction = nullptr;
    QAction *m_previewAction = nullptr;
    bool m_wordWrap = true;
    QString m_lastDir;
};
