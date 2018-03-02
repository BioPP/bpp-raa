%define _prefix /usr

URL: https://github.com/BioPP/bpp-raa

Name: bpp-raa
Version: 2.4.0
Release: 1
License: CECILL-2.0
Vendor: The Bio++ Project
Source: %{name}-%{version}.tar.gz
Summary: Bio++ Remote Acnuc Access library
Group: Development/Libraries/C and C++
Requires: bpp-core = %{version}
Requires: bpp-seq = %{version}

BuildRoot: %{_builddir}/%{name}-root
BuildRequires: cmake >= 2.8.11
BuildRequires: gcc-c++ >= 4.7.0
BuildRequires: libbpp-core4 = %{version}
BuildRequires: libbpp-core-devel = %{version}
BuildRequires: libbpp-seq12 = %{version}
BuildRequires: libbpp-seq-devel = %{version}
BuildRequires: zlib >= 1.2.3
BuildRequires: zlib-devel >= 1.2.3

AutoReq: yes
AutoProv: yes

%description
This library contains utilitary and classes to query public database (GenBank, EMBL, SwissProt, etc) using acnuc.
It is part of the Bio++ project.

%package -n libbpp-raa4
Summary: Bio++ Remote Acnuc Access library
Group: Development/Libraries/C and C++

%description -n libbpp-raa4
This library contains utilitary and classes to query public database (GenBank, EMBL, SwissProt, etc) using acnuc.
It is part of the Bio++ project.

%package -n libbpp-raa-devel
Summary: Libraries, includes to develop applications with %{_basename}
Group: Development/Libraries/C and C++
Requires: libbpp-raa4 = %{_version}
Requires: libbpp-seq12 = %{_version}
Requires: libbpp-seq-devel = %{_version}
Requires: libbpp-core4 = %{_version}
Requires: libbpp-core-devel = %{_version}
Requires: zlib >= 1.2.3
Requires: zlib-devel >= 1.2.3

%description -n libbpp-raa-devel
The libbpp-raa-devel package contains the header files and static libraries for
building applications which use %{name}.

%prep
%setup -q

%build
CFLAGS="$RPM_OPT_FLAGS"
CMAKE_FLAGS="-DCMAKE_INSTALL_PREFIX=%{_prefix} -DBUILD_TESTING=OFF"
cmake $CMAKE_FLAGS .
make

%install
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%post -n libbpp-raa4 -p /sbin/ldconfig

%postun -n libbpp-raa4 -p /sbin/ldconfig

%files -n libbpp-raa4
%defattr(-,root,root)
%doc AUTHORS.txt COPYING.txt INSTALL.txt ChangeLog
%{_prefix}/%{_lib}/lib*.so.*

%files -n libbpp-raa-devel
%defattr(-,root,root)
%doc AUTHORS.txt COPYING.txt INSTALL.txt ChangeLog
%dir %{_prefix}/%{_lib}/cmake/
%dir %{_prefix}/%{_lib}/cmake/bpp-raa
%{_prefix}/%{_lib}/lib*.so
%{_prefix}/%{_lib}/lib*.a
%{_prefix}/%{_lib}/cmake/bpp-raa/bpp-raa*.cmake
%{_prefix}/include/*

%changelog
* Mon Feb 25 2018 Julien Dutheil <julien.dutheil@univ-montp2.fr> 2.4.0-1
- Increased interface number
- Removed dynamic exceptions declarations.
* Tue Jun 06 2017 Julien Dutheil <julien.dutheil@univ-montp2.fr> 2.3.1-1
- Increased interface number
* Wed May 10 2017 Julien Dutheil <julien.dutheil@univ-montp2.fr> 2.3.0-1
- Compatibility update.
* Mon Sep 22 2014 Julien Dutheil <julien.dutheil@univ-montp2.fr> 2.2.0-1
- Compatibility update.
* Thu Mar 07 2013 Julien Dutheil <julien.dutheil@univ-montp2.fr> 2.1.0-1
- Compatibility update.
* Thu Feb 09 2012 Julien Dutheil <julien.dutheil@univ-montp2.fr> 2.0.3-1
- Compatibility update.
* Thu Jun 09 2011 Julien Dutheil <julien.dutheil@univ-montp2.fr> 2.0.2-1
- Compatibility update.
* Mon Feb 28 2011 Julien Dutheil <julien.dutheil@univ-montp2.fr> 2.0.1-1
* Mon Feb 07 2011 Julien Dutheil <julien.dutheil@univ-montp2.fr> 2.0.0-1
* Thu Mar 25 2010 Julien Dutheil <julien.dutheil@univ-montp2.fr> 1.0.0-1
* Wed Jun 24 2009 Julien Dutheil <jdutheil@birc.au.dk> 0.1.0-1
- First draft of the spec file

