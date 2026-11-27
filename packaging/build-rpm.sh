#!/bin/bash
# Builds an .rpm package of BlitzPad for AlmaLinux / Rocky / RHEL 9 and 10 (and Fedora).
#
#   bash packaging/build-rpm.sh [output-dir]
#
# Run it on the system you're packaging for (an AlmaLinux 9 .rpm suits all EL9 systems).
# It offers to install what it needs with sudo (rpm-build, the build dependencies, and
# the CRB repository that Qt's development files come from), downloads QScintilla and
# compiles it in, so the finished package only needs Qt 6 from AppStream.
# The result installs with:  sudo dnf install ./blitzpad-<version>-1.el9.<arch>.rpm
set -euo pipefail
cd "$(dirname "$0")/.."
OUT="${1:-..}"; [ $# -gt 0 ] && shift   # anything after it goes to rpmbuild
ASSUME_YES="${ASSUME_YES:-0}"
. ./deps.sh
deps_detect

VERSION=$(sed -n 's/^project(blitzpad VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
command -v dnf >/dev/null || { echo "This script needs dnf (AlmaLinux, Rocky, RHEL, Fedora)."; exit 1; }

# Tools and build dependencies.
need=()
for p in rpm-build gcc-c++ make cmake qt6-qtbase-devel tar gzip; do
    rpm -q --whatprovides "$p" >/dev/null 2>&1 || need+=("$p")
done
command -v curl >/dev/null || command -v wget >/dev/null || need+=(curl)
if [ ${#need[@]} -gt 0 ]; then
    deps_bold "Needed to build the .rpm: ${need[*]}"
    if [ "$DEPS_FAMILY" = el ]; then
        deps_el_check_repos
        EL_NEED_EPEL=0      # the .rpm build doesn't need EPEL
        [ "$EL_NEED_CRB" = 1 ] && echo "The CRB (CodeReady Builder) repository will be enabled first."
    fi
    deps_ask "Install them now with sudo?" || { echo "Can't build without them."; exit 1; }
    [ "$DEPS_FAMILY" = el ] && [ "${EL_NEED_CRB:-0}" = 1 ] && deps_el_enable_repos
    deps_sudo dnf install -y "${need[@]}"
fi

top=$(mktemp -d)
trap 'rm -rf "$top"' EXIT
mkdir -p "$top"/{SOURCES,SPECS,BUILD,RPMS,SRPMS}

# Source0: this source tree (without build output).
tar czf "$top/SOURCES/blitzpad-$VERSION.tar.gz" \
    --exclude=./build --exclude=./third_party --exclude=./prebuilt --exclude='./*.zip' .
# Source1: QScintilla.
deps_download_qscintilla "$top/SOURCES/QScintilla-$QSCI_VERSION.tar.gz"

sed -e "s/@VERSION@/$VERSION/g" -e "s/@DATE@/$(LC_ALL=C date '+%a %b %d %Y')/" \
    -e "s/^%global qsci_version .*/%global qsci_version $QSCI_VERSION/" \
    packaging/blitzpad.spec > "$top/SPECS/blitzpad.spec"

rpmbuild --define "_topdir $top" -ba "$top/SPECS/blitzpad.spec" "$@" 2>&1 | grep -vE '^\+ |^Executing' || true
mkdir -p "$OUT"
found=0
for f in "$top"/RPMS/*/blitzpad-"$VERSION"-*.rpm "$top"/SRPMS/blitzpad-"$VERSION"-*.src.rpm; do
    [ -e "$f" ] || continue
    cp "$f" "$OUT/"; echo "Wrote $(cd "$OUT" && pwd)/$(basename "$f")"; found=1
done
[ "$found" = 1 ] || { echo "rpmbuild failed (see above)."; exit 1; }
