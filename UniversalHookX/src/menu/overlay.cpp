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

struct VecPointer {
    void* pointer;
    unsigned int stride;
    int scale;

    bool getVector(Vector4& result) {
        float* data = (float*)pointer;

        float buffer[16];
        if (!Utils::safeReadArray((float*)pointer, buffer, 16)) {
            LOG("Failed to read vector from pointer\n");
            return false;
        }

        float* fResult = (float*)&result;
        for (int i = 0; i < 4; i++)
            fResult[i] = buffer[i * stride] * scale;

        return true;
    }
};

struct View {
    VecPointer position;
    VecPointer forward;
    VecPointer up;

    float* fovPtr = nullptr; 
    float fov = 1.223f;

    float getFov() {
        if (fovPtr != nullptr) {
            float result;
            if ( Utils::safeDeref(fovPtr, result)) {
                return result;
            } else {
                LOG("Failed to read fov from pointer\n");
            }
        }
        return fov;
    }

    Matrix4 getMatrix(bool& success, float aspect) {
        Matrix4 result{0};
        success = true;

        Vector4 forwardVec{0};
        Vector4 upVec{0};
        Vector4 positionVec{0};

        if (!forward.getVector(forwardVec) || !up.getVector(upVec) || !position.getVector(positionVec)) {
            LOG("Failed to read camera vectors\n");
            success = false;
            return result;
        }

        result = Matrix4::camera(positionVec, forwardVec, upVec);
        // Convert from camera to view matrix
        result = result.orthoInverse(success);
        
        return result;
    }

    Vector4 projectPoint(Matrix4& mat, Vector4& point, float fov, float width, float height) {
        Vector4 result = mat * point;

        float aspect = width / height;
        float scale = 1.0f / result.z / tanf(fov / 2);
        
        result.x *= scale;
        result.y *= scale * aspect;

        result.x = (result.x + 1) * width / 2;
        result.y = (1 - result.y) * height / 2;
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

extern "C" __declspec(dllexport) void __stdcall updateObject(uint_ptr id, float* position, uint32_t timeout) {
    const std::lock_guard<std::mutex> lock(apiMutex);
    
    LOG("Args: %p, %p, %d\n", (void*)id, (void*)position, timeout);
    // LOG("Args: %p, %f, %f, %f, %d\n", (void*)id, position[0], position[1], position[2], timeout);

    // Object object;
    // object.id = id;

    // Vector4* positionPtr = (Vector4*)position;
    // object.position = *positionPtr;
    // object.position.w = 1;

    // object.timeout = timeout + GetTickCount();
    // objects[id] = object;
}

extern "C" __declspec(dllexport) void __stdcall setFovPtr(float* pointer) {
    const std::lock_guard<std::mutex> lock(apiMutex);
    // LOG("Args: %p\n", (void*)pointer);
    view.fovPtr = pointer;
}

extern "C" __declspec(dllexport) void __stdcall setCameraPosPtr(void* pointer, unsigned int stride, int scale) {
    const std::lock_guard<std::mutex> lock(apiMutex);
    // LOG("Args: %p, %d, %d\n", pointer, stride, scale);
    view.position = { pointer, stride, scale };
}

extern "C" __declspec(dllexport) void __stdcall setCameraForwardPtr(void* pointer, unsigned int stride, int scale) {
    const std::lock_guard<std::mutex> lock(apiMutex);
    // LOG("Args: %p, %d, %d\n", pointer, stride, scale);
    view.forward = { pointer, stride, scale };
}

extern "C" __declspec(dllexport) void __stdcall setCameraUpPtr(void* pointer, unsigned int stride, int scale) {
    const std::lock_guard<std::mutex> lock(apiMutex);
    // LOG("Args: %p, %d, %d\n", pointer, stride, scale);
    view.up = { pointer, stride, scale };
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
            if (view.fovPtr == nullptr) {
                float sliderWidth = 400.0f;
                ImGui::SetNextItemWidth(sliderWidth);
                ImGui::SetCursorPos(ImVec2((winSize.x - sliderWidth) / 2, 0.0f));
                ImGui::SliderFloat("Fov", &view.fov, 0.1f, 3.14159265358979323846f);
            }

            ImGui::SetCursorPos(ImVec2(10.0f, 20.0f));
            ImGui::Checkbox("Always show IDs", &uiSettings.alwaysShowIds);
            ImGui::Checkbox("Hide unselected", &uiSettings.hideUnselected);
        }

        Vector4 vec{0};
        char buf[255];
        if (view.forward.getVector(vec)) {
            snprintf(buf, sizeof(buf), "Forward: %f, %f, %f", vec.x, vec.y, vec.z);
            ImGui::Text(buf);
        }
        if (view.up.getVector(vec)) {
            snprintf(buf, sizeof(buf), "Up: %f, %f, %f", vec.x, vec.y, vec.z);
            ImGui::Text(buf);
        }
        if (view.position.getVector(vec)) {
            snprintf(buf, sizeof(buf), "Position: %f, %f, %f", vec.x, vec.y, vec.z);
            ImGui::Text(buf);
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

            float dx = pos.x - targetPos.x;
            float dy = pos.y - targetPos.y;
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

            if (position.z < 0)
                continue;

            auto color = selected ? COL_HIGHLIGHT : COL_NEUTRAL;
            auto textColor = selected ? COL_HIGHLIGHT : COL_WHITE;
            auto radius = selected ? 7.0f : 5.0f;

            ImVec2 screenPos = ImVec2(position.x, position.y);
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
