#include "osp.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "IconsMaterialDesignIcons_c.h"
#include <SDL2/SDL_log.h>

Osp::Osp() :
    mShowWorkspace(true),
    mStatusMessage("Initializing...") {
}

Osp::~Osp() {
}

bool Osp::setup(Settings settings) {
    // Setup sound engine
    mSettings = settings;
    if (!mSoundEngine.setup(mSettings.mDataPath.c_str())) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to initialize SoundEngine\n");
        return false;
    }

    // Apply user configuration
    auto& io = ImGui::GetIO();
    switch (mSettings.mStyle) {
        case 0: ImGui::StyleColorsDark(); break;
        case 1: ImGui::StyleColorsLight(); break;
        case 2: ImGui::StyleColorsClassic(); break;
    }

    if (mSettings.mFont >= 0 && mSettings.mFont < io.Fonts->Fonts.Size) {
        io.FontDefault = io.Fonts->Fonts[mSettings.mFont];
    }
   
    // Setup file manager
    if (!mFileManager.setup()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to initialize File systems.\n");
        return false;
    }

    // OK
    mStatusMessage = "Ready";
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "OSP initialized.\n");
    return true;
}

void Osp::cleanup() {
    mSoundEngine.cleanup();
    mFileManager.cleanup();
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "OSP cleanup.\n");
}

