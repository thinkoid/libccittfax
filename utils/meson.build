# -*- mode: meson; -*-

executable('generate_trie',
  files('generate_trie.c'),
  dependencies : ccittfax_dep,
)

executable('mkbmp',
  files('mkbmp.c'),
  dependencies : ccittfax_dep,
  include_directories : [inc_thirdparty],
  c_args : ['-Wno-format-zero-length', '-Wno-unused-function'],
)
