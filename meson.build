project('ohhey', 'cpp', default_options: ['cpp_std=c++17'])

cli_deps = [
  dependency('tclap', required: true),
  dependency('dlib-1', required: true),
]

cpp_args = []

is_release = get_option('release')

if is_release 
  cpp_args += '-O3'
endif

common_files = [
  'src/cli/add.cpp',
  'src/lib/webcam-v4l2/webcam.cpp',
  'src/lib/compare.cpp',
  'src/lib/util.cpp',
]

cli_files = common_files + [
  'src/cli/main.cpp',
]

executable('ohhey', cli_files, 
  dependencies: cli_deps, 
  cpp_args: cpp_args, 
  install: true)

pam_link_flags = ['-lpam', '-lpam_misc']

pam_lib_files = common_files + [
  'src/pam/pam_ohhey.cpp',
]

library('pam_ohhey', pam_lib_files, 
  version : '0.0.1', 
  soversion : '0',
  dependencies: cli_deps, 
  link_args: pam_link_flags, 
  cpp_args: cpp_args, 
  install: true,
  install_dir: '/lib/security')

pam_ohhey_test_files = common_files + [
  'src/pam/pam_ohhey.cpp',
  'src/pam/pam_ohhey_test.cpp',
]

executable('pam_ohhey_test', pam_ohhey_test_files, dependencies: cli_deps, cpp_args: cpp_args, 
  link_args: pam_link_flags)

install_data(sources : 'etc/config.ini', install_dir : '/etc/ohhey')

dlib_data_files = [
  'dlib-data/dlib_face_recognition_resnet_model_v1.dat',
  'dlib-data/mmod_human_face_detector.dat',
  'dlib-data/shape_predictor_5_face_landmarks.dat']

# install_data(sources : dlib_data_files, install_dir : '/usr/local/share/ohhey')
install_data(sources : dlib_data_files)
