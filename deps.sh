#!/bin/bash
# BlitzPad dependency checker. Sourced by install.sh and build.sh, or run on its own:
#
#   ./deps.sh        check what BlitzPad needs, list what's missing, ask, install with sudo
#   ./deps.sh -y     the same, answering yes
#   ./deps.sh --check  only list what's missing (installs nothing; exit 1 if something required is missing)
#
# Supported: Zorin OS, Ubuntu, Debian, Linux Mint, Pop!_OS (apt) · Fedora (dnf)
#            AlmaLinux, Rocky Linux, RHEL, CentOS Stream, Oracle Linux 9/10 (dnf + EPEL + CRB)
#            Arch, Manjaro, EndeavourOS (pacman) · openSUSE (zypper)
#
# Callers set ASSUME_YES=1 to skip questions. After deps_main, QSCI_NEEDS_SOURCE=1 means
# the distro has no QScintilla package, and build.sh builds it from source instead.
# DEPS_OK=0 means a required package is still missing, so building would fail.

QSCI_VERSION=2.14.1
QSCI_SHA256=2c120a56d60363ff4bc73b8a763dc69fae5f84681c24efcb03064c7df103052b
# PyPI's copy holds the full C++ source and has a known checksum; Riverbank is the fallback.
QSCI_URLS="https://files.pythonhosted.org/packages/a9/f6/a7aa4b495dcee4c521b87205de9363fb62ee5fdc8eab91d4ddb97257c85b/QScintilla-$QSCI_VERSION.tar.gz
https://www.riverbankcomputing.com/static/Downloads/QScintilla/$QSCI_VERSION/QScintilla_src-$QSCI_VERSION.tar.gz"

: "${ASSUME_YES:=0}"
QSCI_NEEDS_SOURCE=0
DEPS_OK=1

deps_bold() { printf '\033[1m%s\033[0m\n' "$*"; }
# deps_ask "Question?" -> 0 for yes. Default Yes; with no terminal it is No unless -y.
deps_ask() {
    if [ "$ASSUME_YES" = 1 ]; then echo "$1 [Y/n] y"; return 0; fi
    if [ ! -t 0 ]; then echo "$1 [Y/n] n (no terminal, use -y to answer yes)"; return 1; fi
    local reply
    read -r -p "$1 [Y/n] " reply
    case "${reply:-y}" in [Yy]*) return 0 ;; *) return 1 ;; esac
}
# Run as root: directly when we are root, otherwise through sudo.
deps_sudo() {
    if [ "$(id -u)" = 0 ]; then "$@"
    elif command -v sudo >/dev/null; then sudo "$@"
    else echo "sudo isn't installed. Run this as root, or install sudo first."; return 1
    fi
}

# ---- 1. Work out the OS and its package manager -----------------------------
deps_detect() {
    local ID=unknown ID_LIKE="" PRETTY_NAME="" VERSION_ID=""
    [ -r /etc/os-release ] && . /etc/os-release
    OS_NAME="${PRETTY_NAME:-$ID}"
    OS_MAJOR="${VERSION_ID%%.*}"
    DEPS_FAMILY=""
    local id
    for id in $ID ${ID_LIKE:-}; do
        case "$id" in
            debian|ubuntu) DEPS_FAMILY=apt; break ;;
            # Fedora first: Fedora's ID_LIKE is empty, the EL clones list "rhel centos fedora".
            fedora) DEPS_FAMILY=dnf; break ;;
            almalinux|rocky|rhel|centos|ol|circle|eurolinux|navy) DEPS_FAMILY=el; break ;;
            arch) DEPS_FAMILY=pacman; break ;;
            opensuse*|suse|sles) DEPS_FAMILY=zypper; break ;;
        esac
    done
    DEPS_ID="$ID"
}

