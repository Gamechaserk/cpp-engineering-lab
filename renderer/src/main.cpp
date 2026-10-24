#include<iostream>
#include <direct.h>      // _mkdir：C++14 没有 std::filesystem
#include <cstdio>
#include "render/Framebuffer.h"
#include "io/PpmWriter.h"
#include "render/Rasterizer.h"   // ← 新增这一行 include



static void drawBlock(Framebuffer& fb, int cx, int cy, const Vec3& color, int r = 2) {
    for (int y = cy - r; y <= cy + r; ++y) {
        for (int x = cx - r; x <= cx + r; ++x) {
            if (x < 0 || y < 0 || x >= fb.width() || y >= fb.height()) { continue; }  // 这里必须自己有边界检查
            fb.set(x, y, color);
        }
    }
}

int main() {
    _mkdir("output");                       // 目录已存在时会返回 -1，忽略即可
    const int W = 256, H = 256;
    const Vec3 white(1.0f, 1.0f, 0.5f), black(0.0f, 0.0f, 0.5f), red(1.0f, 0.0f, 0.5f);
    const Vertex3D A{ 0.0f, 0.9f, -0.5f }, B{ -0.9f, -0.9f, -0.5f }, C{ 0.9f, -0.9f, -0.5f };
    const Vertex3D D{ 0.0f, 0.0f, 0.0f }, E{ -0.5f, 0.5f, 0.0f }, F{ 0.5f, 0.5f, 0.0f };

    //第一项
    {
        Framebuffer fb(W, H);
        fb.clear(white);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                fb.set(x, y, Vec3(static_cast<float>(x) / (W - 1),
                    static_cast<float>(y) / (H - 1),
                    0.5f));
        const char* path = "output/gradient.ppm";
        if (!writePpm(path, fb)) { std::printf("write failed: %s\n", path); return 1; }
        std::printf("wrote %s (%dx%d)\n", path, W, H);
    }

    //第二项
    // ── 步骤 1：三点验证（临时产物，朝上还是朝下一眼可判）──
    {
        Framebuffer fb(W, H);
        for (int y = 0; y < H; ++y) { for (int x = 0; x < W; ++x) { fb.set(x, y, black); } }
        drawBlock(fb, toScreenX(A.x, W), toScreenY(A.y, H), white);
        drawBlock(fb, toScreenX(B.x, W), toScreenY(B.y, H), white);
        drawBlock(fb, toScreenX(C.x, W), toScreenY(C.y, H), white);
        if (!writePpm("output/dots.ppm", fb)) { std::printf("write failed\n"); return 1; }
        std::printf("wrote output/dots.ppm\n");
    }
    // ── 步骤 2/3：实心三角形（正式交付物）──
    {
        Framebuffer fb(W, H);
        for (int y = 0; y < H; ++y) { for (int x = 0; x < W; ++x) { fb.set(x, y, black); } }
        const int written = rasterizeTriangle(fb, A, B, C, white);
        std::printf("triangle written pixels = %d\n", written);
        if (!writePpm("output/triangle.ppm", fb)) { std::printf("write failed\n"); return 1; }
        std::printf("wrote output/triangle.ppm\n");
    }

    //第三项
    // ── 步骤 1：顺序A──
    {
        Framebuffer fb(W, H);
        fb.clear(black);
        const int written = rasterizeTriangle(fb, A, B, C, white);
        const int written2 = rasterizeTriangle(fb, D, E, F, red);

        std::printf("order_a: A=%d B=%d\n", written, written2);
        if (!writePpm("output/order_a.ppm", fb)) { std::printf("write failed\n"); return 1; }
        std::printf("wrote output/order_a.ppm\n");
    }
    // ── 步骤 2：顺序B──
    {
        Framebuffer fb(W, H);
        fb.clear(black);
        const int written = rasterizeTriangle(fb, D, E, F, red);
        const int written2 = rasterizeTriangle(fb, A, B, C, white);

        std::printf("order_b: A=%d B=%d\n", written, written2);
        if (!writePpm("output/order_b.ppm", fb)) { std::printf("write failed\n"); return 1; }
        std::printf("wrote output/order_b.ppm\n");
    }

    return 0;
}