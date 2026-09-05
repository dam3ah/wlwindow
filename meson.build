project(
  'wlwindow',
  'cpp', 'c',
  version : '0.1',
  meson_version : '>= 1.3.0',
  default_options : ['warning_level=3', 'cpp_std=c++11'],
)

wl_scanner = find_program('wayland-scanner', required: true)

xdg_xml = files('/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml')

wl_h_gen = custom_target('xdg-shell-header', input: xdg_xml, output: 'xdg-shell-protocol.h', command : [wl_scanner, 'client-header', '@INPUT@', '@OUTPUT@'], depend_files : xdg_xml, install: false)

wl_c_gen = custom_target('xdg-shell-code', input: xdg_xml, output: 'xdg-shell-protocol.c', command : [wl_scanner, 'private-code', '@INPUT@', '@OUTPUT@' ], depend_files : xdg_xml, install: false)

dependencies = [
  dependency('wayland-client'),
  dependency('rt', required: false),
]

args = ['-fext-numeric-literals']

build_type = get_option('buildtype')
if build_type == 'debug'
  dependencies += dependency('spdlog')
  args += ['-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE', '-DDEBUG=1']
endif


inc_dirs = include_directories('include', '.')
# These arguments are only used to build the shared library
# not the executables that use the library.

sources = [
  'src/window.cpp',
  'src/wlw.cpp',
  'src/error.cpp',
  wl_h_gen,
  wl_c_gen,
]

lib = static_library(
  'wlw',
  sources,
  include_directories: inc_dirs,
  install : false,
  gnu_symbol_visibility : 'hidden',
  dependencies : dependencies,
  cpp_args: args,
)

# Make this library usable as a Meson subproject.
#wlwindow_dep = declare_dependency(
#  include_directories : inc_dirs,
#  dependencies : dependencies,
#  link_with : lib,
#)
#meson.override_dependency('wlwindow', wlwindow_dep)

# Make this library usable from the system's
# package manager.
#install_headers('window.hpp', subdir : 'wlwindow')

#pkg_mod = import('pkgconfig')
#pkg_mod.generate(
#  lib,
#  description : 'Simple WayLand window library.',
#  subdirs : 'wlwindow',
#)
