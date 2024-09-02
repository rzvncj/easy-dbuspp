project('easy-dbuspp',
    'cpp',
    default_options: [ 'cpp_std=c++17', 'werror=true', 'cpp_args=-W -Wall' ],
    license: 'AGPL-3.0',
    version: '0',
    meson_version: '>= 0.58')

dep_gio = dependency('gio-2.0')
dep_asio = dependency('asio')

incdir = include_directories('include')

easy_dbuspp = library('easy-dbuspp',
   [
      'src/object.cpp',
      'src/proxy.cpp',
      'src/org_freedesktop_dbus_proxy.cpp',
      'src/session_manager.cpp'
   ],
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_asio,
   ],
   install : true
)

executable('dbus-test-service',
   'examples/service/main.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
   ],
   link_with: easy_dbuspp
)

executable('dbus-test-proxy',
   'examples/proxy/main.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
   ],
   link_with: easy_dbuspp
)
