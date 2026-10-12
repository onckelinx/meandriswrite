#pragma once

#include <QString>

// Linux desktop integration: the app menu entry and the default app for Markdown files.
// Uses the freedesktop.org standards (applications folder, mimeapps.list), so it works on
// KDE, GNOME, XFCE and others, and only writes in the user's home folder.
namespace DesktopIntegration {

enum class InstallKind {
    None,     // run from a build folder (or not on Linux): no integration
    AppImage, // the menu entry is ours to add or remove
    Package,  // installed from a .deb/.rpm: the package owns the menu entry
};

InstallKind installKind();

// The desktop file id used for the menu entry and in mimeapps.list.
QString desktopFileId();

// AppImage only: the menu entry in ~/.local/share/applications, pointing at the AppImage.
bool isInMenu();
bool setInMenu(bool on, QString *error);
// The menu entry for the given executable, based on packaging/linux/meandriswrite.desktop.
QString desktopEntry(const QString &executable);
// Points the menu entry at the AppImage again if the file was moved.
void refreshMenuEntry();

bool isDefaultForMarkdown();
// Turning it off restores the app that was the default before.
bool setDefaultForMarkdown(bool on, QString *error);

} // namespace DesktopIntegration
