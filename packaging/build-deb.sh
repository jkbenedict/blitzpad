#!/bin/bash
# Builds a .deb package of BlitzPad for Zorin OS / Ubuntu / Debian / Linux Mint.
#
#   bash packaging/build-deb.sh [build-dir] [output-dir]
#
# Run it on the oldest release you want to support: a .deb built on Ubuntu 24.04
# installs on Ubuntu 24.04+, Zorin OS 18, Mint 22 and Debian 13, but not on older ones.
# The package's dependencies are worked out by dpkg-shlibdeps from the real binary,
# so `sudo apt install ./blitzpad_*.deb` pulls in Qt 6 and QScintilla automatically.
# Needs: build-essential cmake qt6-base-dev libqscintilla2-qt6-dev dpkg-dev
set -euo pipefail
cd "$(dirname "$0")/.."
BUILD="${1:-build}"
OUT="${2:-..}"

VERSION=$(sed -n 's/^project(blitzpad VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
ARCH=$(dpkg --print-architecture)
REV=1
PKG="blitzpad_${VERSION}-${REV}_${ARCH}"

# Build against the system QScintilla (a Debian package), install to /usr.
cmake -S . -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DQSCINTILLA_SOURCE_DIR= -DCMAKE_INSTALL_PREFIX=/usr >/dev/null
cmake --build "$BUILD" -j"$(nproc)"

stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
root="$stage/$PKG"
DESTDIR="$root" cmake --install "$BUILD" >/dev/null
strip "$root/usr/bin/blitzpad"
install -Dm644 data/icons/blitzpad-128.png "$root/usr/share/pixmaps/blitzpad.png"
install -Dm644 LICENSE.md "$root/usr/share/doc/blitzpad/copyright"
install -Dm644 README.md "$root/usr/share/doc/blitzpad/README.md"
install -Dm644 MANIFEST.md "$root/usr/share/doc/blitzpad/MANIFEST.md"
gzip -9n -c CHANGELOG.md > "$root/usr/share/doc/blitzpad/changelog.gz"

# Work out the shared-library dependencies (Qt 6, QScintilla, libc...).
mkdir -p "$stage/debian"
printf 'Source: blitzpad\n\nPackage: blitzpad\nArchitecture: any\n' > "$stage/debian/control"
depends=$(cd "$stage" && dpkg-shlibdeps -O "$PKG/usr/bin/blitzpad" 2>/dev/null | sed -n 's/^shlibs:Depends=//p')
size=$(du -sk "$root/usr" | cut -f1)

mkdir -p "$root/DEBIAN"
cat > "$root/DEBIAN/control" <<EOF
Package: blitzpad
Version: ${VERSION}-${REV}
Architecture: ${ARCH}
Maintainer: JK Benedict <noreply@blitzpad.invalid>
Installed-Size: ${size}
Depends: ${depends}
Recommends: qt6-wayland, p7zip-full | 7zip, fonts-dejavu-core, fonts-noto-mono, locales
Section: editors
Priority: optional
Description: fast Notepad++-style text editor
 BlitzPad is a fast graphical text editor for Linux in the style of Notepad++:
 syntax colouring for 60 languages, regex find and replace, 59 encodings and
 code pages, and Save Compressed (7-Zip).
EOF
# Refresh the menu and icon caches after installing or removing.
for script in postinst postrm; do
    cat > "$root/DEBIAN/$script" <<'EOF'
#!/bin/sh
set -e
if command -v update-desktop-database >/dev/null 2>&1; then update-desktop-database -q /usr/share/applications || true; fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then gtk-update-icon-cache -f -t -q /usr/share/icons/hicolor || true; fi
exit 0
EOF
    chmod 755 "$root/DEBIAN/$script"
done

mkdir -p "$OUT"
dpkg-deb --root-owner-group --build "$root" "$OUT/$PKG.deb" >/dev/null
echo "Wrote $(cd "$OUT" && pwd)/$PKG.deb"
echo "Depends: $depends"
