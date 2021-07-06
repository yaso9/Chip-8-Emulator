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
FetchContent_Declare(
  imgui_club
  GIT_REPOSITORY https://github.com/ocornut/imgui_club.git
  GIT_TAG        02e679b7f4cfb01f9480dcbcac59552879f96573
)

FetchContent_MakeAvailable(SFML)
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR ${imgui_SOURCE_DIR})
FetchContent_MakeAvailable(imgui-sfml)

FetchContent_Populate(imgui_club)
include_directories(${imgui_club_SOURCE_DIR})

find_package(Threads REQUIRED)
find_package(CURL REQUIRED) 
include_directories(${CURL_INCLUDE_DIR})
target_link_libraries(Chip-8-Emulator ${CMAKE_THREAD_LIBS_INIT} ${CURL_LIBRARIES} sfml-graphics sfml-audio ImGui-SFML::ImGui-SFML)

include_directories(${SFML_INCLUDE_DIR})
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)