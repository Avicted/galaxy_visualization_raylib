project('raylib', 'c')

raylib_inc = include_directories(
  'src',
  'src/external/glfw/include',
  'src/external/glfw/src'
)

raylib_c_args = [
  '-D_GNU_SOURCE',
  '-DPLATFORM_DESKTOP',
  '-DGRAPHICS_API_OPENGL_33',
  '-D_GLFW_X11',
  '-Wno-missing-braces',
  '-Werror=pointer-arith',
  '-fno-strict-aliasing',
]

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