void Osp::render() {
    char temp[256];
    auto& io = ImGui::GetIO();
    auto& style = ImGui::GetStyle();
    const auto songMetaData = mSoundEngine.getMetaData();
    const auto fmState = mFileManager.getState();
    const auto sndState = mSoundEngine.getState();

    // Manage state of FileManager and SoundEngine
    if (fmState != FileManager::State::READY) {
        switch (fmState) {
            case FileManager::State::ERROR:
                mStatusMessage = mFileManager.getError();
                break;
            default:
                break;
        }
    }
    else {
        if (fmState == FileManager::State::ERROR) {
            mFileManager.clearError();
        } 

        // SoundEngine states
        switch (sndState) {
            case SoundEngine::State::FINISHED_NATURAL:
                // todo: goto next track / file
            case SoundEngine::State::FINISHED:        
                mSoundEngine.stop();
                break;
            case SoundEngine::State::LOADING:
                // Add a little rotating bar to the text
                mStatusMessage = std::string("Loading ");
                mStatusMessage.push_back("|/-\\"[(int)(ImGui::GetTime() / 0.1f) & 3]);
                break;
            case SoundEngine::State::STARTED:
                mStatusMessage = "Playing...";
                break;
            case SoundEngine::State::PAUSED:
                mStatusMessage = "Paused.";
                break;
            case SoundEngine::State::READY:
                mStatusMessage = "Ready.";
                break;
            case SoundEngine::State::ERROR:
                mStatusMessage = mSoundEngine.getError();
                break;
            default:
                break;
        }
    }


    // Manage UI states / rendering
    auto windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    if (!mShowWorkspace) {
        windowFlags |= ImGuiWindowFlags_NoBackground;
    }

    // Main Window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::Begin("Workspace", nullptr, windowFlags);
    ImGui::PopStyleVar(1);

        // Menu
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Application")) {
                if (ImGui::MenuItem(ICON_MDI_DESKTOP_MAC_DASHBOARD " Show Workspace", nullptr, mShowWorkspace, true)) {
                    mShowWorkspace = !mShowWorkspace;
                }
                if (ImGui::BeginMenu(ICON_MDI_PALETTE " Theme")) {
                    if (ImGui::Combo("Style", &mSettings.mStyle, "Dark\0Light\0Classic\0")) {
                        switch (mSettings.mStyle) {
                            case 0: ImGui::StyleColorsDark(); break;
                            case 1: ImGui::StyleColorsLight(); break;
                            case 2: ImGui::StyleColorsClassic(); break;
                        }
                    }

                    const auto defaultFont = io.Fonts->Fonts[mSettings.mFont];
                    if (ImGui::BeginCombo("Font", defaultFont->GetDebugName())) {
                        for (auto n=0; n<io.Fonts->Fonts.Size; n++) {
                            ImFont* font = io.Fonts->Fonts[n];
                            ImGui::PushID((void*) font);
                            if (ImGui::Selectable(font->GetDebugName(), font == defaultFont)) {
                                io.FontDefault = font;
                                mSettings.mFont = n;
                            }
                            ImGui::PopID();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(ICON_MDI_SETTINGS " Configuration", true)) {
                    ImGui::MenuItem("TODO", nullptr, false, true);
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                if (ImGui::MenuItem(ICON_MDI_LOGOUT " Quit", nullptr, false, fmState == FileManager::State::READY)) {
                    SDL_Event event;
                    event.type = SDL_QUIT;
                    SDL_PushEvent(&event);
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem(mMetricsWindow.getTitle().c_str(), nullptr, false, true)) {
                    mMetricsWindow.setVisible(true);
                }
                if (ImGui::MenuItem(mAboutWindow.getTitle().c_str())) {
                    mAboutWindow.setVisible(true);
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();
            ImGui::SameLine(); 
            ImGui::Text("Status: %s", mStatusMessage.c_str());      

            sprintf(temp, "%.1f FPS", io.Framerate);
            ImGui::SameLine((ImGui::GetWindowContentRegionWidth() - ImGui::CalcTextSize(temp).x) - style.WindowPadding.x);
            ImGui::Separator();
            ImGui::SameLine();
            ImGui::TextColored(style.Colors[ImGuiCol_PlotHistogramHovered], "%s", temp);  

            ImGui::EndMenuBar();
        }

        // Workspace
        if (mShowWorkspace) {
            ImGui::Columns(2, "workspaceSeparator", false);

            //Explorer
            auto explorerData = (ExplorerFrame::FrameData) {
                .currentPath = mFileManager.getCurrentPath(),
                .listing = mFileManager.getCurrentPathEntries(),
                .isWorking = fmState == FileManager::State::LOADING
            };
            
            mExplorerFrame.render(explorerData, [&](FileSystem::Entry item) {
                handleExplorerItemClick(item, mFileManager.getCurrentPath());
            });

            ImGui::NextColumn();

            // Player
            const auto playerData = (PlayerFrame::FrameData) {
                .state = sndState,
                .metaData = songMetaData
            };

            mPlayerFrame.render(playerData,
                [&](PlayerFrame::ButtonId button) { 
                    handlePlayerButtonClick(button);
                },
                [&](int seek) { 
                    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Seek: %d", seek);
                });

            // Song meta data
            const auto metaData = (MetaDataFrame::FrameData) {
                .state = sndState,
                .metaData = songMetaData
            };

            mMetaDataFrame.render(metaData);

            ImGui::Columns(1);
        }
        
    ImGui::End();

    // Popups
    mMetricsWindow.render();
    mAboutWindow.render();
}

void Osp::handleExplorerItemClick(FileSystem::Entry item, std::filesystem::path currentExplorerPath) {
    const auto sndState = mSoundEngine.getState();

    if (item.folder) {
        if (! mFileManager.navigate(item.name.c_str())) {
            // If FileManager process don't started because of an error
            mStatusMessage = mFileManager.getError();
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s", mStatusMessage.c_str());
        } else {
            // If we were in error state it's time to clean
            if (sndState == SoundEngine::State::ERROR) {
                mSoundEngine.clearError();
            }
        }
    } else {
        // todo threading for load song ?
        const auto file = std::shared_ptr<File>(mFileManager.getFile(currentExplorerPath.append(item.name)));
        if (! mSoundEngine.load(file)) {
                mStatusMessage = std::string("Error loading file: ").append(mSoundEngine.getError());
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s", mStatusMessage.c_str());
        }
    }
}

void Osp::handlePlayerButtonClick(PlayerFrame::ButtonId button) {
    const auto sndState = mSoundEngine.getState();

    switch (button) {
        case PlayerFrame::ButtonId::PLAY:
            switch (sndState) {
                case SoundEngine::State::STARTED:
                    mSoundEngine.pause();
                    break;
                case SoundEngine::State::PAUSED:
                    mSoundEngine.play();
                    break;
                default:
                    break;
            }
        break;
        case PlayerFrame::ButtonId::STOP:
            switch (sndState) {
                case SoundEngine::State::STARTED:
                case SoundEngine::State::PAUSED:
                    mSoundEngine.stop();
                    break;
                default:
                    break;
            }
        break;
        case PlayerFrame::ButtonId::NEXT:
            switch (sndState) {
                case SoundEngine::State::STARTED:
                case SoundEngine::State::PAUSED:
                    if (!mSoundEngine.nextTrack()) {
                        // todo: next file
                        mSoundEngine.stop();
                    }
                    break;
                default:
                    break;
            }
        break;
        case PlayerFrame::ButtonId::PREV:
            switch (sndState) {
                case SoundEngine::State::STARTED:
                case SoundEngine::State::PAUSED:
                    if (!mSoundEngine.prevTrack()) {
                        // todo: prev file
                        mSoundEngine.stop();
                    }
                    break;
                default:
                    break;
            }
        break;
    }
}
