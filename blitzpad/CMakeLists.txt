cmake_minimum_required(VERSION 3.16)
# The product version lives here only. package.sh bumps it for every new zip.
project(blitzpad VERSION 0.10.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# QScintilla (Qt port of Scintilla, the editing engine Notepad++ uses).
# Packaged on most distros: libqscintilla2-qt6-dev (Debian/Ubuntu/Zorin),
# qscintilla-qt6-devel (Fedora), qscintilla-qt6 (Arch).
find_path(QSCI_INCLUDE_DIR Qsci/qsciscintilla.h
  PATHS ${Qt6_DIR}/../../../include
  PATH_SUFFIXES qt6 x86_64-linux-gnu/qt6 aarch64-linux-gnu/qt6)
find_library(QSCI_LIBRARY NAMES qscintilla2_qt6 qscintilla2-qt6)
if(NOT QSCI_INCLUDE_DIR OR NOT QSCI_LIBRARY)
  message(FATAL_ERROR "QScintilla for Qt 6 not found. Install libqscintilla2-qt6-dev (or your distro's equivalent).")
endif()

add_executable(blitzpad
  data/icons.qrc
  src/main.cpp
  src/mainwindow.cpp
  src/mainwindow.h
  src/editor.cpp
  src/editor.h
  src/languages.cpp
  src/languages.h
  src/search.cpp
  src/search.h
  src/finddialog.cpp
  src/finddialog.h
  src/compress.cpp
  src/compress.h
  src/prefs.cpp
  src/prefs.h
  src/theme.cpp
  src/theme.h
  src/optionsdialogs.cpp
  src/optionsdialogs.h
  src/userlang.cpp
  src/userlang.h
  src/fileformatdialog.cpp
  src/fileformatdialog.h
)
target_include_directories(blitzpad PRIVATE ${QSCI_INCLUDE_DIR})
# Build time for the About window, e.g. "14:05 18-Nov-2026" (set when CMake configures).
string(TIMESTAMP BLITZPAD_BUILD_DATE "%H:%M %d-%b-%Y")
target_compile_definitions(blitzpad PRIVATE
  BLITZPAD_VERSION="${PROJECT_VERSION}"
  BLITZPAD_BUILD_DATE="${BLITZPAD_BUILD_DATE}")
target_link_libraries(blitzpad PRIVATE Qt6::Widgets ${QSCI_LIBRARY})

include(GNUInstallDirs)
install(TARGETS blitzpad RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/blitzpad.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
foreach(size 16 24 32 48 64 128 256 512)
  install(FILES data/icons/blitzpad-${size}.png
          DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps RENAME blitzpad.png)
endforeach()
install(FILES data/icons/blitzpad.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

# Tests: cmake --build build && ctest --test-dir build
enable_testing()
add_executable(search_test tests/search_test.cpp src/search.cpp src/search.h)
target_include_directories(search_test PRIVATE ${QSCI_INCLUDE_DIR})
target_link_libraries(search_test PRIVATE Qt6::Widgets ${QSCI_LIBRARY})
add_test(NAME search COMMAND search_test)
add_executable(compress_test tests/compress_test.cpp src/compress.cpp src/compress.h)
target_link_libraries(compress_test PRIVATE Qt6::Widgets)
add_test(NAME compress COMMAND compress_test)
