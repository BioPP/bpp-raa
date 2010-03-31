%define name bpp-raa
%define version 1.0.0
%define release 1
%define _prefix /usr

Summary: The Bio++ Remote Acnuc Access library.
Name: %{name}
Version: %{version}
Release: %{release}
Vendor: The Bio++ Project
Source: http://kimura.univ-montp2.fr/BioPP/Repositories/sources/%{name}-%{version}.tar.gz
License: CeCILL 2
Group: System Environment/Libraries
BuildRoot: %{_builddir}/%{name}-root
Packager: Julien Dutheil
AutoReqProv: no
Requires: libstdc++
Requires: zlib >= 1.2.3
Requires: bpp-utils = 1.5.0
Requires: bpp-numcalc = 1.8.0
Requires: bpp-seq = 1.7.0

%description
This library contains utilitary and classes to query public database GenBank, EMBL, SwissProt, etc) using acnuc.
It is part of the Bio++ project.

%package devel
Summary: Libraries, includes to develop applications with %{name}.
Group: Development/Libraries
Requires: %{name} = %{version}
Requires: zlib-devel >= 1.2.3
Requires: bpp-utils-devel = 1.5.0
Requires: bpp-numcalc-devel = 1.8.0
Requires: bpp-seq-devel = 1.7.0

%description devel
The %{name}-devel package contains the header files and static libraries for
building applications which use %{name}.

%prep
%setup -q

%build
CFLAGS="$RPM_OPT_FLAGS" cmake -DCMAKE_INSTALL_PREFIX=%{_prefix}
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc AUTHORS COPYING INSTALL ChangeLog
%{_prefix}/lib/lib*.so
%{_prefix}/lib/lib*.so.*

%files devel
%defattr(-,root,root)
%doc AUTHORS COPYING INSTALL ChangeLog
%{_prefix}/lib/lib*.a
%{_prefix}/include/*

%changelog
* Thu Mar 25 2010 Julien Dutheil <julien.dutheil@univ-montp2.fr>
- Version 1.0.0
* Wed Jun 24 2009 Julien Dutheil <jdutheil@birc.au.dk>
- First draft of the spec file

