// ── CRT 泄漏检查开关： 必须在【所有 #include 之前】定义，否则无效 ──
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#include <gtest/gtest.h>

// ── Math：数学库（U04，测试由教练提供）──
//  期望值一律【手算】写死，不从实现里抄公式（K13.18）
//  浮点规则：精确可表示的值用 EXPECT_FLOAT_EQ；经过乘/除/开方的用 EXPECT_NEAR 带容差
#include "math/Vec3.h"

TEST(Math, DotAndCross) {
    const Vec3 a(1.0f, 0.0f, 0.0f), b(0.0f, 1.0f, 0.0f);

    EXPECT_FLOAT_EQ(a.dot(b), 0.0f);        // 垂直 → 点积恰为 0
    EXPECT_FLOAT_EQ(a.dot(a), 1.0f);        // 自己点自己 = 长度平方

    const Vec3 c = a.cross(b);              // 手算：(1,0,0)×(0,1,0) = (0,0,1)
    EXPECT_FLOAT_EQ(c.x, 0.0f);
    EXPECT_FLOAT_EQ(c.y, 0.0f);
    EXPECT_FLOAT_EQ(c.z, 1.0f);

    const Vec3 d = b.cross(a);              //  反交换律：b×a = -(a×b)
    EXPECT_FLOAT_EQ(d.z, -1.0f);
}
TEST(Math, NormalizedHasUnitLengthAndKeepsSource) {
    const Vec3 v(3.0f, 0.0f, 4.0f);         // 长度 5（勾股）
    const Vec3 n = v.normalized();

    EXPECT_NEAR(n.x, 0.6f, 1e-6f);          // 3/5，手算
    EXPECT_NEAR(n.z, 0.8f, 1e-6f);          // 4/5
    EXPECT_NEAR(n.dot(n), 1.0f, 1e-6f);     // 单位向量：自己点自己 = 1

    EXPECT_FLOAT_EQ(v.x, 3.0f);             //  原向量没被改：normalized() 非破坏性
    EXPECT_FLOAT_EQ(v.z, 4.0f);
}
TEST(Math, ZeroVectorNormalizeDoesNotProduceNaN) {
    const Vec3 z(0.0f, 0.0f, 0.0f);
    const Vec3 n = z.normalized();

    EXPECT_FALSE(n.x != n.x) << "NaN 会传染：x 不能是 NaN";   //  NaN != NaN 是唯一自检手段
    EXPECT_FLOAT_EQ(n.x, 0.0f);
    EXPECT_FLOAT_EQ(n.y, 0.0f);
    EXPECT_FLOAT_EQ(n.z, 0.0f);
}
TEST(Math, ScalarMultiplyBothOrders) {
    const Vec3 v(1.0f, 2.0f, 3.0f);

    const Vec3 a = v * 2.0f;                // 成员 operator*
    EXPECT_FLOAT_EQ(a.x, 2.0f);
    EXPECT_FLOAT_EQ(a.z, 6.0f);

    const Vec3 b = 2.0f * v;                //  左操作数是标量 → 只能由【自由函数】提供
    EXPECT_FLOAT_EQ(b.y, 4.0f);
}
TEST(Math, AddAndSub) {
    const Vec3 a(1.0f, 2.0f, 3.0f), b(4.0f, 5.0f, 6.0f);

    const Vec3 s = a + b;
    EXPECT_FLOAT_EQ(s.x, 5.0f); EXPECT_FLOAT_EQ(s.y, 7.0f); EXPECT_FLOAT_EQ(s.z, 9.0f);

    const Vec3 d = b - a;
    EXPECT_FLOAT_EQ(d.x, 3.0f); EXPECT_FLOAT_EQ(d.y, 3.0f); EXPECT_FLOAT_EQ(d.z, 3.0f);

    EXPECT_FLOAT_EQ(a.x, 1.0f);             //  b - a 之后 a 也没变（值语义）
}
TEST(Math, LayoutIsZeroOverhead) {
    //  用 sizeof 实证"struct 就是它的成员本身"，不是指针、没有额外开销
    EXPECT_EQ(sizeof(Vec3), 12u);           // 3 个 float，无 padding、无虚表
    EXPECT_EQ(sizeof(Vec3&), 12u);          //  引用没有自己的大小：sizeof(引用) = 被引用类型的大小
    EXPECT_EQ(sizeof(Vec3*), sizeof(void*)); // 指针才是 8 字节（x64）
}

