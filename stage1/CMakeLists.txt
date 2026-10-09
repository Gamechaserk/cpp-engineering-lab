cmake_minimum_required(VERSION 3.20)
project(SmartPtr LANGUAGES CXX)

add_executable(ptr_test my_shared_ptr.cpp)

# ---- MSVC 专用设置 ----
if (MSVC)
    target_compile_options(ptr_test PRIVATE /W4)

    # Debug：ASan + 动态 CRT（保留 CRT 泄漏检测能力）
    target_compile_options(ptr_test PRIVATE $<$<CONFIG:Debug>:/fsanitize=address>)
    target_link_options(ptr_test PRIVATE $<$<CONFIG:Debug>:/INCREMENTAL:NO>)

    # Release：静态链接 CRT + 静态 ASan → exe 自包含，任何机器可跑
    # （需要 CMake >= 3.15 且使用 VS 2019+ 的 MSVC 工具集）
    set_property(TARGET ptr_test PROPERTY MSVC_RUNTIME_LIBRARY
        "$<$<CONFIG:Debug>:MultiThreadedDebugDLL>$<$<CONFIG:Release>:MultiThreaded>")
endif()

# /RTC1 与 ASan 冲突（Debug）
if (MSVC)
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endif()