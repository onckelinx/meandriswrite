# Sourced by build.sh and packaging/package-macos.sh on macOS.
# Qt 6.8.3 as installed by aqtinstall into ~/Qt (see the README, "Build from source").
# Set QT_ROOT to use another Qt folder.
qt_root="${QT_ROOT:-$HOME/Qt/6.8.3/macos}"
if [[ ! -x "$qt_root/bin/macdeployqt" ]]; then
    echo "Qt not found in $qt_root: see the README, \"Build from source\" (or set QT_ROOT)." >&2
    exit 1
fi
