cmake_minimum_required(VERSION 3.20)
project(CrumbltClient VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# ---- Dependencies ----
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)

# ---- imgui (FetchContent, docking branch) ----
include(FetchContent)
FetchContent_Declare(
  imgui_src
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui_src)

add_library(imgui_lib STATIC
  ${imgui_src_SOURCE_DIR}/imgui.cpp
  ${imgui_src_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_src_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_src_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui_lib PUBLIC
  ${imgui_src_SOURCE_DIR}
  ${imgui_src_SOURCE_DIR}/backends
)

# ---- Luau ----
FetchContent_Declare(
  luau
  GIT_REPOSITORY https://github.com/luau-lang/luau.git
  GIT_TAG        0.634
)
set(LUAU_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(luau)

# ---- Photon SDK ----
set(PHOTON_SDK_PATH "${CMAKE_SOURCE_DIR}/libs/photon")
set(PHOTON_LIB_SUFFIX "vc17_release_windows_md_x64")
set(PHOTON_LIBS
  ${PHOTON_SDK_PATH}/LoadBalancing-cpp/lib/LoadBalancing-cpp_${PHOTON_LIB_SUFFIX}.lib
  ${PHOTON_SDK_PATH}/Photon-cpp/lib/Photon-cpp_${PHOTON_LIB_SUFFIX}.lib
  ${PHOTON_SDK_PATH}/Common-cpp/lib/Common-cpp_${PHOTON_LIB_SUFFIX}.lib
)

# ---- Source files ----
set(SOURCES
  src/main.cpp
  src/core/App.cpp
  src/core/Auth.cpp
  src/core/Http.cpp
  src/rlimgui/rlImGui.cpp
  src/scene/Scene.cpp
  src/scene/SceneLoader.cpp
  src/network/PhotonClient.cpp
  src/scripting/ScriptRuntime.cpp
  src/renderer/Renderer3D.cpp
)

add_executable(CrumbltClient ${SOURCES})

target_include_directories(CrumbltClient PRIVATE
  src
  src/core
  src/rlimgui
  ${imgui_src_SOURCE_DIR}
  ${imgui_src_SOURCE_DIR}/backends
  ${PHOTON_SDK_PATH}
  ${PHOTON_SDK_PATH}/Chat-cpp/inc
  ${PHOTON_SDK_PATH}/Common-cpp/inc
  ${PHOTON_SDK_PATH}/LoadBalancing-cpp/inc
  ${PHOTON_SDK_PATH}/Photon-cpp/inc
  ${PHOTON_SDK_PATH}/PhotonVoice-cpp/inc
  ${PHOTON_SDK_PATH}/LoadBalancing-cpp/inc/Internal
  ${PHOTON_SDK_PATH}/Common-cpp/inc/Internal
  ${PHOTON_SDK_PATH}/Common-cpp/inc/Helpers
  ${PHOTON_SDK_PATH}/Photon-cpp/inc/Enums
  ${PHOTON_SDK_PATH}/Photon-cpp/inc/Internal
  ${PHOTON_SDK_PATH}/PhotonVoice-cpp/inc/Internal
  ${PHOTON_SDK_PATH}/LoadBalancing-cpp/inc/Enums
  ${PHOTON_SDK_PATH}/Chat-cpp/inc/Internal
  ${PHOTON_SDK_PATH}/Chat-cpp/inc/Enums
  ${PHOTON_SDK_PATH}/PhotonVoice-cpp/inc/Enums
)

target_link_libraries(CrumbltClient PRIVATE
  imgui_lib
  CURL::libcurl
  nlohmann_json::nlohmann_json
  raylib
  Luau.VM
  Luau.Compiler
  ${PHOTON_LIBS}
)

if(WIN32)
  target_compile_definitions(CrumbltClient PRIVATE NOMINMAX)
  target_link_libraries(CrumbltClient PRIVATE dwmapi ws2_32)
  set_target_properties(CrumbltClient PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:CONSOLE"
  )
endif()

add_custom_command(TARGET CrumbltClient POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:CrumbltClient>/assets
)
