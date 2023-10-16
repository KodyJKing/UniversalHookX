#include "hook_mouse.hpp"
#include "../console/console.hpp"
#include <Windows.h>
#include <memory>
#include "../dependencies/minhook/MinHook.h"
#include "../menu/menu.hpp"

static std::add_pointer_t<int WINAPI(BOOL bShow)> oShowCursor;
int WINAPI hkShowCursor(BOOL bShow) {
    if (Menu::bShowMenu) {
        auto result = oShowCursor(true);
        return bShow ? result : -1;
	} else {
        return oShowCursor(bShow);
	}
}

static std::add_pointer_t<BOOL WINAPI(int x, int y)> oSetCursorPos;
BOOL WINAPI hkSetCursorPos(int x, int y) {
    if (Menu::bShowMenu) {
        return true;
    } else {
        return oSetCursorPos(x, y);
	}
}

namespace Mouse {

	void Hook() {

		static MH_STATUS statusShowCursor = MH_CreateHook(
			reinterpret_cast<void**>(ShowCursor), &hkShowCursor, 
			reinterpret_cast<void**>(&oShowCursor) 
		);
        static MH_STATUS statusSetCursorPos = MH_CreateHook(
            reinterpret_cast<void**>(SetCursorPos), &hkSetCursorPos,
            reinterpret_cast<void**>(&oSetCursorPos));

		MH_EnableHook(ShowCursor);
        MH_EnableHook(SetCursorPos);

		LOG("[+] Original ShowCursor: %llX\n", oShowCursor);
        LOG("[+] Original SetCursorPos: %llX\n", oSetCursorPos);

	}

	void Unhook() {

	}

}
