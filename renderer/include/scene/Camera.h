#pragma once

// ============================================================================
//  U0 前置 · 相机（11-03/11-04 A 轨）
//  分工：本文件 + 测试 + CMake 由教练提供；Camera.cpp 由学员实现
//
//  ★ 依赖已经完成的 Mat4：viewMatrix() 就是 lookAt()，projMatrix() 就是 perspective()
//    —— 这个类不是新知识，只是把它们**收拢到一处**，并定死 MVP 的乘法顺序
//
//  ★★★ 一条口径（本文件唯一要记的）★★★
//      mvpMatrix(model) = proj * view * model
//      矩阵右乘先作用 ⇒ 顶点先被 model 变换，再 view，再 proj
// ============================================================================

#include "math/Mat4.h"
#include "math/Vec3.h"

struct Camera {
    Vec3 position;     // 相机世界位置
    Vec3 target;       // 注视点（世界空间）
    Vec3 up;           // 参考上方向（世界空间，不必归一化；不能与视线平行）

    float fovYRad;     // 垂直视场角（**弧度**）
    float aspect;      // 宽 / 高
    float nearZ;       // 近平面距离（> 0）
    float farZ;        // 远平面距离（> nearZ）

    // 默认值：位置 (0,0,5)、看原点、up=(0,1,0)、fovY=45°、aspect=1、near=0.1、far=100
    Camera();

    // world -> view     （= Mat4::lookAt(position, target, up)）
    Mat4 viewMatrix() const;

    // view  -> clip     （= Mat4::perspective(fovYRad, aspect, nearZ, farZ)）
    Mat4 projMatrix() const;

    // ★ = projMatrix() * viewMatrix() * model   （右乘先做）
    Mat4 mvpMatrix(const Mat4& model) const;

    // 世界点 -> clip space 齐次坐标
    // ★ 不做透视除法（除法是管线里单独的一步）
    Vec4 projectToClip(const Vec3& worldPoint, const Mat4& model) const;
};