namespace Math {

    class Vector4 {
    public:
        float x;
        float y;
        float z;
        float w;
        Vector4& operator+=(const Vector4& rhs);
        Vector4& operator-=(const Vector4& rhs);
        Vector4 operator+(const Vector4& rhs);
        Vector4 operator-(const Vector4& rhs);
        Vector4 scale(float scalar);
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
        Matrix4 multiply(const Matrix4& rhs);
        Matrix4 transpose();
        void print();
    };

}