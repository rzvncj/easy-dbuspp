project('easy-dbuspp',
    'cpp',
    default_options: [ 'cpp_std=c++17' ],
    license: 'AGPL-3.0',
    version: '0',
    meson_version: '>= 0.58')

dep_gio = dependency('gio-2.0')

executable('dbus-test',
     [
        'dbus_object.cpp',
        'session_manager.cpp',
        'main.cpp',
     ],
     dependencies: [
        dep_gio
     ],
)
