cmake_minimum_required(VERSION 3.0.0)
project(Chip-8-Emulator VERSION 0.1.0)

add_executable(Chip-8-Emulator ./src/main.cpp)

include(FetchContent)

set(SFML_VERSION 2.5.1)
set(IMGUI_VERSION 1.82)
set(IMGUI_SFML_VERSION 2.3)

FetchContent_Declare(
  SFML
  URL "https://github.com/SFML/SFML/archive/${SFML_VERSION}.zip"
)
FetchContent_Declare(
  imgui
  URL "https://github.com/ocornut/imgui/archive/v${IMGUI_VERSION}.zip"
)
FetchContent_Declare(
  imgui-sfml
  GIT_REPOSITORY https://github.com/eliasdaler/imgui-sfml.git
  GIT_TAG        v2.3
)

FetchContent_Populate(SFML)
FetchContent_Populate(imgui)
FetchContent_Populate(imgui-sfml)

add_subdirectory(${sfml_SOURCE_DIR} ${sfml_BINARY_DIR} EXCLUDE_FROM_ALL)
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SFML_FIND_SFML CACHE BOOL OFF)
set(IMGUI_SFML_IMGUI_DEMO CACHE BOOL ON)
add_subdirectory(${imgui-sfml_SOURCE_DIR} ${imgui-sfml_BINARY_DIR} EXCLUDE_FROM_ALL)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED) 
include_directories(${CURL_INCLUDE_DIR})
target_link_libraries(Chip-8-Emulator ${CMAKE_THREAD_LIBS_INIT} ${CURL_LIBRARIES} sfml-graphics sfml-audio ImGui-SFML::ImGui-SFML)

include_directories(${SFML_INCLUDE_DIR})
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)