project('galaxy_visualization_raylib', 'c',
  version : '0.4.0',
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

raylib_static = get_option('raylib_static')

if raylib_static
  raylib_sub = subproject('raylib')
  raylib_dep = raylib_sub.get_variable('raylib_dep')
else
  raylib_dep = dependency('raylib', required: true, static: false)
endif

embed_assets = get_option('embed_assets')
extra_c_args = ['-DAPP_VERSION="' + meson.project_version() + '"']
platform_c_args = []

if host_machine.system() != 'windows'
  platform_c_args += ['-march=native']
endif

# Generate embedded asset headers at configure time
if embed_assets
  message('EMBED_ASSETS enabled - generating embedded headers...')
  run_command('scripts/generate_embedded.sh', meson.project_build_root() / 'embedded', check: true)
  extra_c_args += ['-DEMBED_ASSETS', '-I' + meson.project_build_root()]
endif

inc_dir = include_directories('include', 'src', 'vendor')

sources = [
    'src/unity_build.c',
]

exe = executable(
    'galaxy_visualization_raylib', 
    sources,
    dependencies: raylib_dep,
    c_args: [
    ] + platform_c_args + extra_c_args,
    link_args : [
      '-lm',
    ],
    include_directories: inc_dir,
    install: false,
)

meson.add_install_script('cp', '-r', 'assets', meson.project_build_root() + '/assets')
