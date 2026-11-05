#include "math/Mat4.h"


namespace {

Mat4 MakeIdentity() {
    Mat4 r;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r.m[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    return r;
}

} // namespace

Mat4::Mat4() {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

Mat4 Mat4::identity() {
    return MakeIdentity();
}

Mat4 Mat4::translate(float tx, float ty, float tz) {
    Mat4 r;
    r.m[0][3] = tx, r.m[1][3] = ty, r.m[2][3] = tz;
    return r;
}

Mat4 Mat4::scale(float sx, float sy, float sz) {
    Mat4 r;
    r.m[0][0] = sx, r.m[1][1] = sy, r.m[2][2] = sz;
    return r;
}

Mat4 Mat4::rotateX(float radians) {
    Mat4 r;
    r.m[1][1] = std::cos(radians), r.m[1][2] = 0.0f - std::sin(radians);
    r.m[2][1] = std::sin(radians), r.m[2][2] = std::cos(radians);
    return r;
}
Mat4 Mat4::rotateY(float radians) {
    Mat4 r;
    r.m[0][0] = std::cos(radians), r.m[0][2] = std::sin(radians);
    r.m[2][0] = 0.0f-std::sin(radians), r.m[2][2] = std::cos(radians);
    return r;
}
Mat4 Mat4::rotateZ(float radians) {
    Mat4 r;
    r.m[0][0] = std::cos(radians), r.m[0][1] = 0.0f - std::sin(radians);
    r.m[1][0] = std::sin(radians), r.m[1][1] = std::cos(radians);
    return r;
}

Mat4 Mat4::perspective(float fovYRad, float aspect, float n, float f) {
    float t = tan(fovYRad / 2);
    Mat4 r;
    r.m[0][0] = 1.0f / (t * aspect), r.m[1][1] = 1.0f / t, r.m[2][2] = -(n + f) / (f - n);
    r.m[2][3] = -2.0f * n * f / (f - n), r.m[3][2] = -1.0f, r.m[3][3] = 0;
    return r;
}

Mat4 Mat4::lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Mat4 R, T;
    Vec3 d = (center - eye).normalized();//前方向
    Vec3 r = d.cross(up).normalized();//右方向
    Vec3 u = r.cross(d);//修正后的上方向
    R.m[0][0] = r.x, R.m[0][1] = r.y, R.m[0][2] = r.z;
    R.m[1][0] = u.x, R.m[1][1] = u.y, R.m[1][2] = u.z;
    R.m[2][0] = -d.x, R.m[2][1] = -d.y, R.m[2][2] = -d.z;
    T.m[0][3] = -eye.x, T.m[1][3] = -eye.y, T.m[2][3] = -eye.z;
    return R * T;
}

Mat4 Mat4::operator*(const Mat4& o) const {
    Mat4 r;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++)  s += this->m[i][k] * o.m[k][j];
            r.m[i][j] = s;
        }
    }
    return r;
}

Vec4 Mat4::operator*(const Vec4& v) const {
    (void)v;
    float x[4] = { v.x,v.y,v.z,v.w };
    float r[4] = { 0.0f,0.0f,0.0f,0.0f };
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            r[i] += this->m[i][j] * x[j];
        }
    }
    return Vec4(r[0],r[1],r[2],r[3]);
}

Vec4 Mat4::transformPoint(const Vec3& p) const {
    // 把 p 扩成 (x,y,z,1) 再乘 ——  不做透视除法
    Vec4 r(p.x, p.y, p.z, 1.0f);
    return (*this) * r;
}

Vec4 Mat4::transformDir(const Vec3& d) const {
    // 把 d 扩成 (x,y,z,0) 再乘 —— w=0 自动抵消平移
    Vec4 r(d.x, d.y, d.z, 0.0f);
    return (*this) * r;
}