#include<iostream>
#include <direct.h>      // _mkdir：C++14 没有 std::filesystem
#include <cstdio>
#include "render/Framebuffer.h"
#include "io/PpmWriter.h"
#include "render/Rasterizer.h"   
#include "scene/Camera.h"
#include "render/VertPipeline.h"
#include "render/Shading.h"



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
    const Vec3 white(1.0f, 1.0f, 1.0f), black(0.0f, 0.0f, 0.0f), red(1.0f, 0.0f, 0.0f);
    const Vertex3D A{ 0.0f, 0.9f, 0.5f }, B{ -0.9f, -0.9f, 0.5f }, C{ 0.9f, -0.9f, 0.5f };
    const Vertex3D D{ 0.0f, 0.0f, -0.5f }, E{ -0.5f, 0.5f, -0.5f }, F{ 0.5f, 0.5f, -0.5f };

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

    //第四项：世界坐标立方体 —— 前置 0 的闸门图
    {
        Framebuffer fb(W, H);
        fb.clear(black);

        Camera cam;                                       // (0,0,5) 看原点，fovY 45°
        const Mat4 model = Mat4::rotateY(0.6f) * Mat4::rotateX(0.35f);   // ★ 转一下才看得出透视
        const Mat4 mvp = cam.mvpMatrix(model);

        const float s = 1.0f;
        const Vec3 v[8] = {
            {-s,-s,-s}, { s,-s,-s}, { s, s,-s}, {-s, s,-s},
            {-s,-s, s}, { s,-s, s}, { s, s, s}, {-s, s, s} };

        const int tri[12][3] = {
            {0,2,1},{0,3,2},   // z=-s
            {4,5,6},{4,6,7},   // z=+s
            {0,1,5},{0,5,4},   // y=-s
            {3,7,6},{3,6,2},   // y=+s
            {0,4,7},{0,7,3},   // x=-s
            {1,2,6},{1,6,5} }; // x=+s

        //  六个面六种颜色 —— 不然单色只能看到一个六边形轮廓，看不出"棱边不歪"
        const Vec3 faceColor[6] = {
            Vec3(1.0f, 0.2f, 0.2f), Vec3(0.2f, 1.0f, 0.2f),
            Vec3(0.2f, 0.4f, 1.0f), Vec3(1.0f, 1.0f, 0.2f),
            Vec3(1.0f, 0.2f, 1.0f), Vec3(0.2f, 1.0f, 1.0f) };

        int drawn = 0, dropped = 0;
        for (int t = 0; t < 12; ++t) {
            Vertex3D a{}, b{}, c{};
            if (!worldTriangleToNdc(mvp, v[tri[t][0]], v[tri[t][1]], v[tri[t][2]], a, b, c)) {
                ++dropped; continue;
            }
            rasterizeTriangle(fb, a, b, c, faceColor[t / 2]);
            ++drawn;
        }
        std::printf("cube: drawn=%d dropped=%d\n", drawn, dropped);
        if (!writePpm("output/cube.ppm", fb)) { std::printf("write failed\n"); return 1; }
        std::printf("wrote output/cube.ppm\n");
    }

    //第五项：光照 —— 立方体 + 点光源 + Blinn-Phong（flat shading）
    {
        Framebuffer fb(W, H);
        fb.clear(black);

        Camera cam;
        const Mat4 model = Mat4::rotateY(0.6f) * Mat4::rotateX(0.35f);
        const Mat4 mvp = cam.mvpMatrix(model);

        const float s = 1.0f;
        const Vec3 v[8] = {
            {-s,-s,-s}, { s,-s,-s}, { s, s,-s}, {-s, s,-s},
            {-s,-s, s}, { s,-s, s}, { s, s, s}, {-s, s, s} };

        const int tri[12][3] = {
            {0,2,1},{0,3,2},   // z=-s
            {4,5,6},{4,6,7},   // z=+s
            {0,1,5},{0,5,4},   // y=-s
            {3,7,6},{3,6,2},   // y=+s
            {0,4,7},{0,7,3},   // x=-s
            {1,2,6},{1,6,5} }; // x=+s

        // ★ 六个面的法线（模型空间）。顺序与 tri 的 t/2 一一对应
        const Vec3 faceNormal[6] = {
            Vec3(0.0f, 0.0f,-1.0f), Vec3(0.0f, 0.0f, 1.0f),
            Vec3(0.0f,-1.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f),
            Vec3(-1.0f,0.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f) };

        const Vec3 faceAlbedo[6] = {
            Vec3(1.0f, 0.2f, 0.2f), Vec3(0.2f, 1.0f, 0.2f),
            Vec3(0.2f, 0.4f, 1.0f), Vec3(1.0f, 1.0f, 0.2f),
            Vec3(1.0f, 0.2f, 1.0f), Vec3(0.2f, 1.0f, 1.0f) };

        render::Light light;
        light.position = Vec3(3.0f, 4.0f, 6.0f);   // 偏右上前，能同时看出高光和背光面
        light.color = Vec3(1.0f, 1.0f, 1.0f);
        light.intensity = 1.0f;

        render::Material mat;
        mat.ka = 0.15f;
        mat.ks = 0.4f;
        mat.shininess = 32.0f;

        for (int t = 0; t < 12; ++t) {
            Vertex3D a{}, b{}, c{};
            if (!worldTriangleToNdc(mvp, v[tri[t][0]], v[tri[t][1]], v[tri[t][2]], a, b, c)) continue;

            const int f = t / 2;
            const Vec3& w0 = v[tri[t][0]];
            const Vec3& w1 = v[tri[t][1]];
            const Vec3& w2 = v[tri[t][2]];
            const float inv3 = 1.0f / 3.0f;

            //  着色点取面中心，然后变换到【世界空间】
            const Vec3 center((w0.x + w1.x + w2.x) * inv3,
                (w0.y + w1.y + w2.y) * inv3,
                (w0.z + w1.z + w2.z) * inv3);
            const Vec3 Pw = model.transformPoint(center).xyz();

            //  法线也必须变换到【世界空间】（model 只含旋转 ⇒ 直接用即可）
            const Vec3 Nw = model.transformDir(faceNormal[f]).xyz().normalized();

            //  V = 从着色点指向相机
            const Vec3 Vw = (cam.position - Pw).normalized();

            mat.albedo = faceAlbedo[f];
            const Vec3 color = render::blinnPhong(Pw, Nw, Vw, light, mat);
            rasterizeTriangle(fb, a, b, c, color);
        }

        if (!writePpm("output/cube_lit.ppm", fb)) { std::printf("write failed\n"); return 1; }
        std::printf("wrote output/cube_lit.ppm\n");
    }

    return 0;
}