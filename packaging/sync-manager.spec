Name:      sync-service
Version:   0.1.4
Release:   1
License:   Apache-2.0
Summary:   Sync manager daemon
Group:     Social & Content/API
Source0:   %{name}-%{version}.tar.gz
Source1:   sync-manager.service
Source2:   org.tizen.sync.service
Source3:   org.tizen.sync.conf

BuildRequires: cmake
BuildRequires: pkgconfig(capi-appfw-app-manager)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-network-connection)
BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(pkgmgr)
BuildRequires: pkgconfig(pkgmgr-info)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(vconf-internal-keys)
BuildRequires: pkgconfig(libxml-2.0)
BuildRequires: pkgconfig(glib-2.0) >= 2.26
BuildRequires: pkgconfig(gio-unix-2.0)
BuildRequires: pkgconfig(accounts-svc)
BuildRequires: pkgconfig(alarm-service)
BuildRequires: pkgconfig(bundle)
BuildRequires: pkgconfig(cynara-client)
BuildRequires: pkgconfig(cynara-session)
BuildRequires: pkgconfig(cynara-creds-gdbus)
%if "%{?profile}" == "mobile"
BuildRequires: pkgconfig(calendar-service2)
BuildRequires: pkgconfig(contacts-service2)
%endif
BuildRequires: pkgconfig(capi-content-media-content)
BuildRequires: pkgconfig(libtzplatform-config)


%if "%{?profile}" == "tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif


%description
sync manager service and sync framework which manages sync of registered applications


%package -n libcore-sync-client
Summary:    Sync manager client library
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n libcore-sync-client
sync client provides sync adapter functionality to register sync adapters and to get callbacks.

%package -n libcore-sync-client-devel
Summary:    Sync manager client library (Development)
Group:      Application Framework/Development

%description -n  libcore-sync-client-devel
sync client provides sync adapter functionality to register sync adapters and to get callbacks.

%define _pkgdir /usr
#%define _bindir %{_pkgdir}/bin
#%define _systemd_dir /usr/lib/systemd/system

%prep
%setup -q
cp %{SOURCE2} .

%build
_CONTAINER_ENABLE=OFF

%if "%{?profile}" == "mobile"
_CALENDAR_CONTACTS_ENABLE=ON
_FEATURE_MOBILE_PROFILE=ON
%else
_CALENDAR_CONTACTS_ENABLE=OFF
_FEATURE_MOBILE_PROFILE=OFF
%endif

cmake \
	-DCMAKE_INSTALL_PREFIX=%{_pkgdir} \
	-DPACKAGE_NAME=%{name} \
	-DBINDIR=%{_bindir} \
	-DLIBDIR=%{_libdir} \
	-DINCLUDEDIR=%{_includedir} \
	-D_SEC_FEATURE_CONTAINER_ENABLE:BOOL=${_CONTAINER_ENABLE} \
	-D_SEC_FEATURE_CALENDAR_CONTACTS_ENABLE:BOOL=${_CALENDAR_CONTACTS_ENABLE} \
	-D_SEC_FEATURE_MOBILE_PROFILE:BOOL=${_FEATURE_MOBILE_PROFILE} \
	-DVERSION=%{version}


make %{?jobs:-j%jobs}

%install
%make_install
mkdir -p %{buildroot}%{_unitdir_user}/default.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_unitdir_user}/sync-manager.service
ln -s ../sync-manager.service %{buildroot}%{_unitdir_user}/default.target.wants/sync-manager.service

mkdir -p %{buildroot}/usr/share/dbus-1/services
install -m 0644 %SOURCE2 %{buildroot}/usr/share/dbus-1/services/org.tizen.sync.service

mkdir -p %{buildroot}%{_sysconfdir}/dbus-1/session.d
install -m 0644 %SOURCE3 %{buildroot}%{_sysconfdir}/dbus-1/session.d/org.tizen.sync.conf

#mkdir -p %{buildroot}%{TZ_SYS_DATA}/sync-manager

%clean
rm -rf %{buildroot}

%post -n libcore-sync-client -p /sbin/ldconfig
%post -n libcore-sync-client-devel -p /sbin/ldconfig

%post
#chown system:system %{TZ_SYS_DATA}/sync-manager/
#systemctl enable sync-manager.service
#systemctl start sync-manager.service

%postun -n libcore-sync-client -p /sbin/ldconfig
%postun -n libcore-sync-client-devel -p /sbin/ldconfig

%files -n sync-service
%manifest sync-service.manifest
%defattr(-,root,root,-)
%config %{_sysconfdir}/dbus-1/session.d/org.tizen.sync.conf
%{_bindir}/*
#%{_unitdir}/*
#%{TZ_SYS_DATA}/sync-manager/
%{_unitdir_user}/sync-manager.service
%{_unitdir_user}/default.target.wants/sync-manager.service
%attr(0644,root,root) /usr/share/dbus-1/services/org.tizen.sync.service

%files -n libcore-sync-client
%manifest libcore-sync-client.manifest
%defattr(-,root,root,-)
%{_libdir}/libcore-sync-client.so*

%files -n libcore-sync-client-devel
%manifest libcore-sync-client.manifest
%defattr(-,root,root,-)
%{_includedir}/*sync*.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/libcore-sync-client.so*

