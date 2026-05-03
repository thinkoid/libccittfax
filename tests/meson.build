# -*- mode: meson; -*-

inc_src   = include_directories('../src')
test_args = ['-Wno-format-zero-length', '-Wno-unused-function']

# Tests that only need libccittfax
foreach t : [
  'test_cf_buffer',
  'test_cf_byte_align',
  'test_cf_get_color',
  'test_cf_new_capacity',
  'test_skip_to_newline',
  'test_cfd_g4',
  'test_cfc_g4',
]
  exe = executable(t,
    files(t + '.c'),
    dependencies : ccittfax_dep,
    c_args : test_args,
  )
  test(t, exe)
endforeach

# Tests that include private src/ headers or sources directly
foreach t : [
  'cfc_common',
  'test_cf_find_changing',
  'test_cf_setbit',
  'test_cf_setbits',
]
  exe = executable(t,
    files(t + '.c'),
    dependencies : ccittfax_dep,
    include_directories : inc_src,
    c_args : test_args,
  )
  test(t, exe)
endforeach

# Diagnostic executables — not registered as tests (no harness, always exit 0)
foreach t : ['change', 'trie']
  executable(t,
    files(t + '.c'),
    dependencies : ccittfax_dep,
    include_directories : inc_src,
    c_args : test_args,
  )
endforeach