// ── Rasterizer：B2-U01 视口变换 + 重心坐标光栅化（测试由教练产出，2026-10-23 规则变更）──
//  期望值一律【手算 / 独立定理】写死，不从实现里抄公式（K13.18）
//  ★ 像素个数的【精确】期望值用 Pick 定理独立算出（与实现无任何共同公式）：
//      闭合约定（w >= 0，含边界像素）下：像元数 = I + B
//      其中 B = 各边 gcd(|dx|,|dy|) 之和（边界格点数）
//           I = A - B/2 + 1（内部格点数，A 为鞋带公式面积）
#include "render/Rasterizer.h"

namespace {

// 101×101 黑底：W-1 = 100 ⇒ ndc 取 0.2 的整数倍时正好落到整数像素，便于手算
Framebuffer makeBlank101() {
    Framebuffer fb(101, 101);
    const Vec3 black(0.0f, 0.0f, 0.0f);
    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            fb.set(x, y, black);
        }
    }
    return fb;
}

}  // namespace

TEST(Rasterizer, ViewportMapsNdcEndpointsToPixelEndpoints) {
    EXPECT_EQ(toScreenX(-1.0f, 101), 0);      // NDC 左边缘 → 第 0 列
    EXPECT_EQ(toScreenX(0.0f, 101), 50);      // 正中心
    EXPECT_EQ(toScreenX(1.0f, 101), 100);     // 右边缘 → W-1（不是 W：决策点 1）
    EXPECT_EQ(toScreenX(-0.5f, 101), 25);
    EXPECT_EQ(toScreenX(0.5f, 101), 75);

    EXPECT_EQ(toScreenX(-1.0f, 256), 0);
    EXPECT_EQ(toScreenX(1.0f, 256), 255);     // 映射到最后一列，不越界一格
    EXPECT_EQ(toScreenX(0.0f, 256), 128);     // 127.5 → roundf → 128
}
TEST(Rasterizer, ViewportFlipsYAxis) {
    // ★ 回归测试：toScreenY 曾把 X 的公式照抄过来、漏掉 y 翻转
    EXPECT_EQ(toScreenY(1.0f, 256), 0);       // NDC 的【上】→ 第 0 行（图像顶部）
    EXPECT_EQ(toScreenY(-1.0f, 256), 255);    // NDC 的【下】→ 最后一行
    EXPECT_EQ(toScreenY(1.0f, 101), 0);
    EXPECT_EQ(toScreenY(-1.0f, 101), 100);
    EXPECT_EQ(toScreenY(0.5f, 101), 25);      // 位置更高的点 → 更小的行号
    EXPECT_EQ(toScreenY(-0.5f, 101), 75);

    EXPECT_LT(toScreenY(0.9f, 256), toScreenY(-0.9f, 256));   // 单调反向
}
TEST(Rasterizer, ViewportDoesNotClampOffScreenCoords) {
    // 接口契约（教练错误 #13）：裁剪发生在【包围盒】那一步，不在坐标映射里
    EXPECT_LT(toScreenX(-3.0f, 101), 0);      // 远超左边界 → 负列号
    EXPECT_GT(toScreenX(3.0f, 101), 100);     // 远超右边界 → 大于 W-1

    // ★ y 轴已翻转 ⇒ 方向与 x 轴【相反】（教练错误 #14：这里第一版照抄了 x 轴的方向）
    //   ndcY 越大 = 越靠上 ⇒ 行号越小
    EXPECT_LT(toScreenY(3.0f, 101), 0);       // 远超上边界 → 负行号
    EXPECT_GT(toScreenY(-3.0f, 101), 100);    // 远超下边界 → 大于 H-1
}
TEST(Rasterizer, TriangleCoverageMatchesPickTheoremExactly) {
    Framebuffer fb = makeBlank101();
    const Vec3 white(1.0f, 1.0f, 1.0f);

    // NDC → 像素：A(0,0.8)→(50,10)   B(-0.8,-0.8)→(10,90)   C(0.8,-0.8)→(90,90)
    const Vertex2D a{ 0.0f, 0.8f }, b{ -0.8f, -0.8f }, c{ 0.8f, -0.8f };
    const int written = rasterizeTriangle(fb, a, b, c, white);

    // 期望值独立手算（Pick 定理）：
    //   A = |鞋带| / 2 = 6400 / 2 = 3200
    //   B = gcd(40,80) + gcd(80,0) + gcd(40,80) = 40 + 80 + 40 = 160
    //   I = A - B/2 + 1 = 3200 - 80 + 1 = 3121
    //   闭合约定 ⇒ 像元数 = I + B = 3281（精确值，不是近似）
    EXPECT_EQ(written, 3281);

    // 三个顶点像素必须被写上（边界归属本三角形 = 闭区间约定的实证）
    EXPECT_FLOAT_EQ(fb.at(50, 10).x, 1.0f);
    EXPECT_FLOAT_EQ(fb.at(10, 90).x, 1.0f);
    EXPECT_FLOAT_EQ(fb.at(90, 90).x, 1.0f);

    // 明显在三角形之外的像素不许被碰
    EXPECT_FLOAT_EQ(fb.at(90, 10).x, 0.0f);
    EXPECT_FLOAT_EQ(fb.at(11, 11).x, 0.0f);
}
TEST(Rasterizer, WindingOrderDoesNotChangeCoverage) {
    const Vec3 white(1.0f, 1.0f, 1.0f);
    const Vertex2D a{ 0.0f, 0.8f }, b{ -0.8f, -0.8f }, c{ 0.8f, -0.8f };

    Framebuffer fb1 = makeBlank101();
    const int cw = rasterizeTriangle(fb1, a, b, c, white);     // 屏幕空间：CW
    Framebuffer fb2 = makeBlank101();
    const int ccw = rasterizeTriangle(fb2, a, c, b, white);    // 交换两顶点 → CCW

    // 归一化（除以有符号面积）使 inside test 与朝向无关（决策点 4）
    EXPECT_EQ(cw, ccw);

    int diff = 0;
    for (int y = 0; y < 101; ++y) {
        for (int x = 0; x < 101; ++x) {
            if (fb1.at(x, y).x != fb2.at(x, y).x) { ++diff; }
        }
    }
    EXPECT_EQ(diff, 0);      // 还要逐像素完全相同
}
TEST(Rasterizer, DegenerateTriangleWritesNothing) {
    const Vec3 white(1.0f, 1.0f, 1.0f);

    // ① 三点共线（NDC 的 y=x 对角线）→ 像素 (0,100) (50,50) (100,0)
    Framebuffer fb1 = makeBlank101();
    const Vertex2D p{ -1.0f, -1.0f }, q{ 0.0f, 0.0f }, r{ 1.0f, 1.0f };
    EXPECT_EQ(rasterizeTriangle(fb1, p, q, r, white), 0);

    // ② 三点映射到同一像素 → 面积在【整数域】精确为 0（选 int area 的回报：不需要 eps）
    Framebuffer fb2(256, 256);
    const Vertex2D s{ 0.0f, 0.0f }, t{ 0.0f, 0.0f }, u{ 0.0f, 0.0f };
    EXPECT_EQ(rasterizeTriangle(fb2, s, t, u, white), 0);
}
TEST(Rasterizer, OffScreenTrianglesAreClippedByBoundingBoxNotByMapping) {
    const Vec3 white(1.0f, 1.0f, 1.0f);

    // ① 完全在右侧屏外：像素 x = 110 / 120 / 130 ⇒ clamp 后 x0 > x1 ⇒ 空包围盒
    Framebuffer fb1 = makeBlank101();
    const Vertex2D a{ 1.2f, -0.5f }, b{ 1.4f, 0.5f }, c{ 1.6f, 0.0f };
    EXPECT_EQ(rasterizeTriangle(fb1, a, b, c, white), 0);

    // ② 完全在左侧屏外：像素 x 全为负 ⇒ 同样得到空包围盒
    Framebuffer fb2 = makeBlank101();
    const Vertex2D d{ -1.6f, -0.5f }, e{ -1.4f, 0.5f }, f{ -1.2f, 0.0f };
    EXPECT_EQ(rasterizeTriangle(fb2, d, e, f, white), 0);

    // ③ 部分在屏外：必须被 clamp 且【ASan 不报越界】
    //    若把 clamp 删掉，这里立刻变成 heap-buffer-overflow，Debug 构建下必然失败
    Framebuffer fb3 = makeBlank101();
    const Vertex2D g{ 0.0f, 0.5f }, h{ -3.0f, -3.0f }, i{ 3.0f, -3.0f };
    const int written = rasterizeTriangle(fb3, g, h, i, white);
    EXPECT_GT(written, 0);
    EXPECT_LE(written, 101 * 101);
}

int main(int argc, char** argv) {
    //  进程退出时自动 dump 泄漏：此时静态对象已析构 → 不会把 gtest 注册表误报成泄漏
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}