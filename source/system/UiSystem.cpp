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
    {
        throw std::runtime_error("ImGui_ImplSDL2_InitForOpenGL failed");
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330 core"))
    {
        throw std::runtime_error("ImGui_ImplOpenGL3_Init failed");
    }

    // ImGui configuration & style
    auto& io = ImGui::GetIO();
    io.LogFilename = nullptr;
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

#if defined(__SWITCH__)
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.MouseDrawCursor = true;

    auto touchEnabled = mConfig.get("touch_enabled", false);
    if (touchEnabled)
    {
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    }
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
    io.Fonts->AddFontFromFileTTF(DATAPATH "font/Roboto-Medium.ttf", 22.0f);
    ImWchar iconRanges[] = { 0x0030, 0xFFCB, 0 };
    ImFontConfig imFontConfig;
    imFontConfig.MergeMode = true;
    imFontConfig.GlyphMinAdvanceX = 20.0f;
    imFontConfig.GlyphMaxAdvanceX = 20.0f;
    io.Fonts->AddFontFromFileTTF(DATAPATH "font/materialdesignicons-webfont.ttf", 20.0f, &imFontConfig, iconRanges);
    io.Fonts->AddFontFromFileTTF(DATAPATH "font/AtariST8x16SystemFont.ttf", 16.0f);
    io.Fonts->Build();

    // Load and create a texture containing a bunch of sprites related to the UI
    mIconAtlas.setup(DATAPATH "atlas/uiatlas.json");

    // Playlist have no selection
    mPlaylist.index = -1;

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

/* Well, don't ask me why this function is so long. I never had the chance to do it, that's it ! */
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
    if (ImGui::BeginMainMenuBar())
    {
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
            {
                pushNotification(NotificationMessageEvent::ERROR, "This is an error notification");
            }
            if (ImGui::MenuItem("\ufd4a INFO notification"))
            {
                pushNotification(NotificationMessageEvent::INFO, "This is an info notification");
            }
            ImGui::EndMenu();
        }
#endif

        auto fpsText = fmt::format("{:.0f} FPS", io.Framerate).c_str();
        auto textSize = ImGui::CalcTextSize(fpsText);
        auto textOffsetX = (ImGui::GetWindowContentRegionWidth() - style.FramePadding.x) - textSize.x;

        // Status message & FPS
        ImGui::Separator();
        ImGui::TextColored(style.Colors[ImGuiCol_PlotHistogramHovered], "%s", mStatusMessage.c_str());

        ImGui::SameLine(textOffsetX, 0);
        ImGui::Separator();
        ImGui::TextColored(style.Colors[ImGuiCol_PlotHistogramHovered], "%s", fpsText);

        ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleVar();

    // ----------------------------------------------------------
    // ----------------------------------------------------------
    // Main Window (fullscreen workspace)
    // Reusable variables
    ImVec2 windowPos, windowPivot, windowSize, windowMinSize, windowMaxSize;
    ImGuiWindowFlags windowFlags;
    ImGuiTableFlags tableFlags;
    ImGuiTabItemFlags tabFlags;
    ImGuiTabBarFlags tabBarFlags;

    if (mShowWorkSpace)
    {
        // Setup the window to take full space on screen
        windowPos = ImVec2(io.DisplaySize.x/2, io.DisplaySize.y);
        windowPivot = ImVec2(0.5f, 1.0f);
        windowSize = ImVec2(io.DisplaySize.x, io.DisplaySize.y - (ImGui::GetCursorPosY() + 1));
        windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize
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
        auto currentPath = mCurrentPath.empty() ? mLanguageFile.getc("mount_points") : mCurrentPath;
        ImGui::TextColored(style.Colors[ImGuiCol_PlotHistogram], "\uf24b"); ImGui::SameLine(); ImGui::TextUnformatted(currentPath.c_str());
        ImGui::Spacing();

        // Show current entries in mCurrentPath using a table
        tableFlags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable
            | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_BordersInnerV;

        if (ImGui::BeginTable("Files Table", 2, tableFlags))
        {
            ImGui::TableSetupScrollFreeze(0, 2);
            ImGui::TableSetupColumn(mLanguageFile.getc("filename"), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(mLanguageFile.getc("filesize"), ImGuiTableColumnFlags_None, 0.20f);
            ImGui::TableHeadersRow();

            // Save cursor position & draw visible rows
            auto savedWindowPos = ImGui::GetWindowPos();
            auto savedWindowSize = ImGui::GetWindowSize();
            auto clipper = ImGuiListClipper(mCurrentPathItems.size());
            while (clipper.Step())
            {
                for (auto row=clipper.DisplayStart; row<clipper.DisplayEnd; ++row)
                {
                    auto& item = mCurrentPathItems[row];
                    auto rowId = item.name.c_str();
                    auto rowIsSelected = ImGui::IsPopupOpen(rowId);

                    // Column 1 - File icon+name
                    ImGui::TableNextColumn();
                    if (item.isFolder)
                    {
                        ImGui::TextColored(style.Colors[ImGuiCol_PlotHistogram], item.name == ".." ? "\uf259" : "\uf24b");
                    }
                    else
                    {
                        ImGui::TextColored(style.Colors[ImGuiCol_PlotLines], "\uf214");
                    }

                    ImGui::SameLine();
                    if (ImGui::Selectable(rowId, rowIsSelected, ImGuiSelectableFlags_SpanAllColumns))
                    {
                        if (item.isFolder || isFileSupported(item.name))
                        {
                            processFileItemSelection(world, item, false);
                        }
                        else
                        {
                            // The file is not usable by any audio plugin
                            auto message = fmt::format("{:s} {:s}", mLanguageFile.getc("files.unsupported_file_type"), item.name);
                            pushNotification(NotificationMessageEvent::Type::INFO, message);
                            TRACE("{:s}", message);
                        }
                    }

                    // Context menu (right click)
                    if (item.name != ".." && ImGui::BeginPopupContextItem(rowId, ImGuiPopupFlags_MouseButtonRight))
                    {
                        if (ImGui::MenuItem("\uf416 Add to playlist", nullptr, false, !item.isFolder && isFileSupported(item.name)))
                        {
                            processFileItemSelection(world, item, true);
                        }
                        ImGui::EndPopup();
                    }

                    // Column 2 - File size
                    ImGui::TableNextColumn();
                    if (!item.isFolder)
                    {
                        auto fileSizeStr = fmt::format("{:d} Kb ", (uint32_t) (item.size / 1024));
                        ImGui::TextUnformatted(fileSizeStr.c_str());
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
                windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings
                    | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
                ImGui::SetNextWindowPos(savedWindowPos);
                ImGui::SetNextWindowSize(savedWindowSize);
                ImGui::SetNextWindowBgAlpha(0.9f);
                if (!ImGui::Begin("##filesTableLoadingOverlay", nullptr, windowFlags))
                {
                    ImGui::PopStyleVar();
                }
                else
                {
                    ImGui::PopStyleVar();

                    // Add a dots loading text
                    auto strLoading = mLanguageFile.get("ic_loading");
                    auto fullStrLoading = std::string(strLoading).append("...");
                    auto textSize = ImGui::CalcTextSize(fullStrLoading.c_str());

                    auto numPoints = (int) (ImGui::GetTime() * 3) % 4;
                    for (auto i=0; i<numPoints; ++i)
                    {
                        strLoading.push_back('.');
                    }

                    // Center in the table
                    auto buttonSize = ImVec2(200, textSize.y * 1.5f);
                    auto spaceAvail = ImGui::GetContentRegionAvail();

                    ImGui::SetCursorPosX((spaceAvail.x/2) - textSize.x/2);
                    ImGui::SetCursorPosY((spaceAvail.y/2) - (textSize.y + buttonSize.y));
                    ImGui::Text("%s", strLoading.c_str());
                    ImGui::NewLine();
                    ImGui::SetCursorPosX(spaceAvail.x/2 - buttonSize.x/2);
                    if (ImGui::Button(mLanguageFile.getc("cancel"), buttonSize))
                    {
                        world->emit<FileSystemCancelTaskEvent>({});
                    }
                    ImGui::End();
                }
            }
        }

        ImGui::NextColumn();

        // ----------------------------------------------------------
        // ----------------------------------------------------------
        // Right panel - player controls
        ImVec2 buttonSize, buttonUV, buttonST;
        auto textureId = mIconAtlas.getTextureId();
        auto playSprite = mIconAtlas.getAtlasRect("play");
        auto pauseSprite = mIconAtlas.getAtlasRect("pause");
        auto stopSprite = mIconAtlas.getAtlasRect("stop");
        auto nextSprite = mIconAtlas.getAtlasRect("next");
        auto prevSprite = mIconAtlas.getAtlasRect("prev");
        auto playButtonSprite = mAudioSystemStatus != PLAYING ? playSprite : pauseSprite;
        // All button are the same size anyway so we use playButtonSprite.width to compute startX
        auto startX = ((ImGui::GetContentRegionAvailWidth() / 2) - (((playButtonSprite.width * 4) + (style.ItemSpacing.x * 5)) / 2));

        buttonSize = ImVec2(playButtonSprite.width, playButtonSprite.height);
        buttonUV = ImVec2(playButtonSprite.left, playButtonSprite.top);
        buttonST = ImVec2(playButtonSprite.right, playButtonSprite.bottom);
        ImGui::NewLine(); ImGui::SameLine(startX);
        ImGui::PushID(ImGui::GetID("playerButtonPlay"));
        if (ImGui::ImageButton((ImTextureID)(intptr_t) textureId, buttonSize, buttonUV, buttonST))
        {
            switch (mAudioSystemStatus)
            {
                case PLAYING:   world->emit<AudioSystemPlayTaskEvent>({.type = AudioSystemPlayTaskEvent::PAUSE}); break;
                case PAUSED:    world->emit<AudioSystemPlayTaskEvent>({.type = AudioSystemPlayTaskEvent::PLAY}); break;
                default: break;
            }
        }
        ImGui::PopID();

        buttonSize = ImVec2(stopSprite.width, stopSprite.height);
        buttonUV = ImVec2(stopSprite.left, stopSprite.top);
        buttonST = ImVec2(stopSprite.right, stopSprite.bottom);
        ImGui::SameLine(); ImGui::PushID(ImGui::GetID("playerButtonStop"));
        if (ImGui::ImageButton((ImTextureID)(intptr_t) textureId, buttonSize, buttonUV, buttonST))
        {
            world->emit<AudioSystemPlayTaskEvent>({.type = AudioSystemPlayTaskEvent::STOP});
        }
        ImGui::PopID();

        buttonSize = ImVec2(prevSprite.width, prevSprite.height);
        buttonUV = ImVec2(prevSprite.left, prevSprite.top);
        buttonST = ImVec2(prevSprite.right, prevSprite.bottom);
        ImGui::SameLine(); ImGui::PushID(ImGui::GetID("playerButtonPrev"));
        if (ImGui::ImageButton((ImTextureID)(intptr_t) textureId, buttonSize, buttonUV, buttonST))
        {
            world->emit<AudioSystemPlayTaskEvent>({.type = AudioSystemPlayTaskEvent::PREV_SUBSONG});
        }
        ImGui::PopID();

        buttonSize = ImVec2(nextSprite.width, nextSprite.height);
        buttonUV = ImVec2(nextSprite.left, nextSprite.top);
        buttonST = ImVec2(nextSprite.right, nextSprite.bottom);
        ImGui::SameLine(); ImGui::PushID(ImGui::GetID("playerButtonNext"));
        if (ImGui::ImageButton((ImTextureID)(intptr_t) textureId, buttonSize, buttonUV, buttonST))
        {
            world->emit<AudioSystemPlayTaskEvent>({.type = AudioSystemPlayTaskEvent::NEXT_SUBSONG});
        }
        ImGui::PopID();

        // ----------------------------------------------------------
        // ----------------------------------------------------------
        // Right panel - track information if a song is loaded (wrapped in a frame)
        windowSize = ImVec2(ImGui::GetContentRegionAvailWidth(), 108);
        ImGui::Spacing();
        if (ImGui::BeginChild("##playerWrapper", windowSize, false, 0))
        {
            if (mCurrentPluginUsed.has_value())
            {
                mCurrentPluginUsed.value().drawPlayerStats(world, mLanguageFile, deltaTime);
            }
            else
            {
                auto text = mLanguageFile.getc("no_file_loaded");
                auto textSize = ImGui::CalcTextSize(text);
                auto textOffsetX = windowSize.x/2 - textSize.x/2;
                auto textOffsetY = windowSize.y/2 - textSize.y/2;
                ImGui::SetCursorPos(ImVec2(textOffsetX, textOffsetY));
                ImGui::TextUnformatted(text);
            }
        }
        ImGui::EndChild();

        // ----------------------------------------------------------
        // ----------------------------------------------------------
        // Right panel - Tabs bar
        auto tabBarFlags = ImGuiTabBarFlags_None;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 6));
        if (!ImGui::BeginTabBar("##workspaceTabs", tabBarFlags))
        {
            ImGui::PopStyleVar();
        }
        else
        {
            ImGui::PopStyleVar();

            // ----------------------------------------------------------
            // ----------------------------------------------------------
            // Tabs bar - Playlist
            tabFlags = ImGuiTabItemFlags_NoTooltip;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 6));
            if (!ImGui::BeginTabItem(mLanguageFile.getc("playlist"), nullptr, tabFlags))
            {
                ImGui::PopStyleVar();
            }
            else
            {
                ImGui::PopStyleVar();
                // Summary / buttons
                auto itemsCount = mPlaylist.paths.size();
                auto itemCountStr = fmt::format("{:d} items", itemsCount).c_str();
                auto textSize = ImGui::CalcTextSize(itemCountStr);
                auto textOffsetX = (ImGui::GetContentRegionAvailWidth() + style.FramePadding.x * 2) - textSize.x;

                ImGui::Spacing();
                ImGui::SmallButton("\uf49f");
                ImGui::SameLine();
                ImGui::SmallButton("\uf413");
                ImGui::SameLine(textOffsetX, 0);
                ImGui::TextUnformatted(itemCountStr);
                ImGui::Spacing();

                // Show current entries
                tableFlags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg
                    | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV;

                if (ImGui::BeginTable("Playlist table", 3, tableFlags))
                {
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthAlwaysAutoResize);

                    auto clipper = ImGuiListClipper(itemsCount);
                    while (clipper.Step())
                    {
                        for (auto row=clipper.DisplayStart; row<clipper.DisplayEnd; ++row)
                        {
                            auto& item = mPlaylist.paths[row];
                            auto filename = std::filesystem::path(item).filename();

                            // Column [LEFT] - icon
                            ImGui::TableNextColumn();
                            if (mPlaylist.index == row)
                            {
                                ImGui::TextUnformatted("\ufa12");
                            }
                            else
                            {
                                ImGui::TextDisabled("\ufa13");
                            }

                            // Column [MIDDLE] - name
                            ImGui::TableNextColumn();
                            auto selectableFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                            if (ImGui::Selectable(filename.c_str(), false, selectableFlags))
                            {
                                processPlaylistSelection(world, row, false);
                            }

                            // Column [RIGHT] - Button(s)
                            auto buttonDeleteId = fmt::format("\ufa78##playListRowDelete{:d}", row);
                            ImGui::TableNextColumn();
                            if (ImGui::SmallButton(buttonDeleteId.c_str()))
                            {
                                processPlaylistSelection(world, row, true);
                            }
                        }
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }

            // ----------------------------------------------------------
            // ----------------------------------------------------------
            // Tabs bar - Current file information
            if (mCurrentPluginUsed.has_value())
            {
                tabFlags = ImGuiTabItemFlags_NoTooltip;
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 6));
                if (!ImGui::BeginTabItem(mLanguageFile.getc("metadata"), nullptr, tabFlags))
                {
                    ImGui::PopStyleVar();
                }
                else
                {
                    ImGui::PopStyleVar();
                    ImGui::Spacing();
                    mCurrentPluginUsed.value().drawMetadata(world, mLanguageFile, deltaTime);
                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
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
        {
            ImGui::OpenPopup(windowTitle);
        }

        // Center on screen
        windowPos = ImVec2(io.DisplaySize.x/2, io.DisplaySize.y/2);
        windowPivot = ImVec2(0.5f, 0.5f);
        windowSize = ImVec2(512, -1);
        windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        auto appLogo = mIconAtlas.getAtlasRect("logo");

        ImGui::SetNextWindowSizeConstraints(windowSize, windowSize);
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
            {
                ImGui::BulletText("%s %s", pluginInfo.name.c_str(), pluginInfo.version.c_str());
            }
            ImGui::NewLine();
            ImGui::TextUnformatted(mLanguageFile.getc("about.integrated_fonts"));
            ImGui::BulletText("Atari ST 8x16 System");
            ImGui::BulletText("Roboto");
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
        {
            ImGui::OpenPopup(windowTitle);
        }

        // Center on screen
        windowPos = ImVec2(io.DisplaySize.x/2, io.DisplaySize.y/2);
        windowPivot = ImVec2(0.5f, 0.5f);
        windowSize = ImVec2(640, 400);
        windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        ImGui::SetNextWindowSizeConstraints(windowSize, windowSize);
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
#if defined(__SWITCH__)
            bool mouseEmulation = mConfig.get("mouse_emulation", true);
            if (ImGui::Checkbox(mLanguageFile.getc("settings.mouse_emulation"), &mouseEmulation))
            {
                mConfig.set("mouse_emulation", mouseEmulation);
                ImGui_ImplSDL2_SetMouseEmulationWithGamepad(mouseEmulation);
            }

            bool touchEnabled =  mConfig.get("touch_enabled", false);
            if (ImGui::Checkbox(mLanguageFile.getc("settings.touch_enabled"), &touchEnabled))
            {
                mConfig.set("touch_enabled", touchEnabled);
                if (touchEnabled)
                {
                    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
                }
                else
                {
                    io.ConfigFlags &= ~ImGuiConfigFlags_IsTouchScreen;
                }
            }
#endif
            // Display each plugin settings in it's own tab
            tabBarFlags = ImGuiTableFlags_None;
            ImGui::NewLine();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 6));
            ImGui::BeginTabBar("##pluginSettings", tabBarFlags);
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
    {
        ImGui::ShowDemoWindow(&mShowDemoWindow);
    }

    if (mShowMetricsWindow)
    {
        ImGui::ShowMetricsWindow(&mShowMetricsWindow);
    }
#endif

    // ----------------------------------------------------------
    // ----------------------------------------------------------
    // Notifications
    // We can not show much notifications on screen.
    // Let's use a dumb solution to solve the problem: each time we have more than 9, remove the firsts in the stack until we have 9.
    // This take in account the smallest resolution the app is targetting (1280, 720).
    while (mNotifications.size() > 9)
    {
        mNotifications.pop_front();
    }

    // Compute the position of the first notification (top-right corner under the menu bar)
    auto positionX = io.DisplaySize.x - (style.WindowPadding.x + style.DisplaySafeAreaPadding.x);
    auto positionY = ImGui::GetCursorPosY() + style.WindowPadding.y + style.DisplaySafeAreaPadding.y;
    windowPos = ImVec2(positionX, positionY);
    windowMinSize = ImVec2(450, 50);
    windowMaxSize = ImVec2(450, 250);
    windowPivot = ImVec2(1, 0);
    windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove
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
            ImGui::SetNextWindowSizeConstraints(windowMinSize, windowMaxSize);
            ImGui::SetNextWindowBgAlpha(0.95f);

            auto notifId = fmt::format("##Notif{:d}", rowId).c_str();
            if (ImGui::Begin(notifId, nullptr, windowFlags))
            {
                if (it->type == NotificationMessageEvent::ERROR)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.69f, 0.2f, 0.2f, 1.0f));
                    ImGui::TextWrapped("%s", it->message.c_str());
                    ImGui::PopStyleColor();
                }
                else
                {
                    ImGui::TextWrapped("%s", it->message.c_str());
                }
                ImGui::NewLine();
                auto progressSize = ImVec2(ImGui::GetContentRegionAvailWidth(), 6);
                ImGui::ProgressBar(it->displayTimeMs / mNotificationDisplayTimeMs, progressSize, "");

                // Next notification "y" start
                windowPos.y += ImGui::GetWindowSize().y + style.WindowPadding.y;
                ++it;
                ++rowId;

                ImGui::End();
            }
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
    {
        mPluginInformations.push_back(pluginInfo);
    }
}

