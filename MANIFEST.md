# BlitzPad manifest

What BlitzPad needs on each Linux distribution, and what each install method puts on
your system. `install.sh` (through `deps.sh`) checks these packages, lists the missing
ones, asks, and installs them with `sudo`. The `.deb` and `.rpm` packages declare them,
so `apt` and `dnf` pull them in for you.

Keep this file in step with `REQUIRED` / `RECOMMENDED` in `deps.sh`, the `Depends` and
`Recommends` in `packaging/build-deb.sh`, and the `BuildRequires` / `Recommends` in
`packaging/blitzpad.spec`.

## Status (beta 0.23)

BlitzPad is **tested on Debian-based systems** (Zorin OS 18, apt / `.deb`) **and RHEL-based
systems** (AlmaLinux 9 / 10, dnf / `.rpm`). Every other distribution below is expected to
work but not yet tested.

| Distribution | Package format | Status |
|---|---|---|
| **Zorin OS 18** | `.deb`, `install.sh` (apt) | ✅ **Tested** |
| **AlmaLinux 9 / 10** | `.rpm`, `install.sh` (dnf + EPEL + CRB) | ✅ **Tested** |
| Zorin OS 17, Ubuntu 24.04+, Debian 13, Linux Mint 22+, Pop!_OS 24.04 | `.deb`, `install.sh` (apt) | 🧪 Expected to work |
| Ubuntu 22.04, Debian 12, Linux Mint 21 | `install.sh` (apt); the `.deb` is built on Ubuntu 24.04 and needs a newer system | 🧪 Expected to work |
| Rocky Linux, RHEL, CentOS Stream, Oracle Linux 9 / 10 | `.rpm`, `install.sh` (dnf + EPEL + CRB) | 🧪 Expected to work |
| Fedora 39+ | `install.sh` (dnf), or an `.rpm` built there with `packaging/build-rpm.sh` | 🧪 Expected to work |
| Arch Linux, Manjaro, EndeavourOS | `install.sh` (pacman) | 🧪 Expected to work |
| openSUSE Tumbleweed / Leap 15.6 | `install.sh` (zypper) | 🧪 Expected to work |

## Dependencies

**Required** packages are needed to build BlitzPad (and, for Qt and QScintilla, to run it).
**Recommended** packages add the Qt Wayland plugin, 7-Zip for *Save Compressed*,
monospace fonts with wide Unicode coverage, and UTF-8 locale data. BlitzPad runs without
them. Where two names are given ("a or b"), either one will do.

### Zorin OS, Ubuntu, Debian, Linux Mint, Pop!_OS (apt)

| | Packages |
|---|---|
| Required | `build-essential` `cmake` `qt6-base-dev` `libqscintilla2-qt6-dev` |
| Recommended | `qt6-wayland` · `p7zip-full` or `7zip` · `fonts-dejavu-core` · `fonts-noto-mono` · `locales` |
| `.deb` runtime | `libqt6core6t64`, `libqt6gui6t64`, `libqt6widgets6t64`, `libqscintilla2-qt6-15`, `libc6`, `libstdc++6`, `libgcc-s1` (worked out from the binary by `dpkg-shlibdeps`) |

### AlmaLinux, Rocky Linux, RHEL, CentOS Stream, Oracle Linux 9 and 10 (dnf)

| | Packages |
|---|---|
| Repositories | **EPEL** (`epel-release`) and **CRB** / CodeReady Builder (`crb enable`, or `subscription-manager` on RHEL). The installer enables both after asking. |
| Required | `gcc-c++` `make` `cmake` `qt6-qtbase-devel` (AppStream, with CRB) · `qscintilla-qt6-devel` (EPEL; if missing, QScintilla 2.14.1 is downloaded and compiled into BlitzPad) |
| Recommended | `qt6-qtwayland` · `7zip` or `p7zip-plugins` (EPEL) · `dejavu-sans-mono-fonts` · `google-noto-sans-mono-fonts` · `glibc-langpack-en` |
| Also used | `curl` or `wget`, and `tar`, only when QScintilla is built from source |
| `.rpm` runtime | `qt6-qtbase`, `qt6-qtbase-gui` (as `libQt6Core.so.6`, `libQt6Gui.so.6`, `libQt6Widgets.so.6`, `libQt6PrintSupport.so.6`, added by `rpmbuild`), `libstdc++`, `glibc`. QScintilla is built in, so EPEL is not needed to run the `.rpm`. |
| `.rpm` build | `rpm-build` plus the required packages above (CRB enabled; EPEL not needed) |

