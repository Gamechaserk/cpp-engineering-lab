#include "render/Shading.h"

namespace render {

Vec3 blinnPhong(const Vec3& P, const Vec3& N, const Vec3& V,
                const Light& L, const Material& M) {
    Vec3 Ldir = (L.position - P).normalized();
    float ndl = Ldir.dot(N);
    Vec3 H = (Ldir + V).normalized();
    float ndh = N.dot(H);

    Vec3 ambient = M.ka * M.albedo;
    Vec3 diffuse = std::fmax(0.0f, ndl) * M.albedo;
    float specular = std::pow(std::fmax(0.0f, ndh), M.shininess) * M.ks;

    Vec3 light = L.color * L.intensity;
    float lightr = ambient.x + light.x * (specular + diffuse.x);
    float lightg = ambient.y + light.y * (specular + diffuse.y);
    float lightb = ambient.z + light.z * (specular + diffuse.z);
    lightr = std::fmin(1.0f, std::fmax(0.0f, lightr));
    lightg = std::fmin(1.0f, std::fmax(0.0f, lightg));
    lightb = std::fmin(1.0f, std::fmax(0.0f, lightb));
    return Vec3(lightr, lightg, lightb);
}

float signedArea2D(float ax, float ay,float bx, float by,float cx, float cy) {
    float area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    return area;
}

} // namespace render