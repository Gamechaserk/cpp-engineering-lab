// 【写代码前的用例清单】
// 1) dot((1,0,0),(0,1,0)) == 0 ；cross((1,0,0),(0,1,0)) == (0,0,1)
// 2) (3,0,4).normalized() 长度 == 1，且 == (0.6, 0, 0.8)（且原向量不变）
#pragma once
#include<cmath>

struct Vec3{
	float x, y, z;
	Vec3() :x(0.0f), y(0.0f), z(0.0f) {}
	Vec3(float x, float y, float z) :x(x), y(y), z(z) {}
	Vec3& operator+=(const Vec3& o){
		x += o.x; y += o.y; z += o.z;
		return *this;
	}
	Vec3& operator-=(const Vec3& o) {
		x -= o.x; y -= o.y; z -= o.z;
		return *this;
	}
	Vec3 operator+(const Vec3& o) const {
		Vec3 r = *this;
		r += o;
		return r;
	}
	Vec3 operator-(const Vec3& o) const {
		Vec3 r = *this;
		r -= o;
		return r;
	}
	Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); } // 标量乘
	float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; } // 点积 →光照要用
	Vec3 cross(const Vec3& o) const { return Vec3(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x); } // 叉积→算法线要用
	Vec3 normalized() const { 
		const float len2 = x * x + y * y + z * z;
		if (len2 <= 0.0f) return Vec3(0.0f, 0.0f, 0.0f);
		const float len = std::sqrt(len2);
		return Vec3(x / len, y / len, z / len);
	} // 返回新向量
};

inline Vec3 operator*(float s, const Vec3& v) {
	return Vec3(v.x * s, v.y * s, v.z * s);
}