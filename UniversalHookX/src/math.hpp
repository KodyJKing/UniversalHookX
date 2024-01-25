namespace Math {

    class Vector4 {
    public:
        union {
            float v[4];
            struct {
                float x;
                float y;
                float z;
                float w;
            } c;
        };
        
        Vector4& operator+=(const Vector4& rhs);
        Vector4& operator-=(const Vector4& rhs);
        Vector4 operator+(const Vector4& rhs);
        Vector4 operator-(const Vector4& rhs);
        Vector4 operator*(const float scalar);
        Vector4 operator/(const float scalar);
        float dot(const Vector4& rhs);
        float length();
    };

    // Matrix is assumed to be in column-major order.
    class Matrix4 {
        public:
        union {
            float m[4][4];
            float mm[16];
            struct {
                Vector4 x;
                Vector4 y;
                Vector4 z;
                Vector4 w;
            } columns;
        };
        Matrix4 operator*(const Matrix4& rhs);
        Vector4 operator*(const Vector4& rhs);
        Matrix4 transpose();
        Matrix4 inverse(bool& success);
        static Matrix4 perspective(float fov, float aspect, float near, float far);
        void print();
    };

}