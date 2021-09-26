project('ohhey', 'cpp', default_options: ['cpp_std=c++17'])

cmake = import('cmake')

v4l2_proj = cmake.subproject('libv4l2cpp')

cli_deps = [
  dependency('tclap', required: true),
  dependency('dlib', required: true),
  v4l2_proj.dependency('libv4l2cpp'),
]

cpp_args = []

if get_option('release')
  cpp_args += '-O3'
  cpp_args += '-fdata-sections'
  cpp_args += '-ffunction-sections'
endif

cli_files = [
  'src/cli/main.cpp',
]

executable('ohhey', cli_files, dependencies: cli_deps, cpp_args: cpp_args)
