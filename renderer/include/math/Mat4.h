#pragma once

// ============================================================================
//  U0 前置 · 齐次坐标 Vec4 与 4×4 变换矩阵 Mat4  —— 接口契约
//  分工：本文件 + 测试 + CMake 由教练提供；Mat4.cpp 由学员实现
//
//  ★★★ 五条口径（定死，实现与测试都按这个来，不许改）★★★
//  ① 存储 = **行主序（row-major）**：m[row][col]
//  ② 变换 = **列向量在右**：v' = M * v（M 在左）
//  ③ 平移在**第 4 列**：m[0][3]=tx, m[1][3]=ty, m[2][3]=tz
//  ④ **右手系，相机看向 −Z**（与 OpenGL 一致）
//  ⑤ **NDC z ∈ [−1, +1]**；透视除法后 w 被丢弃
//
//  ★ 与 OpenGL / GLSL 的差异（记住这一条，批次 4 会用到）：
//    GLSL 的 mat4 是**列主序**，而我们是行主序
//    ⇒ 将来把矩阵传给 GPU 时，**需要转置**（上传前 transpose 一次）
//    ⇒ 现在选行主序的理由：CPU 侧 `m[row][col]` 直观、调试时能直接读
// ============================================================================

#include <cmath>

#include "math/Vec3.h"

// ---------------------------------------------------------------------------
// 齐次坐标
// ---------------------------------------------------------------------------
struct Vec4 {
    float x, y, z, w;

    Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    Vec3 xyz() const { return Vec3(x, y, z); }
};

// ---------------------------------------------------------------------------
// 4×4 矩阵
// ---------------------------------------------------------------------------
struct Mat4 {
    float m[4][4];   //  m[row][col]，行主序

    // 默认构造 = **单位矩阵**（不是全 0！）
    Mat4();

    static Mat4 identity();
    static Mat4 translate(float tx, float ty, float tz);
    static Mat4 scale(float sx, float sy, float sz);
    static Mat4 rotateX(float radians);   
    static Mat4 rotateY(float radians);   // 绕 Y 轴，右手系：+x 转向 −z
    static Mat4 rotateZ(float radians);   

    // 透视投影
    //   fovYRad : 垂直视场角（**弧度**）
    //   aspect  : 宽 / 高
    //   n, f    : 近 / 远平面距离（都取正值，且 n < f）
    // ★ 要求：w_clip = −z_view  ⇒ 相机前方 z_view < 0 时 w_clip > 0
    // ★ 要求：z_view = −n ⇒ z_ndc = −1 ；z_view = −f ⇒ z_ndc = +1
    static Mat4 perspective(float fovYRad, float aspect, float n, float f);

    // 视图矩阵：world → view（相机位于原点、看向 −Z）
    //   eye    : 相机世界位置
    //   center : 注视点
    //   up     : 世界上方向（不必已归一化，但**不能与 forward 平行**）
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up);

    Mat4 operator*(const Mat4& o) const;
    Vec4 operator*(const Vec4& v) const;

    // 点变换（w=1）→ **clip space 齐次坐标**
    // ★ 关键：**不做透视除法**。除法是管线里单独的一步（题 2 的考点）
    Vec4 transformPoint(const Vec3& p) const;

    // 方向 / 法线变换（w=0，不带平移）
    // ★ 适用范围：变换只含【旋转 + 均匀缩放 + 平移】时成立（本项目就这个范围）
    // ★ 非均匀缩放要改用 (M⁻¹)ᵀ —— 本项目不做
    Vec4 transformDir(const Vec3& d) const;
};