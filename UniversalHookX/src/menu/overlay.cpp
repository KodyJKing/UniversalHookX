#include "overlay.hpp"
#include "overlay_pub.h"
#include "../math.hpp"

#include "../console/console.hpp"
#include "./menu.hpp"

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

    float fov = 1.223f;

    float getFov() {
        return fov;
    }

    Matrix4 getMatrix(bool& success, float aspect) {
        Matrix4 result;
        success = true;

        if (storage == ViewStorage::VSPointer)
            result = pointer->matrix;
        else
            result = data.matrix;
            
        if (order == MatrixOrder::VTRowMajor)
            result = result.transpose();

        switch (matrixType) {
            case MatrixType::MTCamera:
                // Convert to view matrix
                result = result.inverse(success);

            // case MatrixType::MTView:
            //     // Convert to view-projection
            //     result = result * Matrix4::perspective(getFov(), aspect, 0.1f, 1000);

            // case MatrixType::MTViewProjection:
            //     // Convert to screen space
            //     result = result * Matrix4::scale(1, -1, 1) * Matrix4::translate(1, 1, 0) * Matrix4::scale(0.5f, 0.5f, 1);

            default:
                break;
        }
        
        return result;
    }

    Vector4 projectPoint(Matrix4& mat, Vector4& point, float fov, float width, float height) {
        Vector4 result = mat * point;

        float aspect = width / height;
        float scale = 1.0f / result.c.z / tanf(fov / 2);
        
        result.c.x *= scale;
        result.c.y *= scale * aspect;

        result.c.x = (result.c.x + 1) * width / 2;
        result.c.y = (1 - result.c.y) * height / 2;
        return result;
    }

};

struct Object {
    uint_ptr id;
    Vector4 position;
    uint32_t timeout;
    Vector4 screenPosition;
};

///////////////////////

std::mutex apiMutex;
View view;
std::map<uint_ptr, Object> objects;

void clearStaleObjects() {
    uint32_t now = GetTickCount();
    for (auto it = objects.begin(); it != objects.end();) {
        if (it->second.timeout < now)
            it = objects.erase(it);
        else
            ++it;
    }
}

extern "C" __declspec(dllexport) void __stdcall setCameraMatrix(void* pointer, MatrixOrder order) {
    const std::lock_guard<std::mutex> lock(apiMutex);
    
    view.storage = ViewStorage::VSPointer;
    view.type = ViewType::VTMatrix;
    view.order = order;
    view.matrixType = MatrixType::MTCamera;
    view.pointer = (ViewValue*)pointer;

    // LOG("Args: %p, %d\n", pointer, order);
}

extern "C" __declspec(dllexport) void __stdcall updateObject(uint_ptr id, float* position, uint32_t timeout) {
    const std::lock_guard<std::mutex> lock(apiMutex);
    
    Object object;
    object.id = id;
    object.position.c.x = position[0];
    object.position.c.y = position[1];
    object.position.c.z = position[2];
    object.position.c.w = 1;
    object.timeout = timeout + GetTickCount();
    objects[id] = object;

    // LOG("Args: %p, %f, %f, %f, %d\n", (void*)id, position[0], position[1], position[2], timeout);
}

///////////////////////

ImVec2 addImVec2(ImVec2 a, ImVec2 b) {
    return ImVec2(a.x + b.x, a.y + b.y);
}

void drawTextOutlined(ImDrawList* drawList, ImVec2 pos, ImU32 color, const char* text) {
    drawList->AddText(addImVec2(pos, ImVec2(1, 1)), IM_COL32(0, 0, 0, 255), text);
    drawList->AddText(addImVec2(pos, ImVec2(1, -1)), IM_COL32(0, 0, 0, 255), text);
    drawList->AddText(addImVec2(pos, ImVec2(-1, 1)), IM_COL32(0, 0, 0, 255), text);
    drawList->AddText(addImVec2(pos, ImVec2(-1, -1)), IM_COL32(0, 0, 0, 255), text);
    drawList->AddText(pos, color, text);
}

namespace Overlay {

    void Draw() {
        const std::lock_guard<std::mutex> lock(apiMutex);

        ImVec2 winSize = ImGui::GetWindowSize();

        if (Menu::bShowMenu) {
            float sliderWidth = 400.0f;
            ImGui::SetNextItemWidth(sliderWidth);
            ImGui::SetCursorPos(ImVec2((winSize.x - sliderWidth) / 2, 0.0f));
            ImGui::SliderFloat("Fov", &view.fov, 0.1f, 3.14159265358979323846f);
        }

        float aspect = winSize.x / winSize.y;

        bool getMatSuccess = false;
        Matrix4 mat = view.getMatrix(getMatSuccess, aspect);
        if (!getMatSuccess) {
            // Make text red
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
            ImGui::Text("Failed to get matrix");
            ImGui::PopStyleColor();
            return;
        }

        // static int callCount = 0;
        // if (callCount++ % 100 == 0)
        //     mat.print();

        clearStaleObjects();

        // Get mouse pos
        auto mousePos = ImGui::GetMousePos();
        auto targetPos = Menu::bShowMenu ? mousePos : ImVec2(winSize.x / 2, winSize.y / 2);

        // Update screen positions, recall nearest to target
        uint_ptr nearestId = 0;
        float nearestDist = 1e+10f;
        for (auto& object : objects) {
            auto pos = view.projectPoint(
                mat, object.second.position,
                view.getFov(),
                winSize.x,
                winSize.y
            );
            object.second.screenPosition = pos;

            float dx = pos.c.x - targetPos.x;
            float dy = pos.c.y - targetPos.y;
            float dist = dx * dx + dy * dy;
            
            if (dist < nearestDist) {
                nearestDist = dist;
                nearestId = object.second.id;
            }
        }

        auto drawList = ImGui::GetWindowDrawList();
        for (auto& object : objects) {
            Vector4 position = object.second.screenPosition;

            if (position.c.z < 0)
                continue;

            bool selected = object.second.id == nearestId;
            auto color = selected ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255);
            auto textColor = selected ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 255, 255, 255);
            auto radius = selected ? 7.0f : 5.0f;

            ImVec2 screenPos = ImVec2(position.c.x, position.c.y);
            drawList->AddCircleFilled(screenPos, radius, color);
            ImVec2 textPos = ImVec2(screenPos.x, screenPos.y + 10);

            char displayText[255];
            snprintf(displayText, sizeof(displayText), "%llX", (unsigned long long)object.second.id);

            if (selected) {
                drawList->AddLine(targetPos, screenPos, color, 2.0f);

                // Copy address to clipboard if P is pressed
                if (GetAsyncKeyState('P') & 0x8000)
                    ImGui::SetClipboardText(displayText);
            }

            // Center text
            ImVec2 textSize = ImGui::CalcTextSize(displayText);
            textPos.x -= textSize.x / 2;
            drawTextOutlined(drawList, textPos, textColor, displayText);
            // drawList->AddText(textPos, textColor, buff);
        }
    }

}
