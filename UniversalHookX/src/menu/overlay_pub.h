#pragma once

typedef unsigned int uint32_t;

enum ViewStorage {
    pointer = 0,
    value = 1,
};

enum ViewType {
    matrix = 0,
    vecQuat = 1,
    vecEuler = 2,
};

enum MatrixOrder {
    rowMajor = 0,
    columnMajor = 1,
};

extern "C" __declspec(dllexport) void __stdcall setViewMatrixPointer(void* pointer, MatrixOrder order);

extern "C" __declspec(dllexport) void __stdcall updateObject(uint32_t id, float x, float y, float z, uint32_t timeout);
