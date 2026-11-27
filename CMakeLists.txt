cmake_minimum_required(VERSION 3.16)
# The product version lives here only. package.sh bumps it for every new zip.
project(blitzpad VERSION 0.22.0 LANGUAGES CXX)

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
# qscintilla-qt6-devel (Fedora, EPEL), qscintilla-qt6 (Arch).
# Where no package exists (some AlmaLinux / RHEL releases), build.sh downloads the
# QScintilla source and passes -DQSCINTILLA_SOURCE_DIR=..., and it is compiled in
# here as a static library, so the finished program needs no extra library.
set(QSCINTILLA_SOURCE_DIR "" CACHE PATH "Build QScintilla from this source tree instead of using the system one")
if(QSCINTILLA_SOURCE_DIR)
  find_package(Qt6 REQUIRED COMPONENTS PrintSupport)
  set(_q "${QSCINTILLA_SOURCE_DIR}")
  file(GLOB _qsci_sources
    "${_q}/src/*.cpp" "${_q}/scintilla/src/*.cpp"
    "${_q}/scintilla/lexers/*.cpp" "${_q}/scintilla/lexlib/*.cpp")
  list(FILTER _qsci_sources EXCLUDE REGEX "LexLPeg\\.cpp$")
  file(GLOB _qsci_headers "${_q}/src/*.h" "${_q}/src/Qsci/*.h")
  add_library(qscintilla_bundled STATIC ${_qsci_sources} ${_qsci_headers})
  target_compile_definitions(qscintilla_bundled PUBLIC SCINTILLA_QT SCI_LEXER INCLUDE_DEPRECATED_FEATURES)
  target_include_directories(qscintilla_bundled
    PUBLIC "${_q}/src"
    PRIVATE "${_q}/scintilla/include" "${_q}/scintilla/lexlib" "${_q}/scintilla/src")
  target_link_libraries(qscintilla_bundled PUBLIC Qt6::Widgets Qt6::PrintSupport)
  set_target_properties(qscintilla_bundled PROPERTIES POSITION_INDEPENDENT_CODE ON)
  target_compile_options(qscintilla_bundled PRIVATE -w)
  set(QSCI_INCLUDE_DIR "${_q}/src")
  set(QSCI_LIBRARY qscintilla_bundled)
  message(STATUS "QScintilla: building from source in ${_q}")
else()
  find_path(QSCI_INCLUDE_DIR Qsci/qsciscintilla.h
    PATHS ${Qt6_DIR}/../../../include
    PATH_SUFFIXES qt6 x86_64-linux-gnu/qt6 aarch64-linux-gnu/qt6)
  find_library(QSCI_LIBRARY NAMES qscintilla2_qt6 qscintilla2-qt6)
  if(NOT QSCI_INCLUDE_DIR OR NOT QSCI_LIBRARY)
    message(FATAL_ERROR "QScintilla for Qt 6 not found. Run ./install.sh (it installs it, or builds it "
      "from source when your distro has no package), or install libqscintilla2-qt6-dev / qscintilla-qt6-devel.")
  endif()
  message(STATUS "QScintilla: ${QSCI_LIBRARY}")
endif()

add_executable(blitzpad
  data/icons.qrc
  src/main.cpp
  src/mainwindow.cpp
  src/mainwindow.h
  src/editor.cpp
  src/editor.h
  src/smartindent.cpp
  src/smartindent.h
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
  src/encodings.cpp
  src/encodings.h
  src/codepage_tables.inc
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
add_executable(languages_test tests/languages_test.cpp src/languages.cpp src/languages.h
               src/prefs.cpp src/prefs.h src/userlang.cpp src/userlang.h)
target_include_directories(languages_test PRIVATE ${QSCI_INCLUDE_DIR})
target_link_libraries(languages_test PRIVATE Qt6::Widgets ${QSCI_LIBRARY})
add_test(NAME languages COMMAND languages_test)
add_executable(encodings_test tests/encodings_test.cpp src/encodings.cpp src/encodings.h)
target_link_libraries(encodings_test PRIVATE Qt6::Core)
add_test(NAME encodings COMMAND encodings_test)
add_executable(smartindent_test tests/smartindent_test.cpp src/smartindent.cpp src/smartindent.h)
target_include_directories(smartindent_test PRIVATE ${QSCI_INCLUDE_DIR})
target_link_libraries(smartindent_test PRIVATE Qt6::Widgets ${QSCI_LIBRARY})
add_test(NAME smartindent COMMAND smartindent_test)
