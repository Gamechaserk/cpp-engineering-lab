#pragma once

// ============================================================================
//  U03 · 光照（Blinn-Phong）与背面剔除 —— 接口契约
//  分工：本文件 + 测试 + CMake 由教练提供；Shading.cpp 由学员实现
//  依赖：math/Vec3.h（dot / cross / normalized 已有）
//  约定：传入的方向向量均为**单位向量**，且都是"从着色点指向外侧"
// ============================================================================

#include "math/Vec3.h"

namespace render {

// 点光源（世界空间）
struct Light {
    Vec3  position;    // 光源位置
    Vec3  color;       // 线性 RGB（各通道 0..1）
    float intensity;   // 强度标量（可 > 1，最终靠 clamp 收）
};

// 材质（简化 Blinn-Phong）
struct Material {
    Vec3  albedo;      // 基础色（等价于 kd）
    float ka;          // 环境光系数
    float ks;          // 高光系数
    float shininess;   // 高光指数（越大高光越集中）
};

// ----------------------------------------------------------------------------
// 1) Blinn-Phong 单光源着色
//    P : 世界空间上的着色点
//    N : 单位法线（世界空间，朝外）
//    V : 单位视线（着色点 -> 相机）
//    L : 点光源
//    M : 材质
// 返回 : ambient + diffuse + specular，各通道 clamp 到 [0, 1]
//
// ★ 实现要求（测试会检查）:
//    a) 光线方向 Ldir = (L.position - P).normalized()
//    b) 漫反射 = max(0, N·Ldir) —— **不能出现负数**（背面必须为 0）
//    c) 半程向量 H = (Ldir + V).normalized()
//       高光 = max(0, N·H)^shininess   ← 注意是 **N·H**，不是反射向量 R·V
//    d) ambient = ka * albedo
//    e) 三项都乘上 L.color * L.intensity（ambient 是否乘光强：按(d)不乘）
//    f) 最终各通道 clamp 到 [0, 1]
// ----------------------------------------------------------------------------
Vec3 blinnPhong(const Vec3& P, const Vec3& N, const Vec3& V,
                const Light& L, const Material& M);

// ----------------------------------------------------------------------------
// 2) 屏幕空间有向面积（用于背面剔除）
//    参数是三个屏幕空间点（像素坐标，按 Framebuffer 约定 y 向下）
// 返回 : 2 倍有向面积
//    > 0 / < 0 对应两种绕向；== 0 表示三点共线（退化三角形，应丢弃）
//
// ★ 实现要求（测试会检查）:
//    a) 交换任意两个顶点 ⇒ 结果取反（反对称）
//    b) 输入是整数像素坐标时，结果必须是精确整数（不要引入浮点误差）
//    c) 三点共线 / 重合 ⇒ 恰好为 0
//    d) 符号约定必须与 Rasterizer.cpp 里的边函数一致（否则正面会被剔掉）
// ----------------------------------------------------------------------------
float signedArea2D(float ax, float ay,
                   float bx, float by,
                   float cx, float cy);

} // namespace render