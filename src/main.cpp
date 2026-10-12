#include <QApplication>
#include <QCommandLineParser>
#include <QFileOpenEvent>
#include <QTimer>

#include "app/MainWindow.h"
#include "integration/DesktopIntegration.h"
#include "integration/IntegrationDialog.h"

namespace {

// macOS delivers files opened from Finder (double-click, "Open With") as FileOpen events.
class FileOpenFilter : public QObject {
public:
    explicit FileOpenFilter(MainWindow *window) : QObject(window), m_window(window) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::FileOpen) {
            m_window->openFile(static_cast<QFileOpenEvent *>(event)->file());
            return true;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    MainWindow *m_window;
};

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("meandriswrite"));
    QApplication::setOrganizationName(QStringLiteral("meandriswrite")); // settings: ~/.config/meandriswrite/
    QApplication::setApplicationVersion(QStringLiteral(MEANDRISWRITE_VERSION));
    QApplication::setDesktopFileName(QStringLiteral("meandriswrite")); // Wayland: match the menu entry's icon

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Lightweight text and Markdown editor"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("files"), QStringLiteral("Files to open."), QStringLiteral("[files...]"));
    parser.process(app);

    DesktopIntegration::refreshMenuEntry();

    MainWindow window;
    app.installEventFilter(new FileOpenFilter(&window));

    window.newDocument();
    for (const QString &path : parser.positionalArguments())
        window.openFile(path);

    window.show();
    QTimer::singleShot(0, &window, [&window] { IntegrationDialog::askOnFirstLaunch(&window); });
    return app.exec();
}
