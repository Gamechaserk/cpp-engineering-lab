// ============================================================================
//  U03 · 光照（Blinn-Phong）与背面剔除  ·  测试
//  沿用工程既有约定：gtest + 自己写 main（含 CRT 泄漏检查）+ gtest_discover_tests
// ============================================================================

#include <cmath>

#include "gtest/gtest.h"
#include "math/Vec3.h"
#include "render/Shading.h"

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

namespace {

using render::Light;
using render::Material;

// 造一组"最容易出错的"默认参数：只有环境光
Material MakeMaterialOnlyAmbient() {
    Material M;
    M.albedo    = Vec3(0.5f, 0.5f, 0.5f);
    M.ka        = 0.2f;
    M.ks        = 0.0f;
    M.shininess = 16.0f;
    return M;
}

Material MakeMaterialPureWhite() {
    Material M;
    M.albedo    = Vec3(1.0f, 1.0f, 1.0f);
    M.ka        = 0.0f;
    M.ks        = 0.0f;
    M.shininess = 16.0f;
    return M;
}

Light MakeLightAbove() {
    Light L;
    L.position  = Vec3(0.0f, 10.0f, 0.0f);
    L.color     = Vec3(1.0f, 1.0f, 1.0f);
    L.intensity = 1.0f;
    return L;
}

} // namespace

// ---------------------------------------------------------------------------
// 2) 有向面积：反对称 / 整数精确 / 退化
// ---------------------------------------------------------------------------

TEST(Shading, AreaIsAntisymmetric) {
    const float abc = render::signedArea2D(0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 3.0f);
    const float acb = render::signedArea2D(0.0f, 0.0f, 0.0f, 3.0f, 4.0f, 0.0f);
    EXPECT_FLOAT_EQ(abc, -acb);
}

TEST(Shading, AreaIsExactForIntegerInputs) {
    // 直角三角形，两倍面积 = 12
    EXPECT_FLOAT_EQ(std::fabs(render::signedArea2D(0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 3.0f)), 12.0f);
}

TEST(Shading, AreaIsZeroForDegenerate) {
    EXPECT_FLOAT_EQ(render::signedArea2D(0.0f, 0.0f, 1.0f, 1.0f, 2.0f, 2.0f), 0.0f);  // 共线
    EXPECT_FLOAT_EQ(render::signedArea2D(5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f), 0.0f);  // 重合
}

// ---------------------------------------------------------------------------
// 1) Blinn-Phong
// ---------------------------------------------------------------------------

TEST(Shading, DiffuseFullWhenLightAlongNormal) {
    const Vec3 P(0.0f, 0.0f, 0.0f);
    const Vec3 N(0.0f, 1.0f, 0.0f);
    const Vec3 V(0.0f, 1.0f, 0.0f);
    const Vec3 c = render::blinnPhong(P, N, V, MakeLightAbove(), MakeMaterialPureWhite());
    EXPECT_NEAR(c.x, 1.0f, 1e-3f);   // N·L = 1
    EXPECT_NEAR(c.y, 1.0f, 1e-3f);
    EXPECT_NEAR(c.z, 1.0f, 1e-3f);
}

TEST(Shading, DiffuseIsZeroWhenLightBehindSurface) {
    // ★ 经典坑：不 clamp 就会出现负的漫反射（背面越亮）
    const Vec3 P(0.0f, 0.0f, 0.0f);
    const Vec3 N(0.0f, 1.0f, 0.0f);
    const Vec3 V(0.0f, 1.0f, 0.0f);

    Light L = MakeLightAbove();
    L.position = Vec3(0.0f, -10.0f, 0.0f);   // 光在表面下方

    const Vec3 c = render::blinnPhong(P, N, V, L, MakeMaterialPureWhite());
    EXPECT_NEAR(c.x, 0.0f, 1e-3f);
    EXPECT_NEAR(c.y, 0.0f, 1e-3f);
    EXPECT_NEAR(c.z, 0.0f, 1e-3f);
}

TEST(Shading, AmbientUsesKaTimesAlbedo) {
    const Vec3 P(0.0f, 0.0f, 0.0f);
    const Vec3 N(0.0f, 1.0f, 0.0f);
    const Vec3 V(0.0f, 1.0f, 0.0f);

    Light L = MakeLightAbove();
    L.position = Vec3(0.0f, -10.0f, 0.0f);   // 背面 ⇒ 只剩 ambient

    const Vec3 c = render::blinnPhong(P, N, V, L, MakeMaterialOnlyAmbient());
    EXPECT_NEAR(c.x, 0.1f, 1e-3f);   // 0.2 * 0.5
    EXPECT_NEAR(c.y, 0.1f, 1e-3f);
    EXPECT_NEAR(c.z, 0.1f, 1e-3f);
}

TEST(Shading, SpecularShrinksAsShininessGrows) {
    // 光在正上方，视线偏离法线 30°（用"顺序"判断，不依赖具体数值）
    const Vec3 P(0.0f, 0.0f, 0.0f);
    const Vec3 N(0.0f, 1.0f, 0.0f);
    const Vec3 V(0.5f, 0.8660254f, 0.0f);   // 与 +y 成 30°，单位向量

    Material M;
    M.albedo = Vec3(0.0f, 0.0f, 0.0f);      // 去掉漫反射
    M.ka     = 0.0f;
    M.ks     = 1.0f;

    M.shininess = 4.0f;
    const float tight4 = render::blinnPhong(P, N, V, MakeLightAbove(), M).x;

    M.shininess = 64.0f;
    const float tight64 = render::blinnPhong(P, N, V, MakeLightAbove(), M).x;

    EXPECT_GT(tight4, tight64);   // 指数越大 ⇒ 偏离镜面时衰减越快
    EXPECT_GT(tight4, 0.0f);
    EXPECT_GT(tight64, 0.0f);
}

TEST(Shading, ResultIsClampedToUnitRange) {
    const Vec3 P(0.0f, 0.0f, 0.0f);
    const Vec3 N(0.0f, 1.0f, 0.0f);
    const Vec3 V(0.0f, 1.0f, 0.0f);

    Light L = MakeLightAbove();
    L.intensity = 100.0f;

    Material M;
    M.albedo    = Vec3(1.0f, 1.0f, 1.0f);
    M.ka        = 1.0f;
    M.ks        = 1.0f;
    M.shininess = 16.0f;

    const Vec3 c = render::blinnPhong(P, N, V, L, M);
    EXPECT_LE(c.x, 1.0f);
    EXPECT_LE(c.y, 1.0f);
    EXPECT_LE(c.z, 1.0f);
    EXPECT_GE(c.x, 0.0f);
    EXPECT_GE(c.y, 0.0f);
    EXPECT_GE(c.z, 0.0f);
}

int main(int argc, char** argv) {
    //  进程退出时自动 dump 泄漏：此时静态对象已析构 → 不会把 gtest 注册表误报成泄漏
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}