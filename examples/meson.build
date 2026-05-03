# -*- mode: meson; -*-

executable('cfc',
  files('cfc.c', 'image.c'),
  dependencies : ccittfax_dep,
  include_directories : [inc_thirdparty],
  link_args : ['-lm'],
  c_args : ['-Wno-format-zero-length', '-Wno-unused-function', '-D_POSIX_C_SOURCE=200809L'],
)

executable('cfd',
  files('cfd.c', 'file.c'),
  dependencies : ccittfax_dep,
  c_args : ['-Wno-format-zero-length', '-Wno-unused-function', '-D_POSIX_C_SOURCE=200809L'],
)
