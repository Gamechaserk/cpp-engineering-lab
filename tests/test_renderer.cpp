// ── CRT 泄漏检查开关：★ 必须在【所有 #include 之前】定义，否则无效 ──
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#include <gtest/gtest.h>

// TODO(U04)：数学库测试（Math.*）—— 等 math/Vec3.h 落地后由教练提供

int main(int argc, char** argv) {
    // ★ 进程退出时自动 dump 泄漏：此时静态对象已析构 → 不会把 gtest 注册表误报成泄漏
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}