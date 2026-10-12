# Sourced by build.sh and packaging/package-windows.sh on Windows (Git Bash).
# Puts Qt, the MinGW compiler, CMake and Ninja on PATH, as installed by aqtinstall into C:\Qt
# (see the README, "Build from source"). Set QT_ROOT to use another Qt folder.
qt_root="$(cygpath -u "${QT_ROOT:-C:/Qt/6.8.3/mingw_64}")"
qt_tools="$(dirname "$(dirname "$qt_root")")/Tools"
export PATH="$qt_root/bin:$qt_tools/mingw1310_64/bin:$qt_tools/CMake_64/bin:$qt_tools/Ninja:$PATH"

# Copies the Qt DLLs, plugins and MinGW runtime DLLs next to an exe, so it runs on any Windows.
# The TUIO touch plugin is left out: it only drags in Qt Network.
deploy_qt() {
    windeployqt --no-translations --no-system-d3d-compiler --no-system-dxc-compiler --no-opengl-sw \
        --compiler-runtime --exclude-plugins qtuiotouchplugin "$1" >/dev/null
}