void UiSystem::receive(ECS::World* world, const AudioSystemPlayEvent& event)
{
    TRACE("Received AudioSystemPlayEvent: {:d}", event.type);

    if (event.type != AudioSystemPlayEvent::STOPPED)
    {
        // Refresh current information about used plugin in needed
        if (!mCurrentPluginUsed.has_value() || (mCurrentPluginUsed.value().name != event.pluginName))
        {
            for (auto pluginInfo : mPluginInformations)
            {
                if (pluginInfo.name == event.pluginName)
                {
                    mCurrentPluginUsed.emplace(pluginInfo);
                    break;
                }
            }
        }
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
    pushNotification(event.type, event.message);
}

void UiSystem::pushNotification(NotificationMessageEvent::Type type, std::string message)
{
    // Add a new notification object into the list
    mNotifications.push_back
    ({
        .type = type,
        .message = message,
        .displayTimeMs = mNotificationDisplayTimeMs
    });
}

bool UiSystem::isFileSupported(std::string path)
{
    // Check extension supported (convert to lower case beforehand)
    std::transform(path.begin(), path.end(), path.begin(), ::tolower);
    auto fileExtension = std::filesystem::path(path).extension();

    for (auto& pluginInfo : mPluginInformations)
    {
        for (auto extension : pluginInfo.supportedExtensions)
        {
            if (extension == fileExtension)
            {
                return true;
            }
        }
    }

    return false;
}

void UiSystem::processFileItemSelection(ECS::World* world, DirectoryLoadedEvent::Item item, bool addToPlaylist)
{
    // Build the item path and send it to the filesystem to be loaded or add it to the playlist
    auto itemPath =  std::filesystem::path(mCurrentPath) / item.name;

    if (addToPlaylist)
    {
        mPlaylist.paths.push_back(itemPath);
    }
    else
    {
        world->emit<FileSystemLoadTaskEvent>
        ({
            .type = item.isFolder
                ? FileSystemLoadTaskEvent::LOAD_DIRECTORY
                : FileSystemLoadTaskEvent::LOAD_FILE,
            .path = itemPath
        });
    }
}

 void UiSystem::processPlaylistSelection(ECS::World* world, int selectedIndex, bool remove)
 {
     if (remove)
     {
        mPlaylist.paths.erase(mPlaylist.paths.begin()+selectedIndex);
     }
    else
    {
        mPlaylist.index = selectedIndex;
        world->emit<FileSystemLoadTaskEvent>
        ({
            .type = FileSystemLoadTaskEvent::LOAD_FILE,
            .path = mPlaylist.paths[selectedIndex]
        });
    }
 }
