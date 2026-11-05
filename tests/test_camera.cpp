// ============================================================================
//  U0 前置 · Camera 测试（5 条）
//  沿用工程约定：gtest + 自己写 main（CRT 泄漏检查）+ gtest_discover_tests
// ============================================================================

#include <cmath>

#include "gtest/gtest.h"
#include "math/Mat4.h"
#include "math/Vec3.h"
#include "scene/Camera.h"

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

namespace {

const float kPi = 3.14159265358979323846f;

bool MatNear(const Mat4& a, const Mat4& b, float eps = 1e-4f) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (std::fabs(a.m[i][j] - b.m[i][j]) > eps) return false;
        }
    }
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// 1. 默认相机的视图矩阵：相机自身落在视图空间原点
// ---------------------------------------------------------------------------
TEST(Camera, DefaultViewPutsPositionAtOrigin) {
    Camera cam;
    const Vec4 p = cam.viewMatrix().transformPoint(cam.position);
    EXPECT_NEAR(p.x, 0.0f, 1e-4f);
    EXPECT_NEAR(p.y, 0.0f, 1e-4f);
    EXPECT_NEAR(p.z, 0.0f, 1e-4f);
}

// ---------------------------------------------------------------------------
// 2. ★ MVP 的顺序：mvpMatrix(M) 必须等于 P * V * M（右乘先做）
// ---------------------------------------------------------------------------
TEST(Camera, MvpOrderIsProjTimesViewTimesModel) {
    Camera cam;
    const Mat4 M = Mat4::translate(1.0f, 2.0f, -3.0f);
    const Mat4 expected = cam.projMatrix() * cam.viewMatrix() * M;
    EXPECT_TRUE(MatNear(cam.mvpMatrix(M), expected));
}

// ---------------------------------------------------------------------------
// 3. ★ 相机前方的点，clip 空间的 w 必须是正数（否则裁剪与除法全错）
// ---------------------------------------------------------------------------
TEST(Camera, ProjectToClipKeepsWPositiveInFront) {
    Camera cam;                                   // 位置 (0,0,5)，看 -Z
    const Vec4 c = cam.projectToClip(Vec3(0.0f, 0.0f, 0.0f), Mat4::identity());
    EXPECT_GT(c.w, 0.0f);                        // 原点在相机前方 ⇒ w > 0
}

// ---------------------------------------------------------------------------
// 4. 注视点经 MVP + 透视除法后，落在屏幕中心（NDC x = y = 0）
// ---------------------------------------------------------------------------
TEST(Camera, TargetLandsOnScreenCenter) {
    Camera cam;
    const Vec4 c = cam.projectToClip(cam.target, Mat4::identity());
    ASSERT_GT(c.w, 0.0f);
    EXPECT_NEAR(c.x / c.w, 0.0f, 1e-4f);
    EXPECT_NEAR(c.y / c.w, 0.0f, 1e-4f);
}

// ---------------------------------------------------------------------------
// 5. aspect 只压 X，不压 Y
// ---------------------------------------------------------------------------
TEST(Camera, AspectCompressesXOnly) {
    Camera cam1;  cam1.aspect = 1.0f;
    Camera cam2;  cam2.aspect = 2.0f;

    const Vec3 p(1.0f, 1.0f, 0.0f);              // 一个偏心点
    const Vec4 a = cam1.projectToClip(p, Mat4::identity());
    const Vec4 b = cam2.projectToClip(p, Mat4::identity());

    const float ax = a.x / a.w, ay = a.y / a.w;
    const float bx = b.x / b.w, by = b.y / b.w;

    EXPECT_NEAR(bx, ax * 0.5f, 1e-4f);           // aspect 翻倍 ⇒ x_ndc 减半
    EXPECT_NEAR(by, ay, 1e-4f);                  // y 不变
}

int main(int argc, char** argv) {
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}