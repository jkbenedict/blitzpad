# Changelog

All notable changes to BlitzPad are listed here, newest first.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
versions follow [Semantic Versioning](https://semver.org/). Every release zip bumps
the minor version (`package.sh` does this automatically).

## [0.11.0] - 2026-11-18

### Fixed
- **README icon missing on GitHub.** Release zips used to wrap everything in a
  `blitzpad/` folder, so unzipping into a repository put the README's images one level too
  deep. Zips now hold the files at the top level, so unzipping into the repository puts
  `README.md`, `CHANGELOG.md`, `data/` and `docs/` at its root.

### Changed
- The README header uses the real app icon (`data/icons/blitzpad-256.png`).
- README: install command for the flat zip (`unzip blitzpad-0.11.zip -d blitzpad`), plus
  steps for updating the GitHub repository from a release zip.

## [0.10.0] - 2026-11-18

The beta release. (Version 0.9 was skipped on purpose.)

### Changed
- **README.md is now the GitHub landing page**: icon, badges, a beta notice, a feature
  overview, screenshots (`docs/screenshots/`), install guide, supported systems, and the
  QA checklist for **Build 1.0**.
- Release zips now include the `docs/` folder.

## [0.8.0] - 2026-11-18

### Changed
- **About window rearranged**, top to bottom: "-- blitzpad --", the 64x64 BlitzPad icon,
  "Version x.y.z (HH:MM DD-Mon-YYYY)" with the build time in 24-hour format,
  "By JK Benedict" with the GitHub link, the new motto ("I was tired of a slow
  TextEditor, so I worked fast for faster work, later!"), and an OKAY button.
- The build time is now recorded to the minute (it was the date only).

## [0.7.0] - 2026-11-18

### Added
- **BlitzPad icon**: a page with a lightning bolt, as a scalable SVG plus PNGs at
  16, 24, 32, 48, 64, 128, 256 and 512 px (`data/icons/`). It is also built into the
  program, so the window has its icon even before installing.
- **`install.sh`** (executable). It runs `build.sh` (packages, build, tests), offers to run
  `package.sh` for a release zip (off by default, because that raises the version), and
  installs either:
  - *just for you*: `~/.local/bin`, `~/.local/share/applications`,
    `~/.local/share/icons/hicolor/<size>/apps`, or
  - *for all users* (sudo): `/usr/bin`, `/usr/share/applications`,
    `/usr/share/icons/hicolor/<size>/apps`, `/usr/share/pixmaps`.

  Files are installed readable by everyone. It then runs `gtk-update-icon-cache` and
  `update-desktop-database`. Options: `--user`, `--system`, `-y`, `--skip-build`,
  `--package`, `--uninstall`.
- `cmake --install` now installs the icons too.

### Changed
- The menu entry (`blitzpad.desktop`) uses the BlitzPad icon instead of the generic
  text-editor one. It also gains `StartupWMClass`, `TryExec`, keywords and more file
  types, and passes `desktop-file-validate`.
- The app tells the desktop which `.desktop` file it belongs to, so GNOME, KDE and
  others (including on Wayland) show the BlitzPad icon in the dock and taskbar.
- `build.sh` no longer installs by itself. Its last question now runs `install.sh`.

## [0.6.0] - 2026-11-18

### Added
- **File format detection** when a file opens: by extension, by known file names
  (Makefile, CMakeLists.txt, LICENSE...), and by content (`#!` lines, `<?xml`,
  HTML doctype, diffs). Files with no extension open as plain text.
- **Unknown formats prompt**: if a file's extension isn't recognised, BlitzPad asks
  whether to open it as **Plain Text** or **Create File Format...**. Cancel opens it as
  plain text. "Always open .xyz files as plain text" stops the question for that
  extension (stored in `[file-types]` in preferences.ini).
- **Options > Create File Format...**: define your own language without code.
  - *General*: name, file extensions, case-sensitive keywords.
  - *Grammar & Symbols*: line comment, block comment start/end, string quote
    characters, escape character, operators/symbols, number highlighting, two
    keyword lists.
  - *Flow*: code folding on opening/closing tokens (symbols like `{ }` or words like
    `begin end`), or folding by indentation (Python/YAML style).
  - *Colours*: colour and bold per style.
  - Live preview, plus editing and deleting existing formats.
  Formats are saved as `~/.blitzpad/languages/<name>.ini`, appear under "Your formats"
  in the Language menu, are detected by extension, and work with Grammar Colour
  Overrides and Toggle Comment.
- **Start-up check of `~/.blitzpad/`**: every start creates `~/.blitzpad/`,
  `~/.blitzpad/languages/` (with a README.txt describing the format files) and a fully
  populated `preferences.ini` if any of them are missing, and fills in any missing
  setting with its default.

### Changed
- Toggle Comment uses each language's own comment marker (`//`, `#`, `;`, `--`, or
  your format's), and says so in the status bar when a language has none.
- More plain-text file types: `.out`, `.nfo`, and LICENSE, COPYING, AUTHORS, TODO, NEWS.

### Fixed
- Toggle Comment put the marker in the wrong column on lines with trailing spaces.

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
