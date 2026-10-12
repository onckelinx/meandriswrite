#pragma once

#include <QDialog>

class QCheckBox;

// Asks whether to add meandriswrite to the app menu (AppImage only) and to make it the
// default app for Markdown files. Shown once on first launch and from Tools.
class IntegrationDialog : public QDialog {
    Q_OBJECT

public:
    // Shows the first-launch question if it has not been answered yet.
    static void askOnFirstLaunch(QWidget *parent);
    // Shows the dialog with the current settings (Tools → Desktop Integration).
    static void showSettings(QWidget *parent);

private:
    IntegrationDialog(bool firstLaunch, QWidget *parent);
    bool apply();

    QCheckBox *m_menu = nullptr;
    QCheckBox *m_markdown;
    QCheckBox *m_dontAsk = nullptr;
};
