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

int main(int argc, char** argv) {
    //  进程退出时自动 dump 泄漏：此时静态对象已析构 → 不会把 gtest 注册表误报成泄漏
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}