# ---- 2. What each family needs ----------------------------------------------
# Entries are package names; "a|b" means either one will do (the first one the
# repositories offer gets installed). REQUIRED is needed to build; RECOMMENDED adds
# Wayland support, Save Compressed (7-Zip), good monospace fonts and UTF-8 locales.
deps_lists() {
    case "$DEPS_FAMILY" in
      apt)
        REQUIRED="build-essential cmake qt6-base-dev libqscintilla2-qt6-dev"
        RECOMMENDED="qt6-wayland p7zip-full|7zip fonts-dejavu-core fonts-noto-mono locales" ;;
      dnf)
        REQUIRED="gcc-c++ make cmake qt6-qtbase-devel qscintilla-qt6-devel"
        RECOMMENDED="qt6-qtwayland 7zip|p7zip-plugins dejavu-sans-mono-fonts google-noto-sans-mono-fonts glibc-langpack-en" ;;
      el)
        # QScintilla comes from EPEL where it is packaged; otherwise build.sh builds it.
        # Qt 6 development headers pull in packages from CRB (CodeReady Builder).
        REQUIRED="gcc-c++ make cmake qt6-qtbase-devel qscintilla-qt6-devel"
        RECOMMENDED="qt6-qtwayland 7zip|p7zip-plugins dejavu-sans-mono-fonts google-noto-sans-mono-fonts glibc-langpack-en" ;;
      pacman)
        REQUIRED="base-devel cmake qt6-base qscintilla-qt6"
        RECOMMENDED="qt6-wayland 7zip|p7zip ttf-dejavu noto-fonts" ;;
      zypper)
        REQUIRED="gcc-c++ make cmake qt6-base-devel qscintilla-qt6-devel"
        RECOMMENDED="qt6-wayland 7zip|p7zip-full dejavu-fonts google-noto-sans-mono-fonts glibc-locale" ;;
      *) REQUIRED=""; RECOMMENDED="" ;;
    esac
}

deps_is_installed() {
    case "$DEPS_FAMILY" in
        apt) dpkg-query -W -f='${Status}' "$1" 2>/dev/null | grep -q "install ok installed" ;;
        dnf|el|zypper) rpm -q --whatprovides "$1" >/dev/null 2>&1 ;;
        pacman) pacman -Qi "$1" >/dev/null 2>&1 || pacman -Qg "$1" >/dev/null 2>&1 ;;
        *) return 0 ;;
    esac
}
# Can the enabled repositories install this package?
deps_is_available() {
    case "$DEPS_FAMILY" in
        apt) [ -n "$(apt-cache policy "$1" 2>/dev/null | sed -n 's/^ *Candidate: //p' | grep -v '(none)')" ] ;;
        dnf|el) dnf -q list --available "$1" >/dev/null 2>&1 ;;
        pacman) pacman -Si "$1" >/dev/null 2>&1 || pacman -Sg "$1" >/dev/null 2>&1 ;;
        zypper) zypper --non-interactive -q info "$1" 2>/dev/null | grep -q "^Name *: *$1" ;;
        *) return 1 ;;
    esac
}
deps_install() {
    case "$DEPS_FAMILY" in
        apt) deps_sudo apt-get update && deps_sudo apt-get install -y "$@" ;;
        dnf|el) deps_sudo dnf install -y "$@" ;;
        pacman) deps_sudo pacman -S --needed --noconfirm "$@" ;;
        zypper) deps_sudo zypper --non-interactive install "$@" ;;
    esac
}

