#!/bin/bash
# BlitzPad installer.
#
#   ./install.sh               build (via build.sh) and install, asking for your choices
#   ./install.sh --user        install just for you (no sudo):  ~/.local/...
#   ./install.sh --system      install for all users (sudo):    /usr/local/... (or /usr/..., see below)
#   ./install.sh --prefix=DIR  with --system, install under DIR instead (e.g. --prefix=/usr)
#   ./install.sh --skip-build  install the program already built in ./build
#   ./install.sh --package     also make a release zip with package.sh (bumps the version)
#   ./install.sh --uninstall   remove BlitzPad (asks which install, or use with --user/--system)
#   ./install.sh --skip-deps   don't check for (or install) missing packages
#   ./install.sh -y            answer yes to every question
#
# Before building, it checks the packages BlitzPad needs (deps.sh), lists the missing
# ones, asks, and installs them with sudo: apt on Zorin / Ubuntu / Debian / Mint,
# dnf on Fedora and on AlmaLinux / Rocky / RHEL (where it also enables EPEL and CRB).
#
# All-users installs go to /usr/local (bin, share/applications, share/icons) on the
# distros the installer knows (Zorin / Ubuntu / Debian / Mint, Fedora, AlmaLinux / Rocky /
# RHEL, Arch, openSUSE): their rules keep /usr/bin for the package manager, and
# /usr/local is where software you build yourself belongs. On a system the installer
# doesn't know, or where /usr/local/bin isn't on PATH, it uses /usr instead.
# The dependencies for each distro are listed in MANIFEST.md.
#
# Your settings in ~/.blitzpad/ are never touched by installing or uninstalling.
set -euo pipefail
cd "$(dirname "$0")"

MODE=""; PREFIX=""; SKIP_BUILD=0; PACKAGE=ask; UNINSTALL=0; ASSUME_YES=0; CHECK_DEPS=1
for arg in "$@"; do
    case "$arg" in
        --user) MODE=user ;;
        --system) MODE=system ;;
        --skip-build) SKIP_BUILD=1 ;;
        --package) PACKAGE=yes ;;
        --no-package) PACKAGE=no ;;
        --uninstall) UNINSTALL=1 ;;
        --skip-deps) CHECK_DEPS=0 ;;
        -y|--yes) ASSUME_YES=1 ;;
        --prefix=*) PREFIX="${arg#--prefix=}" ;;
        -h|--help) sed -n '2,25p' "$0"; exit 0 ;;
        *) echo "Unknown option: $arg (see ./install.sh --help)"; exit 1 ;;
    esac
done

bold() { printf '\033[1m%s\033[0m\n' "$*"; }
# ask "Question?" default(y|n) -> 0 for yes
ask() {
    local def="${2:-y}" hint="[Y/n]" reply
    [ "$def" = n ] && hint="[y/N]"
    if [ "$ASSUME_YES" = 1 ]; then echo "$1 $hint y"; return 0; fi
    if [ ! -t 0 ]; then echo "$1 $hint $def (no terminal)"; [ "$def" = y ]; return; fi
    read -r -p "$1 $hint " reply
    case "${reply:-$def}" in [Yy]*) return 0 ;; *) return 1 ;; esac
}
SIZES="16 24 32 48 64 128 256 512"

# Pick the all-users prefix for this OS (see the top of this file).
. ./deps.sh
system_prefix() {
    [ -n "$PREFIX" ] && { echo "${PREFIX%/}"; return; }
    deps_detect
    if [ -n "$DEPS_FAMILY" ] && echo ":$PATH:" | grep -q ":/usr/local/bin:"; then
        echo /usr/local
    else
        echo /usr
    fi
}

choose_mode() {
    [ -n "$MODE" ] && return
    local sp; sp=$(system_prefix)
    if [ "$ASSUME_YES" = 1 ]; then
        MODE=system; echo "Install BlitzPad for all users ($sp/bin/blitzpad, needs sudo)? [Y/n] y"; return
    fi
    if [ ! -t 0 ]; then MODE=user; echo "No terminal, so installing just for you (use --system for all users)."; return; fi
    if ask "Install BlitzPad for all users on this computer? ($sp/bin/blitzpad, needs sudo; No installs just for you in ~/.local)" y; then
        MODE=system
    else
        MODE=user
    fi
}

