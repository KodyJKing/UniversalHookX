#include "math.hpp"
#include <math.h>
#include <cstdio>

namespace Math {

    // Vector4 Implementation
    Vector4& Vector4::operator+=(const Vector4& rhs) {
        this->x += rhs.x;
        this->y += rhs.y;
        this->z += rhs.z;
        this->w += rhs.w;
        return *this;
    }
    Vector4& Vector4::operator-=(const Vector4& rhs) {
        this->x -= rhs.x;
        this->y -= rhs.y;
        this->z -= rhs.z;
        this->w -= rhs.w;
        return *this;
    }
    Vector4 Vector4::operator+(const Vector4& rhs) {
        Vector4 result = *this;
        result += rhs;
        return result;
    }
    Vector4 Vector4::operator-(const Vector4& rhs) {
        Vector4 result = *this;
        result -= rhs;
        return result;
    }
    Vector4 Vector4::scale(float scalar) {
        this->x *= scalar;
        this->y *= scalar;
        this->z *= scalar;
        this->w *= scalar;
        return *this;
    }
    float Vector4::dot(const Vector4& rhs) {
        return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z + this->w * rhs.w;
    }
    float Vector4::length() {
        return sqrtf(this->dot(*this));
    }

    // Matrix4 Implementation
    Matrix4 Matrix4::multiply(const Matrix4& rhs) {
        Matrix4 result;
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                result.m[c][r] = 0;
                for (int i = 0; i < 4; i++) {
                    result.m[c][r] += this->m[i][r] * rhs.m[c][i];
                }
            }
        }
        return result;
    }

    Matrix4 Matrix4::transpose() {
        Matrix4 result;
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                result.m[r][c] = this->m[c][r];
        return result;
    }

    void Matrix4::print() {
        printf("Matrix4:\n");
        for (int r = 0; r < 4; r++)
            printf("%.4f %.4f %.4f %.4f\n", this->m[0][r], this->m[1][r], this->m[2][r], this->m[3][r]);
    }

}