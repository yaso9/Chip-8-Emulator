cmake_minimum_required(VERSION 3.0.0)
project(Chip-8-Emulator VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(Chip-8-Emulator ./src/main.cpp)

find_package(SFML 2.5 COMPONENTS graphics audio REQUIRED)
target_link_libraries(Chip-8-Emulator sfml-graphics sfml-audio)

include_directories(${SFML_INCLUDE_DIR})
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)