project('easy-dbuspp',
    'cpp',
    default_options: [ 'cpp_std=c++17', 'werror=true', 'cpp_args=-W -Wall' ],
    license: 'AGPL-3.0',
    version: '0',
    meson_version: '>= 0.58')

dep_gio = dependency('gio-2.0')

incdir = include_directories('include')

easy_dbuspp = library('easy-dbuspp',
   [
      'src/g_thread_pool.cpp',
      'src/object.cpp',
      'src/proxy.cpp',
      'src/org_freedesktop_dbus_proxy.cpp',
      'src/session_manager.cpp'
   ],
   include_directories: incdir,
   dependencies: [
      dep_gio,
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

test_methods = executable('methods',
   'tests/methods.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
   ],
   link_with: easy_dbuspp
)

test('methods', test_methods)

test_properties = executable('properties',
   'tests/properties.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
   ],
   link_with: easy_dbuspp
)

test('properties', test_properties)

run_target('cppcheck', command : ['cppcheck', '--enable=all',
   '--std=c++17', '--suppress=missingIncludeSystem', '--suppress=returnByReference',
   '--suppress=unusedFunction', '--check-level=exhaustive',
   '--project=' + join_paths(meson.project_build_root(), 'compile_commands.json')])
