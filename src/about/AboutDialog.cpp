#include "about/AboutDialog.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QPixmap>
#include <QWidget>

namespace AboutDialog {

namespace {

const int kLogoWidth = 160; // logical pixels; logo.png is twice as wide, for HiDPI screens

} // namespace

void show(QWidget *parent)
{
    QMessageBox box(parent);
    box.setWindowTitle(QCoreApplication::translate("AboutDialog", "About Meandris Write"));

    const qreal ratio = parent->devicePixelRatio();
    QPixmap logo = QPixmap(QStringLiteral(":/logo.png"))
                       .scaledToWidth(qRound(kLogoWidth * ratio), Qt::SmoothTransformation);
    logo.setDevicePixelRatio(ratio);
    box.setIconPixmap(logo);

    const QString version =
        QCoreApplication::translate("AboutDialog", "Version %1").arg(QCoreApplication::applicationVersion());
    box.setTextFormat(Qt::RichText);
    box.setTextInteractionFlags(Qt::TextBrowserInteraction); // the link opens in the browser
    box.setText(QStringLiteral("<h2>Meandris Write</h2>"
                               "<p>%1<br>%2</p>"
                               "<p>© 2026 Rik Onckelinx - meandris.ai</p>"
                               "<p>%3<br>"
                               "<a href=\"https://github.com/onckelinx/meandriswrite\">"
                               "github.com/onckelinx/meandriswrite</a></p>")
                    .arg(version,
                         QCoreApplication::translate("AboutDialog", "Lightweight text and Markdown editor"),
                         QCoreApplication::translate("AboutDialog", "Free software under the GNU GPL v3 or later.")));
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

} // namespace AboutDialog