# Is an entry ("a" or "a|b") satisfied?
deps_entry_installed() {
    local alt
    for alt in ${1//|/ }; do deps_is_installed "$alt" && return 0; done
    return 1
}
# Choose what to install for an entry: the first alternative the repos offer.
deps_entry_pick() {
    local alt
    for alt in ${1//|/ }; do
        if deps_is_available "$alt"; then DEPS_PICK="$alt"; return 0; fi
    done
    DEPS_PICK=""; return 1
}
deps_pretty() { echo "${1//|/ or }"; }

# ---- 3. EPEL and CRB on AlmaLinux, Rocky, RHEL and friends -------------------
deps_el_repo_enabled() {  # $1 = regex for the repo id
    dnf -q repolist --enabled 2>/dev/null | grep -v '^repo id' | awk '{print $1}' | grep -qiE "$1"
}
deps_el_check_repos() {
    EL_NEED_EPEL=0; EL_NEED_CRB=0
    rpm -q epel-release >/dev/null 2>&1 || EL_NEED_EPEL=1
    deps_el_repo_enabled '^(crb|powertools|codeready-builder|ol[0-9]+_codeready_builder)' || EL_NEED_CRB=1
}
deps_el_enable_repos() {
    if [ "$EL_NEED_EPEL" = 1 ]; then
        deps_bold "Enabling EPEL (Extra Packages for Enterprise Linux)..."
        case "$DEPS_ID" in
            rhel)
                deps_sudo dnf install -y "https://dl.fedoraproject.org/pub/epel/epel-release-latest-$OS_MAJOR.noarch.rpm" ;;
            ol)
                deps_sudo dnf install -y "oracle-epel-release-el$OS_MAJOR" \
                  || deps_sudo dnf install -y "https://dl.fedoraproject.org/pub/epel/epel-release-latest-$OS_MAJOR.noarch.rpm" ;;
            *)
                deps_sudo dnf install -y epel-release \
                  || deps_sudo dnf install -y "https://dl.fedoraproject.org/pub/epel/epel-release-latest-$OS_MAJOR.noarch.rpm" ;;
        esac || { echo "Couldn't enable EPEL. Some packages may be unavailable."; }
    fi
    if [ "$EL_NEED_CRB" = 1 ]; then
        deps_bold "Enabling CRB (CodeReady Builder)..."
        if [ "$DEPS_ID" = rhel ] && command -v subscription-manager >/dev/null; then
            deps_sudo subscription-manager repos --enable "codeready-builder-for-rhel-$OS_MAJOR-$(uname -m)-rpms"
        elif [ "$DEPS_ID" = ol ]; then
            deps_sudo dnf config-manager --set-enabled "ol${OS_MAJOR}_codeready_builder"
        elif command -v crb >/dev/null; then
            deps_sudo crb enable            # helper script that ships with epel-release
        else
            deps_sudo dnf config-manager --set-enabled crb 2>/dev/null \
              || { deps_sudo dnf install -y dnf-plugins-core && deps_sudo dnf config-manager --set-enabled crb; } \
              || deps_sudo dnf config-manager setopt crb.enabled=1      # dnf5
        fi || echo "Couldn't enable CRB. Some -devel packages may be unavailable."
    fi
}

# Is QScintilla for Qt 6 usable from the system?
deps_have_system_qsci() {
    local d l
    for d in /usr/include/qt6 /usr/include/*-linux-gnu/qt6 /usr/include /usr/local/include/qt6 /usr/local/include; do
        [ -f "$d/Qsci/qsciscintilla.h" ] || continue
        for l in /usr/lib*/libqscintilla2[_-]qt6.so /usr/lib/*-linux-gnu/libqscintilla2[_-]qt6.so \
                 /usr/local/lib*/libqscintilla2[_-]qt6.so; do
            [ -e "$l" ] && return 0
        done
    done
    return 1
}

