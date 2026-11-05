// ============================================================================
//  tools/cornell.cpp —— 离线光线追踪出图（独立工具）
//
//  ★★★ 归属声明（务必保留，橱窗/简历里也要写清）★★★
//    这是【光线追踪】路线，与本仓库的光栅化渲染器
//    （Rasterizer + VertPipeline + Camera）是【两条不同的技术路线】。
//    它复用 math/Vec3 与 render::blinnPhong，但不经过任何光栅化步骤。
//
//  依赖：math/Vec3.h、render/Shading.h（复用 blinnPhong）
//  输出：output/cornell.ppm
// ============================================================================

#include <cmath>
#include <cstdio>
#include <vector>

#include <direct.h>

#include "math/Vec3.h"
#include "render/Shading.h"

namespace {

const float kPi = 3.14159265358979323846f;

// ------ 小工具（Vec3 没有 Vec3*float，这里补两个本地 helper，不改公共头）------
Vec3 Mul(const Vec3& v, float s) { return Vec3(v.x * s, v.y * s, v.z * s); }
Vec3 Add(const Vec3& a, const Vec3& b) { return Vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
Vec3 Sub(const Vec3& a, const Vec3& b) { return Vec3(a.x - b.x, a.y - b.y, a.z - b.z); }

// ------ 材质 ------
struct Surface {
    Vec3  albedo;
    float ka, ks, shininess;
    int   kind;      // 0 = 漫反射 / 高光   1 = 发光
};

struct Sphere { Vec3 c; float r; Surface mat; };
struct Plane  { Vec3 n; float d; Surface mat; };   // 平面方程：n·p = d

struct Hit {
    float   t = 1e30f;
    Vec3    p;
    Vec3    n;
    Surface mat;
    bool    hit = false;
};

// ------ 场景常量（Cornell 风格盒子，前面开口）------
const float kRoomX  = 1.0f;    // x ∈ [-1, 1]
const float kRoomY  = 2.0f;    // y ∈ [ 0, 2]
const float kRoomZ0 = -1.5f;   // 后墙
const float kRoomZ1 = 1.0f;    // 开口

const Surface kRed  { Vec3(0.72f, 0.16f, 0.16f), 0.30f, 0.06f,  8.0f, 0 };
const Surface kBlue { Vec3(0.16f, 0.20f, 0.72f), 0.30f, 0.06f,  8.0f, 0 };
const Surface kGrey { Vec3(0.72f, 0.72f, 0.72f), 0.30f, 0.04f,  8.0f, 0 };
const Surface kWhite{ Vec3(0.82f, 0.82f, 0.84f), 0.18f, 0.75f, 96.0f, 0 };
const Surface kBlack{ Vec3(0.045f,0.045f,0.050f), 0.32f, 0.05f,  8.0f, 0 };
const Surface kLamp { Vec3(1.00f, 1.00f, 1.00f), 0.00f, 0.00f,  1.0f, 1 };

const Sphere kSpheres[2] = {
    { Vec3(-0.42f, 0.46f, -0.55f), 0.46f, kWhite },
    { Vec3( 0.44f, 0.55f,  0.05f), 0.55f, kBlack }
};

// 法线一律朝【房间内部】
const Plane kPlanes[5] = {
    { Vec3( 0.0f,  1.0f, 0.0f),  0.0f,    kGrey },  // 地板 y = 0
    { Vec3( 0.0f, -1.0f, 0.0f), -kRoomY,  kGrey },  // 天花板 y = 2
    { Vec3( 0.0f,  0.0f, 1.0f),  kRoomZ0, kGrey },  // 后墙 z = -1.5
    { Vec3( 1.0f,  0.0f, 0.0f), -kRoomX,  kRed  },  // 左墙 x = -1
    { Vec3(-1.0f,  0.0f, 0.0f), -kRoomX,  kBlue }   // 右墙 x = +1
};

// 面光源：天花板上的矩形，中心 + 半宽/半深
const float kLightY  = kRoomY - 0.02f;
const float kLightHX = 0.28f;
const float kLightHZ = 0.28f;
const Vec3  kLightC(0.0f, kLightY, -0.35f);

// ------ 求交 ------
bool hitSphere(const Sphere& s, const Vec3& o, const Vec3& d, float tmin, float tmax, Hit& h) {
    const Vec3  oc = Sub(o, s.c);
    const float a  = d.dot(d);
    const float b  = 2.0f * oc.dot(d);
    const float c  = oc.dot(oc) - s.r * s.r;
    const float disc = b * b - 4.0f * a * c;
    if (disc < 0.0f) return false;
    const float sq = std::sqrt(disc);
    float t = (-b - sq) / (2.0f * a);
    if (t < tmin) t = (-b + sq) / (2.0f * a);
    if (t < tmin || t > tmax) return false;

    h.t   = t;
    h.p   = Add(o, Mul(d, t));
    h.n   = Sub(h.p, s.c).normalized();      // ★ 球面法线精确 ⇒ 天然平滑，不用插值
    h.mat = s.mat;
    h.hit = true;
    return true;
}

bool hitPlane(const Plane& pl, const Vec3& o, const Vec3& d, float tmin, float tmax, Hit& h) {
    const float denom = pl.n.dot(d);
    if (std::fabs(denom) < 1e-8f) return false;
    const float t = (pl.d - pl.n.dot(o)) / denom;
    if (t < tmin || t > tmax) return false;

    const Vec3 p = Add(o, Mul(d, t));
    // 只保留房间范围内的交点（否则墙面会无限延伸）
    if (p.x < -kRoomX || p.x > kRoomX) return false;
    if (p.y <  0.0f   || p.y > kRoomY) return false;
    if (p.z < kRoomZ0 || p.z > kRoomZ1) return false;

    h.t = t; h.p = p; h.n = pl.n; h.mat = pl.mat; h.hit = true;
    return true;
}

bool intersect(const Vec3& o, const Vec3& d, float tmax, Hit& h) {
    bool any = false;
    Hit  best;
    for (int i = 0; i < 5; ++i) {
        Hit hh;
        if (hitPlane(kPlanes[i], o, d, 1e-4f, any ? best.t : tmax, hh)) { best = hh; any = true; }
    }
    for (int i = 0; i < 2; ++i) {
        Hit hh;
        if (hitSphere(kSpheres[i], o, d, 1e-4f, any ? best.t : tmax, hh)) { best = hh; any = true; }
    }
    if (any) h = best;
    return any;
}

// 阴影射线：从 p 沿 n 偏移一点，朝光源方向打
bool occluded(const Vec3& p, const Vec3& n, const Vec3& toLight, float dist) {
    const Vec3 dir = toLight.normalized();
    const Vec3 org = Add(p, Mul(n, 1e-3f));
    Hit h;
    return intersect(org, dir, dist - 2e-3f, h);
}

// ------ 着色 ------
Vec3 shade(const Hit& h, const Vec3& eye) {
    if (h.mat.kind == 1) return Vec3(8.0f, 8.0f, 8.0f);   // 灯：超亮，最后被 gamma 压成白

    render::Material m;
    m.albedo    = h.mat.albedo;
    m.ka        = 0.0f;                    // ★ 让 blinnPhong 只算 diffuse+specular
    m.ks        = h.mat.ks;
    m.shininess = h.mat.shininess;

    const Vec3 V = Sub(eye, h.p).normalized();   // ★ V = 从着色点指向相机

    Vec3 acc(0.0f, 0.0f, 0.0f);
    const int N = 3;                              // 3×3 = 9 个面光源采样 ⇒ 软阴影
    const float invN = 1.0f / static_cast<float>(N);

    for (int iz = 0; iz < N; ++iz) {
        for (int ix = 0; ix < N; ++ix) {
            const float fx = ((static_cast<float>(ix) + 0.5f) * invN * 2.0f) - 1.0f;
            const float fz = ((static_cast<float>(iz) + 0.5f) * invN * 2.0f) - 1.0f;
            const Vec3 lp(kLightC.x + fx * kLightHX, kLightY, kLightC.z + fz * kLightHZ);

            const Vec3  toL  = Sub(lp, h.p);
            const float dist = std::sqrt(toL.dot(toL));
            if (occluded(h.p, h.n, toL, dist)) continue;      // 被挡 ⇒ 这盏灯不再贡献

            render::Light L;
            L.position  = lp;
            L.color     = Vec3(1.0f, 1.0f, 1.0f);
            L.intensity = 9.0f / static_cast<float>(N * N);   // 采样摊薄后的亮度
            acc = Add(acc, render::blinnPhong(h.p, h.n, V, L, m));
        }
    }

    // ★ ambient 单独加一次（不乘光；blinnPhong 里已将 ka 置 0）
    return Add(acc, Mul(h.mat.albedo, h.mat.ka));
}

} // namespace

int main() {
    _mkdir("output");

    const int W = 640, H = 400;
    const Vec3  eye (0.0f, 1.0f, 1.15f);
    const float fovY  = 60.0f * kPi / 180.0f;
    const float aspect = static_cast<float>(W) / static_cast<float>(H);
    const float ty    = std::tan(fovY * 0.5f);

    std::vector<unsigned char> img(static_cast<size_t>(W) * H * 3, 0);

    for (int y = 0; y < H; ++y) {
        const float v = (1.0f - 2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(H)) * ty;
        for (int x = 0; x < W; ++x) {
            const float u = (2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(W) - 1.0f) * ty * aspect;
            Vec3 dir(u, v, -1.0f);            // 相机看向 −Z，eye 与注视点同高
            dir = dir.normalized();

            Hit  h;
            Vec3 c(0.0f, 0.0f, 0.0f);
            if (intersect(eye, dir, 1e30f, h)) c = shade(h, eye);

            // ★ gamma 校正：光照在线性空间算，写 sRGB 前要 pow(1/2.2)
            float r = c.x < 0.0f ? 0.0f : c.x;
            float g = c.y < 0.0f ? 0.0f : c.y;
            float b = c.z < 0.0f ? 0.0f : c.z;
            r = std::pow(r, 1.0f / 2.2f); if (r > 1.0f) r = 1.0f;
            g = std::pow(g, 1.0f / 2.2f); if (g > 1.0f) g = 1.0f;
            b = std::pow(b, 1.0f / 2.2f); if (b > 1.0f) b = 1.0f;

            const size_t idx = (static_cast<size_t>(y) * W + x) * 3;
            img[idx + 0] = static_cast<unsigned char>(r * 255.0f + 0.5f);
            img[idx + 1] = static_cast<unsigned char>(g * 255.0f + 0.5f);
            img[idx + 2] = static_cast<unsigned char>(b * 255.0f + 0.5f);
        }
        if ((y % 50) == 0) std::printf("row %d / %d\n", y, H);
    }

    FILE* fp = nullptr;
    if (fopen_s(&fp, "output/cornell.ppm", "wb") != 0 || fp == nullptr) {
        std::printf("fopen failed\n");
        return 1;
    }
    std::fprintf(fp, "P6\n%d %d\n255\n", W, H);
    std::fwrite(img.data(), 1, img.size(), fp);
    std::fclose(fp);
    std::printf("wrote output/cornell.ppm (%dx%d)\n", W, H);
    return 0;
}