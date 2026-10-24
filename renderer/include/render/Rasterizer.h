// renderer/include/render/Rasterizer.h
#pragma once
#include "math/Vec3.h"
#include "render/Framebuffer.h"

// 本渲染器约定：NDC 空间按【逆时针 CCW】为正面（OpenGL 惯例；U03 背面剔除依赖它）
// 注意：视口变换的 y 翻转会把屏幕空间里的朝向反过来

// 3D 顶点：NDC 坐标（x ∈ [-1,1] 右正，y ∈ [-1,1] 【上】正，z ∈ [-1,1] 【小 = 近】）
// ★ U02：由 Vertex2D 升级而来 —— 多一个 z，用来做深度测试
struct Vertex3D {
    float x, y, z;
};

// NDC → 像素坐标：★【不做裁剪】—— 可能返回负数或 >= width/height
// 裁剪统一由【包围盒 clamp】那一步完成：完全在屏幕外的三角形会得到空包围盒（x0 > x1）
// 契约由 test_renderer.cpp 的 Rasterizer.ViewportDoesNotClampOffScreenCoords 钉住（教练错误 #13）
int toScreenX(float ndcX, int width);
int toScreenY(float ndcY, int height);

// 把实心三角形画进 fb（带深度测试）；返回【真正通过深度测试并被写入的像素个数】
//
// 契约：
//  ① 深度用 U01 的重心坐标在三个顶点之间线性插值：z = w0*a.z + w1*b.z + w2*c.z
//     （NDC z 在屏幕空间是【仿射】的 ⇒ 线性插值就是精确的，本单元不需要 1/z 校正）
//  ② 测试用【严格小于】：z < fb.depthAt(x, y) 才写入 ⇒ 更近的片元赢
//  ③ ★ 平局（深度完全相等）由"先到者"胜出 ⇒ 【共面 / 共享边的几何不保证顺序无关】。
//     这是一条【已知边界】：U03 网格化时会大规模遇到，届时要靠"与顺序无关的第二键"
//     （例如三角形编号）来裁决平局，而【不能】靠"谁后画"——那等于用要证的东西证明自己。
int rasterizeTriangle(Framebuffer& fb, const Vertex3D& a, const Vertex3D& b,
    const Vertex3D& c, const Vec3& color);