cmake_minimum_required(VERSION 3.16)
project(fmt_reflect)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 6.2.1
)
FetchContent_MakeAvailable(fmt)
message(STATUS "fmt source dir: ${fmt_SOURCE_DIR}")

add_executable(fmt_reflect main.cpp)
target_link_libraries(fmt_reflect PUBLIC fmt::fmt)