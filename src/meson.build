libccittfax = static_library(
    'ccittfax', [
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
        'lib.c'],
    c_args : '-DVERSION=@0@'.format(meson.project_version()),
    include_directories : includes,
    install : true)
