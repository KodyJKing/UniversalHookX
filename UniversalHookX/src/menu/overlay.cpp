#include "overlay.hpp"
#include "overlay_pub.h"
#include "../math.hpp"

#include "../dependencies/imgui/imgui.h"
#include "../dependencies/imgui/imgui_impl_win32.h"

#include <mutex>
#include <map>

using namespace Math;

union ViewValue {
    Matrix4 matrix;
};

struct View {
    ViewStorage storage = ViewStorage::value;
    ViewType type = ViewType::matrix;
    MatrixOrder order = MatrixOrder::columnMajor;

    ViewValue data;
    ViewValue* pointer = nullptr;

    Matrix4 getMatrix() {
        Matrix4 result;
        if (storage == ViewStorage::pointer)
            result = pointer->matrix;
        else
            result = data.matrix;
        if (order == MatrixOrder::rowMajor)
            result = result.transpose();
        return result;
    }
};

struct Object {
    uint32_t id;
    Vector4 position;
    uint32_t timeout;
};

///////////////////////

std::mutex apiMutex;
View view;
std::map<uint32_t, Object> objects;

extern "C" __declspec(dllexport) void __stdcall setViewMatrixPointer(void* pointer, MatrixOrder order) {
    const std::lock_guard<std::mutex> lock(apiMutex);
    
    view.storage = ViewStorage::pointer;
    view.type = ViewType::matrix;
    view.order = order;
}

extern "C" __declspec(dllexport) void __stdcall updateObject(uint32_t id, float x, float y, float z, uint32_t timeout) {
    const std::lock_guard<std::mutex> lock(apiMutex);
    
    Object object;
    object.id = id;
    object.position.x = x;
    object.position.y = y;
    object.position.z = z;
    object.position.w = 1;
    object.timeout = timeout;
    objects[id] = object;
}

///////////////////////

namespace Overlay {

    void Draw() {
        const std::lock_guard<std::mutex> lock(apiMutex);
        
        static int callCount = 0;
        if (callCount++ % 100 == 0) {
            Matrix4 matrix = view.getMatrix();
            matrix.print();
        }

        // Draw text in center of window
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - ImGui::CalcTextSize("Hello, world!").x / 2, 0));
        ImGui::Text("Hello, world!");
    }

}
