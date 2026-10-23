// renderer/include/render/Framebuffer.h
#pragma once
#include <vector>
#include "math/Vec3.h"

// 约定：Framebuffer(0,0) = 图像【左上角】；行号向下增大（与 PPM 一致）
//       数学库的 y 轴【向上】→ U01 视口变换时需做一次 y 翻转
class Framebuffer {
public:
    Framebuffer(int width, int height);
    int width() const;
    int height() const;
    void set(int x, int y, const Vec3& color);
    const Vec3& at(int x, int y) const;
private:
    std::size_t index(int x, int y) const;
    int m_width, m_height;
    std::vector<Vec3> m_pixels;
};