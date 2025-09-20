project('galaxy_visualization_raylib', 'c',
  version : '0.2.1',
  default_options : [
    'c_std=c99',
    
    'buildtype=debug',
    'optimization=0',
    'c_args=-ggdb',
    #'buildtype=release',
    #'optimization=3',

    'warning_level=3',
    'c_args=-Wall',
    'c_args=-Wextra',
    'c_args=-Werror',
  ]
)

raylib_dep = dependency('raylib', required: true)

inc_dir = include_directories('include')

exe = executable(
    'galaxy_visualization_raylib', 
    'src/main.c',
    dependencies: raylib_dep,
    link_args : [
      '-lm',
    ],
    include_directories: inc_dir,
    install: false,
)

meson.add_install_script('cp', '-r', 'assets', meson.project_build_root() + '/assets')
