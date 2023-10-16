#include "../../backend.hpp"
#include "./hook_dinput8.hpp"

#ifdef ENABLE_BACKEND_DI8

#include "../../console/console.hpp"
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

IDirectInput8* createDirectInput8( ) {
	HINSTANCE hinst = GetModuleHandle(NULL);
    IDirectInput8* pDirectInput;
    auto hr = DirectInput8Create(
			hinst, 
			DIRECTINPUT_VERSION,
			IID_IDirectInput8,
			(void**)&pDirectInput,
			NULL
	);
    if (FAILED(hr))
        return NULL;
	return pDirectInput;
}

IDirectInputDevice8* createMouseDevice(IDirectInput8* dinput) {
	IDirectInputDevice8* pMouseDevice;
    auto hr = dinput->CreateDevice(GUID_SysMouse, &pMouseDevice, NULL);
    if (FAILED(hr))
        return NULL;
    return pMouseDevice;
}

namespace Dump {
	void __IDirectInput8(IDirectInput8* dinput) {
		// offset 0x18
		printf("CreateDevice");
		dinput->CreateDevice(GUID_SysMouse, NULL, NULL);
	}
	void __IDirectInputDevice8(IDirectInputDevice8* device) {
		// offset 0x48
		printf("GetDeviceState");
		device->GetDeviceState(0, NULL);
	}
	void dump() {
        LOG("__IDirectInput8: %llX\n", reinterpret_cast<unsigned long long>(Dump::__IDirectInput8));
        LOG("__IDirectInputDevice8: %llX\n", reinterpret_cast<unsigned long long>(Dump::__IDirectInputDevice8));
	}
}

// Notes:
// https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee417896(v=vs.85)
// https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416610(v=vs.85)
// Look out for devices with dwDevType == DI8DEVTYPE_MOUSE

namespace DI8 {

	void Hook() {

		auto dinput = createDirectInput8( );
        if (dinput) {
            LOG("[+] Created DirectInput8.\n");

            auto mouse = createMouseDevice(dinput);
            if (mouse) {
				LOG("[+] Created mouse device.\n");

				// Hook GetDeviceState

				mouse->Release( );
            }

			dinput->Release( );
        }

		Dump::dump( );
	}

	void Unhook() {

	}

}

#else
namespace DI8 {
    void Hook( ) {
    }
    void Unhook( ) {
    }
}
#endif
