// ============================================================================
//  U0 前置 · VertPipeline 测试（6 条）
//
//  ★ 设计意图：这 6 条**只依赖 Mat4**（已实现、已绿），不依赖 Camera（待实现）
//    ⇒ 你可以先让这 6 条变绿，再回头接 Camera
//  ★ 其中第 3 条「近大远小」就是 11-04 闸门的判据本身
// ============================================================================

#include <cmath>

#include "gtest/gtest.h"
#include "math/Mat4.h"
#include "math/Vec3.h"
#include "render/VertPipeline.h"

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

namespace {

const float kPi = 3.14159265358979323846f;

// 相机在 (0,0,camZ) 看原点，fov 45°，aspect 1，n=0.1，f=100
Mat4 MakeMvp(float camZ) {
    const Mat4 V = Mat4::lookAt(Vec3(0.0f, 0.0f, camZ), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
    const Mat4 P = Mat4::perspective(kPi * 0.25f, 1.0f, 0.1f, 100.0f);
    return P * V;
}

} // namespace

// ---------------------------------------------------------------------------
// 1. clip 空间的 w 就是"点到相机的距离"
// ---------------------------------------------------------------------------
TEST(VertPipeline, WorldToClipCarriesDepthInW) {
    const Mat4 mvp = MakeMvp(5.0f);
    const Vec4 c = worldToClip(mvp, Vec3(0.0f, 0.0f, 0.0f));
    EXPECT_NEAR(c.w, 5.0f, 1e-4f);
}

// ---------------------------------------------------------------------------
// 2. 透视除法：三个顶点都除过 w 了
// ---------------------------------------------------------------------------
TEST(VertPipeline, TriangleIsDividedByW) {
    const Mat4 mvp = MakeMvp(5.0f);
    Vertex3D a{}, b{}, c{};
    const bool ok = worldTriangleToNdc(mvp, Vec3(0.0f, 0.0f, 0.0f),
                                       Vec3(1.0f, 0.0f, 0.0f),
                                       Vec3(0.0f, 1.0f, 0.0f), a, b, c);
    ASSERT_TRUE(ok);
    // 原点正对相机 ⇒ 屏幕中心附近；x=1 的点 ⇒ x_ndc 应为正且不算大
    EXPECT_NEAR(a.x, 0.0f, 1e-4f);
    EXPECT_NEAR(a.y, 0.0f, 1e-4f);
    EXPECT_GT(b.x, 0.0f);
    EXPECT_GT(c.y, 0.0f);
}

// ---------------------------------------------------------------------------
// 3. ★★ 近大远小 —— 这就是 11-04 闸门的判据
// ---------------------------------------------------------------------------
TEST(VertPipeline, NearTriangleIsBiggerThanFar) {
    const float camZ = 5.0f;
    const Mat4 mvp = MakeMvp(camZ);

    // 同一个三角形，放在 z=0（距离 5）与 z=-45（距离 50）
    Vertex3D na{}, nb{}, nc{};
    ASSERT_TRUE(worldTriangleToNdc(mvp, Vec3(0.0f, 0.0f, 0.0f),
                                   Vec3(1.0f, 0.0f, 0.0f),
                                   Vec3(0.0f, 1.0f, 0.0f), na, nb, nc));

    Vertex3D fa{}, fb{}, fc{};
    ASSERT_TRUE(worldTriangleToNdc(mvp, Vec3(0.0f, 0.0f, -45.0f),
                                   Vec3(1.0f, 0.0f, -45.0f),
                                   Vec3(0.0f, 1.0f, -45.0f), fa, fb, fc));

    const float nearWidth = nb.x - na.x;   // 近处三角形的 NDC 宽度
    const float farWidth = fb.x - fa.x;    // 远处三角形的 NDC 宽度

    EXPECT_GT(nearWidth, 0.0f);
    EXPECT_GT(farWidth, 0.0f);
    // 距离比 5:50 = 1:10 ⇒ NDC 宽度比 10:1
    EXPECT_NEAR(nearWidth / farWidth, 10.0f, 0.01f);
}

// ---------------------------------------------------------------------------
// 4. 相机后面的三角形必须被丢弃（w <= 0）
// ---------------------------------------------------------------------------
TEST(VertPipeline, TriangleBehindCameraIsDropped) {
    const Mat4 mvp = MakeMvp(5.0f);
    Vertex3D a{}, b{}, c{};
    // z = +10 在相机（z=5）的背后
    const bool ok = worldTriangleToNdc(mvp, Vec3(0.0f, 0.0f, 10.0f),
                                       Vec3(1.0f, 0.0f, 10.0f),
                                       Vec3(0.0f, 1.0f, 10.0f), a, b, c);
    EXPECT_FALSE(ok);
}

// ---------------------------------------------------------------------------
// 5. 相机前方的三角形要保留
// ---------------------------------------------------------------------------
TEST(VertPipeline, TriangleAheadIsKept) {
    const Mat4 mvp = MakeMvp(5.0f);
    Vertex3D a{}, b{}, c{};
    EXPECT_TRUE(worldTriangleToNdc(mvp, Vec3(-1.0f, -1.0f, 0.0f),
                                   Vec3(1.0f, -1.0f, 0.0f),
                                   Vec3(0.0f, 1.0f, 0.0f), a, b, c));
}

// ---------------------------------------------------------------------------
// 6. 正对相机的三角形，NDC 落在 [-1, 1] 内
// ---------------------------------------------------------------------------
TEST(VertPipeline, CenteredTriangleFitsInNdc) {
    const Mat4 mvp = MakeMvp(5.0f);
    Vertex3D a{}, b{}, c{};
    ASSERT_TRUE(worldTriangleToNdc(mvp, Vec3(-1.0f, -1.0f, 0.0f),
                                   Vec3(1.0f, -1.0f, 0.0f),
                                   Vec3(0.0f, 1.0f, 0.0f), a, b, c));
    EXPECT_GE(a.x, -1.0f); EXPECT_LE(a.x, 1.0f);
    EXPECT_GE(a.y, -1.0f); EXPECT_LE(a.y, 1.0f);
    EXPECT_GE(b.x, -1.0f); EXPECT_LE(b.x, 1.0f);
    EXPECT_GE(c.y, -1.0f); EXPECT_LE(c.y, 1.0f);
    // z 也在 NDC 范围内
    EXPECT_GE(a.z, -1.0f); EXPECT_LE(a.z, 1.0f);
}

int main(int argc, char** argv) {
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}