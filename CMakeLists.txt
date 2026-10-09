cmake_minimum_required(VERSION 3.20)
project(SmartPtr LANGUAGES CXX)

if (MSVC)
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endif()

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "$<$<CONFIG:Debug>:MultiThreadedDebugDLL>$<$<CONFIG:Release>:MultiThreaded>"
    )
endif()

enable_testing()
add_library(smartptr_options INTERFACE)

if (MSVC)
    target_compile_options(smartptr_options INTERFACE
        /W4                                 # MSVC最高警告等级
        $<$<CONFIG:Debug>:/fsanitize=address> # Debug配置开启ASan内存检测
    )
    target_link_options(smartptr_options INTERFACE
        $<$<CONFIG:Debug>:/INCREMENTAL:NO>    # Debug关闭增量链接，ASan要求
    )
endif()

add_library(smartptr_headers INTERFACE)
target_include_directories(smartptr_headers INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    )
add_executable(ptr_tests tests/test_shared_ptr.cpp)
target_link_libraries(ptr_tests PRIVATE smartptr_options smartptr_headers)