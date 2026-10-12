#include "integration/IntegrationDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include "integration/DesktopIntegration.h"

namespace {

const char *const kAnsweredKey = "integration/answered";

} // namespace

void IntegrationDialog::askOnFirstLaunch(QWidget *parent)
{
    if (DesktopIntegration::installKind() == DesktopIntegration::InstallKind::None)
        return;
    if (QSettings().value(QLatin1String(kAnsweredKey)).toBool())
        return;
    IntegrationDialog dialog(true, parent);
    dialog.exec();
}

void IntegrationDialog::showSettings(QWidget *parent)
{
    IntegrationDialog dialog(false, parent);
    dialog.exec();
}

IntegrationDialog::IntegrationDialog(bool firstLaunch, QWidget *parent) : QDialog(parent)
{
    const bool appImage = DesktopIntegration::installKind() == DesktopIntegration::InstallKind::AppImage;
    setWindowTitle(tr("Desktop Integration"));

    auto *layout = new QVBoxLayout(this);
    if (firstLaunch)
        layout->addWidget(new QLabel(tr("<b>Set up Meandris Write on this computer?</b>")));

    if (appImage) {
        m_menu = new QCheckBox(tr("Add to the app menu"));
        m_menu->setChecked(firstLaunch || DesktopIntegration::isInMenu());
        layout->addWidget(m_menu);
    }
    m_markdown = new QCheckBox(tr("Open .md files with Meandris Write by default"));
    m_markdown->setChecked(firstLaunch || DesktopIntegration::isDefaultForMarkdown());
    layout->addWidget(m_markdown);

    // The default app needs a menu entry.
    if (m_menu) {
        connect(m_menu, &QCheckBox::toggled, this, [this](bool on) {
            if (!on)
                m_markdown->setChecked(false);
        });
        connect(m_markdown, &QCheckBox::toggled, this, [this](bool on) {
            if (on)
                m_menu->setChecked(true);
        });
    }

    if (firstLaunch) {
        layout->addSpacing(8);
        layout->addWidget(new QLabel(tr("You can change this later under Tools → Desktop Integration.")));
        m_dontAsk = new QCheckBox(tr("Don't ask again"));
        layout->addWidget(m_dontAsk);
    }

    auto *buttons = new QDialogButtonBox;
    QPushButton *ok = buttons->addButton(firstLaunch ? tr("Set Up") : tr("Apply"), QDialogButtonBox::AcceptRole);
    buttons->addButton(firstLaunch ? tr("Not Now") : tr("Cancel"), QDialogButtonBox::RejectRole);
    ok->setDefault(true);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        if (apply())
            accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, [this] {
        if (m_dontAsk && m_dontAsk->isChecked())
            QSettings().setValue(QLatin1String(kAnsweredKey), true);
        reject();
    });
}

bool IntegrationDialog::apply()
{
    QString error;
    // Menu first when adding (the default needs it), default first when removing.
    bool ok = true;
    if (m_menu && m_menu->isChecked())
        ok = DesktopIntegration::setInMenu(true, &error);
    if (ok)
        ok = DesktopIntegration::setDefaultForMarkdown(m_markdown->isChecked(), &error);
    if (ok && m_menu && !m_menu->isChecked())
        ok = DesktopIntegration::setInMenu(false, &error);

    if (!ok) {
        QMessageBox::warning(this, windowTitle(), tr("Could not change the desktop settings.\n\n%1").arg(error));
        return false;
    }
    QSettings().setValue(QLatin1String(kAnsweredKey), true);
    return true;
}
