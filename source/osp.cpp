#include "osp.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "IconsMaterialDesignIcons_c.h"

#include <SDL2/SDL_events.h>
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

    ImGui_ImplSDL2_SetMouseEmulationWithGamepad(mSettings.mouseEmulation);

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
                selectNextTrack(true, true);
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
            case SoundEngine::State::FINISHED:        
            case SoundEngine::State::READY:
                mStatusMessage = "Ready.";
                break;
            case SoundEngine::State::LOADED:
                mStatusMessage = "Song loaded, ready to play.";
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
            ImGui::MenuItem(ICON_MDI_DESKTOP_MAC_DASHBOARD " Show Workspace", nullptr, &mShowWorkspace, true);
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
                if (ImGui::MenuItem(ICON_MDI_CURSOR_DEFAULT_OUTLINE " Mouse emulation", nullptr, &mSettings.mouseEmulation, true)) {
                    ImGui_ImplSDL2_SetMouseEmulationWithGamepad(mSettings.mouseEmulation);
                }
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
        ImGui::Text("Status: %s", mStatusMessage.c_str());      

        sprintf(temp, "%.1f FPS", io.Framerate);
        ImGui::SameLine((ImGui::GetWindowContentRegionWidth() - ImGui::CalcTextSize(temp).x) - style.WindowPadding.x);
        ImGui::Separator();
        ImGui::TextColored(style.Colors[ImGuiCol_PlotHistogramHovered], "%s", temp);  

        ImGui::EndMenuBar();
    }

    // Workspace
    if (mShowWorkspace) {
        ImGui::Columns(2, "workspaceSeparator", false);

        //Explorer
        const std::string selectedItem = !mLastFileSelected.empty()
            ? mLastFileSelected
            : mFileManager.getLastFolder();

        mExplorerFrame.render({
                .currentPath = mFileManager.getCurrentPath(),
                .listing = mFileManager.getCurrentPathEntries(),
                .selectedItemName = selectedItem,
                .isWorking = fmState == FileManager::State::LOADING
            },
            [&](FileSystem::Entry item) {
                handleExplorerItemClick(item, mFileManager.getCurrentPath());
            });

        ImGui::NextColumn();

        // Player
        mPlayerFrame.render({
                .state = sndState,
                .metaData = songMetaData
            },
            [&](PlayerFrame::ButtonId button) { 
                handlePlayerButtonClick(button);
            });

        // Song meta data
        mMetaDataFrame.render({
                .state = sndState,
                .metaData = songMetaData
            });

        ImGui::Columns(1);
    }        
    ImGui::End();

    // Popups
    mMetricsWindow.render();
    mAboutWindow.render();
}

void Osp::selectNextTrack(bool skipInvalid, bool autoPlay) {
    if (!mSoundEngine.nextTrack()) {
        if (auto nextFileName = getNextFileName();
            nextFileName.empty() == false) {

            if (!engineLoad(mFileManager.getCurrentPath(), nextFileName)) {
                // Go until we found something to play
                if (skipInvalid) {
                    selectNextTrack(skipInvalid, autoPlay);
                }
            } else if (autoPlay) {
                mSoundEngine.play();
            }
        }
        else {
            mSoundEngine.stop();
        }
    }
}

void Osp::selectPrevTrack(bool skipInvalid, bool autoPlay) {
    if (!mSoundEngine.prevTrack()) {
        if (auto prevFileName = getPrevFileName();
            prevFileName.empty() == false) {
        
            if (!engineLoad(mFileManager.getCurrentPath(), prevFileName)) {
                // Go until we found something to play
                if (skipInvalid) {
                    selectPrevTrack(skipInvalid, autoPlay);
                }
            } else if (autoPlay) {
                mSoundEngine.play();
            }
        }
        else {
            mSoundEngine.stop();
        }
    }
}

bool Osp::engineLoad(std::string path, std::string filename) {
    const auto file = std::shared_ptr<File>(mFileManager.getFile(
        std::string(path).append("/").append(filename)));
    
    mLastFileSelected = filename;
    if (! mSoundEngine.load(file)) {
        const auto error = mSoundEngine.getError();
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s", error.c_str());
        mStatusMessage = std::string("Error loading file: ").append(error);
        return false;
    }

    return true;
}

std::string Osp::getPrevFileName() const {
    const auto items = mFileManager.getCurrentPathEntries();
    const auto itemCount = items.size();
    
    // No item selected, return first file from the end if exists
    if (mLastFileSelected.empty()) {
        for (size_t i=itemCount-1; i>0; i--) {
            auto entry = items[i];
            if (!entry.folder) {
                return entry.name; 
            }
        }
        return mLastFileSelected;
    }

    // Try to find the previous item
    bool byPass = false;
    for (size_t i=itemCount-1; i>1; i--) {
        auto entry = items[i];
        if (entry.name == mLastFileSelected || byPass) {
            auto previous = items[i-1];
            if (previous.folder) {
                byPass = true;
                continue;
            }

            return previous.name;
        }
    }

    return "";
}

std::string Osp::getNextFileName() const {
    const auto items = mFileManager.getCurrentPathEntries();
    const auto itemCount = items.size();
    
    // No item selected, return first file item if exists
    if (mLastFileSelected.empty()) {
        for (size_t i=0; i<itemCount; i++) {
            auto entry = items[i];
            if (!entry.folder) {
                return entry.name;
            }
        }
        return mLastFileSelected;
    }


    // Try to find the next item
    bool byPass = false;
    for (size_t i=0; i<itemCount-1; i++) {
        auto entry = items[i];
        if (entry.name == mLastFileSelected || byPass) {
            auto next = items[i+1];
            if (next.folder) {
                byPass = true;
                continue;
            }

            return next.name;
        }
    }

    return "";
}

void Osp::handleExplorerItemClick(const FileSystem::Entry item, const std::filesystem::path currentExplorerPath) {
    const auto sndState = mSoundEngine.getState();

    if (item.folder) {
        mLastFileSelected.clear();
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
        if (mLastFileSelected != item.name || sndState == SoundEngine::State::FINISHED) {
            if (engineLoad(mFileManager.getCurrentPath(), item.name)) {
                mSoundEngine.play();
            }
        }
    }
}

void Osp::handlePlayerButtonClick(const PlayerFrame::ButtonId button) {
    const auto sndState = mSoundEngine.getState();

    switch (button) {
        case PlayerFrame::ButtonId::PLAY:
            switch (sndState) {
                case SoundEngine::State::STARTED:
                    mSoundEngine.pause();
                    break;
                case SoundEngine::State::PAUSED:
                case SoundEngine::State::LOADED:
                    mSoundEngine.play();
                    break;
                case SoundEngine::State::FINISHED:
                    if (! mLastFileSelected.empty()) {
                        if (engineLoad(mFileManager.getCurrentPath(), mLastFileSelected)) {
                            mSoundEngine.play();
                        }
                    }
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
                case SoundEngine::State::FINISHED:
                case SoundEngine::State::LOADED:
                    selectNextTrack(true, sndState == SoundEngine::State::STARTED);
                    break;
                default:
                    break;
            }
        break;
        case PlayerFrame::ButtonId::PREV:
            switch (sndState) {
                case SoundEngine::State::STARTED:
                case SoundEngine::State::PAUSED:
                case SoundEngine::State::FINISHED:
                case SoundEngine::State::LOADED:
                    selectPrevTrack(true, sndState == SoundEngine::State::STARTED);
                    break;
                default:
                    break;
            }
        break;
    }
}
