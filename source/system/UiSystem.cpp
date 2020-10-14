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
#include "UiSystem.h"

#include <stdexcept>
#include <filesystem>
#include <fmt/format.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>

#include "../event/file/FileSystemLoadTaskEvent.h"
#include "../event/file/FileSystemCancelTaskEvent.h"
#include "../event/audio/AudioSystemPlayTaskEvent.h"
#include "../config.h"

UiSystem::UiSystem(Config config, LanguageFile languageFile, SDL_Window* window) :
ECS::EntitySystem(),
mWindow(window),
mConfig(config),
mLanguageFile(languageFile),
mAudioSystemStatus(STOPPED),
mShowWorkSpace(true),
mShowDemoWindow(false),
mShowMetricsWindow(false),
mShowSettingsWindow(false),
mShowAboutWindow(false),
mFileSystemLoading(false),
mNotificationDisplayTimeMs(5000)
{
}

UiSystem::~UiSystem()
{
    mWindow = nullptr;
}

void UiSystem::configure(ECS::World* world)
{
    TRACE(">>>");

    // Set up ImGui
    ImGui::CreateContext();

    auto mouseEmulation = mConfig.get("mouse_emulation", true);
    auto glContext = SDL_GL_GetCurrentContext();
    if (!ImGui_ImplSDL2_InitForOpenGL(mWindow, glContext, mouseEmulation))
        throw std::runtime_error("ImGui_ImplSDL2_InitForOpenGL failed");

    if (!ImGui_ImplOpenGL3_Init("#version 330 core"))
        throw std::runtime_error("ImGui_ImplOpenGL3_Init failed");

    // ImGui configuration & style
    auto& io = ImGui::GetIO();
    io.LogFilename = nullptr;
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.FontGlobalScale = 1.0f;

#if defined(__SWITCH__)
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.MouseDrawCursor = true;

    auto touchEnabled = mConfig.get("touch_enabled", false);
    if (touchEnabled)
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
#endif

    auto& style = ImGui::GetStyle();
    switch (mConfig.get("style", 0))
    {
        default:
        case 0: ImGui::StyleColorsDark();       break;
        case 1: ImGui::StyleColorsLight();      break;
        case 2: ImGui::StyleColorsClassic();    break;
    }

    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.WindowRounding = 2;
    style.TabRounding = 2;
    style.PopupRounding = 2;
    style.ChildRounding = 2;
    style.FrameRounding = 2;
    style.FrameBorderSize = 1;
    style.ScrollbarRounding = 2;
    style.ScrollbarSize = 20;
    style.MouseCursorScale = 1;

    // Use AtariST8x16SystemFont font merged with material icon
    io.Fonts->AddFontFromFileTTF(DATAPATH "font/AtariST8x16SystemFont.ttf", 16.0f);
    ImWchar iconRanges[] = { 0x0030, 0xFFCB, 0 };
    ImFontConfig imFontConfig;
    imFontConfig.MergeMode = true;
    imFontConfig.GlyphMinAdvanceX = 16.0f;
    imFontConfig.GlyphMaxAdvanceX = 16.0f;
    io.Fonts->AddFontFromFileTTF(DATAPATH "font/materialdesignicons-webfont.ttf", 16.0f, &imFontConfig, iconRanges);
    io.Fonts->Build();

    // Load and create a texture containing a bunch of sprites related to the UI
    mIconAtlas.setup(DATAPATH "atlas/uiatlas.json");

    // Subscribe for events
    world->subscribe<SDL_Event>(this);
    world->subscribe<NotificationMessageEvent>(this);
    world->subscribe<DirectoryLoadedEvent>(this);
    world->subscribe<FileSystemBusyEvent>(this);
    world->subscribe<AudioSystemConfiguredEvent>(this);
    world->subscribe<AudioSystemPlayEvent>(this);

    mStatusMessage = mLanguageFile.get("status.ready");
}

void UiSystem::unconfigure(ECS::World* world)
{
    TRACE(">>>");

    // Unsubscribe for events
    world->unsubscribe<SDL_Event>(this);
    world->unsubscribe<NotificationMessageEvent>(this);
    world->unsubscribe<DirectoryLoadedEvent>(this);
    world->unsubscribe<FileSystemBusyEvent>(this);
    world->unsubscribe<AudioSystemConfiguredEvent>(this);
    world->unsubscribe<AudioSystemPlayEvent>(this);

    // Release the texture atlas resources
    mIconAtlas.cleanup();

    // Quit ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(nullptr);
}

