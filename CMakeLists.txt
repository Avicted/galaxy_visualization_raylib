cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+

project(galaxy_visualization_raylib VERSION 0.1.0 LANGUAGES CXX)

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

# Set the cpp standard to
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
    target_compile_options(${libName} PRIVATE "-w")
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

# Set compile flags 

# Print the OS
message(STATUS "----------------------------------------------")
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "----------------------------------------------")

# If linux, set the compile flags
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wc99-extensions -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-const-variable -Wno-unused-lambda-capture -Wno-unused-local-typedef -Wno-unused-value -Wno-unused-const-variable")
endif()

# If windows, set the compile flags for msvc
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

# print the compile flags
message(STATUS "----------------------------------------------")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "----------------------------------------------")

add_executable(${PROJECT_NAME} frontend.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

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

# copy the fonts folder to the build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts)

# copy the input_data folder to the build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/input_data $<TARGET_FILE_DIR:${PROJECT_NAME}>/input_data)


# Copy the shaders dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)