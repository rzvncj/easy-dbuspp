project('easy-dbuspp',
    'cpp',
    default_options: [ 'cpp_std=c++17', 'werror=true', 'cpp_args=-W -Wall' ],
    license: 'AGPL-3.0',
    version: '0',
    meson_version: '>= 0.58')

compiler = meson.get_compiler('cpp')

dep_gio = [
   dependency('gio-2.0'),
   dependency('gio-unix-2.0')
]
dep_threads = dependency('threads')

incdir = include_directories('include')

conf = configuration_data()

if build_machine.endian() == 'big'
  conf.set('IS_BIG_ENDIAN', 1)
endif

configure_file(output: 'config.h', configuration: conf)

pkg = import('pkgconfig')

install_headers(
   'include/bus_watcher.h',
   'include/bus_watcher.inl',
   'include/easydbuspp.h',
   'include/g_thread_pool.h',
   'include/idle_detector.h',
   'include/idle_detector.inl',
   'include/main_loop.h',
   'include/object.h',
   'include/object.inl',
   'include/org_freedesktop_dbus_proxy.h',
   'include/params.h',
   'include/proxy.h',
   'include/proxy.inl',
   'include/session_manager.h',
   'include/session_manager.inl',
   'include/type_mapping.h',
   'include/types.h',
)

easy_dbuspp = library('easydbuspp',
   [
      'src/g_thread_pool.cpp',
      'src/object.cpp',
      'src/proxy.cpp',
      'src/org_freedesktop_dbus_proxy.cpp',
      'src/session_manager.cpp',
      'src/bus_watcher.cpp',
      'src/main_loop.cpp',
      'src/idle_detector.cpp',
   ],
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_threads,
   ],
   install: true
)

pkg.generate(easy_dbuspp)

firefox_openurl = executable('firefox_openurl',
   'examples/firefox_openurl.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_threads,
   ],
   link_with: easy_dbuspp
)

idle_detect = executable('idle_detect',
   'examples/idle_detect.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_threads,
   ],
   link_with: easy_dbuspp
)

unix_fd_object = executable('unix_fd_object',
   'examples/unix_fd_object.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_threads,
   ],
   link_with: easy_dbuspp
)

unix_fd_proxy = executable('unix_fd_proxy',
   'examples/unix_fd_proxy.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_threads,
   ],
   link_with: easy_dbuspp
)

test_methods = executable('methods',
   'tests/methods.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_threads,
   ],
   link_with: easy_dbuspp
)
test('methods', test_methods, is_parallel: false)

test_dbus_context = executable('dbus_context',
   'tests/dbus_context.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_threads,
   ],
   link_with: easy_dbuspp
)
test('dbus_context', test_dbus_context, is_parallel: false)

test_properties = executable('properties',
   'tests/properties.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_threads,
   ],
   link_with: easy_dbuspp
)
test('properties', test_properties, is_parallel: false)

test_pre_request_handler = executable('pre_request_handler',
   'tests/pre_request_handler.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_threads,
   ],
   link_with: easy_dbuspp
)
test('pre_request_handler', test_pre_request_handler, is_parallel: false)

test_broadcast_signal = executable('broadcast_signal',
   'tests/broadcast_signal.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_threads,
   ],
   link_with: easy_dbuspp
)
test('broadcast_signal', test_broadcast_signal, is_parallel: false)

test_unicast_signal = executable('unicast_signal',
   'tests/unicast_signal.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_threads,
   ],
   link_with: easy_dbuspp
)
test('unicast_signal', test_unicast_signal, is_parallel: false)

test_param_names = executable('param_names',
   'tests/param_names.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_threads,
   ],
   link_with: easy_dbuspp
)
test('param_names', test_param_names, is_parallel: false)

test_watcher = executable('watcher',
   'tests/watcher.cpp',
   include_directories: incdir,
   dependencies: [
      dep_gio,
      dep_threads,
   ],
   link_with: easy_dbuspp
)
test('watcher', test_watcher, is_parallel: false)

cppcheck = find_program('cppcheck', required : false)

if cppcheck.found()
   run_target('cppcheck', command : ['cppcheck', '--enable=all',
      '--std=c++17', '--suppress=missingIncludeSystem', '--suppress=returnByReference',
      '--suppress=unusedFunction', '--check-level=exhaustive',
      '--project=' + join_paths(meson.project_build_root(), 'compile_commands.json')])
endif
