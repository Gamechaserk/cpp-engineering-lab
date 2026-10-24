//本渲染器约定：NDC 空间按【逆时针 CCW】为正面；边缘像素按闭区间 >= 0 归属
#include "render/Rasterizer.h"
#include <cmath>
#include <algorithm>

int toScreenX(float ndcX, int width) {
    const float t = (ndcX + 1.0f) * 0.5f;
    return static_cast<int>(std::roundf(t * (width - 1)));
}
int toScreenY(float ndcY, int height) {
    const float t = (1.0f - ndcY) * 0.5f;
    return static_cast<int>(std::roundf(t * (height - 1)));
}

int rasterizeTriangle(Framebuffer& fb, const Vertex3D& a, const Vertex3D& b,
    const Vertex3D& c, const Vec3& color) {
    // ① 三点 → 像素坐标（先取整：之后叉积全是【整数】运算 ⇒ inside test 无浮点误差）
    const int ax = toScreenX(a.x, fb.width());  const int ay = toScreenY(a.y, fb.height());
    const int bx = toScreenX(b.x, fb.width());  const int by = toScreenY(b.y, fb.height());
    const int cx = toScreenX(c.x, fb.width());  const int cy = toScreenY(c.y, fb.height());

    // ② 有符号面积的 2 倍：area = (B-A) × (C-A)
    const int area = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay);
    // ③ 退化守卫（决策点 5）：面积为零立刻返回 0
    if (area == 0) return 0;
    const float inv = 1.0f / static_cast<float>(area);

    // ④ 包围盒 + clamp 到 [0,W-1] / [0,H-1]（决策点 6）
    int x0 = std::min(ax, std::min(bx, cx));
    int x1 = std::max(ax, std::max(bx, cx));
    int y0 = std::min(ay, std::min(by, cy));
    int y1 = std::max(ay, std::max(by, cy));
    x0 = std::max(x0, 0);
    x1 = std::min(x1, fb.width()-1);
    y0 = std::max(y0, 0);
    y1 = std::min(y1, fb.height()-1);

    int written = 0;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            // ⑤ edge function：e0 = 边AB × (P-A)；e1 = 边BC × (P-B)；e2 = 边CA × (P-C)
            const int e0 = (bx - ax) * (y - ay) - (x - ax) * (by - ay);
            const int e1 = (cx - bx) * (y - by) - (x - bx) * (cy - by);
            const int e2 = (ax - cx) * (y - cy) - (x - cx) * (ay - cy);

            // ⑥ 归一化 + inside test（决策点 4）
            const float w0 = inv * static_cast<float>(e0);
            const float w1 = inv * static_cast<float>(e1);
            const float w2 = inv * static_cast<float>(e2);
            const float z = w0 * c.z + w1 * a.z + w2 * b.z;

            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                if (z < fb.depthAt(x, y)) {
                    fb.setDepth(x, y, z, color);
                    ++written;
                }
            }
        }
    }

    return written;
}
