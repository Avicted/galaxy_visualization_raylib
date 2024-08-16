cmake_minimum_required(VERSION 3.11)

project(galaxy_visualization_raylib VERSION 0.2.0 LANGUAGES CXX)

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Includes dir
include_directories(${CMAKE_SOURCE_DIR}/includes)

# -------------------------------------------------------------------------------------

# Print the OS
message(STATUS "----------------------------------------------")
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "----------------------------------------------")

# Find the system-installed Raylib library
find_package(raylib REQUIRED)


# Add your executable
add_executable(${PROJECT_NAME} src/frontend.cpp)

# Link against raylib
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Set compile flags specific to your project
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin") # macOS
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic 
        -Werror 
        -Wno-unused-parameter 
        -Wno-unused-variable 
        -Wno-unused-function 
        -Wno-unused-but-set-variable 
        -Wno-unused-value 
        -Wno-unused-result 
        -Wno-unused-local-typedefs 
        -Wno-missing-field-initializers
    )
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4) # Example for MSVC
endif()

# Print the compile flags for your target
get_target_property(PROJECT_COMPILE_FLAGS ${PROJECT_NAME} COMPILE_OPTIONS)
message(STATUS "----------------------------------------------")
message(STATUS "Compile flags for ${PROJECT_NAME}: ${PROJECT_COMPILE_FLAGS}")
message(STATUS "----------------------------------------------")

# -------------------------------------------------------------------------------------

# Copy resources to the build directory after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/input_data $<TARGET_FILE_DIR:${PROJECT_NAME}>/input_data)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)
