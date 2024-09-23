project(
  'sdeventplus',
  'cpp',
  version: '0.1',
  meson_version: '>=0.57.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++20',
  ])  

cpp = meson.get_compiler('cpp')

if cpp.has_header('poll.h')
  add_project_arguments('-DPLDM_HAS_POLL=1', language: 'cpp')
endif

subdir('dump')
