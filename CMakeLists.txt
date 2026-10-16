cmake_minimum_required(VERSION 3.20)
project(SmartPtr LANGUAGES CXX)

# ── 1) 去掉 MSVC Debug 默认的 /RTC1（与 ASan 不兼容）──
if (MSVC)
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endif()

# ── 2) CRT 策略 ──
# 当前：Debug = /MDd，Release = /MT
# 若改用「统一的 DLL CRT」→ 换成 "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "$<$<CONFIG:Debug>:MultiThreadedDebugDLL>$<$<CONFIG:Release>:MultiThreaded>")
endif()

enable_testing()

# ── 3) 接口库：编译 / 链接选项 ──
add_library(lab_options INTERFACE)
if (MSVC)
    target_compile_options(lab_options INTERFACE
        /W4
        $<$<CONFIG:Debug>:/fsanitize=address>)

    # ASan 的容器注解会与「未开 ASan 的第三方静态库」冲突（LNK2038）
    # 关掉注解 → 我们自己的 .obj 也发 "0"，与 gtest.lib 一致
    # 宏名以此处 STL 头文件里的真实开关为准（见取证命令）
    target_compile_definitions(lab_options INTERFACE
        _DISABLE_VECTOR_ANNOTATION
        _DISABLE_STRING_ANNOTATION)

    target_link_options(lab_options INTERFACE
        $<$<CONFIG:Debug>:/INCREMENTAL:NO>)
endif()

# ── 4) 接口库：头文件路径 ──
add_library(lab_headers INTERFACE)
target_include_directories(lab_headers INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# ── 5) 定位 ASan 运行时（只做一次，全项目共用）──
if (MSVC)
    # 用 CMake 实际使用的编译器路径推导，不依赖 shell 的 PATH
    get_filename_component(SMARTPTR_MSVC_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_file(SMARTPTR_ASAN_DLL
        NAMES clang_rt.asan_dynamic-x86_64.dll
        PATHS "${SMARTPTR_MSVC_BIN_DIR}"
        NO_DEFAULT_PATH)
    message(STATUS "ASan runtime: ${SMARTPTR_ASAN_DLL}")
endif()

# ── 5b) ASan 运行时拷贝助手：给任意 target 挂上「把 ASan 运行时拷到 exe 旁边」
#        必须在 gtest_discover_tests(<target>) 之前调用（POST_BUILD 按定义顺序执行）
function(smartptr_copy_asan_runtime target)
    if (MSVC AND SMARTPTR_ASAN_DLL)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SMARTPTR_ASAN_DLL}"
                    "$<TARGET_FILE_DIR:${target}>"
            VERBATIM)
    elseif (MSVC)
        message(WARNING "未找到 ASan 运行时；${target} 的 Debug 版只能在 VS 开发环境里运行")
    endif()
endfunction()

# ── 6) 拉取 GoogleTest（FetchContent）──
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2          # 固定版本；1.17+ 要求 C++17，本项目仍是 C++14
)
# gtest 默认用静态 CRT（/MT），与项目的 /MDd 混用 → 一个进程里出现两套 CRT
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# ── 7) U01：手写智能指针测试（23 个 gtest 用例）──
add_executable(ptr_tests tests/test_shared_ptr.cpp)
target_link_libraries(ptr_tests PRIVATE
    lab_options
    lab_headers
    GTest::gtest)      # 只用 gtest、不用 gtest_main：我们自己写 main（要配置 CRT 泄漏检查）

smartptr_copy_asan_runtime(ptr_tests)   # 必须在 gtest_discover_tests 之前
gtest_discover_tests(ptr_tests)

# ── 8) U02：MyVector 测试（30 个 gtest 用例）──
add_executable(vector_tests tests/test_my_vector.cpp)
target_link_libraries(vector_tests PRIVATE
    lab_options
    lab_headers
    GTest::gtest)      # 同样自己写 main

smartptr_copy_asan_runtime(vector_tests)   # 必须在 gtest_discover_tests 之前
gtest_discover_tests(vector_tests)