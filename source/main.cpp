/*
 * This file is part of OSP (https://github.com/notnotme/osp).
 * Copyright (c) 2020 Romain Graillot
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdexcept>

#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <fmt/format.h>
#if defined(__SWITCH__)
#include <switch.h>
#endif

#include <ECS.h>

#include "system/AudioSystem.h"
#include "system/FileSystem.h"
#include "system/RenderSystem.h"
#include "system/UiSystem.h"
#include "tools/ConfigFile.h"
#include "config.h"

int main(int argc, char *argv[])
{
    // Initial screen configuration
    auto w = 1280;
    auto h = 720;

    // Initialize platform specifique + SDL and OpenGL
#if defined(__SWITCH__)
    // 1280x720 = screen size in portable mode
    auto switchIsDocked = false;
    if (R_FAILED(romfsInit()))
        throw std::runtime_error("romfsInit failed");

    if (R_FAILED(setInitialize()))
        throw std::runtime_error("setsysInitialize failed");

    TRACE("SWITCH started romfs and set modules.");
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0)
        throw std::runtime_error(SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    auto windowFlags = (int) SDL_WINDOW_OPENGL;
#if !defined(__SWITCH__)
    windowFlags |= (int) SDL_WINDOW_RESIZABLE;
#endif

    auto* window = SDL_CreateWindow("OSP", 0, 0, w, h, windowFlags);
    if (!window)
        throw std::runtime_error(SDL_GetError());

    TRACE("SDL_Window created.");

    auto context = SDL_GL_CreateContext(window);
    SDL_SetWindowSize(window, w, h);
    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);

    if (gladLoadGL() == 0)
        throw std::runtime_error("gladLoadGL failed");

    TRACE("OpenGL context created.");

#if defined(__SWITCH__)
    // Critical on platform with only a gamepad
    if (SDL_GameControllerOpen(0) == nullptr)
        throw std::runtime_error(SDL_GetError());
#endif

    // Try to load app config from disk
    ConfigFile configFile;
    try
    {
        configFile.load(APP_CONFIG_NAME);
        TRACE("App config loaded {:s}", APP_CONFIG_NAME);
    }
    catch(const std::exception& e)
    {
        TRACE("App config not found {:s}", APP_CONFIG_NAME);
    }

    auto config = configFile.getGroupOrCreate("main");

    // Load the language file
    LanguageFile languageFile;
    try
    {
        auto folder = fmt::format("{:s}lang", DATAPATH);
        auto lang = config.get("lang", (int) LanguageFile::Language::ENGLISH);
        if (lang < 0 || lang >= LanguageFile::Language::MAX_LANG)
            lang = 0;

        languageFile.load(folder, (LanguageFile::Language) lang);
        TRACE("Language file loaded {:s}", languageFile.getFilename());
    }
    catch(const std::exception& e)
    {
        TRACE("Language file not found {:s}", languageFile.getFilename());
    }

    // Initialize the ECS and start the differents Systems (order matter)
    auto* world = ECS::World::createWorld();
    world->registerSystem(new RenderSystem(window));
    world->registerSystem(new UiSystem(config, languageFile, window));
    world->registerSystem(new FileSystem(config, languageFile));
    world->registerSystem(new AudioSystem(config));

    TRACE("ECS created.");

    SDL_Event event;
    auto done = false;
    auto time = 0ul;
    auto frequency = SDL_GetPerformanceFrequency();
    while (!done)
    {
        // Process SDL_QUIT event but pass the rest into the ECS event system
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    done = true;
                break;
                case SDL_CONTROLLERBUTTONDOWN:
                    if (event.cbutton.which == 0)
                    {
                        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START)
                        {
                            SDL_Event quitEvent;
                            quitEvent.type = SDL_QUIT;
                            SDL_PushEvent(&quitEvent);
                        }
                    }
                default:
                    world->emit<SDL_Event>(event);
            }
        }

#if defined(__SWITCH__)
        // Check docked status to change screen size if needed
        switch (appletGetOperationMode())
        {
            default:
            case AppletOperationMode_Handheld:
                if (switchIsDocked == true)
                {
                    world->emit<ScreenSizeChangeEvent>({.size = glm::ivec2(1280, 720)});
                    switchIsDocked = false;
                }
            break;
            case AppletOperationMode_Docked:
                if (switchIsDocked == false)
                {
                    world->emit<ScreenSizeChangeEvent>({.size = glm::ivec2(1920, 1080)});
                    switchIsDocked = true;
                }
            break;
        }
#endif

        // Update systems
        auto currentTime = SDL_GetPerformanceCounter();
        auto elapsed = time > 0 ? (float)((double)(currentTime - time) / frequency) : (float)(1.0f / 60.0f);
        time = currentTime;
        world->tick(elapsed);

        SDL_GL_SwapWindow(window);
    }

    // clean up
    world->destroyWorld();
    TRACE("ECS destroyed.");

    SDL_GL_DeleteContext(context);
    TRACE("OpenGL context destroyed.");

    SDL_DestroyWindow(window);
    TRACE("SDL_Window destroyed.");

    // Save settings
    configFile.save(APP_CONFIG_NAME);
    TRACE("App config saved {:s}", APP_CONFIG_NAME);

    SDL_Quit();

#if defined(__SWITCH__)
    if (R_FAILED(romfsExit()))
        throw std::runtime_error("romfsExit failed");

    setExit();
    TRACE("SWITCH stopped romfs and set module.");
#endif

    return 0;
}
