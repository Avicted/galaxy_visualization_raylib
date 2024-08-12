cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+

project(galaxy_visualization_raylib VERSION 0.1.1 LANGUAGES CXX)

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

##########################################################################################
# Add dependencies with FetchContent
##########################################################################################

function(add_git_dependency libName gitURL gitTag)  
    FetchContent_Declare(${libName}
        GIT_REPOSITORY ${gitURL}
        GIT_TAG        ${gitTag}
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    FetchContent_MakeAvailable(${libName})
    target_compile_options(${libName} PRIVATE "-w") # Disable warnings for the dependency
endfunction()

# Add Raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # don't build the supplied example games
add_git_dependency(
    raylib
    https://github.com/raysan5/raylib.git
    4.5.0
)

# -------------------------------------------------------------------------------------

# Our Project

# Print the OS
message(STATUS "----------------------------------------------")
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "----------------------------------------------")

# Add your executable
add_executable(${PROJECT_NAME} frontend.cpp)

# Link against raylib
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Set compile flags specific to your project
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
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

# Web Configurations
if (${PLATFORM} STREQUAL "Web")
    # Tell Emscripten to build an example.html file.
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
endif()

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()

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
