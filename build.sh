#!/bin/bash
# BlitzPad build and install script.
#
#   ./build.sh                interactive: checks packages, builds, tests, offers to install
#   ./build.sh -y             answer "yes" to every question (unattended)
#   ./build.sh --no-install   build only; never ask about /usr/bin
#
# Supported: Zorin OS, Ubuntu, Debian, Linux Mint, Pop!_OS (apt) · Fedora (dnf)
#            Arch, Manjaro, EndeavourOS (pacman) · openSUSE (zypper)
set -euo pipefail
cd "$(dirname "$0")"

ASSUME_YES=0
OFFER_INSTALL=1
for arg in "$@"; do
    case "$arg" in
        -y|--yes) ASSUME_YES=1 ;;
        --no-install) OFFER_INSTALL=0 ;;
        -h|--help) sed -n '2,9p' "$0"; exit 0 ;;
        *) echo "Unknown option: $arg"; exit 1 ;;
    esac
done

bold() { printf '\033[1m%s\033[0m\n' "$*"; }
# ask "Question?" -> 0 for yes. Default is Yes; with no terminal it is No unless -y.
ask() {
    if [ "$ASSUME_YES" = 1 ]; then echo "$1 [Y/n] y"; return 0; fi
    if [ ! -t 0 ]; then echo "$1 [Y/n] n (no terminal)"; return 1; fi
    local reply
    read -r -p "$1 [Y/n] " reply
    case "${reply:-y}" in [Yy]*) return 0 ;; *) return 1 ;; esac
}

# ---- 1. Work out the OS and its package manager -----------------------------
ID=unknown; ID_LIKE=""; PRETTY_NAME=""
[ -r /etc/os-release ] && . /etc/os-release
family=""
for id in $ID ${ID_LIKE:-}; do
    case "$id" in
        debian|ubuntu) family=apt; break ;;
        fedora|rhel|centos) family=dnf; break ;;
        arch) family=pacman; break ;;
        opensuse*|suse|sles) family=zypper; break ;;
    esac
done
bold "BlitzPad build on ${PRETTY_NAME:-$ID}"

# Build tools and libraries, then runtime extras: Wayland support, 7-Zip for
# Save Compressed, monospace fonts with wide Unicode coverage, UTF-8 locales.
case "$family" in
  apt)
    BUILD_PKGS="build-essential cmake qt6-base-dev libqscintilla2-qt6-dev"
    RUN_PKGS="qt6-wayland p7zip-full fonts-dejavu-core fonts-noto-mono locales"
    is_installed() { dpkg-query -W -f='${Status}' "$1" 2>/dev/null | grep -q "install ok installed"; }
    install_pkgs() { sudo apt-get update && sudo apt-get install -y "$@"; } ;;
  dnf)
    BUILD_PKGS="gcc-c++ make cmake qt6-qtbase-devel qscintilla-qt6-devel"
    RUN_PKGS="qt6-qtwayland p7zip p7zip-plugins dejavu-sans-mono-fonts google-noto-sans-mono-fonts glibc-langpack-en"
    is_installed() { rpm -q "$1" >/dev/null 2>&1; }
    install_pkgs() { sudo dnf install -y "$@"; } ;;
  pacman)
    BUILD_PKGS="base-devel cmake qt6-base qscintilla-qt6"
    RUN_PKGS="qt6-wayland 7zip ttf-dejavu noto-fonts"
    is_installed() { pacman -Qi "$1" >/dev/null 2>&1; }
    install_pkgs() { sudo pacman -S --needed --noconfirm "$@"; } ;;
  zypper)
    BUILD_PKGS="gcc-c++ make cmake qt6-base-devel qscintilla-qt6-devel"
    RUN_PKGS="qt6-wayland 7zip dejavu-fonts google-noto-sans-mono-fonts glibc-locale"
    is_installed() { rpm -q "$1" >/dev/null 2>&1; }
    install_pkgs() { sudo zypper --non-interactive install "$@"; } ;;
  *)
    echo "build.sh doesn't know this Linux. Install these yourself, then run it again:"
    echo "  a C++17 compiler, CMake 3.16+, Qt 6 Widgets (dev), QScintilla for Qt 6 (dev),"
    echo "  the Qt 6 Wayland plugin, 7-Zip, DejaVu or Noto monospace fonts, a UTF-8 locale."
    BUILD_PKGS=""; RUN_PKGS=""
    is_installed() { return 0; }
    install_pkgs() { :; } ;;
esac

# ---- 2. Check for missing packages and offer to install them -----------------
missing=()
for p in $BUILD_PKGS $RUN_PKGS; do
    is_installed "$p" || missing+=("$p")
done
if [ ${#missing[@]} -gt 0 ]; then
    bold "Missing packages: ${missing[*]}"
    echo "Build tools and Qt/QScintilla are required. The rest add Wayland support,"
    echo "Save Compressed (7-Zip), good monospace fonts, and UTF-8 locale data."
    if ask "Install them now with your package manager? (needs your password)"; then
        install_pkgs "${missing[@]}"
    else
        echo "Skipping. The build will fail if a required package is missing."
    fi
else
    echo "All needed packages are already installed."
fi

# A UTF-8 locale keeps file names and non-English text showing correctly.
if command -v locale >/dev/null && ! locale -a 2>/dev/null | grep -qiE 'utf-?8'; then
    bold "No UTF-8 locale is set up on this system."
    if [ "$family" = apt ] && ask "Generate en_US.UTF-8 now?"; then
        sudo locale-gen en_US.UTF-8
    else
        echo "Set one up with your system's locale settings if text looks wrong."
    fi
fi

# ---- 3. Build and test ------------------------------------------------------
bold "Building..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
bold "Running tests..."
ctest --test-dir build --output-on-failure
echo
bold "Built $(./build/blitzpad --version)."

# ---- 4. Offer to install to /usr/bin -----------------------------------------
if [ "$OFFER_INSTALL" = 1 ]; then
    if ask "Install BlitzPad to /usr/bin and add it to the applications menu?"; then
        sudo install -Dm755 build/blitzpad /usr/bin/blitzpad
        sudo install -Dm644 data/blitzpad.desktop /usr/share/applications/blitzpad.desktop
        if command -v update-desktop-database >/dev/null; then
            sudo update-desktop-database -q /usr/share/applications || true
        fi
        bold "Installed. Start it from the applications menu or type: blitzpad"
        echo "To remove it later: sudo rm /usr/bin/blitzpad /usr/share/applications/blitzpad.desktop"
    else
        echo "Not installed. Run it from here with: ./build/blitzpad"
    fi
fi
echo "Settings are saved in ~/.blitzpad/preferences.ini"
