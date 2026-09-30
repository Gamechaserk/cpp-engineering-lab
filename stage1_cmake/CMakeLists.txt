cmake_minimum_required(VERSION 3.20)
project(CppLab LANGUAGES CXX)

add_executable(cpplab
    src/main.cpp
    src/math.cpp
)

target_include_directories(cpplab PRIVATE include)
target_compile_options(cpplab PRIVATE /W4)

# ---- ASan?? Debug?----
string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
target_compile_options(cpplab PRIVATE $<$<CONFIG:Debug>:/fsanitize=address>)
target_link_options(cpplab PRIVATE $<$<CONFIG:Debug>:/INCREMENTAL:NO>)