void UiSystem::tick(ECS::World* world, float deltaTime)
{
    // Render all the UI at once
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(mWindow);
    ImGui::NewFrame();

    auto& io = ImGui::GetIO();
    auto& style = ImGui::GetStyle();

    // ----------------------------------------------------------
    // ----------------------------------------------------------
    // Main menu
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu(mLanguageFile.getc("application")))
    {
        ImGui::MenuItem(mLanguageFile.getc("ic_show_workspace"), nullptr, &mShowWorkSpace);
        ImGui::MenuItem(mLanguageFile.getc("ic_show_settings"), nullptr, &mShowSettingsWindow);
        ImGui::Separator();
        if (ImGui::MenuItem(mLanguageFile.getc("ic_quit")))
        {
            auto quitEvent = (SDL_Event) {.type = SDL_QUIT};
            SDL_PushEvent(&quitEvent);
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(mLanguageFile.getc("help")))
    {
        ImGui::MenuItem(mLanguageFile.getc("ic_about"), nullptr, &mShowAboutWindow);
        ImGui::EndMenu();
    }

#ifndef NDEBUG
    // Menu only shown if DEBUG build is active
    if (ImGui::BeginMenu("Debug"))
    {
        ImGui::MenuItem("\ufd4a ImGui Demo", nullptr, &mShowDemoWindow);
        ImGui::MenuItem("\ufd4a ImGui Metrics", nullptr, &mShowMetricsWindow);
        ImGui::Separator();
        if (ImGui::MenuItem("\ufd4a ERROR notification"))
            world->emit<NotificationMessageEvent>
            ({
                .type = NotificationMessageEvent::ERROR,
                .message = "This is an error notification"
            });
        if (ImGui::MenuItem("\ufd4a INFO notification"))
            world->emit<NotificationMessageEvent>
            ({
                .type = NotificationMessageEvent::INFO,
                .message = "This is an info notification"
            });
        ImGui::EndMenu();
    }
#endif

    // Status message
    ImGui::Separator();
    ImGui::TextColored(style.Colors[ImGuiCol_NavHighlight], "%s", mStatusMessage.c_str());

    // FPS
    auto fps = fmt::format("{:.1f} FPS", io.Framerate).c_str();
    ImGui::SameLine((ImGui::GetWindowContentRegionWidth() - ImGui::CalcTextSize(fps).x) - style.FramePadding.x);
    ImGui::Separator();
    ImGui::TextColored(style.Colors[ImGuiCol_PlotHistogramHovered], "%s", fps);

    ImGui::EndMainMenuBar();
    ImGui::PopStyleVar();

    // ----------------------------------------------------------
    // ----------------------------------------------------------
    // Main Window (fullscreen)
    if (mShowWorkSpace)
    {
        // Setup the window to take full space on screen
        auto windowPos = ImVec2(io.DisplaySize.x/2, io.DisplaySize.y);
        auto windowPivot = ImVec2(0.5f, 1.0f);
        auto windowSize = ImVec2(io.DisplaySize.x, io.DisplaySize.y - (ImGui::GetCursorPosY() + 1));
        auto windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove;

        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPivot);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::Begin("WorkSpace", nullptr, windowFlags);
        ImGui::PopStyleVar(2);
        ImGui::Columns(2, "WorkSpaceHSeparator", false);

        // ----------------------------------------------------------
        // ----------------------------------------------------------
        // Left panel: file browser
        ImGui::Text("\uf24b %s", mCurrentPath.c_str());

        // Show current entries in mCurrentPath using a table
        auto tableFlags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg
        | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_BordersInnerV
        | ImGuiTableFlags_Resizable;

        if (ImGui::BeginTable("Files Table", 2, tableFlags))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn(mLanguageFile.getc("explorer.filename"), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(mLanguageFile.getc("explorer.filesize"), ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 6);
            ImGui::TableHeadersRow();

            // Save cursor position & draw visible rows
            auto savedWindowPos = ImGui::GetWindowPos();
            auto savedWindowSize = ImGui::GetWindowSize();
            auto clipper = ImGuiListClipper(mCurrentPathItems.size());
            while (clipper.Step())
            {
                for (auto row=clipper.DisplayStart; row<clipper.DisplayEnd; ++row)
                {
                    auto* item = &mCurrentPathItems[row];

                    // Column 1 - File icon+name
                    ImGui::TableNextColumn();
                    auto itemName = std::string("");
                    if (item->isFolder && item->name == "..")
                        itemName = fmt::format("\uf259 {:s}", item->name);
                    else
                        itemName = fmt::format("{:s} {:s}", item->isFolder ? "\uf24b" : "\uf214", item->name);

                    if (ImGui::Selectable(itemName.c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
                        world->emit<FileSystemLoadTaskEvent>
                        ({
                            .type = item->isFolder
                                ? FileSystemLoadTaskEvent::LOAD_DIRECTORY
                                : FileSystemLoadTaskEvent::LOAD_FILE,
                            .path = item->name
                        });

                    // Column 2 - File size
                    ImGui::TableNextColumn();
                    if (!item->isFolder)
                    {
                        auto size = fmt::format("{:d} Kb ", (uint32_t) (item->size / 1024));
                        ImGui::TextUnformatted(size.c_str());
                    }
                    else
                    {
                        ImGui::TextDisabled("--");
                    }
                }
            }
            ImGui::EndTable();

            if (mFileSystemLoading)
            {
                // If the FileSYstem is doing some work, show an overlay on top of the table
                // with animated text and a cancel button
                auto windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings
                | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
                ImGui::SetNextWindowPos(savedWindowPos);
                ImGui::SetNextWindowSize(savedWindowSize);
                ImGui::SetNextWindowBgAlpha(0.8f);
                ImGui::Begin("##filesTableLoadingOverlay", nullptr, windowFlags);
                ImGui::PopStyleVar();

                // Add a dots loading text
                auto strLoading = mLanguageFile.get("ic_loading");
                auto fullStrLoading = std::string(strLoading).append("...");
                auto textSize = ImGui::CalcTextSize(fullStrLoading.c_str());

                auto numPoints = (int) (ImGui::GetTime() * 3) % 4;
                for (auto i=0; i<numPoints; ++i)
                    strLoading.push_back('.');

                // Center in the table
                auto buttonSize = ImVec2(200, textSize.y * 1.5f);
                auto spaceAvail = ImGui::GetContentRegionAvail();

                ImGui::SetCursorPosX((spaceAvail.x/2) - textSize.x/2);
                ImGui::SetCursorPosY((spaceAvail.y/2) - (textSize.y + buttonSize.y));
                ImGui::Text("%s", strLoading.c_str());
                ImGui::NewLine();
                ImGui::SetCursorPosX(spaceAvail.x/2 - buttonSize.x/2);
                if (ImGui::Button(mLanguageFile.getc("cancel"), buttonSize))
                    world->emit<FileSystemCancelTaskEvent>({});
                ImGui::End();
            }
        }

        ImGui::NextColumn();

        // ----------------------------------------------------------
        // ----------------------------------------------------------
        // Right panel - player controls
        ImGui::NewLine();

        ImVec2 buttonSize, buttonUV, buttonST;
        auto textureId = mIconAtlas.getTextureId();
        auto playSprite = mIconAtlas.getAtlasRect("play");
        auto pauseSprite = mIconAtlas.getAtlasRect("pause");
        auto stopSprite = mIconAtlas.getAtlasRect("stop");
        auto nextSprite = mIconAtlas.getAtlasRect("next");
        auto prevSprite = mIconAtlas.getAtlasRect("prev");
        auto playButtonSprite = mAudioSystemStatus != PLAYING ? playSprite : pauseSprite;
        // All button are the same size anyway so we use playButtonSprite.width to compute startX
        auto startX = ((ImGui::GetContentRegionAvailWidth() / 2) - (((playButtonSprite.width * 4) + (style.ItemSpacing.x * 6)) / 2));

        ImGui::NewLine();
        ImGui::SameLine(startX);

        buttonSize = ImVec2(playButtonSprite.width, playButtonSprite.height);
        buttonUV = ImVec2(playButtonSprite.left, playButtonSprite.top);
        buttonST = ImVec2(playButtonSprite.right, playButtonSprite.bottom);
        ImGui::PushID(ImGui::GetID("playerButtonPlay"));
        if (ImGui::ImageButton((ImTextureID)(intptr_t) textureId, buttonSize, buttonUV, buttonST))
            switch (mAudioSystemStatus)
            {
                case PLAYING:
                    world->emit<AudioSystemPlayTaskEvent>({.type = AudioSystemPlayTaskEvent::PAUSE});
                break;
                case PAUSED:
                    world->emit<AudioSystemPlayTaskEvent>({.type = AudioSystemPlayTaskEvent::PLAY});
                break;
                default:
                break;
            }
        ImGui::PopID();

        ImGui::SameLine();

        buttonSize = ImVec2(stopSprite.width, stopSprite.height);
        buttonUV = ImVec2(stopSprite.left, stopSprite.top);
        buttonST = ImVec2(stopSprite.right, stopSprite.bottom);
        ImGui::PushID(ImGui::GetID("playerButtonStop"));
        if (ImGui::ImageButton((ImTextureID)(intptr_t) textureId, buttonSize, buttonUV, buttonST))
            world->emit<AudioSystemPlayTaskEvent>({.type = AudioSystemPlayTaskEvent::STOP});
        ImGui::PopID();

        ImGui::SameLine();

        buttonSize = ImVec2(prevSprite.width, prevSprite.height);
        buttonUV = ImVec2(prevSprite.left, prevSprite.top);
        buttonST = ImVec2(prevSprite.right, prevSprite.bottom);
        ImGui::PushID(ImGui::GetID("playerButtonPrev"));
        if (ImGui::ImageButton((ImTextureID)(intptr_t) textureId, buttonSize, buttonUV, buttonST))
            world->emit<AudioSystemPlayTaskEvent>({.type = AudioSystemPlayTaskEvent::PREV_SUBSONG});
        ImGui::PopID();

        ImGui::SameLine();

        buttonSize = ImVec2(nextSprite.width, nextSprite.height);
        buttonUV = ImVec2(nextSprite.left, nextSprite.top);
        buttonST = ImVec2(nextSprite.right, nextSprite.bottom);
         ImGui::PushID(ImGui::GetID("playerButtonNext"));
        if (ImGui::ImageButton((ImTextureID)(intptr_t) textureId, buttonSize, buttonUV, buttonST))
            world->emit<AudioSystemPlayTaskEvent>({.type = AudioSystemPlayTaskEvent::NEXT_SUBSONG});
        ImGui::PopID();

        // ----------------------------------------------------------
        // ----------------------------------------------------------
        // Right panel - track information if a song is loaded
        if (mCurrentPluginUsed.has_value())
        {
            ImGui::NewLine();
            auto pluginInfo = mCurrentPluginUsed.value();
            pluginInfo.drawStats(world, mLanguageFile, deltaTime);
        }

        // End work space
        ImGui::Columns(1);
        ImGui::End();
    }

    // ----------------------------------------------------------
    // ----------------------------------------------------------
    // About window
    if (mShowAboutWindow)
    {
        auto windowTitle = mLanguageFile.getc("about");
        if (!ImGui::IsPopupOpen(windowTitle))
            ImGui::OpenPopup(windowTitle);

        // Center on screen
        auto windowPos = ImVec2(io.DisplaySize.x/2, io.DisplaySize.y/2);
        auto windowPivot = ImVec2(0.5f, 0.5f);
        auto sizeConstraint = ImVec2(480,-1);
        auto windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        auto appLogo = mIconAtlas.getAtlasRect("logo");

        ImGui::SetNextWindowSizeConstraints(sizeConstraint, sizeConstraint);
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPivot);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 6));
        if (!ImGui::BeginPopupModal(windowTitle, &mShowAboutWindow, windowFlags))
        {
            ImGui::PopStyleVar();
        }
        else
        {
            ImGui::PopStyleVar();

            auto logoUV = ImVec2(appLogo.left, appLogo.top);
            auto logoST = ImVec2(appLogo.right, appLogo.bottom);
            auto logoSize = ImVec2(appLogo.width, appLogo.height);
            ImGui::SetCursorPosX(ImGui::GetContentRegionAvailWidth()/2 - logoSize.x/2 + style.WindowPadding.x);
            ImGui::Image((ImTextureID)(intptr_t) mIconAtlas.getTextureId(), logoSize, logoUV, logoST);
            ImGui::Spacing();
            ImGui::TextWrapped("%s", mLanguageFile.getc("about.introduction"));
            ImGui::NewLine();
            ImGui::TextUnformatted("Version: " GIT_VERSION " (" GIT_COMMIT ")");
            ImGui::TextUnformatted("Build date: " BUILD_DATE);
            ImGui::NewLine();
            ImGui::TextUnformatted(mLanguageFile.getc("about.make_use_of"));
            ImGui::BulletText("SDL %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
            ImGui::BulletText("Dear ImGui %s", ImGui::GetVersion());
            for (auto pluginInfo : mPluginInformations)
                ImGui::BulletText("%s %s", pluginInfo.name.c_str(), pluginInfo.version.c_str());
            ImGui::NewLine();
            ImGui::TextUnformatted(mLanguageFile.getc("about.integrated_fonts"));
            ImGui::BulletText("Atari ST 8x16 System");
            ImGui::BulletText("Material Icons");
            ImGui::EndPopup();
        }
    }

    // ----------------------------------------------------------
    // ----------------------------------------------------------
    // Settings window
    if (mShowSettingsWindow)
    {
        auto windowTitle = mLanguageFile.getc("settings");
        if (!ImGui::IsPopupOpen(windowTitle))
            ImGui::OpenPopup(windowTitle);

        // Center on screen
        auto windowPos = ImVec2(io.DisplaySize.x/2, io.DisplaySize.y/2);
        auto windowPivot = ImVec2(0.5f, 0.5f);
        auto sizeConstraint = ImVec2(512,400);
        auto windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        ImGui::SetNextWindowSizeConstraints(sizeConstraint, sizeConstraint);
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPivot);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 6));
        if (!ImGui::BeginPopupModal(windowTitle, &mShowSettingsWindow, windowFlags))
        {
            ImGui::PopStyleVar();
        }
        else
        {
            ImGui::PopStyleVar();

            auto language = mConfig.get("lang", LanguageFile::Language::ENGLISH);
            if (ImGui::Combo(mLanguageFile.getc("settings.lang"), &language, "English\0Français\0"))
            {
                // Set setting & reload language file
                mConfig.set("lang", language);
                mLanguageFile.reload((LanguageFile::Language) language);
                TRACE("Language file reloaded: {:s}", mLanguageFile.getFilename());
            }

#if defined(__SWITCH__)
            bool mouseEmulation = mConfig.get("mouse_emulation", true);
            if (ImGui::Checkbox(mLanguageFile.getc("settings.mouse_emulation"), &mouseEmulation))
            {
                mConfig.set("mouse_emulation", mouseEmulation);
                ImGui_ImplSDL2_SetMouseEmulationWithGamepad(mouseEmulation);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("todo");


            bool touchEnabled =  mConfig.get("touch_enabled", false);
            if (ImGui::Checkbox(mLanguageFile.getc("settings.touch_enabled"), &touchEnabled))
            {
                mConfig.set("touch_enabled", touchEnabled);
                if (touchEnabled)
                    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
                else
                    io.ConfigFlags &= ~ImGuiConfigFlags_IsTouchScreen;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("todo");
#endif

            int appStyle = mConfig.get("style", 0);
            if (ImGui::Combo(mLanguageFile.getc("settings.style"), &appStyle, "Dark\0Light\0Classic\0"))
            {
                mConfig.set("style", appStyle);
                switch (appStyle)
                {
                    case 0: ImGui::StyleColorsDark();       break;
                    case 1: ImGui::StyleColorsLight();      break;
                    case 2: ImGui::StyleColorsClassic();    break;
                }
            }

            // Display each plugin settings in it's own tab
            ImGui::NewLine();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 6));
            ImGui::BeginTabBar("##pluginSettings", 0);
            ImGui::PopStyleVar();
            for (auto pluginInfo : mPluginInformations)
            {
                auto tabFlags = ImGuiTabItemFlags_NoTooltip;

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 6));
                if (ImGui::BeginTabItem(pluginInfo.name.c_str(), nullptr, tabFlags))
                {
                    ImGui::PopStyleVar();
                    pluginInfo.drawSettings(world, mLanguageFile, deltaTime);
                    ImGui::EndTabItem();
                }
                else
                {
                    ImGui::PopStyleVar();
                }
            }
            ImGui::EndTabBar();
            ImGui::EndPopup();
        }
    }

    // ----------------------------------------------------------
    // ----------------------------------------------------------
    // ImGui windows
