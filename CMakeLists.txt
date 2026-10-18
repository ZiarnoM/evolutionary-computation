cmake_minimum_required(VERSION 3.16)
project(evolutionary_computation CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_compile_options(-Wall -Wextra)

file(GLOB CORE_SRC CONFIGURE_DEPENDS src/core/*.cpp)
file(GLOB METHOD_SRC CONFIGURE_DEPENDS src/methods/*.cpp)
add_library(ec STATIC ${CORE_SRC} ${METHOD_SRC})
target_include_directories(ec PUBLIC src)

add_executable(run src/main.cpp)
target_link_libraries(run ec)
target_compile_definitions(run PRIVATE PROJECT_DIR="${CMAKE_SOURCE_DIR}")

enable_testing()
add_executable(tests tests/tests.cpp)
target_link_libraries(tests ec)
target_compile_definitions(tests PRIVATE DATA_DIR="${CMAKE_SOURCE_DIR}/data")
add_test(NAME tests COMMAND tests)
