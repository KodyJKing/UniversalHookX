#pragma once

typedef unsigned int uint32_t;

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
};

extern "C" __declspec(dllexport) void setCameraMatrix(void* pointer, MatrixOrder order);

extern "C" __declspec(dllexport) void updateObject(uint32_t id, float x, float y, float z, uint32_t timeout);