#ifndef NDEBUG
    if (mShowDemoWindow)
        ImGui::ShowDemoWindow(&mShowDemoWindow);

    if (mShowMetricsWindow)
        ImGui::ShowMetricsWindow(&mShowMetricsWindow);
#endif

    // ----------------------------------------------------------
    // ----------------------------------------------------------
    // Notifications
    // We can not show much notifications on screen.
    // Let's use a dumb solution to solve the problem: each time we have more than 9, remove the firsts in the stack until we have 9.
    // This take in account the smallest resolution the app is targetting (1280, 720).
    while (mNotifications.size() > 9)
        mNotifications.pop_front();

    // Compute the position of the first notification (top-right corner under the menu bar)
    auto positionX = io.DisplaySize.x - (style.WindowPadding.x + style.DisplaySafeAreaPadding.x);
    auto positionY = ImGui::GetCursorPosY() + style.WindowPadding.y + style.DisplaySafeAreaPadding.y;
    auto windowPivot = ImVec2(1, 0);
    auto windowPos = ImVec2(positionX, positionY);
    auto windowMinSizeConstraint = ImVec2(450, 50);
    auto windowMaxSizeConstraint = ImVec2(450, 250);
    auto windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    auto rowId = 0;
    for (auto it = mNotifications.begin(); it != mNotifications.end(); /*!!*/)
    {
        // Update notification timer
        it->displayTimeMs -= deltaTime * 1000;
        if (it->displayTimeMs <= 0)
        {
            // Remove if no more display time and skip to next notification
            it = mNotifications.erase(it);
            continue;
        }
        else
        {
            // Show it if display time is still valid
            ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPivot);
            ImGui::SetNextWindowSizeConstraints(windowMinSizeConstraint, windowMaxSizeConstraint);
            ImGui::SetNextWindowBgAlpha(0.8f);

            auto notifId = fmt::format("##Notif{:d}", rowId).c_str();
            ImGui::Begin(notifId, nullptr, windowFlags);
            if (it->type == NotificationMessageEvent::ERROR)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::TextWrapped("%s", it->message.c_str());
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::TextWrapped("%s", it->message.c_str());
            }
            ImGui::NewLine();
            auto progressSize = ImVec2(ImGui::GetContentRegionAvailWidth(), 8);
            ImGui::ProgressBar(it->displayTimeMs / mNotificationDisplayTimeMs, progressSize, "");

            // Next notification "y" start
            windowPos.y += ImGui::GetWindowSize().y + style.WindowPadding.y;
            ++it;
            ++rowId;

            ImGui::End();
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UiSystem::receive(ECS::World* world, const SDL_Event& event)
{
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void UiSystem::receive(ECS::World* world, const DirectoryLoadedEvent& event)
{
    TRACE("Received DirectoryLoadedEvent: \"{:s}\" ({:d} items).", event.path, event.items.size());

    mCurrentPath = event.path;
    mCurrentPathItems.clear();
    mCurrentPathItems.insert(mCurrentPathItems.end(), event.items.begin(), event.items.end());
}

void UiSystem::receive(ECS::World* world, const FileSystemBusyEvent& event)
{
    TRACE("Received FileSystemBusyEvent: {:d}.", (int) event.isLoading);
    mFileSystemLoading = event.isLoading;
}

void UiSystem::receive(ECS::World* world, const AudioSystemConfiguredEvent& event)
{
    TRACE("Received AudioSystemConfiguredEvent.");
    for (auto pluginInfo : event.pluginInformations)
        mPluginInformations.push_back(pluginInfo);
}

void UiSystem::receive(ECS::World* world, const AudioSystemPlayEvent& event)
{
    TRACE("Received AudioSystemPlayEvent: {:d}", event.type);

    if (event.type != AudioSystemPlayEvent::STOPPED)
    {
        if (!mCurrentPluginUsed.has_value() || (mCurrentPluginUsed.value().name != event.pluginName))
            for (auto pluginInfo : mPluginInformations)
                if (pluginInfo.name == event.pluginName)
                    mCurrentPluginUsed.emplace(pluginInfo);
    }
    else
    {
        mCurrentPluginUsed.reset();
    }

    switch (event.type)
    {
        case AudioSystemPlayEvent::PLAYING:
            mAudioSystemStatus = PLAYING;
            mStatusMessage = std::string("\uf40a ").append(event.filename);
        break;
        case AudioSystemPlayEvent::PAUSED:
            mAudioSystemStatus = PAUSED;
            mStatusMessage = std::string("\uf3e4 ").append(event.filename);
        break;
        case AudioSystemPlayEvent::STOPPED:
            mAudioSystemStatus = STOPPED;
            mStatusMessage = mLanguageFile.getc("status.ready");
        break;
        default:
        break;
    }
}

void UiSystem::receive(ECS::World* world, const NotificationMessageEvent& event)
{
    TRACE("Received NotificationMessageEvent type {:d} : {:s}.", event.type, event.message);

    mNotifications.push_back
    ({
        .type = event.type,
        .message = event.message,
        .displayTimeMs = mNotificationDisplayTimeMs
    });
}
