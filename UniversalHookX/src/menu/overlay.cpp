#include "overlay.hpp"
#include "overlay_pub.h"
#include "../math.hpp"

#include "../console/console.hpp"
#include "./menu.hpp"

#include "../dependencies/imgui/imgui.h"
#include "../dependencies/imgui/imgui_impl_win32.h"

#include "../utils/utils.hpp"

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
        Matrix4 result{0};
        success = true;

        if (storage == ViewStorage::VSPointer) {
            //  result = pointer->matrix;
             success = Utils::safeDeref(&pointer->matrix, result);
             if (!success) {
                 LOG("Failed to read matrix from pointer\n");
                 return result;
             }
        } else {
            result = data.matrix;
        }
            
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

struct UISettings {
    bool alwaysShowIds = false;
    bool hideUnselected = false;
};

///////////////////////

std::mutex apiMutex;
View view;
std::map<uint_ptr, Object> objects;
UISettings uiSettings;

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

    #define COL_RED IM_COL32(255, 0, 0, 255)
    #define COL_GREEN IM_COL32(0, 255, 0, 255)
    #define COL_BLUE IM_COL32(0, 0, 255, 255)
    #define COL_WHITE IM_COL32(255, 255, 255, 255)
    #define COL_BLACK IM_COL32(0, 0, 0, 255)
    #define COL_TRANSPARENT IM_COL32(0, 0, 0, 0)
    #define COL_YELLOW IM_COL32(255, 255, 0, 255)
    //
    #define COL_NEUTRAL IM_COL32(0, 255, 0, 128)
    #define COL_HIGHLIGHT COL_GREEN

    void Draw() {
        const std::lock_guard<std::mutex> lock(apiMutex);

        ImVec2 winSize = ImGui::GetWindowSize();
        auto drawList = ImGui::GetWindowDrawList();

        if (Menu::bShowMenu) {
            // Transparent background
            drawList->AddRectFilled(ImVec2(0, 0), winSize, IM_COL32(0, 0, 0, 64));

            // Fov slider
            float sliderWidth = 400.0f;
            ImGui::SetNextItemWidth(sliderWidth);
            ImGui::SetCursorPos(ImVec2((winSize.x - sliderWidth) / 2, 0.0f));
            ImGui::SliderFloat("Fov", &view.fov, 0.1f, 3.14159265358979323846f);

            ImGui::SetCursorPos(ImVec2(10.0f, 20.0f));
            ImGui::Checkbox("Always show IDs", &uiSettings.alwaysShowIds);
            ImGui::Checkbox("Hide unselected", &uiSettings.hideUnselected);
        }

        float aspect = winSize.x / winSize.y;

        bool getMatSuccess = false;
        Matrix4 mat = view.getMatrix(getMatSuccess, aspect);
        if (!getMatSuccess) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
            ImGui::Text("Failed to get matrix");
            ImGui::PopStyleColor();
            return;
        }

        // static int callCount = 0;
        // if (callCount++ % 100 == 0)
        //     mat.print();

        clearStaleObjects();

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

        drawList->AddCircleFilled(targetPos, 3.0f, COL_HIGHLIGHT);

        for (auto& object : objects) {
            bool selected = object.second.id == nearestId;
            if (uiSettings.hideUnselected && !selected)
                continue;

            Vector4 position = object.second.screenPosition;

            if (position.c.z < 0)
                continue;

            auto color = selected ? COL_HIGHLIGHT : COL_NEUTRAL;
            auto textColor = selected ? COL_HIGHLIGHT : COL_WHITE;
            auto radius = selected ? 7.0f : 5.0f;

            ImVec2 screenPos = ImVec2(position.c.x, position.c.y);
            drawList->AddCircleFilled(screenPos, radius, color);

            char displayText[255];
            bool textVisible = selected || uiSettings.alwaysShowIds;
            if (textVisible)
                snprintf(displayText, sizeof(displayText), "%llX", (unsigned long long)object.second.id);

            if (selected) {
                drawList->AddLine(targetPos, screenPos, color, 2.0f);

                // Copy address to clipboard if P is pressed
                if (GetAsyncKeyState('P') & 0x8000)
                    ImGui::SetClipboardText(displayText);
            }

            if (textVisible) {
                drawTextOutlined(
                    drawList, 
                    ImVec2(
                        screenPos.x - ImGui::CalcTextSize(displayText).x / 2,
                        screenPos.y + 10
                    ),
                    textColor, displayText
                );
            }

        }
    }

}
