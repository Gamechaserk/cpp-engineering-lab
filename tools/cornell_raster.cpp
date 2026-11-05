// ============================================================================
//  tools/cornell_raster.cpp —— 用【现有光栅化管线】画 Cornell 盒
//
//  只用已经完成的东西：Camera + MvpMatrix + VertPipeline + Rasterizer + blinnPhong
//  ★ 不引入光线追踪、不引入新算法
//
//  能做到的 / 做不到的（出图时照实说）：
//    ✅ 透视投影、Z-Buffer、flat Blinn-Phong、两次三角面片都正确
//    ✅ 墙是平面 ⇒ flat 着色【完全正确】，墙本来就该是纯色（除了没有距离衰减）
//    ✅ 球用经纬细分 ⇒ 近看有棱，细分后再看接近球
//    ❌ 没有阴影（需要阴影贴图或阴影射线）
//    ❌ 没有面光源的柔和高光（只有一个点光源）
//    ❌ 没有抗锯齿
//    ❌ 没有间接光（红墙不会把红反射到球上）
// ============================================================================

#include <cmath>
#include <cstdio>

#include <direct.h>

#include "io/PpmWriter.h"
#include "math/Mat4.h"
#include "math/Vec3.h"
#include "render/Framebuffer.h"
#include "render/Rasterizer.h"
#include "render/Shading.h"
#include "render/VertPipeline.h"
#include "scene/Camera.h"