# Where things go for each mode.
set_paths() {
    if [ "$MODE" = system ]; then
        SYS_PREFIX=$(system_prefix)
        BIN_DIR="$SYS_PREFIX/bin"; APPS_DIR="$SYS_PREFIX/share/applications"
        ICON_ROOT="$SYS_PREFIX/share/icons/hicolor"; SUDO=sudo
        # /usr/share/pixmaps is only for /usr installs; /usr/local has no pixmaps folder.
        PIXMAPS=""; [ "$SYS_PREFIX" = /usr ] && PIXMAPS=/usr/share/pixmaps
        [ "$(id -u)" = 0 ] && SUDO=""
    else
        BIN_DIR="$HOME/.local/bin"; APPS_DIR="$HOME/.local/share/applications"
        ICON_ROOT="$HOME/.local/share/icons/hicolor"; PIXMAPS=""; SUDO=""
    fi
}

# Is this file owned by a distro package? Then it isn't ours to remove.
owned_by_package() {
    { command -v dpkg >/dev/null && dpkg -S "$1" >/dev/null 2>&1; } ||
    { command -v rpm >/dev/null && rpm -qf "$1" >/dev/null 2>&1; } ||
    { command -v pacman >/dev/null && pacman -Qo "$1" >/dev/null 2>&1; }
}
# Remove an all-users install under one prefix ($1).
remove_system_files() {
    local pre="$1" s f files=()
    files=("$pre/bin/blitzpad" "$pre/share/applications/blitzpad.desktop" "$pre/share/icons/hicolor/scalable/apps/blitzpad.svg")
    for s in $SIZES; do files+=("$pre/share/icons/hicolor/${s}x${s}/apps/blitzpad.png"); done
    [ "$pre" = /usr ] && files+=(/usr/share/pixmaps/blitzpad.png)
    for f in "${files[@]}"; do
        [ -e "$f" ] || continue
        owned_by_package "$f" && { echo "  Leaving $f (it belongs to a distro package)"; continue; }
        $SUDO rm -f "$f"
    done
}
has_system_files() { [ -e "$1/bin/blitzpad" ] || [ -e "$1/share/applications/blitzpad.desktop" ]; }

refresh_caches() {
    # Tell the desktop about the new/removed icon and menu entry.
    if command -v gtk-update-icon-cache >/dev/null; then
        $SUDO gtk-update-icon-cache -f -t -q "$ICON_ROOT" 2>/dev/null || true
    fi
    if command -v update-desktop-database >/dev/null; then
        $SUDO update-desktop-database -q "$APPS_DIR" 2>/dev/null || true
    fi
    if command -v xdg-desktop-menu >/dev/null; then
        xdg-desktop-menu forceupdate 2>/dev/null || true
    fi
}

uninstall() {
    choose_mode; set_paths
    bold "Removing BlitzPad ($MODE install)..."
    if [ "$MODE" = system ]; then
        # Also clears installs made by BlitzPad 0.17 and older, which used /usr.
        local pre
        for pre in /usr/local /usr ${PREFIX:+"${PREFIX%/}"}; do
            has_system_files "$pre" || continue
            echo "  Removing from $pre"
            remove_system_files "$pre"
            APPS_DIR="$pre/share/applications"; ICON_ROOT="$pre/share/icons/hicolor"; refresh_caches
        done
    else
        rm -f "$BIN_DIR/blitzpad" "$APPS_DIR/blitzpad.desktop"
        for s in $SIZES; do rm -f "$ICON_ROOT/${s}x${s}/apps/blitzpad.png"; done
        rm -f "$ICON_ROOT/scalable/apps/blitzpad.svg"
        refresh_caches
    fi
    bold "Removed. Your settings in ~/.blitzpad/ were left in place."
}

if [ "$UNINSTALL" = 1 ]; then uninstall; exit 0; fi

# ---- 1. Check the packages BlitzPad needs, offer to install them with sudo ---
if [ "$CHECK_DEPS" = 1 ]; then
    deps_main
    if [ "$DEPS_OK" = 0 ] && [ "$SKIP_BUILD" = 0 ]; then
        echo "BlitzPad can't be built until the required packages above are installed."
        exit 1
    fi
    echo
fi

# ---- 2. Build (compiles, runs the tests) --------------------------------------
if [ "$SKIP_BUILD" = 0 ]; then
    flags=(--no-install --skip-deps)
    [ "$ASSUME_YES" = 1 ] && flags+=(-y)
    bash ./build.sh "${flags[@]}"
