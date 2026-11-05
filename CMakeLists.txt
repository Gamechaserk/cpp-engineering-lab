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


# ── 9) U04：渲染器核心（静态库：给 exe 和测试共用）──
add_library(renderer_core STATIC
    renderer/src/math/Mat4.cpp           # ★ 前置 0：Mat4 / Vec4
    renderer/src/scene/Camera.cpp        # ★ 前置 0：Camera
    renderer/src/render/Framebuffer.cpp
    renderer/src/render/Rasterizer.cpp
    renderer/src/render/Shading.cpp      # ★ U03：光照 + 背面剔除
    renderer/src/render/VertPipeline.cpp  # ★ 前置 0：世界坐标 -> NDC
    renderer/src/io/PpmWriter.cpp)

# PUBLIC：库自己要用，链接它的目标【也继承】→ 编译选项/宏不会分叉
target_link_libraries(renderer_core PUBLIC
    lab_options
    lab_headers)

# PUBLIC：这是"本库的公开头目录"，谁链它谁自动拿到
target_include_directories(renderer_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer/include)

# ── 10) U04：渲染器可执行文件（画渐变 → 写 PPM）──
add_executable(renderer renderer/src/main.cpp)
target_link_libraries(renderer PRIVATE renderer_core)

# ★ 可执行文件必须挂：把 ASan 运行时拷到 exe 旁边（否则 Debug 版直接 0xC0000135）
smartptr_copy_asan_runtime(renderer)

# ── 11) U04：渲染器测试（Math / Ppm）──
add_executable(renderer_tests tests/test_renderer.cpp)
target_link_libraries(renderer_tests PRIVATE
    renderer_core        # ★ 会自动继承 lab_options / lab_headers / renderer/include
    GTest::gtest)        # 同样自己写 main（沿用 tests/test_my_vector.cpp 末尾那份）

smartptr_copy_asan_runtime(renderer_tests)   # 必须在 gtest_discover_tests 之前
gtest_discover_tests(renderer_tests)


# ── 12) 并发练习：线程 / 锁 / 原子（学习用一次性 target）──
find_package(Threads REQUIRED)

add_executable(thread_demo tools/thread_demo.cpp)
target_link_libraries(thread_demo PRIVATE
    lab_options          # 继承 /W4 + Debug 下的 ASan
    Threads::Threads)    # Windows 上展开为空；Linux 上展开为 -pthread

smartptr_copy_asan_runtime(thread_demo)   # 它也开 ASan，所以照样要把运行时拷到 exe 旁边

# ── 13) 手撕专项：LRU 缓存（自测用 main；不进 CTest，保持 59/59 基线）──
add_executable(lru_cache handwritten/lru_cache.cpp)
target_link_libraries(lru_cache PRIVATE
    lab_options
    Threads::Threads)    # std::mutex 在 Linux 上要 pthread；Windows 上展开为空

smartptr_copy_asan_runtime(lru_cache)

# ── 14) U03：光照（Blinn-Phong）与背面剔除测试（8 个 gtest 用例）──
add_executable(shading_tests tests/test_shading.cpp)
target_link_libraries(shading_tests PRIVATE
    renderer_core        # ★ 自动继承 lab_options / lab_headers / renderer/include
    GTest::gtest)        # 同样自己写 main（CRT 泄漏检查）

smartptr_copy_asan_runtime(shading_tests)   # 必须在 gtest_discover_tests 之前
gtest_discover_tests(shading_tests)

# ── 15) 前置 0：Mat4 / Vec4 测试（12 个 gtest 用例）──
add_executable(mat4_tests tests/test_mat4.cpp)
target_link_libraries(mat4_tests PRIVATE
    renderer_core        # ★ 自动继承 lab_options / lab_headers / renderer/include
    GTest::gtest)        # 同样自己写 main（CRT 泄漏检查）

smartptr_copy_asan_runtime(mat4_tests)   # 必须在 gtest_discover_tests 之前
gtest_discover_tests(mat4_tests)

# ── 16) 前置 0：Camera 测试（5 个 gtest 用例）──
add_executable(camera_tests tests/test_camera.cpp)
target_link_libraries(camera_tests PRIVATE
    renderer_core        # ★ 自动继承 lab_options / lab_headers / renderer/include
    GTest::gtest)        # 同样自己写 main（CRT 泄漏检查）

smartptr_copy_asan_runtime(camera_tests)   # 必须在 gtest_discover_tests 之前
gtest_discover_tests(camera_tests)

# ── 17) 前置 0：VertPipeline 测试（6 个 gtest 用例）──
add_executable(vertpipeline_tests tests/test_vertpipeline.cpp)
target_link_libraries(vertpipeline_tests PRIVATE
    renderer_core        # ★ 自动继承 lab_options / lab_headers / renderer/include
    GTest::gtest)        # 同样自己写 main（CRT 泄漏检查）

smartptr_copy_asan_runtime(vertpipeline_tests)   # 必须在 gtest_discover_tests 之前
gtest_discover_tests(vertpipeline_tests)
