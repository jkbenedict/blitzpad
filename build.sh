#!/bin/bash
# BlitzPad build and install script.
#
#   ./build.sh                interactive: checks packages, builds, tests, offers to install
#   ./build.sh -y             answer "yes" to every question (unattended)
#   ./build.sh --no-install   build only; don't offer to install (./install.sh does that)
#   ./build.sh --skip-deps    don't check packages (./install.sh has already done it)
#
# Supported: Zorin OS, Ubuntu, Debian, Linux Mint, Pop!_OS (apt) · Fedora (dnf)
#            AlmaLinux, Rocky, RHEL, CentOS Stream, Oracle Linux 9+ (dnf + EPEL + CRB)
#            Arch, Manjaro, EndeavourOS (pacman) · openSUSE (zypper)
set -euo pipefail
cd "$(dirname "$0")"

ASSUME_YES=0
OFFER_INSTALL=1
CHECK_DEPS=1
for arg in "$@"; do
    case "$arg" in
        -y|--yes) ASSUME_YES=1 ;;
        --no-install) OFFER_INSTALL=0 ;;
        --skip-deps) CHECK_DEPS=0 ;;
        -h|--help) sed -n '2,11p' "$0"; exit 0 ;;
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

# ---- 1-2. Check packages and offer to install the missing ones (deps.sh) ----
. ./deps.sh
if [ "$CHECK_DEPS" = 1 ]; then
    deps_main
    if [ "$DEPS_OK" = 0 ]; then
        echo "BlitzPad can't be built until the required packages above are installed."
        exit 1
    fi
fi

# No QScintilla package (some AlmaLinux / RHEL releases): build it from source.
cmake_args=(-DCMAKE_BUILD_TYPE=Release -DQSCINTILLA_SOURCE_DIR=)
if ! deps_have_system_qsci; then
    deps_fetch_qscintilla || { echo "BlitzPad needs QScintilla for Qt 6 to build."; exit 1; }
    echo "Using the QScintilla source in $QSCI_SOURCE_DIR (built into BlitzPad, no extra library)."
    cmake_args=(-DCMAKE_BUILD_TYPE=Release -DQSCINTILLA_SOURCE_DIR="$QSCI_SOURCE_DIR")
fi

# ---- 3. Build and test ------------------------------------------------------
bold "Building..."
cmake -S . -B build "${cmake_args[@]}"
cmake --build build -j"$(nproc)"
bold "Running tests..."
ctest --test-dir build --output-on-failure
echo
bold "Built $(./build/blitzpad --version)."

# ---- 4. Offer to install ----------------------------------------------------
# install.sh puts the program, its icon and its menu entry in the right places,
# for just you (~/.local) or for everyone (/usr).
if [ "$OFFER_INSTALL" = 1 ]; then
    if ask "Install BlitzPad now (program, icon and applications-menu entry)?"; then
        if [ "$ASSUME_YES" = 1 ]; then bash ./install.sh --skip-build --no-package -y
        else bash ./install.sh --skip-build --no-package; fi
    else
        echo "Not installed. Run it from here with ./build/blitzpad, or install later with ./install.sh"
    fi
fi
echo "Settings are saved in ~/.blitzpad/preferences.ini"
