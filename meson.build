project('ohhey', 'cpp', default_options: ['cpp_std=c++17'])

cli_deps = [
  dependency('tclap', required: true),
  dependency('dlib', required: true),
]

cpp_args = []

if get_option('release')
  cpp_args += '-O3'
endif

cli_files = [
  'src/cli/main.cpp',
  'src/lib/webcam-v4l2/webcam.cpp',
]

executable('ohhey', cli_files, dependencies: cli_deps, cpp_args: cpp_args)
