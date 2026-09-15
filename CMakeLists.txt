cmake_minimum_required(VERSION 3.25)

project(glasscope VERSION 0.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GLASSCOPE_WARNING_OPTIONS
  -Wall -Wextra -Wpedantic -Wconversion -Wshadow
)

function(glasscope_enable_warnings target)
  target_compile_options(${target} PRIVATE ${GLASSCOPE_WARNING_OPTIONS})
endfunction()

find_package(PkgConfig REQUIRED)
pkg_check_modules(HYPRLAND REQUIRED IMPORTED_TARGET hyprland)
pkg_check_modules(GLESV2 REQUIRED IMPORTED_TARGET glesv2)
pkg_check_modules(LUA REQUIRED IMPORTED_TARGET lua)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lens.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/liquid_lens.glsl"
)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/shaders/liquid_lens.glsl"
     GLASSCOPE_FRAGMENT_SHADER_BODY)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lens.vert"
     GLASSCOPE_VERTEX_SHADER_BODY)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders.hpp"
  @ONLY
)

add_library(glasscope-core STATIC
  src/lens_geometry.cpp
  src/lens_motion.cpp
  src/lens_oscillation.cpp
  src/lens_state.cpp
)
set_target_properties(glasscope-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(glasscope-core PUBLIC include)
glasscope_enable_warnings(glasscope-core)

add_library(glasscope SHARED
  src/plugin.cpp
  src/plugin_config.cpp
  src/plugin_runtime.cpp
  src/lens_renderer.cpp
  src/lens_renderer_resources.cpp
  src/lens_pass_element.cpp
)
set_target_properties(glasscope PROPERTIES
  PREFIX ""
  OUTPUT_NAME "glasscope"
)
target_include_directories(glasscope PRIVATE
  include
  "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_link_libraries(glasscope PRIVATE
  glasscope-core
  PkgConfig::HYPRLAND
  PkgConfig::GLESV2
  PkgConfig::LUA
)
glasscope_enable_warnings(glasscope)
target_compile_options(glasscope PRIVATE -fstack-protector-strong)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(glasscope PRIVATE -fno-gnu-unique)
endif()
target_link_options(glasscope PRIVATE
  -Wl,-z,relro,-z,now
)
