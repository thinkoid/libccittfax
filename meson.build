project('libccittfax', 'c', version : '0.0.1', license : 'WTFPL')

includes = include_directories('include')

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: true)

subdir('include')
subdir('src')
subdir('tests')
subdir('utils')

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : libccittfax,
                 version : '0.0.1',
                 name : 'libccittfax',
                 filebase : 'ccittfax',
                 description : 'CCITTFAX Group 3 and 4.')
