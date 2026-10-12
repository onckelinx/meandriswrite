#include "integration/DesktopIntegration.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>

namespace DesktopIntegration {

namespace {

const char *const kMarkdownMime = "text/markdown";
const char *const kDefaultsSection = "[Default Applications]";
const char *const kPreviousDefaultKey = "integration/previousMarkdownApp";

QString applicationsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/applications");
}

QString menuEntryPath()
{
    return applicationsDir() + QLatin1Char('/') + desktopFileId();
}

QString iconPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/icons/hicolor/256x256/apps/meandriswrite.png");
}

QString appImagePath()
{
    return qEnvironmentVariable("APPIMAGE");
}

QByteArray readResource(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

bool writeFile(const QString &path, const QByteArray &bytes, QString *error)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit())
        return true;
    if (error)
        *error = file.errorString();
    return false;
}

// Lets the desktop pick up new menu entries and "Open With" choices right away.
void notifyDesktop()
{
    QProcess::startDetached(QStringLiteral("update-desktop-database"), {applicationsDir()});
}

// The user's mimeapps.list files, highest priority first: desktop-specific ones
// (e.g. kde-mimeapps.list) before the shared mimeapps.list.
QStringList mimeappsFiles()
{
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QStringList files;
    const QStringList desktops = qEnvironmentVariable("XDG_CURRENT_DESKTOP").split(QLatin1Char(':'), Qt::SkipEmptyParts);
    for (const QString &desktop : desktops)
        files << configDir + QLatin1Char('/') + desktop.toLower() + QStringLiteral("-mimeapps.list");
    files << configDir + QStringLiteral("/mimeapps.list");
    return files;
}

// The first desktop file id listed for the MIME type, or an empty string when the file has none.
// found tells whether the file mentions the type at all.
QString readDefault(const QString &path, bool *found)
{
    *found = false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    const QString key = QLatin1String(kMarkdownMime) + QLatin1Char('=');
    bool inSection = false;
    for (const QString &rawLine : QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'))) {
        const QString line = rawLine.trimmed();
        if (line.startsWith(QLatin1Char('[')))
            inSection = line == QLatin1String(kDefaultsSection);
        else if (inSection && line.startsWith(key)) {
            *found = true;
            return line.mid(key.size()).split(QLatin1Char(';'), Qt::SkipEmptyParts).value(0);
        }
    }
    return {};
}

// Sets the default app for Markdown in one mimeapps.list; an empty id removes the entry.
bool writeDefault(const QString &path, const QString &id, QString *error)
{
    QStringList lines;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
    file.close();
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty())
        lines.removeLast();

    const QString key = QLatin1String(kMarkdownMime) + QLatin1Char('=');
    const QString entry = key + id + QLatin1Char(';');
    int section = -1;
    int sectionEnd = lines.size();
    int existing = -1;
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines[i].trimmed();
        if (line.startsWith(QLatin1Char('['))) {
            if (section >= 0) {
                sectionEnd = i;
                break;
            }
            if (line == QLatin1String(kDefaultsSection))
                section = i;
        } else if (section >= 0 && line.startsWith(key)) {
            existing = i;
        }
    }

    if (existing >= 0) {
        if (id.isEmpty())
            lines.removeAt(existing);
        else
            lines[existing] = entry;
    } else if (!id.isEmpty()) {
        if (section < 0) {
            if (!lines.isEmpty())
                lines << QString();
            lines << QLatin1String(kDefaultsSection) << entry;
        } else {
            // Keep the section's trailing blank lines after the new entry.
            int insertAt = sectionEnd;
            while (insertAt > section + 1 && lines[insertAt - 1].trimmed().isEmpty())
                --insertAt;
            lines.insert(insertAt, entry);
        }
    } else {
        return true;
    }
    return writeFile(path, (lines.join(QLatin1Char('\n')) + QLatin1Char('\n')).toUtf8(), error);
}

QString currentMarkdownDefault()
{
    for (const QString &path : mimeappsFiles()) {
        bool found = false;
        const QString id = readDefault(path, &found);
        if (found)
            return id;
    }
    return {};
}

} // namespace

InstallKind installKind()
{
#ifdef Q_OS_LINUX
    if (!appImagePath().isEmpty())
        return InstallKind::AppImage;
    if (QCoreApplication::applicationDirPath().startsWith(QLatin1String("/usr/")))
        return InstallKind::Package;
#endif
    return InstallKind::None;
}

QString desktopFileId()
{
    return QStringLiteral("meandriswrite.desktop");
}

bool isInMenu()
{
    return QFileInfo::exists(menuEntryPath());
}

QString desktopEntry(const QString &executable)
{
    // Exec arguments with spaces or special characters must be quoted (Desktop Entry spec).
    QString quoted = executable;
    for (const char *special : {"\\", "\"", "`", "$"})
        quoted.replace(QLatin1String(special), QLatin1Char('\\') + QLatin1String(special));
    const QString exec = QStringLiteral("Exec=\"%1\" %F").arg(quoted);

    QStringList lines = QString::fromUtf8(readResource(QStringLiteral(":/meandriswrite.desktop"))).split(QLatin1Char('\n'));
    for (QString &line : lines) {
        if (line.startsWith(QLatin1String("Exec=")))
            line = exec;
    }
    return lines.join(QLatin1Char('\n'));
}

bool setInMenu(bool on, QString *error)
{
    if (!on) {
        // Default apps must have a menu entry, so that setting goes too.
        if (isDefaultForMarkdown() && !setDefaultForMarkdown(false, error))
            return false;
        QFile::remove(menuEntryPath());
        QFile::remove(iconPath());
        notifyDesktop();
        return true;
    }

    const QString appImage = appImagePath();
    if (appImage.isEmpty()) {
        if (error)
            *error = QCoreApplication::translate("DesktopIntegration", "Not running from an AppImage.");
        return false;
    }
    if (!writeFile(iconPath(), readResource(QStringLiteral(":/meandriswrite.png")), error)
        || !writeFile(menuEntryPath(), desktopEntry(appImage).toUtf8(), error))
        return false;
    notifyDesktop();
    return true;
}

void refreshMenuEntry()
{
    if (installKind() != InstallKind::AppImage || !isInMenu())
        return;
    const QByteArray wanted = desktopEntry(appImagePath()).toUtf8();
    QFile file(menuEntryPath());
    if (file.open(QIODevice::ReadOnly) && file.readAll() == wanted)
        return;
    file.close();
    if (writeFile(menuEntryPath(), wanted, nullptr))
        notifyDesktop();
}

bool isDefaultForMarkdown()
{
    return currentMarkdownDefault() == desktopFileId();
}

bool setDefaultForMarkdown(bool on, QString *error)
{
    if (on == isDefaultForMarkdown())
        return true;

    QSettings settings;
    QString id;
    if (on) {
        settings.setValue(QLatin1String(kPreviousDefaultKey), currentMarkdownDefault());
        id = desktopFileId();
    } else {
        id = settings.value(QLatin1String(kPreviousDefaultKey)).toString();
        settings.remove(QLatin1String(kPreviousDefaultKey));
    }

    // Always write the shared file; update desktop-specific files only where they already
    // set Markdown, since they take priority over it.
    const QStringList files = mimeappsFiles();
    for (const QString &path : files) {
        bool found = false;
        readDefault(path, &found);
        if ((found || path == files.last()) && !writeDefault(path, id, error))
            return false;
    }
    notifyDesktop();
    return true;
}

} // namespace DesktopIntegration
