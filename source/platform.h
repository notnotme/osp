#pragma once

#if defined(__SWITCH__)
#include "platform/switch/config.h"
#else
#include "platform/sdl/config.h"
#endif
#include <SDL2/SDL.h>

// This file serve specific purpose for platform:
// - PC/SDL: Dummy implementation, this target is for debug purpose
// - Switch: Init/Close ROMFS, handle docked/hanheld mode
// - ?
bool PLATFORM_setup();
void PLATFORM_cleanup();
void PLATFORM_beforeRender(SDL_Window* window);
