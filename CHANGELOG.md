# Changelog

All notable changes to BlitzPad are listed here, newest first.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
versions follow [Semantic Versioning](https://semver.org/). Every release zip bumps
the minor version (`package.sh` does this automatically).

## [0.22.0] - 2026-11-27 (beta)

### Added
- README: a **"Thank you to Them Crooked Vultures"** section, with links to "Reptiles" and
  their self-titled 2009 album on Spotify, Apple Music, Shazam, YouTube Music, Qobuz and
  Last.fm.

## [0.21.0] - 2026-11-27 (beta)

### Added
- **Smart auto-indent on Enter** (with `auto-indent = true` in `[editor]`, the default):
  - **Your tabs are kept.** If you indented a line with the Tab key, Enter carries that
    indentation onto the next line as tabs, even when `use-tabs` is off (before, it was
    turned into spaces).
  - **A raw Enter ends the indent.** When Enter has just indented a new line and you
    press Enter again without typing anything there, the generated tabs are removed: the
    blank line is left truly empty (no stray tab), and the next line starts at column 0.
    One Ctrl+Z undoes it. Anything you type on the line, including your own Tab, keeps
    the normal behaviour.
  - Works for space indentation the same way.
- A `smartindent` test (ctest) covering tabs kept, raw Enter, typed tabs, spaces, undo
  and auto-indent off.

### Changed
- README and MANIFEST.md: BlitzPad is tested on **Debian-based** (Zorin OS 18) and
  **RHEL-based** (AlmaLinux 9 / 10) systems.

## [0.20.0] - 2026-11-27 (beta)

### Changed
- README: BlitzPad is shown as tested on **AlmaLinux 9 / 10** (RHEL family) as well as
  Zorin OS 18, with badges, the beta notice and the 1.0 checklist updated. Rocky Linux,
  RHEL, CentOS Stream and Oracle Linux are listed as expected to work, matching MANIFEST.md.
- README: a new **"On AlmaLinux and other RHEL-based systems"** section with both ways to
  install there: `install.sh`, or `packaging/build-rpm.sh` then `sudo dnf install`.

## [0.19.0] - 2026-11-27 (beta)

### Changed
- README: the icon at the top now uses `data/icons/blitzpad-32.png`, shown at its real size.

## [0.18.0] - 2026-11-27 (beta)

### Added
- **`.deb` package** for Zorin OS 18 / Ubuntu 24.04+ / Mint 22+ / Debian 13, built by
  `packaging/build-deb.sh`. Its dependencies come from the real binary (`dpkg-shlibdeps`),
  so `sudo apt install ./blitzpad_*.deb` pulls in Qt 6 and QScintilla.
- **`.rpm` package** for AlmaLinux / Rocky / RHEL, from `packaging/blitzpad.spec` and
  `packaging/build-rpm.sh`. QScintilla is compiled in, so the `.rpm` needs only Qt 6 from
  AppStream (no EPEL). The beta ships the source `.rpm`; `build-rpm.sh` turns it into the
  installable `.rpm` on AlmaLinux.
- **MANIFEST.md**: the dependencies for each distribution, what each install method puts
  where, and which distributions are tested (Zorin OS and AlmaLinux) or expected to work.
- `install.sh --prefix=DIR` to choose where an all-users install goes.

### Changed
- `install.sh` now asks **"Install BlitzPad for all users on this computer?"** (Yes by
  default). All-users installs go to `/usr/local/bin`, `/usr/local/share/applications` and
  `/usr/local/share/icons` on the distros the installer knows, since their rules keep
  `/usr/bin` for the package manager; on an unknown system it uses `/usr`.
- Installing for all users removes an older 0.17-or-earlier copy in `/usr`, and
  `--uninstall --system` removes both. Files owned by a distro package are never removed.
- `install.sh -y` now installs for all users (before, it installed just for you).

## [0.17.0] - 2026-11-27

### Changed
- **New icon** again: a blue pixel-art "B" with a text cursor beside it, replacing the 0.16
  icon. All sizes, the SVG and `blitzpad-icon.png` are made from it the same way as in 0.16.

## [0.16.0] - 2026-11-27

### Changed
- **New icon**: a blue pixel-art "B". Every size in `data/icons/` (16 to 512 px) is made
  from the 32 px original. Larger sizes are scaled with hard pixel edges, so they stay
  sharp. `blitzpad.svg` draws the same pixels as vector squares, so it is sharp at any size.

## [0.15.0] - 2026-11-27

### Added
- **AlmaLinux, Rocky Linux, RHEL, CentOS Stream and Oracle Linux 9 / 10** support in the
  installer. It enables the **EPEL** and **CRB** repositories (after asking), then installs
  the packages with `dnf`.
- **`deps.sh`**, a dependency checker shared by `install.sh` and `build.sh`. It detects the
  distribution, lists missing packages (required and recommended separately), asks once,
  and installs them with `sudo` (or directly when run as root). It checks afterwards that
  the required packages really arrived, and stops with a clear message if they didn't.
  Run it alone with `./deps.sh`, or `./deps.sh --check` to only list what's missing.
- **QScintilla from source**: where a distribution has no QScintilla for Qt 6 package,
  `build.sh` downloads QScintilla 2.14.1 (checksum-checked) into `third_party/` and CMake
  compiles it into BlitzPad as a static library (`-DQSCINTILLA_SOURCE_DIR=...`).
- Packages that may go by two names are handled either way (for example `7zip` or
  `p7zip-plugins`); the first one your repositories offer is installed.
- `install.sh --skip-deps` and `build.sh --skip-deps` to skip the package check.

### Changed
- `install.sh` now checks packages itself as its first step, before building, then runs
  `build.sh --skip-deps`. With no terminal and no `-y`, it asks nothing and installs nothing.
- `install.sh --system` no longer needs `sudo` when you are already root.

## [0.14.0] - 2026-11-19

### Added
- **Code pages and encodings**: 59 in all, for opening and saving.
  - *DOS / Terminal*: CP437, CP737, CP775, CP850, CP852, CP855, CP857, CP858, CP860 to
    CP866, CP869, CP720, CP1125, KOI8-R, KOI8-U, and EBCDIC CP037 / CP500.
  - *Windows*: 874 and 1250 to 1258. *ISO 8859*: parts 1 to 16.
  - *Unicode*: UTF-8, UTF-8 with BOM, UTF-16 LE / BE.
  - *East Asian*: Shift_JIS, EUC-JP, GBK, GB18030, Big5, EUC-KR (where Qt has ICU).
  - *Other*: Mac Roman, Mac Cyrillic.

  Single-byte code pages are built into BlitzPad, so they work on every system. Every byte
  survives opening and saving unchanged. The tables are generated by
  `tools/gen_codepages.py`.
- **Encoding menu** (between View and Language):
  - *Reopen with Encoding*;
  - *Convert to (used when saving)*;
  - *Default for Non-UTF-8 Files*, which sets the new `fallback-encoding` key in `[editor]`
    of preferences.ini.
- Files with a UTF-8 or UTF-16 byte-order mark are detected and keep their mark when saved.
- `.nfo`, `.diz` and `.ans` files open as CP437 when they aren't UTF-8.
- A warning when saving would lose characters the encoding can't hold (for example € in
  CP437). It offers "Save as UTF-8".
