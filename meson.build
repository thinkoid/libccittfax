# -*- mode: meson; -*-

project('ccittfax', 'c',
  version : '0.1.0',
  license : 'MIT',
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

inc_lib      = include_directories('include')
inc_thirdparty = include_directories('third-party')

subdir('src')
subdir('examples')
subdir('utils')
subdir('tests')
