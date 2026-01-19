project('galaxy_visualization_raylib', 'c',
  version : '0.5.3',
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
    'c_args=-Wpedantic',
  ]
)

raylib_static = get_option('raylib_static')

if raylib_static
  raylib_sub = subproject('raylib', default_options: [
    'warning_level=0',
    'werror=false',
    'c_args=-w',
  ])
  raylib_dep = raylib_sub.get_variable('raylib_dep')
else
  raylib_dep = dependency('raylib', required: true, static: false)
endif

embed_assets = get_option('embed_assets')
extra_c_args = ['-DAPP_VERSION="' + meson.project_version() + '"']
platform_c_args = []

extra_c_args += ['-Werror']

if host_machine.system() == 'windows'
  extra_c_args += ['-Wno-error=type-limits']
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

if host_machine.system() == 'windows'
  magick_prog = find_program('magick', 'convert', required: true)
  icon_resources = custom_target(
    'app_icon_resources',
    input: files('assets/images/app_icon.png'),
    output: ['app_icon.ico', 'app.rc'],
    command: [
      'bash',
      meson.project_source_root() / 'scripts/generate_windows_icon_resources.sh',
      '@INPUT@',
      '@OUTDIR@',
      magick_prog,
    ],
    build_by_default: true,
  )

  windows_mod = import('windows')
  sources += windows_mod.compile_resources(icon_resources[1])
endif

platform_label = host_machine.system()
if platform_label == 'windows'
  platform_label = 'Win64'
elif platform_label == 'linux'
  platform_label = 'Linux'
endif

exe_name = 'Galaxy_Visualization_' + platform_label + '_v' + meson.project_version()

exe = executable(
    exe_name,
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
