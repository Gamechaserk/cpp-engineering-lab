// renderer/include/render/Rasterizer.h
#pragma once
#include "math/Vec3.h"
#include "render/Framebuffer.h"

// 本渲染器约定：NDC 空间按【逆时针 CCW】为正面（OpenGL 惯例；U03 背面剔除依赖它）
// 注意：视口变换的 y 翻转会把屏幕空间里的朝向反过来

// 2D 顶点：NDC 坐标（x ∈ [-1,1] 右正，y ∈ [-1,1] 【上】正）
struct Vertex2D {
    float x, y;
};

// NDC → 像素坐标：★【不做裁剪】—— 可能返回负数或 >= width/height
// 裁剪统一由【包围盒 clamp】那一步完成：完全在屏幕外的三角形会得到空包围盒（x0 > x1）
// 契约由 test_renderer.cpp 的 Rasterizer.ViewportDoesNotClampOffScreenCoords 钉住（教练错误 #13）
int toScreenX(float ndcX, int width);
int toScreenY(float ndcY, int height);

// 把实心三角形画进 fb；返回【实际写入的像素个数】
// 返回 0 = 一个像素都没写上（退化 / 完全在屏幕外）→ 便于自测
int rasterizeTriangle(Framebuffer& fb, const Vertex2D& a, const Vertex2D& b,
    const Vertex2D& c, const Vec3& color);