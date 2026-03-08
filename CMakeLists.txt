cmake_minimum_required(VERSION 3.20)
project(CrumbltClient VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# ---- Dependencies ----
find_package(imgui CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

# ---- Luau ----
include(FetchContent)
FetchContent_Declare(
  luau
  GIT_REPOSITORY https://github.com/luau-lang/luau.git
  GIT_TAG        0.634
)
set(LUAU_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(luau)

# ---- imgui backends ----
FetchContent_Declare(
  imgui_src
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.9
)
FetchContent_MakeAvailable(imgui_src)

add_library(imgui_backends STATIC
  ${imgui_src_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  ${imgui_src_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)
target_include_directories(imgui_backends PUBLIC
  ${imgui_src_SOURCE_DIR}
  ${imgui_src_SOURCE_DIR}/backends
)
target_link_libraries(imgui_backends PUBLIC imgui::imgui SDL2::SDL2)

# ---- Source files ----
set(SOURCES
  src/main.cpp
  src/core/App.cpp
  src/core/Auth.cpp
  src/core/Http.cpp
  src/scene/Scene.cpp
  src/scene/SceneLoader.cpp
  src/network/PhotonClient.cpp
  src/scripting/ScriptRuntime.cpp
  src/renderer/Renderer.cpp
)

add_executable(CrumbltClient ${SOURCES})

target_include_directories(CrumbltClient PRIVATE src)

target_link_libraries(CrumbltClient PRIVATE
  imgui::imgui
  imgui_backends
  CURL::libcurl
  nlohmann_json::nlohmann_json
  SDL2::SDL2
  SDL2::SDL2main
  Luau.VM
  Luau.Compiler
)

if(WIN32)
  target_compile_definitions(CrumbltClient PRIVATE NOMINMAX)
  target_link_libraries(CrumbltClient PRIVATE dwmapi)
endif()

add_custom_command(TARGET CrumbltClient POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:CrumbltClient>/assets
)
