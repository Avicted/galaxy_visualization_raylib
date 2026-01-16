project('raylib', 'c')

raylib_inc = include_directories(
  'src',
  'src/external/glfw/include',
  'src/external/glfw/src'
)

raylib_sources = files(
  'src/rcore.c',
  'src/rmodels.c',
  'src/rshapes.c',
  'src/rtext.c',
  'src/rtextures.c',
  'src/raudio.c',
  'src/rglfw.c',
  'src/utils.c'
)

host_system = host_machine.system()

if host_system == 'windows'
  raylib_c_args = [
    '-DPLATFORM_DESKTOP',
    '-DGRAPHICS_API_OPENGL_33',
    '-D_GLFW_WIN32',
    '-Wno-missing-braces',
    '-Werror=pointer-arith',
    '-fno-strict-aliasing',
  ]
  raylib_libs = [
    '-lopengl32',
    '-lgdi32',
    '-lwinmm',
    '-lshell32',
    '-luser32',
    '-lkernel32',
  ]
else
  raylib_c_args = [
    '-D_GNU_SOURCE',
    '-DPLATFORM_DESKTOP',
    '-DGRAPHICS_API_OPENGL_33',
    '-D_GLFW_X11',
    '-Wno-missing-braces',
    '-Werror=pointer-arith',
    '-fno-strict-aliasing',
  ]
  raylib_libs = [
    '-lGL',
    '-lX11',
    '-lXrandr',
    '-lXinerama',
    '-lXi',
    '-lXcursor',
    '-lm',
    '-lpthread',
    '-ldl',
    '-lrt',
  ]
endif

raylib_lib = static_library(
  'raylib',
  raylib_sources,
  include_directories: raylib_inc,
  c_args: raylib_c_args,
  link_args: raylib_libs,
  pic: true
)

raylib_dep = declare_dependency(
  link_with: raylib_lib,
  include_directories: raylib_inc,
  compile_args: raylib_c_args,
  link_args: raylib_libs
)
