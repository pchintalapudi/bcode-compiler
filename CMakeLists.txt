cmake_minimum_required(VERSION 3.17.2)
project(oops-bcode-compiler VERSION 0.1.0)

set(CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED true)

include(CTest)
enable_testing()


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -Wextra -Wall -Winit-self -Wold-style-cast -Woverloaded-virtual -Wuninitialized -Winit-self -Wno-unknown-pragmas -fsanitize=undefined -fsanitize-undefined-trap-on-error")

add_executable(oops-bcode-compiler main.cpp)
add_subdirectory(platform_specific)
add_subdirectory(parser)
add_subdirectory(instructions)
add_subdirectory(interpreter)
add_subdirectory(utils)
add_subdirectory(compiler)
add_subdirectory(debug)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT "coops")
include(CPack)
