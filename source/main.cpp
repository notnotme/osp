#include "osp.h"
#include "platform.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "IconsMaterialDesignIcons_c.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <glad/glad.h>

Osp osp;
SDL_Window *sdlWindow = nullptr;
SDL_GLContext glContext = nullptr;
auto applicationExit = false;
auto sdlWindowWidth = 1280, sdlWindowHeight = 720;
ImWchar iconRanges[] = { ICON_MIN_MDI, ICON_MAX_MDI, 0 };

bool setup() {
    // Setup port specific code
    // Needed on switch to handle romfs and setup nxlink in debug build for example
    if (!PLATFORM_setup()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "PLATFORM_setup\n");
        return false;
    }

    // init SDL subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init: %s\n", SDL_GetError());
        return false;
    }

    // create an SDL window 
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    if (sdlWindow = SDL_CreateWindow("OSP", 0, 0, sdlWindowWidth, sdlWindowHeight, SDL_WINDOW_OPENGL);
        sdlWindow == nullptr) {

        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow: %s\n", SDL_GetError());
        return false;
    }

    // Setup window icon (optional)
    if (auto icon = IMG_Load("icon.jpg"); icon != nullptr) {
        SDL_SetWindowIcon(sdlWindow, icon);
        SDL_FreeSurface(icon);
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot set window icon: %s\n", SDL_GetError());
    }

    // Init GL context, Enable vsync and set initial window size, icon
    glContext = SDL_GL_CreateContext(sdlWindow);
    SDL_GL_MakeCurrent(sdlWindow, glContext);
    SDL_GL_SetSwapInterval(1);
    SDL_SetWindowSize(sdlWindow, sdlWindowWidth, sdlWindowHeight);

    // todo: maybe move gl loading to PORT_loadGL
    if (gladLoadGL() == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize OpenGL loader!\n");
        return false;
    }

    // open CONTROLLER_PLAYER_1 
    // when railed, both joycons are mapped to joystick #0,
    // else joycons are individually mapped to joystick #0, joystick #1, ...
    // https://github.com/devkitPro/SDL/blob/switch-sdl2/src/joystick/switch/SDL_sysjoystick.c#L45
    if (SDL_GameControllerOpen(0) == 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SDL_GameControllerOpen: %s\n", SDL_GetError());
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.LogFilename = nullptr;
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    auto& style = ImGui::GetStyle();
    style.FramePadding = ImVec2(8, 8);
    style.WindowRounding = 4;
    style.TabRounding = 4;
    style.PopupRounding = 4;
    style.ChildRounding = 4;
    style.FrameRounding = 3;
    style.FrameBorderSize = 1;
    style.ScrollbarSize = 16;

    // Load Fonts
    ImFontConfig imFontConfig;
    imFontConfig.MergeMode = true;
    imFontConfig.PixelSnapH = true;
    imFontConfig.OversampleH = 1;
    imFontConfig.OversampleV = 1;

    imFontConfig.GlyphMinAdvanceX = 16.0f;
    io.Fonts->AddFontFromFileTTF(DATA_PATH "/font/AtariST8x16SystemFont.ttf", 16.0f);
    io.Fonts->AddFontFromFileTTF(DATA_PATH "/font/" FONT_ICON_FILE_NAME_MDI, 16.0f, &imFontConfig, iconRanges);

    imFontConfig.GlyphMinAdvanceX = 13.0f; // default font size
    io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromFileTTF(DATA_PATH "/font/" FONT_ICON_FILE_NAME_MDI, 13.0f, &imFontConfig, iconRanges);

    // ImGui Platform/Renderer bindings
    if (!ImGui_ImplSDL2_InitForOpenGL(sdlWindow, glContext, true)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ImGui_ImplSDL2_InitForOpenGL failed\n");
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ImGui_ImplOpenGL3_Init failed\n");
        return false;
    }

    Osp::Settings settings {
        .style = 0,
        .font = 0,
        .mouseEmulation = true,
        .touchEnabled = true,
        .dataPath = DATA_PATH
    };

    if (!osp.setup(settings)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "osp.initialize failed\n");
        return false;
    }

    return true;
}

void cleanup() {
    // exit osp
    osp.cleanup();

    // exit ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();

    // Quit SDL
    IMG_Quit();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();

    ImGui::DestroyContext(nullptr);
    sdlWindow = nullptr;
    glContext = nullptr;

    // Cleanup port specific code
    PLATFORM_cleanup();
}

int main(int argc, char *argv[]) {
    if (!setup()) {
        cleanup();
        return -1;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Run.");
    SDL_Event sdlEvent;
    while (!applicationExit) {
        
        // Poll events
        while (SDL_PollEvent(&sdlEvent)) {
            ImGui_ImplSDL2_ProcessEvent(&sdlEvent);
            switch (sdlEvent.type) {
                case SDL_QUIT:
                    applicationExit = true;
                    break;
                case SDL_WINDOWEVENT:
                    if (sdlEvent.window.windowID == SDL_GetWindowID(sdlWindow)) {
                        switch (sdlEvent.window.event) {
                            case SDL_WINDOWEVENT_CLOSE:
                                sdlEvent.type = SDL_QUIT;
                                SDL_PushEvent(&sdlEvent);
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                case SDL_CONTROLLERBUTTONDOWN:
                    // seek for joystick #0
                    if (sdlEvent.cbutton.which == 0) {
                       if (sdlEvent.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
                            // (+) button down, shortway exit
                            sdlEvent.type = SDL_QUIT;
                            SDL_PushEvent(&sdlEvent);
                        }
                    }
                    break;
            }
        }

        // This is used in the switch port to handle docked/handheld mode
        // maybe useful as well for other patform supporting SDL2
        PLATFORM_beforeRender(sdlWindow);
        
        SDL_GetWindowSize(sdlWindow, &sdlWindowWidth, &sdlWindowHeight);
        glViewport(0, 0, sdlWindowWidth, sdlWindowHeight);
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(sdlWindow);

        ImGui::NewFrame();
        osp.render();
        ImGui::Render();
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(sdlWindow);
    }

    cleanup();
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Exit.");
    return 0;
}
