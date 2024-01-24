#include "overlay.hpp"
#include "overlay_pub.h"
#include "../math.hpp"

#include "../console/console.hpp"

#include "../dependencies/imgui/imgui.h"
#include "../dependencies/imgui/imgui_impl_win32.h"

#include <Windows.h>

#include <mutex>
#include <map>

using namespace Math;

union ViewValue {
    Matrix4 matrix;
};

struct View {
    ViewStorage storage = ViewStorage::VSValue;
    ViewType type = ViewType::VTMatrix;
    MatrixOrder order = MatrixOrder::VTColumnMajor;
    MatrixType matrixType = MatrixType::MTCamera;

    ViewValue data;
    ViewValue* pointer = nullptr;

    Matrix4 getMatrix(bool& success) {
        Matrix4 result;
        success = true;

        if (storage == ViewStorage::VSPointer)
            result = pointer->matrix;
        else
            result = data.matrix;
            
        if (order == MatrixOrder::VTRowMajor)
            result = result.transpose();

        if (matrixType == MatrixType::MTCamera)
            result = result.inverse(success);
        
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

extern "C" __declspec(dllexport) void setCameraMatrix(void* pointer, MatrixOrder order) {
    const std::lock_guard<std::mutex> lock(apiMutex);
    
    view.storage = ViewStorage::VSPointer;
    view.type = ViewType::VTMatrix;
    view.order = order;
    view.matrixType = MatrixType::MTCamera;
    view.pointer = (ViewValue*)pointer;

    // LOG("Args: %p, %d\n", pointer, order);
}

extern "C" __declspec(dllexport) void updateObject(uint32_t id, float x, float y, float z, uint32_t timeout) {
    const std::lock_guard<std::mutex> lock(apiMutex);
    
    Object object;
    object.id = id;
    object.position.c.x = x;
    object.position.c.y = y;
    object.position.c.z = z;
    object.position.c.w = 1;
    object.timeout = timeout + GetTickCount();
    objects[id] = object;
}

///////////////////////

namespace Overlay {

    void clearStaleObjects() {
        uint32_t now = GetTickCount();
        for (auto it = objects.begin(); it != objects.end();) {
            if (it->second.timeout < now)
                it = objects.erase(it);
            else
                ++it;
        }
    }

    void Draw() {
        const std::lock_guard<std::mutex> lock(apiMutex);
        
        // Draw text in center of window
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - ImGui::CalcTextSize("Hello, world!").x / 2, 0));
        ImGui::Text("Hello, world!");

        bool getMatSuccess = false;
        Matrix4 viewMat = view.getMatrix(getMatSuccess);
        if (!getMatSuccess) {
            // Make text red
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
            ImGui::Text("Failed to get matrix");
            ImGui::PopStyleColor();
            return;
        }

        static int callCount = 0;
        if (callCount++ % 100 == 0)
            viewMat.print();

        // clearStaleObjects();

        // auto drawList = ImGui::GetWindowDrawList();
        // for (auto& object : objects) {
        //     Vector4 position = object.second.position;
        //     position = viewMat * position;
        //     position = position * 1 / position.c.w;

        //     ImVec2 screenPos = ImVec2(position.c.x, position.c.y);
        //     drawList->AddCircleFilled(screenPos, 10, IM_COL32(255, 0, 0, 255));
        // }
    }

}
