cmake_minimum_required(VERSION 3.25)
project(glasscope VERSION 0.7.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(BUILD_TESTING "Build local regression tests" OFF)
include(CTest)
option(GLASSCOPE_BUILD_PLUGIN "Build the Hyprland/GLES plugin" ON)
option(GLASSCOPE_BUILD_RENDER_TESTS "Build surfaceless EGL renderer regressions" OFF)
option(GLASSCOPE_NATIVE_TESTING "Enable native test observation and helpers; never install this build" OFF)
if(GLASSCOPE_BUILD_RENDER_TESTS AND NOT BUILD_TESTING)
  message(FATAL_ERROR "Renderer tests require BUILD_TESTING=ON")
endif()
if(GLASSCOPE_NATIVE_TESTING AND (NOT GLASSCOPE_BUILD_PLUGIN OR NOT BUILD_TESTING))
  message(FATAL_ERROR "Native testing requires the plugin and BUILD_TESTING")
endif()
function(glasscope_enable_warnings target)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endfunction()

add_library(glasscope-core STATIC
  src/core/lens_motion.cpp
  src/core/lens_oscillation.cpp
  src/core/lens_state.cpp
  src/core/pinned_interaction.cpp
  src/core/visibility.cpp
  src/core/color_probe.cpp
  src/core/input_router.cpp
  src/core/config_validation.cpp
  src/math/damped_spring.cpp
  src/geometry/lens_geometry.cpp
  src/geometry/pinned_shape.cpp
  src/geometry/lens_bounds.cpp
)
set_target_properties(glasscope-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(glasscope-core PUBLIC include)
glasscope_enable_warnings(glasscope-core)

if(GLASSCOPE_BUILD_PLUGIN OR GLASSCOPE_BUILD_RENDER_TESTS)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GLESV2 REQUIRED IMPORTED_TARGET glesv2)
  set(GLASSCOPE_FRAGMENT_PARTS
    lens_uniforms.glsl
    shape_follow.glsl
    shape_pinned.glsl
    surface_deformation.glsl
    optics.glsl
    probe_overlay.glsl
    liquid_lens.glsl
  )
  set(GLASSCOPE_FRAGMENT_SHADER_BODY "")
  set(shader_source_id 0)
  set(shader_source_map "")
  foreach(part IN LISTS GLASSCOPE_FRAGMENT_PARTS)
    set(fragment "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${part}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${fragment}")
    file(READ "${fragment}" fragment_body)
    string(APPEND GLASSCOPE_FRAGMENT_SHADER_BODY "\n#line 1 ${shader_source_id}\n${fragment_body}\n")
    string(APPEND shader_source_map "${shader_source_id}: ${part}\n")
    math(EXPR shader_source_id "${shader_source_id} + 1")
  endforeach()
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/liquid_lens.glsl" "${GLASSCOPE_FRAGMENT_SHADER_BODY}")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/shader-sources.txt" "${shader_source_map}")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lens.vert")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/shaders/lens.vert" GLASSCOPE_VERTEX_SHADER_BODY)
  configure_file(src/render/shaders.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders.hpp" @ONLY)
  add_library(glasscope-render STATIC
    src/render/lens_renderer.cpp
    src/render/shader_program.cpp
    src/render/scene_copy.cpp
    src/render/gl_error.cpp
  )
  set_target_properties(glasscope-render PROPERTIES POSITION_INDEPENDENT_CODE ON)
  target_include_directories(glasscope-render PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}/generated")
  target_link_libraries(glasscope-render PUBLIC glasscope-core PkgConfig::GLESV2)
  glasscope_enable_warnings(glasscope-render)
endif()

if(GLASSCOPE_BUILD_PLUGIN)
  pkg_check_modules(HYPRLAND REQUIRED IMPORTED_TARGET hyprland)
  pkg_check_modules(LUA REQUIRED IMPORTED_TARGET lua)
  add_library(glasscope SHARED
    src/plugin.cpp
    src/plugin_runtime.cpp
    src/hyprland/plugin_config.cpp
    src/hyprland/plugin_bindings.cpp
    src/hyprland/render_driver.cpp
    src/hyprland/lens_pass_element.cpp
    src/hyprland/clipboard_writer.cpp
    src/hyprland/cursor_override.cpp
    src/hyprland/notification.cpp
  )
  set_target_properties(glasscope PROPERTIES PREFIX "" OUTPUT_NAME "glasscope")
  target_include_directories(glasscope PRIVATE src)
  target_link_libraries(glasscope PRIVATE glasscope-render PkgConfig::HYPRLAND PkgConfig::LUA)
  glasscope_enable_warnings(glasscope)
  # Static components must also support plugin unloading.
  foreach(target IN ITEMS glasscope glasscope-core glasscope-render)
    target_compile_options(${target} PRIVATE -fstack-protector-strong)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      target_compile_options(${target} PRIVATE -fno-gnu-unique)
    endif()
  endforeach()
  target_link_options(glasscope PRIVATE -Wl,-z,relro,-z,now)
endif()

if(BUILD_TESTING)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    message(FATAL_ERROR "Local test sources are absent. Configure with BUILD_TESTING=OFF for the source distribution.")
  endif()
  add_subdirectory(tests)
endif()
