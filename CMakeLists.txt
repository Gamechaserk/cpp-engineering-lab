cmake_minimum_required(VERSION 3.20)
project(SmartPtr LANGUAGES CXX)

# ── 1) 去掉 MSVC Debug 默认的 /RTC1（它与 ASan 不兼容）──
# 必须在 project() 之后：CMAKE_CXX_FLAGS_DEBUG 由编译器模块在 project() 里初始化
if (MSVC)
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endif()

# ── 2) CRT 策略：Debug=/MDd，Release=/MT ──
# 必须在任何"会编译的 target"创建之前（它在 target 创建时被读取来初始化属性）
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "$<$<CONFIG:Debug>:MultiThreadedDebugDLL>$<$<CONFIG:Release>:MultiThreaded>")
endif()

enable_testing()

# ── 3) 接口库：编译 / 链接选项 ──
add_library(smartptr_options INTERFACE)
if (MSVC)
    target_compile_options(smartptr_options INTERFACE
        /W4                                     # 最高警告等级
        $<$<CONFIG:Debug>:/fsanitize=address>)  # 仅 Debug 开 ASan
    target_link_options(smartptr_options INTERFACE
        $<$<CONFIG:Debug>:/INCREMENTAL:NO>)     # ASan 要求关闭增量链接
endif()

# ── 4) 接口库：头文件包含路径 ──
add_library(smartptr_headers INTERFACE)
target_include_directories(smartptr_headers INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# ── 5) 测试可执行文件 ──
add_executable(ptr_tests tests/test_shared_ptr.cpp)
target_link_libraries(ptr_tests PRIVATE smartptr_options smartptr_headers)

# ── 6) 【B-1 新增】族 B：把 ASan 运行时拷到 exe 旁边 ──
# 目的：Debug 版 exe 脱离 VS 开发环境（任何终端）都能运行
# ⚠️ 必须写在 gtest_discover_tests() 之前 —— 后者会生成 POST_BUILD 命令去运行 exe，
#    而同一 target 的多个 POST_BUILD 命令按「定义顺序」执行
if (MSVC)
    # 用 CMake 实际使用的编译器路径推导目录，不依赖 shell 的 PATH
    get_filename_component(SMARTPTR_MSVC_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

    find_file(SMARTPTR_ASAN_DLL
        NAMES clang_rt.asan_dynamic-x86_64.dll
        PATHS "${SMARTPTR_MSVC_BIN_DIR}"
        NO_DEFAULT_PATH)

    message(STATUS "ASan runtime: ${SMARTPTR_ASAN_DLL}")

    if (SMARTPTR_ASAN_DLL)
        add_custom_command(TARGET ptr_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SMARTPTR_ASAN_DLL}"
                    "$<TARGET_FILE_DIR:ptr_tests>"
            VERBATIM)
    else()
        message(WARNING "未找到 ASan 运行时；Debug 版 exe 只能在 VS 开发环境里运行")
    endif()
endif()