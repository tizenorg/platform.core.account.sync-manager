Name:      sync-service
Version:   0.0.1
Release:   1
License:   Apache-2.0
Summary:   A Samsung sync framework library in SLP C API
Group:     Social & Content/API
Source0:   %{name}-%{version}.tar.gz

%if "%{?tizen_profile_name}" == "wearable"
ExcludeArch: %{arm} %ix86 x86_64
%endif

%if "%{?tizen_profile_name}" == "tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif

BuildRequires: cmake
BuildRequires: pkgconfig(capi-system-info)
BuildRequires: pkgconfig(capi-system-runtime-info)
BuildRequires: pkgconfig(capi-system-device)
BuildRequires: pkgconfig(capi-appfw-app-manager)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-appfw-package-manager)
BuildRequires: pkgconfig(capi-network-connection)
BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(db-util)
BuildRequires: pkgconfig(pkgmgr)
BuildRequires: pkgconfig(pkgmgr-info)
BuildRequires: pkgconfig(appsvc)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(vconf-internal-keys)
BuildRequires: pkgconfig(libxml-2.0)
BuildRequires: pkgconfig(glib-2.0) >= 2.26
BuildRequires: pkgconfig(gio-unix-2.0)
BuildRequires: pkgconfig(accounts-svc)
BuildRequires: pkgconfig(alarm-service)
BuildRequires: pkgconfig(bundle)
BuildRequires: pkgconfig(calendar-service2)
BuildRequires: pkgconfig(contacts-service2)
BuildRequires: python-xml
#BuildRequires: pkgconfig(vasum)

%description
sync manager service and sync framework which manages sync of registered applications


%package -n libcore-sync-client
Summary:   sync manager client library.
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n libcore-sync-client
sync client provides sync adapter functionality to register sync adapters and to get callbacks.

%package -n libcore-sync-client-devel
Summary:   sync client library (Development)
Group:    Application Framework/Development

%description -n  libcore-sync-client-devel
sync client provides sync adapter functionality to register sync adapters and to get callbacks.

%define _pkgdir /usr
%define _bindir %{_pkgdir}/bin
%define _systemd_dir /usr/lib/systemd/system

%prep
%setup -q

_CONTAINER_ENABLE=ON

cmake \
	-DCMAKE_INSTALL_PREFIX=%{_pkgdir} \
	-DPACKAGE_NAME=%{name} \
	-DBINDIR=%{_bindir} \
	-DLIBDIR=%{_libdir} \
	-D_SEC_FEATURE_CONTAINER_ENABLE:BOOL=${_CONTAINER_ENABLE} \
	-DMANIFESTDIR=%{_manifestdir} \
	-DVERSION=%{version}

make %{?jobs:-j%jobs}

%install
%make_install
mkdir -p %{buildroot}/opt/usr/data/sync-manager

%clean
rm -rf %{buildroot}

%post
chsmack -a sync-manager::db -e sync-manager /opt/usr/data/sync-manager/

chown system:system /opt/usr/data/sync-manager/
systemctl enable sync-manager.service
systemctl start sync-manager.service

%files -n sync-service
%manifest sync-service.manifest
%defattr(-,system,system,-)

%{_bindir}/*
%{_systemd_dir}/*
/opt/usr/data/sync-manager/

%files -n libcore-sync-client
%defattr(-,root,root,-)
%{_libdir}/libcore-sync-client.so*

%files -n libcore-sync-client-devel
%defattr(-,root,root,-)
%{_includedir}/*sync*.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/libcore-sync-client.so*
