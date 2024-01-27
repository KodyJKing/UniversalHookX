#pragma once

typedef unsigned int uint32_t;
typedef unsigned long long uint_ptr;

enum ViewStorage {
    VSPointer = 0,
    VSValue = 1,
};

enum ViewType {
    VTMatrix = 0,
    VTVecQuat = 1,
    VTVecEuler = 2,
};

enum MatrixOrder {
    VTColumnMajor = 0,
    VTRowMajor = 1,
};

enum MatrixType {
    MTCamera = 0,
    MTView = 1,
    MTViewProjection = 2,
};

extern "C" __declspec(dllexport) void __stdcall setCameraMatrix(void* pointer, MatrixOrder order);

extern "C" __declspec(dllexport) void __stdcall updateObject(uint_ptr id, float* position, uint32_t timeout);

extern "C" __declspec(dllexport) void __stdcall setCameraForwardPtr(void* pointer, unsigned int stride, int scale);

extern "C" __declspec(dllexport) void __stdcall setCameraUpPtr(void* pointer, unsigned int stride, int scale);
