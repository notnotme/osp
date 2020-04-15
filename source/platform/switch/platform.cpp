#include "../../platform.h"
#include <switch.h>

#if defined(ENABLE_NXLINK)
#include <unistd.h>
static int s_nxlinkSock = -1;
extern "C" void userAppInit() {
    if (R_FAILED(socketInitializeDefault())) {
        return;
    }

    s_nxlinkSock = nxlinkStdio();
    if (s_nxlinkSock >= 0) {
        printf("printf output now goes to nxlink server\n");
    } else {
        socketExit();
    }
}

extern "C" void userAppExit() {
    if (s_nxlinkSock >= 0) {
        close(s_nxlinkSock);
        socketExit();
        s_nxlinkSock = -1;
    }
}
#endif

bool PLATFORM_setup() {
    // Init romfs to get access to internal files
    Result result = romfsInit();
    if (R_FAILED(result)) {
        SDL_Log("romfsInit failed\n");
        return false;
    }
    return true;
}

void PLATFORM_cleanup() {
    // Free romfs
    Result result = romfsExit();
    if (R_FAILED(result)) {
        SDL_Log("romfsExit failed\n");
    }
}

auto switchIsDocked = false;
void PLATFORM_beforeRender(SDL_Window* window) {
    // Check if we changed applet mode (docked/handheld)
    switch (appletGetOperationMode()) {
        default:
        case AppletOperationMode_Handheld:
            if (switchIsDocked == true) {
                SDL_SetWindowSize(window, 1280, 720);
                switchIsDocked = false;
            }
            break;
        case AppletOperationMode_Docked:
            if (switchIsDocked == false) {
                SDL_SetWindowSize(window, 1920, 1080);
                switchIsDocked = true;
            }
            break;
    }
}