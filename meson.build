project(
  'galaxy_visualization_raylib', 
  'cpp', 
  version: '0.2.0', 
  default_options: [
    'cpp_std=c++20',
    'warning_level=3',
    'buildtype=release',
    'optimization=2'
  ],
)

# Compiler options based on system type
if host_machine.system() == 'linux' or host_machine.system() == 'darwin' # macOS
    add_project_arguments([
        '-Wpedantic',
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
    add_project_arguments(['/W4'], language: 'cpp')
endif

# Find the system-installed Raylib library
raylib_dep = dependency('raylib', required: true)

# Include directories
inc_dir = include_directories('include')

# Define the executable
exe = executable(
    'galaxy_visualization_raylib', 
    'src/frontend.cpp',
    dependencies: raylib_dep,
    include_directories: inc_dir,
    install: false,
)

# Install directories and resources

# Create a resources directory in the build directory
resources_dir = custom_target(
  'resources_dir',
  output: 'resources',
  command: ['mkdir', '-p', meson.build_root() / './resources'],
  build_by_default: true
)

fonts_copy = custom_target(
  'copy_fonts',
  output: 'fonts',
  command: ['cp', '-r', meson.source_root() / './resources/fonts', meson.build_root() / './resources'],
  build_by_default: true
)

copy_input_data = custom_target(
  'copy_input_data',
  output: 'input_data',
  command: ['cp', '-r', meson.source_root() / 'input_data', meson.build_root() / './input_data'],
  build_by_default: true
)

copy_shaders = custom_target(
  'copy_shaders',
  output: 'shaders',
  command: ['cp', '-r', meson.source_root() / 'shaders', meson.build_root() / 'shaders'],
  build_by_default: true
)
