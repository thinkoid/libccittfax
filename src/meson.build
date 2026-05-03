# -*- mode: meson; -*-

ccittfax_sources = files(
  'cf.c',
  'cfc.c',
  'cfc_common.c',
  'cfc_g3_1d.c',
  'cfc_g3_2d.c',
  'cfc_g4.c',
  'cfc_tables.c',
  'cfd.c',
  'cfd_g3_1d.c',
  'cfd_g3_2d.c',
  'cfd_g4.c',
  'cfd_trie.c',
  'lib.c',
)

ccittfax_lib = static_library('ccittfax',
  ccittfax_sources,
  include_directories : inc_lib,
  pic : false,
)

ccittfax_dep = declare_dependency(
  link_with : ccittfax_lib,
  include_directories : inc_lib,
)
