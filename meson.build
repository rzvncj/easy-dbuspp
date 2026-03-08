# SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
#
# SPDX-License-Identifier: AGPL-3.0-only

project(
    'easy-dbuspp',
    'cpp',
    default_options: ['cpp_std=c++17', 'werror=true', 'cpp_args=-W -Wall'],
    license: 'AGPL-3.0',
    version: '0.1.0',
    meson_version: '>= 0.58',
)

compiler = meson.get_compiler('cpp')
if compiler.get_id() == 'gcc' and compiler.version().version_compare('<10.0')
    message('GCC compiler version < 10 found, adding "-lstdc++fs" linking flag')
    add_project_link_arguments(['-lstdc++fs'], language: 'cpp')
endif

dep_gio = [dependency('gio-2.0'), dependency('gio-unix-2.0')]
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

easy_dbuspp = library(
    'easydbuspp',
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
    version: meson.project_version(),
    soversion: '0',
    install: true,
)

pkg.generate(easy_dbuspp)

example_names = [
    'firefox_openurl',
    'idle_detect',
    'unix_fd_object',
    'unix_fd_proxy',
]

foreach name : example_names
    executable(
        name,
        'examples' / name + '.cpp',
        include_directories: incdir,
        dependencies: [
            dep_gio,
            dep_threads,
        ],
        link_with: easy_dbuspp,
    )
endforeach

test_names = [
    'methods',
    'dbus_context',
    'properties',
    'pre_request_handler',
    'broadcast_signal',
    'unicast_signal',
    'param_names',
    'watcher',
]

foreach name : test_names
    exe = executable(
        name,
        'tests' / name + '.cpp',
        include_directories: incdir,
        dependencies: [
            dep_gio,
            dep_threads,
        ],
        link_with: easy_dbuspp,
    )
    test(name, exe, is_parallel: false)
endforeach

cppcheck = find_program('cppcheck', required: false)

if cppcheck.found()
    run_target(
        'cppcheck',
        command: [
            'cppcheck',
            '--enable=all',
            '--std=c++17',
            '--suppress=missingIncludeSystem',
            '--suppress=returnByReference',
            '--suppress=unusedFunction',
            '--suppress=cstyleCast',
            '--check-level=exhaustive',
            '--project='
            + join_paths(meson.project_build_root(), 'compile_commands.json'),
        ],
    )
endif
