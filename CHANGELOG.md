# Changelog

All notable changes to BlitzPad are listed here, newest first.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
versions follow [Semantic Versioning](https://semver.org/). Every release zip bumps
the minor version (`package.sh` does this automatically).

## [0.5.0] - 2026-11-18

### Added
- **About** item at the end of the menu bar. It opens a window with the version,
  build date, author link and motto, and an OK button that returns to editing.
- **"X Tabs | Y MEM"** on the far right of the menu bar. It shows open tabs and RAM use
  in megabytes, and refreshes when a tab opens or closes and every 60 seconds.
  Closing a tab hands freed memory back to the system before the numbers refresh.
- **File > Save Compressed...** saves a copy of the current tab as ZIP, 7-Zip,
  GZip, BZip2 or TAR. ZIP and 7-Zip can be password protected (AES-256; 7-Zip also
  hides the file name). Cancel at any step returns to the File menu. Needs 7-Zip.
- **Options menu (Alt+O)**:
  - *Global Colour Preferences*: foreground and background for default text,
    current line, selection, caret, line numbers, fold margin, matched brace,
    unmatched brace, indentation guides and whitespace symbols.
  - *Grammar Colour Overrides*: per language, foreground and background for every
    syntax style the language has, plus matched and unmatched (broken) braces,
    with a live preview.
  - *Edit preferences.ini* and *Reload Preferences*.
- **`~/.blitzpad/preferences.ini`** stores every setting: colours, per-language
  colours, key bindings for every menu command, editor options (font, font size,
  tab width, tabs or spaces, line numbers, folding, brace matching, auto-indent,
  indentation guides, current-line highlight, caret width, long-line marker),
  view options and window size. The file lists all of them after the first start,
  so you can change anything by hand.
- **Interactive `build.sh`**: detects the distribution (apt, dnf, pacman, zypper),
  lists missing packages (build tools, Qt 6, QScintilla, Qt Wayland plugin, 7-Zip,
  fonts, locale data) and offers to install them. It also offers to set up a UTF-8
  locale if none exists, builds, runs the tests, and asks whether to install to
  `/usr/bin` with a menu entry. `-y` answers yes to everything; `--no-install`
  skips the install question.
- Automated tests for Save Compressed (every format, overwriting, passwords).
- `CHANGELOG.md` (this file).

### Changed
- **Exit** is now Alt+F4. Ctrl+Q was assigned to both Exit and Toggle Comment, so
  neither worked; Ctrl+Q is now only Toggle Comment, as in Notepad++.
- **Redo** also accepts Ctrl+Y (as well as Ctrl+Shift+Z).
- Settings moved from Qt's default location to `~/.blitzpad/preferences.ini`.
- README rewritten: what BlitzPad is, supported systems and required packages.

## [0.4.0] - 2026-11-18

### Fixed
- **Save / Save As dialog never appeared** on some GNOME/Zorin setups. BlitzPad
  now uses Qt's own file dialog by default. *View > Use System File Dialogs*
  switches back to the desktop's dialog.

### Added
- Save As shortcuts: Ctrl+Shift+S and Ctrl+Alt+S (the Notepad++ one).

## [0.3.0] - 2026-11-17

### Changed
- Renamed the project from FastPad to **BlitzPad**: program, binary, menu entry,
  scripts and documentation.
- The version is now set in one place (`CMakeLists.txt`), and `blitzpad --version`
  prints it.

### Added
- `package.sh`: bumps the version, builds, tests and writes `blitzpad-<version>.zip`.

## [0.2.0] - 2026-11-17

### Added
- **Notepad++-style Find / Replace** (Ctrl+F / Ctrl+H) with Normal, Extended
  (`\n \r \t \0 \xHH \uHHHH`) and Regular expression modes. Options for match case,
  whole word and wrap around, plus search history.
- Find Next / Previous (F3 / Shift+F3), Count, Replace, Replace All (one undo step),
  Replace All in All Open Tabs, and Find All in All Open Tabs with a results panel (F7).
- Go to Line (Ctrl+G).
- Automated tests for the search engine.
- Prebuilt binary and `build.sh` in the release zip.

## [0.1.0] - 2026-11-17

### Added
- First version: a C++17 / Qt 6 editor built on QScintilla (the Qt port of
  Scintilla, the editing engine Notepad++ uses).
- Tabs: new, open (several files at once), save, save as, close with an
  unsaved-changes prompt. Ctrl+Tab / Ctrl+PgUp / Ctrl+PgDn switch tabs.
- Syntax colouring for C, C++, Bash, JavaScript/TypeScript, JSON, Python, HTML,
  CSS, XML, Markdown, Makefile, CMake, YAML, INI/config, Diff, SQL, Java and plain
  text. Detected by extension, file name or `#!` line; Language menu to override.
- Line numbers, code folding, brace matching, auto-indent, multiple cursors and
  Alt+drag column selection, duplicate line (Ctrl+D), toggle comment (Ctrl+Q),
  word wrap, zoom, and show whitespace / line endings.
- Keeps each file's encoding (UTF-8 or Latin-1) and line endings (LF, CRLF, CR).
  Saves are atomic.
- `--startup-time` flag for checking start-up speed.
