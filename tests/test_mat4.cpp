// ============================================================================
//  U0 前置 · Mat4 / Vec4 测试
//  沿用工程约定：gtest + 自己写 main（含 CRT 泄漏检查）+ gtest_discover_tests
// ============================================================================

#include <cmath>

#include "gtest/gtest.h"
#include "math/Mat4.h"
#include "math/Vec3.h"

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
// 基础：单位矩阵 / 行主序 / 乘法
// ---------------------------------------------------------------------------

TEST(Mat4, DefaultConstructorIsIdentity) {
    Mat4 I;
    EXPECT_TRUE(MatNear(I, Mat4::identity()));
    EXPECT_FLOAT_EQ(I.m[0][0], 1.0f);
    EXPECT_FLOAT_EQ(I.m[0][1], 0.0f);   // ★ 只有对角线是 1
    EXPECT_FLOAT_EQ(I.m[3][3], 1.0f);
}

TEST(Mat4, RowMajorLayoutIsExplicit) {
    // ★★ 这条钉死"行主序 + 平移在第 4 列"
    const Mat4 T = Mat4::translate(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(T.m[0][3], 1.0f);
    EXPECT_FLOAT_EQ(T.m[1][3], 2.0f);
    EXPECT_FLOAT_EQ(T.m[2][3], 3.0f);
    EXPECT_FLOAT_EQ(T.m[3][0], 0.0f);   // ★ 第 4 行不是平移
    EXPECT_FLOAT_EQ(T.m[3][3], 1.0f);
}

TEST(Mat4, MultiplyByIdentityIsNoOp) {
    const Mat4 T = Mat4::translate(3.0f, -4.0f, 5.0f);
    EXPECT_TRUE(MatNear(T * Mat4::identity(), T));
    EXPECT_TRUE(MatNear(Mat4::identity() * T, T));
}

TEST(Mat4, MultiplyIsAssociative) {
    const Mat4 A = Mat4::translate(1.0f, 2.0f, 3.0f);
    const Mat4 B = Mat4::scale(2.0f, 2.0f, 2.0f);
    const Mat4 C = Mat4::rotateY(kPi * 0.5f);
    EXPECT_TRUE(MatNear((A * B) * C, A * (B * C)));
}

TEST(Mat4, MultiplyVec4MatchesRowTimesColumn) {
    // 用 translate 手算：(1,2,3,1) 平移 (10,20,30) ⇒ (11,22,33,1)
    const Mat4 T = Mat4::translate(10.0f, 20.0f, 30.0f);
    const Vec4 r = T * Vec4(1.0f, 2.0f, 3.0f, 1.0f);
    EXPECT_FLOAT_EQ(r.x, 11.0f);
    EXPECT_FLOAT_EQ(r.y, 22.0f);
    EXPECT_FLOAT_EQ(r.z, 33.0f);
    EXPECT_FLOAT_EQ(r.w, 1.0f);
}

// ---------------------------------------------------------------------------
// 三种基本变换
// ---------------------------------------------------------------------------

TEST(Mat4, TranslateMovesPointButNotDirection) {
    const Mat4 T = Mat4::translate(5.0f, 0.0f, 0.0f);
    const Vec4 p = T.transformPoint(Vec3(1.0f, 1.0f, 1.0f));
    EXPECT_FLOAT_EQ(p.x, 6.0f);
    EXPECT_FLOAT_EQ(p.w, 1.0f);          // 点：w = 1

    const Vec4 d = T.transformDir(Vec3(1.0f, 0.0f, 0.0f));
    EXPECT_FLOAT_EQ(d.x, 1.0f);          // ★ 方向不受平移影响
    EXPECT_FLOAT_EQ(d.w, 0.0f);          // 方向：w = 0
}

TEST(Mat4, ScaleScalesPointComponents) {
    const Mat4 S = Mat4::scale(2.0f, 3.0f, 4.0f);
    const Vec4 p = S.transformPoint(Vec3(1.0f, 1.0f, 1.0f));
    EXPECT_FLOAT_EQ(p.x, 2.0f);
    EXPECT_FLOAT_EQ(p.y, 3.0f);
    EXPECT_FLOAT_EQ(p.z, 4.0f);
}

TEST(Mat4, RotateYQuarterTurnSendsXToMinusZ) {
    // 右手系绕 +Y 转 90°：(1,0,0) ⇒ (0,0,-1)
    const Mat4 R = Mat4::rotateY(kPi * 0.5f);
    const Vec4 p = R.transformPoint(Vec3(1.0f, 0.0f, 0.0f));
    EXPECT_NEAR(p.x, 0.0f, 1e-4f);
    EXPECT_NEAR(p.y, 0.0f, 1e-4f);
    EXPECT_NEAR(p.z, -1.0f, 1e-4f);
}

// ---------------------------------------------------------------------------
// 透视投影（★ 本文件最重要的三条）
// ---------------------------------------------------------------------------

TEST(Mat4, PerspectiveWEqualsMinusZView) {
    // ★★ w_clip = -z_view：相机前方 z_view=-5 ⇒ w=5
    const Mat4 P = Mat4::perspective(kPi * 0.25f, 1.5f, 1.0f, 100.0f);
    const Vec4 c = P.transformPoint(Vec3(0.0f, 0.0f, -5.0f));
    EXPECT_NEAR(c.w, 5.0f, 1e-4f);
}

TEST(Mat4, PerspectiveMapsNearAndFarToNdcEnds) {
    const float n = 1.0f, f = 100.0f;
    const Mat4 P = Mat4::perspective(kPi * 0.25f, 1.5f, n, f);

    const Vec4 atNear = P.transformPoint(Vec3(0.0f, 0.0f, -n));
    EXPECT_NEAR(atNear.z / atNear.w, -1.0f, 1e-4f);   // z_ndc = -1

    const Vec4 atFar = P.transformPoint(Vec3(0.0f, 0.0f, -f));
    EXPECT_NEAR(atFar.z / atFar.w, 1.0f, 1e-4f);      // z_ndc = +1
}

TEST(Mat4, PerspectiveIsNearBiggerThanFar) {
    // 同一世界 x=1，z_view=-1 与 -2 ⇒ x_ndc 应该 2:1（近大远小）
    const Mat4 P = Mat4::perspective(kPi * 0.25f, 1.0f, 1.0f, 100.0f);

    const Vec4 a = P.transformPoint(Vec3(1.0f, 0.0f, -1.0f));
    const Vec4 b = P.transformPoint(Vec3(1.0f, 0.0f, -2.0f));

    const float xa = a.x / a.w;
    const float xb = b.x / b.w;
    EXPECT_GT(xa, 0.0f);
    EXPECT_NEAR(xa / xb, 2.0f, 1e-3f);   // 近的 x_ndc 是远的两倍
}

// ---------------------------------------------------------------------------
// 视图矩阵
// ---------------------------------------------------------------------------

TEST(Mat4, LookAtPutsEyeAtOriginAndCenterOnMinusZ) {
    const Mat4 V = Mat4::lookAt(Vec3(0.0f, 0.0f, 5.0f),   // eye
                                Vec3(0.0f, 0.0f, 0.0f),   // center
                                Vec3(0.0f, 1.0f, 0.0f));  // up

    const Vec4 eyeInView = V.transformPoint(Vec3(0.0f, 0.0f, 5.0f));
    EXPECT_NEAR(eyeInView.x, 0.0f, 1e-4f);
    EXPECT_NEAR(eyeInView.y, 0.0f, 1e-4f);
    EXPECT_NEAR(eyeInView.z, 0.0f, 1e-4f);   // ★ 相机自己落在原点

    const Vec4 centerInView = V.transformPoint(Vec3(0.0f, 0.0f, 0.0f));
    EXPECT_NEAR(centerInView.x, 0.0f, 1e-4f);
    EXPECT_NEAR(centerInView.y, 0.0f, 1e-4f);
    EXPECT_NEAR(centerInView.z, -5.0f, 1e-4f);  // ★ 注视点在 -Z 方向
}

TEST(Mat4, LookAtThenPerspectiveKeepsCenterInScreen) {
    // MVP 串联：注视点经 V、P 后应落在屏幕中心 ⇒ NDC x=y=0
    const Mat4 V = Mat4::lookAt(Vec3(0.0f, 0.0f, 5.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
    const Mat4 P = Mat4::perspective(kPi * 0.25f, 1.5f, 0.1f, 100.0f);

    const Vec4 c = (P * V).transformPoint(Vec3(0.0f, 0.0f, 0.0f));
    EXPECT_NEAR(c.x / c.w, 0.0f, 1e-4f);
    EXPECT_NEAR(c.y / c.w, 0.0f, 1e-4f);
    // z_view = -5 时的正确 z_ndc（n=0.1, f=100）：
    //   (f+n)/(f-n) + 2fn/((f-n)*z_view) = 1.002002 - 0.200200/5 = 0.961962
    EXPECT_NEAR(c.z / c.w, 0.961962f, 1e-4f);
    EXPECT_GT(c.z / c.w, -1.0f);   // 仍在 NDC 范围内
    EXPECT_LT(c.z / c.w, 1.0f);
}

int main(int argc, char** argv) {
    //  进程退出时自动 dump 泄漏：此时静态对象已析构 → 不会把 gtest 注册表误报成泄漏
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
