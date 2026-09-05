# -*- mode: meson; -*-

project('ccittfax', 'c',
  version : '0.1.0',
  license : 'WTFPL',
  default_options : [
    'c_std=c11',
    'warning_level=2',
    'werror=false',
  ],
)

add_project_arguments(
  '-Wno-implicit-fallthrough',
  language : 'c',
)

inc_lib        = include_directories('include')
inc_thirdparty = include_directories('third-party')

subdir('include/ccittfax')
subdir('src')

pkg = import('pkgconfig')
pkg.generate(ccittfax_lib,
  name        : 'ccittfax',
  description : 'CCITT Group 3/4 fax codec library',
  version     : meson.project_version(),
)

subdir('utils')
subdir('examples')
subdir('tests')
