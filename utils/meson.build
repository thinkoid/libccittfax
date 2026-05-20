# -*- mode: meson; -*-

inc_utils = include_directories('.')

# Private static library: support code shared by examples and utils programs.
# Not installed.
support_lib = static_library('support',
  files('file.c', 'image.c'),
  dependencies : ccittfax_dep,
  include_directories : [inc_thirdparty],
  c_args : ['-Wno-format-zero-length', '-Wno-unused-function'],
)

support_dep = declare_dependency(
  link_with : support_lib,
  include_directories : inc_utils,
)

executable('generate_trie',
  files('generate_trie.c'),
  dependencies : ccittfax_dep,
)

executable('mkbmp',
  files('mkbmp.c'),
  dependencies : [ccittfax_dep, support_dep],
  include_directories : [inc_thirdparty],
  c_args : ['-Wno-format-zero-length', '-Wno-unused-function'],
)

executable('mkraw',
  files('mkraw.c'),
  dependencies : [ccittfax_dep, support_dep],
  include_directories : [inc_thirdparty],
  link_args : ['-lm'],
  c_args : ['-Wno-format-zero-length', '-Wno-unused-function'],
)

executable('mkg4',
  files('mkg4.c'),
  dependencies : ccittfax_dep,
  c_args : ['-Wno-format-zero-length'],
)
