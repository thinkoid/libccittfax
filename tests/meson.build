cfc = executable(
  'cfc', [ 'cfc.c', 'image.c' ],
  include_directories : includes,
  dependencies: m_dep,
  link_with : libccittfax)
test('compression test', cfc)

cfd = executable(
  'cfd', [ 'cfd.c', 'file.c' ],
  include_directories : includes,
  link_with : libccittfax)
test('decompression test', cfd)

change = executable(
  'change', 'change.c',
  include_directories : includes,
  link_with : libccittfax)
test('bit change finding test', change)

trie = executable(
  'trie', 'trie.c',
  include_directories : includes,
  link_with : libccittfax)
test('RLE parsing test (using trie)', trie)
