#pragma once

typedef unsigned int uint32_t;
typedef unsigned long long uint_ptr;

extern "C" __declspec(dllexport) void __stdcall updateObject(uint_ptr id, float* position, uint32_t timeout);

extern "C" __declspec(dllexport) void __stdcall setFovPtr(float* pointer);

extern "C" __declspec(dllexport) void __stdcall setCameraPosPtr(void* pointer, unsigned int stride, int scale);

extern "C" __declspec(dllexport) void __stdcall setCameraForwardPtr(void* pointer, unsigned int stride, int scale);

extern "C" __declspec(dllexport) void __stdcall setCameraUpPtr(void* pointer, unsigned int stride, int scale);