- An `encodings` test (ctest): CP437 round-trips, every code page round-trips all 256 bytes,
  plus BOMs, detection and lost-character counting.

### Changed
- The menu-bar counter now reads **"X Tabs | Y MBs"** (it was "X Tabs | Y MEM").

### Fixed
- Files that weren't valid UTF-8 were never detected as such, so they opened as raw bytes
  instead of Latin-1. They now open in the default encoding.

## [0.13.0] - 2026-11-19

### Added
- **PHP**: `.php`, `.phtml`, `.php3` to `.php8` and `.phps` files, plus any file whose first
  line is `<?php` or a `#!` line that runs `php`. PHP is coloured together with the HTML
  around it, so templates look right. Toggle Comment uses `//`.
- **41 new languages in all** (PHP included), for 59 in total: Assembly (NASM and MASM), AviSynth, Batch, C#,
  CoffeeScript, D, Dart, Dockerfile, EDIFACT, Fortran, Fortran 77, Gettext PO, Go, Groovy,
  IDL, Intel HEX, Kotlin, Lua, MATLAB, Motorola S-Record, Objective-C, Octave, Pascal, Perl,
  PHP, POV-Ray, PostScript, Protocol Buffers, Ruby, Rust, Scala, SPICE, Swift, Tcl,
  Tektronix HEX, TeX / LaTeX, TOML, TypeScript, Verilog, VHDL. Each has its own extensions,
  Toggle Comment marker and Grammar Colour Overrides section.
- Better auto-detection: more file names (`Dockerfile`, `Gemfile`, `Rakefile`, `Jenkinsfile`,
  `Cargo.lock`, `.env`...), `#!` lines for php, perl, ruby, lua, tclsh, octave, deno and
  others (including `#!/usr/bin/env -S ...`), and content clues such as `%!PS`,
  `\documentclass` and `@echo off`. A `.m` file starting with `#import` opens as
  Objective-C rather than MATLAB.
- A `languages` test (ctest) that checks detection and that every language has a lexer.
- README: a "Supported languages" section with every language and its extensions, and
  screenshots of PHP and the Language menu.

### Changed
- The **Language menu** is grouped into submenus: Programming, Scripting, Markup & Data,
  Build & Config, and Hardware & Low-level. Plain Text stays at the top and your own formats
  at the bottom.
- TypeScript (`.ts`, `.tsx`) is now its own language with TypeScript keywords (it used
  JavaScript's before). More extensions for existing languages, e.g. `.scss`, `.less`,
  `.vue`, `.jsonc`, `.xslt`, `.pyi`.
- Key bindings in `[shortcuts]` now also cover commands inside submenus.

## [0.12.0] - 2026-11-18

### Added
- **`LICENSE.md`**: BlitzPad is released under the **MIT License** (Copyright (c) 2026
  JK Benedict). The file also lists the third-party components (QScintilla GPL-3.0,
  Qt 6 LGPL-3.0, 7-Zip LGPL-2.1) and explains that compiled programs linking QScintilla
  are distributed under the GPL-3.0 as a whole.
- README: MIT badge, a License section, and a link in the header. Release zips include
  `LICENSE.md` at the top level.

### Changed
- The 1.0 checklist now marks the licence as done.

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