# ---- 4. Check, list, ask, install --------------------------------------------
# deps_main [--check]
deps_main() {
    local check_only=0; [ "${1:-}" = --check ] && check_only=1
    deps_detect; deps_lists
    deps_bold "Checking what BlitzPad needs on $OS_NAME"

    if [ -z "$DEPS_FAMILY" ]; then
        echo "This Linux isn't one the installer knows. Install these yourself, then run it again:"
        echo "  a C++17 compiler, CMake 3.16+, Qt 6 Widgets (dev), QScintilla for Qt 6 (dev),"
        echo "  the Qt 6 Wayland plugin, 7-Zip, DejaVu or Noto monospace fonts, a UTF-8 locale."
        deps_have_system_qsci || QSCI_NEEDS_SOURCE=1
        return 0
    fi

    local e missing_req=() missing_rec=()
    for e in $REQUIRED;    do deps_entry_installed "$e" || missing_req+=("$e"); done
    for e in $RECOMMENDED; do deps_entry_installed "$e" || missing_rec+=("$e"); done
    # Needed only if QScintilla has to be downloaded and built from source.
    command -v curl >/dev/null || command -v wget >/dev/null || missing_rec+=("curl")
    command -v tar >/dev/null || missing_rec+=("tar")
    # QScintilla built from source on an earlier run counts as present.
    if [ -f third_party/QScintilla-$QSCI_VERSION/src/Qsci/qsciscintilla.h ]; then
        local keep=(); for e in "${missing_req[@]}"; do [[ "$e" == *qscintilla* ]] || keep+=("$e"); done
        missing_req=("${keep[@]}")
    fi

    local need_repos=0
    if [ "$DEPS_FAMILY" = el ]; then
        deps_el_check_repos
        [ "$EL_NEED_EPEL" = 1 ] || [ "$EL_NEED_CRB" = 1 ] && [ $(( ${#missing_req[@]} + ${#missing_rec[@]} )) -gt 0 ] && need_repos=1
    fi

    if [ ${#missing_req[@]} -eq 0 ] && [ ${#missing_rec[@]} -eq 0 ]; then
        echo "Everything BlitzPad needs is already installed."
    else
        if [ ${#missing_req[@]} -gt 0 ]; then
            echo "Missing (required to build):"
            for e in "${missing_req[@]}"; do echo "    $(deps_pretty "$e")"; done
        fi
        if [ ${#missing_rec[@]} -gt 0 ]; then
            echo "Missing (recommended: Wayland, Save Compressed, fonts, UTF-8 locale):"
            for e in "${missing_rec[@]}"; do echo "    $(deps_pretty "$e")"; done
        fi
        if [ "$need_repos" = 1 ]; then
            echo "Repositories to enable first:"
            [ "$EL_NEED_EPEL" = 1 ] && echo "    EPEL (Extra Packages for Enterprise Linux)"
            [ "$EL_NEED_CRB" = 1 ]  && echo "    CRB (CodeReady Builder)"
        fi
        if [ "$check_only" = 1 ]; then
            [ ${#missing_req[@]} -eq 0 ]; return
        fi
        if deps_ask "Install them now with sudo (it will ask for your password)?"; then
            [ "$need_repos" = 1 ] && { deps_el_enable_repos || true; }
            [ "$DEPS_FAMILY" = apt ] && { deps_sudo apt-get update -qq || true; }
            local to_install=() unavailable=()
            for e in "${missing_req[@]}" "${missing_rec[@]}"; do
                if deps_entry_pick "$e"; then to_install+=("$DEPS_PICK"); else unavailable+=("$e"); fi
            done
            if [ ${#to_install[@]} -gt 0 ]; then
                deps_bold "Installing: ${to_install[*]}"
                # apt-get update was just run, so call the installer directly.
                case "$DEPS_FAMILY" in
                    apt) deps_sudo apt-get install -y "${to_install[@]}" ;;
                    *) deps_install "${to_install[@]}" ;;
                esac || echo "The package manager reported a problem (see above)."
            fi
            for e in "${unavailable[@]}"; do
                case "$e" in
                    *qscintilla*) echo "QScintilla isn't packaged for $OS_NAME, so it will be built from source." ;;
                    *) echo "Not available from your repositories, skipped: $(deps_pretty "$e")" ;;
                esac
            done
            # Anything required still missing? (QScintilla can come from source instead.)
            local still=()
            for e in "${missing_req[@]}"; do
                [[ "$e" == *qscintilla* ]] && continue
                deps_entry_installed "$e" || still+=("$(deps_pretty "$e")")
            done
            if [ ${#still[@]} -gt 0 ]; then
                DEPS_OK=0
                deps_bold "Still missing: ${still[*]}"
                echo "Check your internet connection and that your repositories are reachable"
                echo "(dnf repolist / apt-get update), then run this again."
            fi
        else
            local e2; for e2 in "${missing_req[@]}"; do [[ "$e2" == *qscintilla* ]] || DEPS_OK=0; done
            echo "Skipping. The build will fail if a required package is missing."
        fi
    fi

    # Needs a source build of QScintilla?
    if ! deps_have_system_qsci && [ ! -f third_party/QScintilla-$QSCI_VERSION/src/Qsci/qsciscintilla.h ]; then
        QSCI_NEEDS_SOURCE=1
    fi

    # A UTF-8 locale keeps file names and non-English text showing correctly.
    if [ "$check_only" = 0 ] && command -v locale >/dev/null && ! locale -a 2>/dev/null | grep -qiE 'utf-?8'; then
        deps_bold "No UTF-8 locale is set up on this system."
        if [ "$DEPS_FAMILY" = apt ] && deps_ask "Generate en_US.UTF-8 now with sudo?"; then
            deps_sudo locale-gen en_US.UTF-8
        else
            echo "Set one up with your system's locale settings if text looks wrong."
        fi
    fi
    return 0
}

# Download the QScintilla source archive to $1 (checksum-checked where known).
deps_download_qscintilla() {
    local tgz="$1" url
    mkdir -p "$(dirname "$tgz")"
    while read -r url; do
        [ -n "$url" ] || continue
        rm -f "$tgz"
        if command -v curl >/dev/null; then curl -fsSL --retry 2 -o "$tgz" "$url" || continue
        elif command -v wget >/dev/null; then wget -q -O "$tgz" "$url" || continue
        else echo "Neither curl nor wget is installed."; return 1
        fi
        # The PyPI copy has a known checksum; the Riverbank copy is a different archive.
        if [[ "$url" == *pythonhosted* ]] && [ "$(sha256sum "$tgz" | cut -d' ' -f1)" != "$QSCI_SHA256" ]; then
            echo "Checksum mismatch for $url, ignoring it."; continue
        fi
        return 0
    done <<<"$QSCI_URLS"
    rm -f "$tgz"
    echo "Couldn't download QScintilla. Check your internet connection."; return 1
}

# Download and unpack the QScintilla source into third_party/ (no sudo needed).
deps_fetch_qscintilla() {
    local dir="third_party/QScintilla-$QSCI_VERSION"
    [ -f "$dir/src/Qsci/qsciscintilla.h" ] && { QSCI_SOURCE_DIR="$PWD/$dir"; return 0; }
    deps_bold "Downloading QScintilla $QSCI_VERSION source (your distro has no package for it)..."
    local tgz="third_party/qscintilla-$QSCI_VERSION.tar.gz"
    deps_download_qscintilla "$tgz" || return 1
    local tmp; tmp=$(mktemp -d third_party/unpack.XXXXXX)
    tar xzf "$tgz" -C "$tmp" || { rm -rf "$tmp"; echo "Couldn't unpack $tgz"; return 1; }
    local top; top=$(find "$tmp" -mindepth 1 -maxdepth 1 -type d | head -1)
    [ -f "$top/src/Qsci/qsciscintilla.h" ] || { rm -rf "$tmp"; echo "That archive doesn't look like QScintilla."; return 1; }
    rm -rf "$dir"; mv "$top" "$dir"; rm -rf "$tmp" "$tgz"
    QSCI_SOURCE_DIR="$PWD/$dir"
}

# Run on its own: ./deps.sh [-y] [--check]
if [ "${BASH_SOURCE[0]}" = "$0" ]; then
    set -uo pipefail
    cd "$(dirname "$0")"
    mode=""
    for arg in "$@"; do
        case "$arg" in
            -y|--yes) ASSUME_YES=1 ;;
            --check) mode=--check ;;
            -h|--help) sed -n '2,13p' "$0"; exit 0 ;;
            *) echo "Unknown option: $arg"; exit 1 ;;
        esac
    done
    deps_main $mode
fi
