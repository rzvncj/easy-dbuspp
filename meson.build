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
test('methods', test_methods, is_parallel: false)

test_properties = executable('properties',
   'tests/properties.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
   ],
   link_with: easy_dbuspp
)
test('properties', test_properties, is_parallel: false)

test_pre_request_handler = executable('pre_request_handler',
   'tests/pre_request_handler.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
   ],
   link_with: easy_dbuspp
)
test('pre_request_handler', test_pre_request_handler, is_parallel: false)

test_broadcast_signal = executable('broadcast_signal',
   'tests/broadcast_signal.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
   ],
   link_with: easy_dbuspp
)
test('broadcast_signal', test_broadcast_signal, is_parallel: false)

test_unicast_signal = executable('unicast_signal',
   'tests/unicast_signal.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
   ],
   link_with: easy_dbuspp
)
test('unicast_signal', test_unicast_signal, is_parallel: false)

run_target('cppcheck', command : ['cppcheck', '--enable=all',
   '--std=c++17', '--suppress=missingIncludeSystem', '--suppress=returnByReference',
   '--suppress=unusedFunction', '--check-level=exhaustive',
   '--project=' + join_paths(meson.project_build_root(), 'compile_commands.json')])