fi
if [ ! -x build/blitzpad ]; then
    echo "build/blitzpad is missing. Run ./build.sh first."; exit 1
fi

# ---- 3. Optional release zip -------------------------------------------------
if [ "$PACKAGE" = ask ] && [ "$ASSUME_YES" = 0 ]; then
    ask "Also make a release zip with package.sh? (this raises the version number)" n && PACKAGE=yes || PACKAGE=no
fi
if [ "$PACKAGE" = yes ]; then
    bash ./package.sh build
fi

# ---- 4. Install ------------------------------------------------------------
choose_mode; set_paths
bold "Installing BlitzPad $(./build/blitzpad --version | awk '{print $2}') ($MODE install)..."
[ -n "$SUDO" ] && echo "(sudo may ask for your password)"

$SUDO install -Dm755 build/blitzpad "$BIN_DIR/blitzpad"
for s in $SIZES; do
    $SUDO install -Dm644 "data/icons/blitzpad-$s.png" "$ICON_ROOT/${s}x${s}/apps/blitzpad.png"
done
$SUDO install -Dm644 data/icons/blitzpad.svg "$ICON_ROOT/scalable/apps/blitzpad.svg"
[ -n "$PIXMAPS" ] && $SUDO install -Dm644 data/icons/blitzpad-128.png "$PIXMAPS/blitzpad.png"

# Outside /usr/bin, point the menu entry at the full path, because ~/.local/bin (and
# on some systems /usr/local/bin) isn't always on PATH for the desktop session.
tmp_desktop=$(mktemp)
if [ "$BIN_DIR" != /usr/bin ]; then
    sed -e "s|^Exec=blitzpad|Exec=$BIN_DIR/blitzpad|" -e "s|^TryExec=blitzpad|TryExec=$BIN_DIR/blitzpad|" \
        data/blitzpad.desktop > "$tmp_desktop"
else
    cp data/blitzpad.desktop "$tmp_desktop"
fi
$SUDO install -Dm644 "$tmp_desktop" "$APPS_DIR/blitzpad.desktop"
rm -f "$tmp_desktop"

# A hicolor folder outside /usr/share needs this file for the icon cache.
if [ "$ICON_ROOT" != /usr/share/icons/hicolor ] && [ ! -f "$ICON_ROOT/index.theme" ] && [ -f /usr/share/icons/hicolor/index.theme ]; then
    $SUDO cp /usr/share/icons/hicolor/index.theme "$ICON_ROOT/index.theme"
fi
# Upgrading from BlitzPad 0.17 or older, which installed for all users into /usr:
# remove that copy so there is only one program and one menu entry.
if [ "$MODE" = system ] && [ "$SYS_PREFIX" != /usr ] && has_system_files /usr; then
    echo "Removing the older all-users install from /usr..."
    remove_system_files /usr
    ( APPS_DIR=/usr/share/applications; ICON_ROOT=/usr/share/icons/hicolor; refresh_caches )
fi
refresh_caches

bold "BlitzPad is installed."
echo "  Program:    $BIN_DIR/blitzpad"
echo "  Menu entry: $APPS_DIR/blitzpad.desktop"
echo "  Icons:      $ICON_ROOT/<size>/apps/blitzpad.png and scalable/apps/blitzpad.svg"
if [ "$MODE" = user ] && ! echo ":$PATH:" | grep -q ":$BIN_DIR:"; then
    echo "  Note: $BIN_DIR isn't on your PATH, so typing 'blitzpad' in a terminal won't work yet."
    echo "        Log out and back in (most distros add it automatically once it exists), or add it to ~/.profile."
fi
if [ "$MODE" = system ] && ! echo ":$PATH:" | grep -q ":$BIN_DIR:"; then
    echo "  Note: $BIN_DIR isn't on your PATH. The menu entry works; for the terminal, add it to PATH."
fi
if [ "$MODE" = system ] && [ -f "$HOME/.local/share/applications/blitzpad.desktop" ]; then
    echo "  Note: a per-user install also exists and takes priority in your menu."
    echo "        Remove it with: ./install.sh --uninstall --user"
fi
echo "If the icon doesn't show straight away, log out and back in."
echo "To uninstall: ./install.sh --uninstall --$MODE"
