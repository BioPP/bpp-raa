%define _basename bpp-raa
%define _version 2.0.3
%define _release 1
%define _prefix /usr

URL: http://biopp.univ-montp2.fr/

Name: %{_basename}
Version: %{_version}
Release: %{_release}
License: CECILL-2.0
Vendor: The Bio++ Project
Source: http://biopp.univ-montp2.fr/repos/sources/%{_basename}-%{_version}.tar.gz
Summary: Bio++ Remote Acnuc Access library
Group: Development/Libraries/C and C++
Requires: bpp-core = %{_version}
Requires: bpp-seq = %{_version}

BuildRoot: %{_builddir}/%{_basename}-root
BuildRequires: cmake >= 2.6.0
BuildRequires: gcc-c++ >= 4.0.0
BuildRequires: libbpp-core2 = %{_version}
BuildRequires: libbpp-core-devel = %{_version}
BuildRequires: libbpp-seq9 = %{_version}
BuildRequires: libbpp-seq-devel = %{_version}
BuildRequires: zlib >= 1.2.3
BuildRequires: zlib-devel >= 1.2.3

AutoReq: yes
AutoProv: yes

%description
This library contains utilitary and classes to query public database GenBank, EMBL, SwissProt, etc) using acnuc.
It is part of the Bio++ project.

%package -n libbpp-raa1
Summary: Bio++ Remote Acnuc Access library
Group: Development/Libraries/C and C++

%description -n libbpp-raa1
This library contains utilitary and classes to query public database GenBank, EMBL, SwissProt, etc) using acnuc.
It is part of the Bio++ project.

%package -n libbpp-raa-devel
Summary: Libraries, includes to develop applications with %{_basename}.
Group: Development/Libraries/C and C++
Requires: libbpp-raa1 = %{_version}
Requires: libbpp-seq9 = %{_version}
Requires: libbpp-seq-devel = %{_version}
Requires: libbpp-core2 = %{_version}
Requires: libbpp-core-devel = %{_version}
Requires: zlib >= 1.2.3
Requires: zlib-devel >= 1.2.3

%description -n libbpp-raa-devel
The %libbpp-raa-devel package contains the header files and static libraries for
building applications which use %{_basename}.

%prep
%setup -q

%build
CFLAGS="$RPM_OPT_FLAGS"
CMAKE_FLAGS="-DCMAKE_INSTALL_PREFIX=%{_prefix} -DBUILD_TESTING=OFF"
if [ %{_lib} == 'lib64' ] ; then
  CMAKE_FLAGS="$CMAKE_FLAGS -DLIB_SUFFIX=64"
fi
cmake $CMAKE_FLAGS .
make

%install
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%post -n libbpp-raa1 -p /sbin/ldconfig

%postun -n libbpp-raa1 -p /sbin/ldconfig

%files -n libbpp-raa1
%defattr(-,root,root)
%doc AUTHORS.txt COPYING.txt INSTALL.txt ChangeLog
%{_prefix}/%{_lib}/lib*.so.*

%files -n libbpp-raa-devel
%defattr(-,root,root)
%doc AUTHORS.txt COPYING.txt INSTALL.txt ChangeLog
%{_prefix}/%{_lib}/lib*.so
%{_prefix}/%{_lib}/lib*.a
%{_prefix}/include/*

%changelog
* Thu Feb 09 2012 Julien Dutheil <julien.dutheil@univ-montp2.fr> 2.0.3-1
- Compatibility update.
* Thu Jun 09 2011 Julien Dutheil <julien.dutheil@univ-montp2.fr> 2.0.2-1
- Compatibility update.
* Mon Feb 28 2011 Julien Dutheil <julien.dutheil@univ-montp2.fr> 2.0.1-1
* Mon Feb 07 2011 Julien Dutheil <julien.dutheil@univ-montp2.fr> 2.0.0-1
* Thu Mar 25 2010 Julien Dutheil <julien.dutheil@univ-montp2.fr> 1.0.0-1
* Wed Jun 24 2009 Julien Dutheil <jdutheil@birc.au.dk> 0.1.0-1
- First draft of the spec file

