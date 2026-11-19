#!/bin/bash
# BlitzPad installer.
#
#   ./install.sh               build (via build.sh) and install, asking for your choices
#   ./install.sh --user        install just for you (no sudo):  ~/.local/...
#   ./install.sh --system      install for all users (sudo):    /usr/...
#   ./install.sh --skip-build  install the program already built in ./build
#   ./install.sh --package     also make a release zip with package.sh (bumps the version)
#   ./install.sh --uninstall   remove BlitzPad (asks which install, or use with --user/--system)
#   ./install.sh -y            answer yes to every question
#
# Your settings in ~/.blitzpad/ are never touched by installing or uninstalling.
set -euo pipefail
cd "$(dirname "$0")"

MODE=""; SKIP_BUILD=0; PACKAGE=ask; UNINSTALL=0; ASSUME_YES=0
for arg in "$@"; do
    case "$arg" in
        --user) MODE=user ;;
        --system) MODE=system ;;
        --skip-build) SKIP_BUILD=1 ;;
        --package) PACKAGE=yes ;;
        --no-package) PACKAGE=no ;;
        --uninstall) UNINSTALL=1 ;;
        -y|--yes) ASSUME_YES=1 ;;
        -h|--help) sed -n '2,12p' "$0"; exit 0 ;;
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
choose_mode() {
    [ -n "$MODE" ] && return
    if [ "$ASSUME_YES" = 1 ] || [ ! -t 0 ]; then MODE=user; echo "Installing for you only (use --system for all users)."; return; fi
    echo "Install BlitzPad:"
    echo "  1) Just for me         (~/.local, no password needed)"
    echo "  2) For all users       (/usr, needs sudo)"
    local reply
    read -r -p "Choose 1 or 2 [1] " reply
    case "${reply:-1}" in 2) MODE=system ;; *) MODE=user ;; esac
}

# Where things go for each mode.
set_paths() {
    if [ "$MODE" = system ]; then
        BIN_DIR=/usr/bin; APPS_DIR=/usr/share/applications
        ICON_ROOT=/usr/share/icons/hicolor; PIXMAPS=/usr/share/pixmaps; SUDO=sudo
    else
        BIN_DIR="$HOME/.local/bin"; APPS_DIR="$HOME/.local/share/applications"
        ICON_ROOT="$HOME/.local/share/icons/hicolor"; PIXMAPS=""; SUDO=""
    fi
}
SIZES="16 24 32 48 64 128 256 512"

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
    $SUDO rm -f "$BIN_DIR/blitzpad" "$APPS_DIR/blitzpad.desktop"
    for s in $SIZES; do $SUDO rm -f "$ICON_ROOT/${s}x${s}/apps/blitzpad.png"; done
    $SUDO rm -f "$ICON_ROOT/scalable/apps/blitzpad.svg"
    [ -n "$PIXMAPS" ] && $SUDO rm -f "$PIXMAPS/blitzpad.png"
    refresh_caches
    bold "Removed. Your settings in ~/.blitzpad/ were left in place."
}

if [ "$UNINSTALL" = 1 ]; then uninstall; exit 0; fi

# ---- 1. Build (checks packages, compiles, runs the tests) -------------------
if [ "$SKIP_BUILD" = 0 ]; then
    flags=(--no-install)
    [ "$ASSUME_YES" = 1 ] && flags+=(-y)
    bash ./build.sh "${flags[@]}"
fi
if [ ! -x build/blitzpad ]; then
    echo "build/blitzpad is missing. Run ./build.sh first."; exit 1
fi

# ---- 2. Optional release zip -------------------------------------------------
if [ "$PACKAGE" = ask ] && [ "$ASSUME_YES" = 0 ]; then
    ask "Also make a release zip with package.sh? (this raises the version number)" n && PACKAGE=yes || PACKAGE=no
fi
if [ "$PACKAGE" = yes ]; then
    bash ./package.sh build
fi

# ---- 3. Install ------------------------------------------------------------
choose_mode; set_paths
bold "Installing BlitzPad $(./build/blitzpad --version | awk '{print $2}') ($MODE install)..."
[ -n "$SUDO" ] && echo "(sudo will ask for your password)"

$SUDO install -Dm755 build/blitzpad "$BIN_DIR/blitzpad"
for s in $SIZES; do
    $SUDO install -Dm644 "data/icons/blitzpad-$s.png" "$ICON_ROOT/${s}x${s}/apps/blitzpad.png"
done
$SUDO install -Dm644 data/icons/blitzpad.svg "$ICON_ROOT/scalable/apps/blitzpad.svg"
[ -n "$PIXMAPS" ] && $SUDO install -Dm644 data/icons/blitzpad-128.png "$PIXMAPS/blitzpad.png"

# For a per-user install, point the menu entry at the full path, because
# ~/.local/bin isn't always on PATH for the desktop session.
tmp_desktop=$(mktemp)
if [ "$MODE" = user ]; then
    sed -e "s|^Exec=blitzpad|Exec=$BIN_DIR/blitzpad|" -e "s|^TryExec=blitzpad|TryExec=$BIN_DIR/blitzpad|" \
        data/blitzpad.desktop > "$tmp_desktop"
else
    cp data/blitzpad.desktop "$tmp_desktop"
fi
$SUDO install -Dm644 "$tmp_desktop" "$APPS_DIR/blitzpad.desktop"
rm -f "$tmp_desktop"

# A per-user icon theme folder needs this file for the icon cache.
if [ "$MODE" = user ] && [ ! -f "$ICON_ROOT/index.theme" ] && [ -f /usr/share/icons/hicolor/index.theme ]; then
    cp /usr/share/icons/hicolor/index.theme "$ICON_ROOT/index.theme"
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
if [ "$MODE" = system ] && [ -f "$HOME/.local/share/applications/blitzpad.desktop" ]; then
    echo "  Note: a per-user install also exists and takes priority in your menu."
    echo "        Remove it with: ./install.sh --uninstall --user"
fi
echo "If the icon doesn't show straight away, log out and back in."
echo "To uninstall: ./install.sh --uninstall --$MODE"
