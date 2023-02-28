# 
# Do NOT Edit the Auto-generated Part!
# Generated by: spectacle version 0.27
# 

Name:       harbour-osmscout

# >> macros

# ignore installed files that are not packed to rpm
%define _unpackaged_files_terminate_build 0

# don't setup rpm provides
%define __provides_exclude_from ^%{_datadir}/.*$

# don't setup rpm requires
# list here all the libraries your RPM installs
%define __requires_exclude ^ld-linux|libmarisa|libosmscout.*$

# << macros

Summary:    OSMScout for Sailfish
Version:    2.29
Release:    1
Group:      Qt/Qt
License:    GPLv2
URL:        https://github.com/Karry/osmscout-sailfish
Source0:    %{name}-%{version}.tar.bz2
Source100:  harbour-osmscout.yaml
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Multimedia)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Positioning)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Sql)
BuildRequires:  pkgconfig(Qt5Svg)
BuildRequires:  pkgconfig(Qt5Xml)
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  cmake
BuildRequires:  chrpath
BuildRequires:  desktop-file-utils
BuildRequires:  git
BuildRequires:  qt5-qttools-linguist

%description
OSM Scout is offline map viewer and routing application.

PackageName: OSM Scout
Type: desktop-application
Categories:
  - Maps
Custom:
  Repo: https://github.com/Karry/osmscout-sailfish
Icon: https://raw.githubusercontent.com/Karry/osmscout-sailfish/master/graphics/harbour-osmscout.svg
Screenshots:
  - https://raw.githubusercontent.com/Karry/osmscout-sailfish/master/graphics/screenshot-04-prague.png
  - https://raw.githubusercontent.com/Karry/osmscout-sailfish/master/graphics/screenshot-05-winter-sports.png
  - https://raw.githubusercontent.com/Karry/osmscout-sailfish/master/graphics/screenshot-06-track.png

%prep
%setup -q -n %{name}-%{version}

# >> setup
# << setup

%build
# >> build pre
#rm -rf rpmbuilddir-%{_arch}
mkdir -p rpmbuilddir-%{_arch}

## for production build:
cd rpmbuilddir-%{_arch} && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DQT_QML_DEBUG=no -DSANITIZER=none -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_RPATH=%{_datadir}/%{name}/lib/: ..
## for debug build, use these cmake arguments instead:
# cd rpmbuilddir-%{_arch} && cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer" -DQT_QML_DEBUG=yes -DSANITIZER=none -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_RPATH=%{_datadir}/%{name}/lib/: ..

cd ..
make -C rpmbuilddir-%{_arch} VERBOSE=1 %{?_smp_mflags}
# << build pre



# >> build post
# << build post

%install
# >> install pre
#rm -rf %{buildroot}
DESTDIR=%{buildroot} make -C rpmbuilddir-%{_arch} install
mkdir -p %{_bindir}
# << install pre

# >> install post

desktop-file-install --delete-original       \
  --dir %{buildroot}%{_datadir}/applications             \
   %{buildroot}%{_datadir}/applications/*.desktop

# remove includes, we don't need it on Jolla phone
rm %{buildroot}%{_includedir}/osmscout/*.h
rm %{buildroot}%{_includedir}/osmscout/*/*.h
# remove desktop qml
rm %{buildroot}%{_datadir}/%{name}/qml/desktop.qml

## Jolla harbour rules:

# -- ship all shared unallowed libraries with the app
mkdir -p %{buildroot}%{_datadir}/%{name}/lib

# << install post

# -- check app rpath to find its libs
chrpath -l %{buildroot}%{_bindir}/%{name}
ls -al     %{buildroot}%{_bindir}/%{name}
sha1sum    %{buildroot}%{_bindir}/%{name}

%files
%defattr(-,root,root,-)
%{_bindir}
%{_datadir}/%{name}
%{_datadir}/%{name}/lib/
%{_datadir}/applications/%{name}.desktop
%{_datadir}/%{name}/map-styles/
%{_datadir}/%{name}/map-icons/
%{_datadir}/icons/hicolor/86x86/apps/%{name}.png
%{_datadir}/icons/hicolor/108x108/apps/%{name}.png
%{_datadir}/icons/hicolor/128x128/apps/%{name}.png
%{_datadir}/icons/hicolor/172x172/apps/%{name}.png
%{_datadir}/icons/hicolor/256x256/apps/%{name}.png
# >> files
# << files

# D-Bus service files and second *.desktop files are not allowed in Harbour (yet),
# we have to create separate package and distribute it via OpenRepos
%package open-url
Summary: Open url support for %{name}
BuildArch: noarch

%description open-url
Open url support for %{name}

PackageName: OSM Scout, url support
Type: desktop-application
Categories:
  - Maps
Custom:
  Repo: https://github.com/Karry/osmscout-sailfish
Icon: https://raw.githubusercontent.com/Karry/osmscout-sailfish/master/graphics/harbour-osmscout.svg
Screenshots:
  - https://raw.githubusercontent.com/Karry/osmscout-sailfish/master/graphics/screenshot-07-geo-url.png

%files open-url
%{_datadir}/applications/%{name}-open-url.desktop
%{_datadir}/dbus-1/services/cz.karry.osmscout.OSMScout.service
