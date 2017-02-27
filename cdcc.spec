Name:		cdcc
Version:	0.1
Release:	1%{?dist}
Summary:	Compile flags database generating compiler wrapper

License:	GPL3
URL:		https://github.com/gicmo/cdcc
Source0:	https://github.com/gicmo/cdcc/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:	cmake
BuildRequires:	pkgconfig(glib-2.0)
BuildRequires:	pkgconfig(gio-2.0)
BuildRequires:	pkgconfig(json-glib-1.0)
BuildRequires:	sqlite-devel
Requires:	gcc

%description
A wrapper for C/C++ compilers, e.g. GCC and clang, that will collect the
compile flags used into a sqlite3 database, from which compile_commands.json
files can be generated.

%prep
%setup -q -n cdcc-%{version}

%build
mkdir -p build
cd build
%cmake .. \
       -DGLOBAL_INSTALL=YES \
       -DCMAKE_INSTALL_PREFIX=/usr \
       -DCMAKE_BUILD_TYPE=Release
make %{?_smp_mflags}

%install
cd build
make install DESTDIR=%{buildroot}

%files
# %license LICENSE
%doc README.md
%{_bindir}/cdcc-*


%changelog
* Mon Feb 27 2017 Chrisian Kellner <christian@kellner.me>
- Initial package
