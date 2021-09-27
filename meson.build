project('ohhey', 'cpp', default_options: ['cpp_std=c++17'])

cli_deps = [
  dependency('tclap', required: true),
  dependency('dlib', required: true),
]

cpp_args = []

if get_option('release')
  cpp_args += '-O3'
endif

common_files = [
  'src/lib/webcam-v4l2/webcam.cpp',
]

cli_files = common_files + [
  'src/cli/main.cpp',
]

executable('ohhey', cli_files, dependencies: cli_deps, cpp_args: cpp_args)

pam_link_flags = ['-lpam', '-lpam_misc']

pam_lib_files = common_files + [
  'src/pam/pam_ohhey.cpp',
]

library('pam_ohhey', pam_lib_files, version : '0.0.1', soversion : '0',
  dependencies: cli_deps, link_args: pam_link_flags, cpp_args: cpp_args)

pam_ohhey_test_files = common_files + [
  'src/pam/pam_ohhey.cpp',
  'src/pam/pam_ohhey_test.cpp',
]

executable('pam_ohhey_test', pam_ohhey_test_files, dependencies: cli_deps, cpp_args: cpp_args, 
  link_args: pam_link_flags)