### Fedora 39+ (dnf)

| | Packages |
|---|---|
| Required | `gcc-c++` `make` `cmake` `qt6-qtbase-devel` `qscintilla-qt6-devel` |
| Recommended | `qt6-qtwayland` · `7zip` or `p7zip-plugins` · `dejavu-sans-mono-fonts` · `google-noto-sans-mono-fonts` · `glibc-langpack-en` |

### Arch Linux, Manjaro, EndeavourOS (pacman)

| | Packages |
|---|---|
| Required | `base-devel` `cmake` `qt6-base` `qscintilla-qt6` |
| Recommended | `qt6-wayland` · `7zip` or `p7zip` · `ttf-dejavu` · `noto-fonts` |

### openSUSE Tumbleweed / Leap 15.6 (zypper)

| | Packages |
|---|---|
| Required | `gcc-c++` `make` `cmake` `qt6-base-devel` `qscintilla-qt6-devel` |
| Recommended | `qt6-wayland` · `7zip` or `p7zip-full` · `dejavu-fonts` · `google-noto-sans-mono-fonts` · `glibc-locale` |

### Any other Linux

A C++17 compiler, CMake 3.16+, Qt 6 Widgets (development files), QScintilla for Qt 6
(development files, or let `build.sh` build it from source), and optionally the Qt 6
Wayland plugin, 7-Zip, DejaVu or Noto monospace fonts and a UTF-8 locale.

## Installed files

| File | Just for me (`install.sh --user`) | All users (`install.sh --system`) | `.deb` / `.rpm` |
|---|---|---|---|
| Program | `~/.local/bin/blitzpad` | `/usr/local/bin/blitzpad` | `/usr/bin/blitzpad` |
| Menu entry | `~/.local/share/applications/blitzpad.desktop` | `/usr/local/share/applications/blitzpad.desktop` | `/usr/share/applications/blitzpad.desktop` |
| Icons (16 to 512 px) | `~/.local/share/icons/hicolor/<size>/apps/blitzpad.png` | `/usr/local/share/icons/hicolor/<size>/apps/blitzpad.png` | `/usr/share/icons/hicolor/<size>/apps/blitzpad.png` |
| Scalable icon | `~/.local/share/icons/hicolor/scalable/apps/blitzpad.svg` | `/usr/local/share/icons/hicolor/scalable/apps/blitzpad.svg` | `/usr/share/icons/hicolor/scalable/apps/blitzpad.svg` |
| Legacy icon | | | `/usr/share/pixmaps/blitzpad.png` |
| Documentation | | | `/usr/share/doc/blitzpad/` |

**Why `/usr/local` for all-users installs?** On every distribution the installer knows
(Zorin / Ubuntu / Debian / Mint, Fedora, AlmaLinux / Rocky / RHEL, Arch, openSUSE), `/usr/bin`
belongs to the package manager, and software you build and install yourself goes in
`/usr/local`. That keeps `apt` and `dnf` from ever clashing with BlitzPad. The installer uses
`/usr` instead on a distribution it doesn't know, or where `/usr/local/bin` isn't on `PATH`.
`--prefix=DIR` picks another place. The `.deb` and `.rpm` packages are managed by the
package manager, so they install to `/usr`.

Installs made by BlitzPad 0.17 and older used `/usr` for all users. Installing 0.18 for all
users removes that older copy, and `install.sh --uninstall --system` removes both.

Your settings live in `~/.blitzpad/` and are never touched by installing, upgrading or
uninstalling.
