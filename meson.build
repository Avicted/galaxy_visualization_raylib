project('galaxy_visualization_raylib', 'c',
  version : '0.3.0',
  default_options : [
    'c_std=c99',
    
    #'buildtype=debug',
    #'optimization=0',
    #'c_args=-ggdb',
    
    'buildtype=release',
    'optimization=3',

    'warning_level=3',
    'c_args=-Wall',
    'c_args=-Wextra',
    'c_args=-Werror',
    'c_args=-Wpedantic',
  ]
)

raylib_dep = dependency('raylib', required: true)

inc_dir = include_directories('include', 'src')

# Unity build: single translation unit for faster compilation
sources = [
    'src/unity_build.c',
]

# Individual sources (use instead of unity_build.c if needed for debugging):
# sources = [
#     'src/main.c',
#     'src/app.c',
#     'src/camera.c',
#     'src/data_loader.c',
#     'src/renderer.c',
#     'src/shaders.c',
#     'src/transforms.c',
#     'src/ui.c',
#     'src/utils.c',
# ]

exe = executable(
    'galaxy_visualization_raylib', 
    sources,
    dependencies: raylib_dep,
    c_args: [
      '-march=native',
    ],
    link_args : [
      '-lm',
    ],
    include_directories: inc_dir,
    install: false,
)

meson.add_install_script('cp', '-r', 'assets', meson.project_build_root() + '/assets')