int main() {
    _mkdir("output");

    const int W = 640, H = 400;
    const float PI = 3.14159265358979323846f;

    Framebuffer fb(W, H);
    fb.clear(Vec3(0.0f, 0.0f, 0.0f));

    // ---- 相机：站在盒子开口处，稍微往里看 ----
    Camera cam;
    cam.position = Vec3(0.0f, 1.15f, 1.95f);
    cam.target   = Vec3(0.0f, 0.95f, -0.60f);
    cam.up       = Vec3(0.0f, 1.0f, 0.0f);
    cam.fovYRad  = 55.0f * PI / 180.0f;
    cam.aspect   = static_cast<float>(W) / static_cast<float>(H);
    cam.nearZ    = 0.05f;
    cam.farZ     = 60.0f;

    const Mat4 mvp = cam.mvpMatrix(Mat4::identity());   // 顶点已是世界坐标
    const Vec3 camPos = cam.position;

    // ---- 唯一的点光源，挂在天花板中间 ----
    render::Light light;
    light.position  = Vec3(0.0f, 1.72f, -0.35f);
    light.color     = Vec3(1.0f, 1.0f, 1.0f);
    light.intensity = 1.0f;   // 配合上面的 1/dist² 衰减

    // ---- 统一的 flat 着色绘制：法线由调用方显式给出（朝外/朝室内）----
    auto shadeAndDraw = [&](const Vec3& p0, const Vec3& p1, const Vec3& p2,
                            const Vec3& nIn,
                            const Vec3& albedo, float ka, float ks, float shin) {
        const Vec3 cx((p0.x + p1.x + p2.x) / 3.0f,
                      (p0.y + p1.y + p2.y) / 3.0f,
                      (p0.z + p1.z + p2.z) / 3.0f);
        const Vec3 n = nIn.normalized();
        const Vec3 V = (camPos - cx).normalized();

        // ★ 距离衰减：blinnPhong 本身不含 1/r²（经验模型的已知缺陷），
        //   在调用前手动乘上去，否则远近物体一样亮、整张图过曝
        const Vec3  toL(light.position.x - cx.x, light.position.y - cx.y, light.position.z - cx.z);
        const float dist2 = toL.dot(toL);
        render::Light lc = light;
        lc.intensity = light.intensity * (1.5f / (dist2 > 0.01f ? dist2 : 0.01f));

        render::Material m;
        m.albedo = albedo; m.ka = ka; m.ks = ks; m.shininess = shin;
        const Vec3 color = render::blinnPhong(cx, n, V, lc, m);

        Vertex3D a{}, b{}, c{};
        if (worldTriangleToNdc(mvp, p0, p1, p2, a, b, c)) {
            rasterizeTriangle(fb, a, b, c, color);
        }
    };

    const Vec3 grey(0.72f, 0.72f, 0.72f);
    const Vec3 red (0.62f, 0.10f, 0.10f);
    const Vec3 blue(0.10f, 0.14f, 0.62f);

    const float RX = 1.0f, RY = 2.0f, Z0 = -1.5f, Z1 = 1.0f;

    // 每个面的法线都朝【房间内部】—— 显式给出，不靠绕向推
    const Vec3 nUp   ( 0.0f,  1.0f,  0.0f);   // 地板
    const Vec3 nDown ( 0.0f, -1.0f,  0.0f);   // 天花板
    const Vec3 nFront( 0.0f,  0.0f,  1.0f);   // 后墙（朝 +z）
    const Vec3 nRight( 1.0f,  0.0f,  0.0f);   // 左墙（朝 +x）
    const Vec3 nLeft (-1.0f,  0.0f,  0.0f);   // 右墙（朝 -x）

    // ---- 5 个面（前面开口，不封）----
    // 地板 y = 0
    shadeAndDraw(Vec3(-RX, 0, Z1), Vec3(RX, 0, Z1), Vec3(RX, 0, Z0), nUp,    grey, 0.26f, 0.02f, 8.0f);
    shadeAndDraw(Vec3(-RX, 0, Z1), Vec3(RX, 0, Z0), Vec3(-RX, 0, Z0), nUp,   grey, 0.26f, 0.02f, 8.0f);
    // 天花板 y = RY
    shadeAndDraw(Vec3(-RX, RY, Z1), Vec3(RX, RY, Z0), Vec3(RX, RY, Z1), nDown, grey, 0.26f, 0.02f, 8.0f);
    shadeAndDraw(Vec3(-RX, RY, Z1), Vec3(-RX, RY, Z0), Vec3(RX, RY, Z0), nDown, grey, 0.26f, 0.02f, 8.0f);
    // 后墙 z = Z0
    shadeAndDraw(Vec3(-RX, 0, Z0), Vec3(RX, 0, Z0), Vec3(RX, RY, Z0), nFront,  grey, 0.26f, 0.02f, 8.0f);
    shadeAndDraw(Vec3(-RX, 0, Z0), Vec3(RX, RY, Z0), Vec3(-RX, RY, Z0), nFront, grey, 0.26f, 0.02f, 8.0f);
    // 左墙 x = -RX（红）
    shadeAndDraw(Vec3(-RX, 0, Z1), Vec3(-RX, 0, Z0), Vec3(-RX, RY, Z0), nRight, red, 0.22f, 0.02f, 8.0f);
    shadeAndDraw(Vec3(-RX, 0, Z1), Vec3(-RX, RY, Z0), Vec3(-RX, RY, Z1), nRight, red, 0.22f, 0.02f, 8.0f);
    // 右墙 x = +RX（蓝）
    shadeAndDraw(Vec3(RX, 0, Z1), Vec3(RX, RY, Z1), Vec3(RX, RY, Z0), nLeft, blue, 0.22f, 0.02f, 8.0f);
    shadeAndDraw(Vec3(RX, 0, Z1), Vec3(RX, RY, Z0), Vec3(RX, 0, Z0), nLeft, blue, 0.22f, 0.02f, 8.0f);

    // ---- 两个球（经纬细分；flat 着色 ⇒ 细分越细越像球）----
    auto drawSphere = [&](const Vec3& c, float r, const Vec3& albedo,
                          float ka, float ks, float shin) {
        const int SLI = 48, STA = 24;
        for (int i = 0; i < STA; ++i) {
            const float ph0 = PI * static_cast<float>(i)     / static_cast<float>(STA);
            const float ph1 = PI * static_cast<float>(i + 1) / static_cast<float>(STA);
            for (int j = 0; j < SLI; ++j) {
                const float th0 = 2.0f * PI * static_cast<float>(j)     / static_cast<float>(SLI);
                const float th1 = 2.0f * PI * static_cast<float>(j + 1) / static_cast<float>(SLI);
                auto pt = [&](float ph, float th) {
                    return Vec3(c.x + r * std::sin(ph) * std::cos(th),
                                c.y + r * std::cos(ph),
                                c.z + r * std::sin(ph) * std::sin(th));
                };
                const Vec3 A = pt(ph0, th0), B = pt(ph0, th1);
                const Vec3 C = pt(ph1, th0), D = pt(ph1, th1);
                if (i != 0) {
                    const Vec3 n((A.x + B.x + C.x) / 3.0f - c.x,
                                 (A.y + B.y + C.y) / 3.0f - c.y,
                                 (A.z + B.z + C.z) / 3.0f - c.z);
                    shadeAndDraw(A, B, C, n, albedo, ka, ks, shin);
                }
                if (i != STA - 1) {
                    const Vec3 n((B.x + D.x + C.x) / 3.0f - c.x,
                                 (B.y + D.y + C.y) / 3.0f - c.y,
                                 (B.z + D.z + C.z) / 3.0f - c.z);
                    shadeAndDraw(B, D, C, n, albedo, ka, ks, shin);
                }
            }
        }
    };

    drawSphere(Vec3(-0.40f, 0.42f, -0.50f), 0.42f,
               Vec3(0.85f, 0.85f, 0.87f), 0.16f, 0.80f, 128.0f);   // 白 / 光滑
    drawSphere(Vec3( 0.42f, 0.50f, -0.25f), 0.50f,
               Vec3(0.06f, 0.06f, 0.07f), 0.28f, 0.12f,  16.0f);   // 黑 / 哑光

    if (!writePpm("output/cornell_raster.ppm", fb)) {
        std::printf("write failed\n");
        return 1;
    }
    std::printf("wrote output/cornell_raster.ppm (%dx%d)\n", W, H);
    return 0;
}