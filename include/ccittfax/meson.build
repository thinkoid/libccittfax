# -*- mode: meson; -*-

conf = configuration_data()
conf.set_quoted('CCITTFAX_VERSION', meson.project_version())

configure_file(
  output        : 'config.h',
  configuration : conf,
  install       : true,
  install_dir   : get_option('includedir') / 'ccittfax',
)
