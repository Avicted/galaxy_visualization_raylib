project('galaxy_visualization_raylib', 'cpp', version: '0.1.1', default_options: ['cpp_std=c++20'])

# Compiler options based on system type
if host_machine.system() == 'linux'
    add_project_arguments([
        '-Wall',
        '-Wextra',
        '-Wpedantic',
        '-Werror',
        '-Wno-unused-parameter',
        '-Wno-unused-variable',
        '-Wno-unused-function',
        '-Wno-unused-const-variable',
        '-Wno-unused-lambda-capture',
        '-Wno-unused-local-typedef',
        '-Wno-unused-value',
        '-Wno-missing-field-initializers',
        '-Wno-unused-but-set-variable'
    ], language: 'cpp')
elif host_machine.system() == 'windows'
    # MSVC specific flags can be added here
    add_project_arguments([
        '/W4'  # Example warning level for MSVC
    ], language: 'cpp')
endif

# Fetching dependencies
raylib_dep = dependency('raylib', method: 'cmake', required: true)

# Executable definition
executable('galaxy_visualization_raylib', 'frontend.cpp',
    dependencies: raylib_dep,
    install: true
)

# Install directories and resources
# Copying directories to the build directory
fonts_copy = custom_target(
  'copy_fonts',
  output: 'fonts',
  command: ['cp', '-r', meson.source_root() / 'fonts', meson.build_root() / 'fonts'],
  build_by_default: true
)

copy_input_data = custom_target(
  'copy_input_data',
  output: 'input_data',
  command: ['cp', '-r', meson.source_root() / 'input_data', meson.build_root() / 'input_data'],
  build_by_default: true
)

copy_shaders = custom_target(
  'copy_shaders',
  output: 'shaders',
  command: ['cp', '-r', meson.source_root() / 'shaders', meson.build_root() / 'shaders'],
  build_by_default: true
)

