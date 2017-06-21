#
#    fty-exdialogue - Example agent
#
#    Copyright (C) 2014 - 2017 Eaton                                        
#                                                                           
#    This program is free software; you can redistribute it and/or modify   
#    it under the terms of the GNU General Public License as published by   
#    the Free Software Foundation; either version 2 of the License, or      
#    (at your option) any later version.                                    
#                                                                           
#    This program is distributed in the hope that it will be useful,        
#    but WITHOUT ANY WARRANTY; without even the implied warranty of         
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
#    GNU General Public License for more details.                           
#                                                                           
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.            
#

# To build with draft APIs, use "--with drafts" in rpmbuild for local builds or add
#   Macros:
#   %_with_drafts 1
# at the BOTTOM of the OBS prjconf
%bcond_with drafts
%if %{with drafts}
%define DRAFTS yes
%else
%define DRAFTS no
%endif


Name:           fty-exdialogue
Version:        1.0.0
Release:        1
Summary:        example agent
License:        GPL-2.0+
URL:            https://42ity.org
Source0:        %{name}-%{version}.tar.gz
Group:          System/Libraries
# Note: ghostscript is required by graphviz which is required by
#       asciidoc. On Fedora 24 the ghostscript dependencies cannot
#       be resolved automatically. Thus add working dependency here!
BuildRequires:  ghostscript
BuildRequires:  asciidoc
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:  pkgconfig
BuildRequires:  systemd-devel
BuildRequires:  systemd
%{?systemd_requires}
BuildRequires:  xmlto
BuildRequires:  gcc-c++
BuildRequires:  zeromq-devel
BuildRequires:  czmq-devel
BuildRequires:  malamute-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
fty-exdialogue example agent.

%package -n libfty_exdialogue0
Group:          System/Libraries
Summary:        example agent shared library

%description -n libfty_exdialogue0
This package contains shared library for fty-exdialogue: example agent

%post -n libfty_exdialogue0 -p /sbin/ldconfig
%postun -n libfty_exdialogue0 -p /sbin/ldconfig

%files -n libfty_exdialogue0
%defattr(-,root,root)
%{_libdir}/libfty_exdialogue.so.*

%package devel
Summary:        example agent
Group:          System/Libraries
Requires:       libfty_exdialogue0 = %{version}
Requires:       zeromq-devel
Requires:       czmq-devel
Requires:       malamute-devel

%description devel
example agent development tools
This package contains development files for fty-exdialogue: example agent

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libfty_exdialogue.so
%{_libdir}/pkgconfig/libfty_exdialogue.pc
%{_mandir}/man3/*
%{_mandir}/man7/*


%prep
#FIXME: %{error:...} did not worked for me
%if %{with python_cffi}
%if %{without drafts}
echo "FATAL: python_cffi not yet supported w/o drafts"
exit 1
%endif
%endif

%setup -q

%build
[ -f autogen.sh ] && sh autogen.sh
%{configure} --enable-drafts=%{DRAFTS} --with-systemd-units
make %{_smp_mflags}


%install
make install DESTDIR=%{buildroot} %{?_smp_mflags}

# remove static libraries
find %{buildroot} -name '*.a' | xargs rm -f
find %{buildroot} -name '*.la' | xargs rm -f


%files
%defattr(-,root,root)
%{_bindir}/fty-exdialogue
%{_mandir}/man1/fty-exdialogue*
%config(noreplace) %{_sysconfdir}/fty-exdialogue/fty-exdialogue.cfg
/usr/lib/systemd/system/fty-exdialogue.service
%dir %{_sysconfdir}/fty-exdialogue
%if 0%{?suse_version} > 1315
%post
%systemd_post fty-exdialogue.service
%preun
%systemd_preun fty-exdialogue.service
%postun
%systemd_postun_with_restart fty-exdialogue.service
%endif

%changelog
