# RPM spec for BlitzPad: AlmaLinux / Rocky / RHEL 9 and 10, and Fedora.
# Build it with packaging/build-rpm.sh, which fills in the version and sources.
#
# QScintilla is compiled into BlitzPad (static) by default, because EPEL doesn't
# always carry qscintilla-qt6. To link the system one instead:
#   rpmbuild -ba --with system_qscintilla blitzpad.spec
%bcond_with system_qscintilla

%global qsci_version 2.14.1
%if %{with system_qscintilla}
%global qsci_arg -DQSCINTILLA_SOURCE_DIR=
%else
%global qsci_arg -DQSCINTILLA_SOURCE_DIR="$PWD/third_party/qscintilla"
%endif

Name:           blitzpad
Version:        @VERSION@
Release:        1%{?dist}
Summary:        Fast Notepad++-style text editor
# BlitzPad is MIT; linked with QScintilla (GPL-3.0), the binary is GPL-3.0 as a whole.
License:        MIT AND GPL-3.0-only
Source0:        blitzpad-%{version}.tar.gz
%if %{without system_qscintilla}
Source1:        QScintilla-%{qsci_version}.tar.gz
%endif

BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  cmake >= 3.16
BuildRequires:  qt6-qtbase-devel
%if %{with system_qscintilla}
BuildRequires:  qscintilla-qt6-devel
%endif
# Shared-library dependencies (Qt 6, libstdc++...) are added automatically by rpmbuild.
Recommends:     qt6-qtwayland
Recommends:     (7zip or p7zip-plugins)
Recommends:     dejavu-sans-mono-fonts
Recommends:     google-noto-sans-mono-fonts
Recommends:     glibc-langpack-en

%description
BlitzPad is a fast graphical text editor for Linux in the style of Notepad++:
syntax colouring for 59 languages, regex find and replace, 59 encodings and
code pages, and Save Compressed (7-Zip).

%prep
%setup -q -c -n blitzpad-%{version}
%if %{without system_qscintilla}
mkdir -p third_party
tar xzf %{SOURCE1} -C third_party
mv third_party/QScintilla-* third_party/qscintilla
%endif

%build
%cmake -DCMAKE_BUILD_TYPE=Release %{qsci_arg}
%cmake_build

%check
export QT_QPA_PLATFORM=offscreen
%ctest

%install
%cmake_install
install -Dm644 data/icons/blitzpad-128.png %{buildroot}%{_datadir}/pixmaps/blitzpad.png

%files
%license LICENSE.md
%doc README.md CHANGELOG.md MANIFEST.md
%{_bindir}/blitzpad
%{_datadir}/applications/blitzpad.desktop
%{_datadir}/icons/hicolor/*/apps/blitzpad.png
%{_datadir}/icons/hicolor/scalable/apps/blitzpad.svg
%{_datadir}/pixmaps/blitzpad.png

%changelog
* @DATE@ JK Benedict - @VERSION@-1
- See CHANGELOG.md
