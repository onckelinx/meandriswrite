cmake_minimum_required(VERSION 3.19)

project(meandriswrite VERSION 1.0.0 LANGUAGES CXX
    DESCRIPTION "Lightweight text and Markdown editor"
    HOMEPAGE_URL "https://github.com/onckelinx/meandriswrite")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

option(MEANDRISWRITE_BUILD_TESTS "Build the unit tests" OFF)

# Qt 6.2 is the oldest version we support (Ubuntu 22.04).
find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets PrintSupport)

# macOS: Qt 6.8.3 links the old AGL framework, which the macOS 26 SDK no longer has.
# Nothing uses it (later Qt versions dropped it too), so leave it out.
if(APPLE AND TARGET WrapOpenGL::WrapOpenGL)
    get_target_property(gl_libs WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
    list(FILTER gl_libs EXCLUDE REGEX "(^-framework AGL|/AGL\\.framework)$")
    set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES INTERFACE_LINK_LIBRARIES "${gl_libs}")
endif()

# Code that does not need widgets lives in a small library so the tests can use it.
add_library(meandriswrite_core STATIC
    src/clipboard/ClipboardMarkdown.cpp
    src/clipboard/ClipboardMarkdown.h
    src/files/TextFile.cpp
    src/files/TextFile.h
    src/integration/DesktopIntegration.cpp
    src/integration/DesktopIntegration.h
    src/markdown/MarkdownRender.cpp
    src/markdown/MarkdownRender.h
)
target_include_directories(meandriswrite_core PUBLIC src)
target_link_libraries(meandriswrite_core PUBLIC Qt6::Gui)
# The menu entry and icon, so an AppImage can add itself to the app menu.
qt_add_resources(meandriswrite_core desktop_files
    PREFIX /
    BASE packaging/linux
    FILES packaging/linux/meandriswrite.desktop packaging/linux/meandriswrite.png
)

add_executable(meandriswrite WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/about/AboutDialog.cpp
    src/about/AboutDialog.h
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/editor/DocumentTab.cpp
    src/editor/DocumentTab.h
    src/editor/FindBar.cpp
    src/editor/FindBar.h
    src/integration/IntegrationDialog.cpp
    src/integration/IntegrationDialog.h
    src/markdown/MarkdownPreview.cpp
    src/markdown/MarkdownPreview.h
    src/print/Printing.cpp
    src/print/Printing.h
)
target_link_libraries(meandriswrite PRIVATE meandriswrite_core Qt6::Widgets Qt6::PrintSupport)
target_compile_definitions(meandriswrite PRIVATE MEANDRISWRITE_VERSION="${PROJECT_VERSION}")
# The logo in the About window.
qt_add_resources(meandriswrite about_files
    PREFIX /
    BASE src/about
    FILES src/about/logo.png
)

set_target_properties(meandriswrite PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/packaging/macos/Info.plist.in
    MACOSX_BUNDLE_GUI_IDENTIFIER io.github.onckelinx.meandriswrite
    MACOSX_BUNDLE_BUNDLE_NAME "Meandris Write"
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_ICON_FILE meandriswrite.icns
)

# macOS: the app icon goes into the bundle's Resources folder.
if(APPLE)
    set(MEANDRISWRITE_ICNS ${CMAKE_SOURCE_DIR}/packaging/macos/meandriswrite.icns)
    set_source_files_properties(${MEANDRISWRITE_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(meandriswrite PRIVATE ${MEANDRISWRITE_ICNS})
endif()

# Windows: the icon and the version info (Explorer, Task Manager).
if(WIN32)
    set(MEANDRISWRITE_ICO ${CMAKE_SOURCE_DIR}/packaging/windows/meandriswrite.ico)
    configure_file(packaging/windows/meandriswrite.rc.in meandriswrite.rc @ONLY)
    target_sources(meandriswrite PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/meandriswrite.rc)
endif()

include(GNUInstallDirs)
install(TARGETS meandriswrite
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(UNIX AND NOT APPLE)
    install(FILES packaging/linux/meandriswrite.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/linux/meandriswrite.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
    install(FILES packaging/linux/io.github.onckelinx.meandriswrite.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
endif()

# .deb and .rpm packages (cpack -G DEB / RPM), built by packaging/release-linux.sh.
set(CPACK_PACKAGE_NAME meandriswrite)
set(CPACK_PACKAGE_VENDOR "Rik Onckelinx - meandris.ai")
set(CPACK_PACKAGE_CONTACT "Rik Onckelinx - meandris.ai <${PROJECT_HOMEPAGE_URL}>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_DESCRIPTION "A very small, fast, native text and Markdown editor with a live preview.
Markdown can be shown as source, rendered, or side by side, and Paste as Markdown
turns text copied from a web page or chatbot into Markdown.")
set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
set(CPACK_RPM_PACKAGE_RELOCATABLE OFF)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
set(CPACK_STRIP_FILES ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION editors)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) # library dependencies are detected automatically
set(CPACK_DEBIAN_PACKAGE_DEPENDS qt6-qpa-plugins)
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS qt6-wayland)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_LICENSE GPL-3.0-or-later)
set(CPACK_RPM_PACKAGE_GROUP Applications/Editors)
set(CPACK_RPM_PACKAGE_URL ${PROJECT_HOMEPAGE_URL})
# Directories that belong to the system, not to this package.
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    /usr/share/applications /usr/share/icons /usr/share/icons/hicolor
    /usr/share/icons/hicolor/256x256 /usr/share/icons/hicolor/256x256/apps /usr/share/metainfo)
include(CPack)

if(MEANDRISWRITE_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(core_test tests/core_test.cpp)
    target_link_libraries(core_test PRIVATE meandriswrite_core Qt6::Test)
    if(WIN32)
        # Windows' own platform needs no screen (the offscreen one finds no fonts there), and
        # QtTest's plain text output does not reach a pipe on Windows; TAP output does.
        add_test(NAME core_test COMMAND core_test -o -,tap)
    else()
        add_test(NAME core_test COMMAND core_test)
        set_tests_properties(core_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
endif()
