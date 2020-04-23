#include "osp.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "platform.h" 
#include "strings.h"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_image.h>

Osp::Osp() :
    mShowWorkspace(true),
    mStatusMessage("Initializing..."),
    mTextureSprites(0) {
}

Osp::~Osp() {
}

bool Osp::setup(const std::string dataPath) {
    mSettings.load(CONFIG_FILENAME);

    // Load spritesheets
    if (!mSpriteCatalog.setup(std::string(dataPath).append("/spritesheet/spritesheet.json"))) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to load the spritesheet catalog: %s.\n", mSpriteCatalog.getError().c_str());
        return false;
    }

    // Load sprite texture
    glGenTextures(1, &mTextureSprites);
    if (mTextureSprites == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create the spritesheet texture.\n");
        return false;
    }

    const auto spritesheet = IMG_Load(std::string(dataPath).append("/spritesheet/spritesheet.png").c_str());
    if (spritesheet == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to load the spritesheet texture.\n");
        return false;
    }
    glBindTexture(GL_TEXTURE_2D, mTextureSprites);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, spritesheet->w, spritesheet->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, spritesheet->pixels);
    SDL_FreeSurface(spritesheet);

    // Setup sound engine
    if (!mSoundEngine.setup(dataPath.c_str())) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to initialize SoundEngine.\n");
        return false;
    }

    // Apply user configuration
    auto& io = ImGui::GetIO();
    switch (mSettings.style) {
        case 0: ImGui::StyleColorsDark(); break;
        case 1: ImGui::StyleColorsLight(); break;
        case 2: ImGui::StyleColorsClassic(); break;
    }

    if (mSettings.font >= 0 && mSettings.font < io.Fonts->Fonts.Size) {
        io.FontDefault = io.Fonts->Fonts[mSettings.font];
    }
   
    // Setup file manager
    if (!mFileManager.setup()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to initialize File systems.\n");
        return false;
    }

    ImGui_ImplSDL2_SetMouseEmulationWithGamepad(mSettings.mouseEmulation);
    if (mSettings.mouseEmulation && !PLATFORM_HAS_MOUSE_CURSOR) {
        io.MouseDrawCursor = true;
    }

    if (mSettings.touchEnabled) {
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    }

    // OK
    mStatusMessage = STR_READY;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "OSP initialized.\n");
    return true;
}

void Osp::cleanup() {
    mSoundEngine.cleanup();
    mFileManager.cleanup();
    mSpriteCatalog.cleanup();
    
    glDeleteTextures(1, &mTextureSprites);
    mTextureSprites = 0;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "OSP cleanup.\n");
}

void Osp::render() {
    auto& io = ImGui::GetIO();
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
                selectNextTrack(mSettings.skipUnsupportedTunes, true);
                break;
            case SoundEngine::State::STARTED:
                mStatusMessage = STR_PLAYING;
                break;
            case SoundEngine::State::PAUSED:
                mStatusMessage = STR_PAUSED;
                break;
            case SoundEngine::State::FINISHED:        
                mStatusMessage = STR_READY;
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
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("Workspace", nullptr, windowFlags);
    ImGui::PopStyleVar(2);

    // Menu
    mMenuBar.render({
            .message = mStatusMessage,
            .fmState = fmState,
            .itemShowWorkspaceCheked = mShowWorkspace,
            .settings = mSettings
        },
        [&](int style) {
            handleStyleChange(style);
        },
        [&](ImFont* font, int n) {
            handleFontChange(font, n);
        },
        [&](MenuBar::MenuAction action) {
            handleMenuBarAction(action);
        });

    ImGui::PopStyleVar(1);

    // Workspace
    if (mShowWorkspace) {
        ImGui::Columns(2, "workspaceSeparator", false);

        //Explorer
        const auto selectedItem = !mLastFileSelected.empty()
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
                .texture = mTextureSprites,
                .playFrame = mSpriteCatalog.getFrame("play"),
                .pauseFrame = mSpriteCatalog.getFrame("pause"),
                .stopFrame = mSpriteCatalog.getFrame("stop"),
                .nextFrame = mSpriteCatalog.getFrame("next"),
                .prevFrame = mSpriteCatalog.getFrame("prev"),
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

    // Other windows & popups
    if (mSettingsWindow.isVisible()) {
        mSettingsWindow.render( {
                .settings = mSettings
            },
            [&](SettingsWindow::ToggleSetting setting) {
                handleSettingsChange(setting);
            });
    }

    if (mMetricsWindow.isVisible()) {
        mMetricsWindow.render();
    }

    if (mAboutWindow.isVisible()) {
        mAboutWindow.render(mTextureSprites, mSpriteCatalog.getFrame("logo"));
    }
}

void Osp::selectNextTrack(bool skipInvalid, bool autoPlay) {
    if (mSettings.skipSubTunes || !mSoundEngine.nextTrack()) {
        if (const auto nextFileName = getNextFileName();
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
    if (mSettings.skipSubTunes || !mSoundEngine.prevTrack()) {
        if (const auto prevFileName = getPrevFileName();
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

std::string Osp::getPrevFileName() const {
    const auto items = mFileManager.getCurrentPathEntries();
    const auto itemCount = items.size();
    
    // No item selected, return first file from the end if exists
    if (mLastFileSelected.empty()) {
        for (size_t i=itemCount-1; i>0; i--) {
            const auto entry = items[i];
            if (!entry.folder) {
                return entry.name; 
            }
        }
        return mLastFileSelected;
    }

    // Try to find the previous item
    bool byPass = false;
    for (size_t i=itemCount-1; i>1; i--) {
        const auto entry = items[i];
        if (entry.name == mLastFileSelected || byPass) {
            const auto previous = items[i-1];
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
            const auto entry = items[i];
            if (!entry.folder) {
                return entry.name;
            }
        }
        return mLastFileSelected;
    }


    // Try to find the next item
    bool byPass = false;
    for (size_t i=0; i<itemCount-1; i++) {
        const auto entry = items[i];
        if (entry.name == mLastFileSelected || byPass) {
            const auto next = items[i+1];
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
                case SoundEngine::State::ERROR:
                    selectNextTrack(mSettings.skipUnsupportedTunes,
                        sndState != SoundEngine::State::FINISHED && sndState != SoundEngine::State::ERROR);
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
                case SoundEngine::State::ERROR:
                    selectPrevTrack(mSettings.skipUnsupportedTunes,
                        sndState != SoundEngine::State::FINISHED && sndState != SoundEngine::State::ERROR);
                    break;
                default:
                    break;
            }
        break;
    }
}

void Osp::handleSettingsChange(const SettingsWindow::ToggleSetting setting) {
    auto& io = ImGui::GetIO();

    switch (setting) {
        case SettingsWindow::ToggleSetting::MOUSE_EMULATION:
            mSettings.mouseEmulation = !mSettings.mouseEmulation;
            ImGui_ImplSDL2_SetMouseEmulationWithGamepad(mSettings.mouseEmulation);
            if (!PLATFORM_HAS_MOUSE_CURSOR) {
                io.MouseDrawCursor = !io.MouseDrawCursor;
            }
            break;
        case SettingsWindow::ToggleSetting::TOUCH_ENABLED:
            mSettings.touchEnabled = !mSettings.touchEnabled;
            if (!mSettings.touchEnabled) {
                io.ConfigFlags &= ~ImGuiConfigFlags_IsTouchScreen;
            } else {
                io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
            }
            break;
        case SettingsWindow::ToggleSetting::AUTOSKIP_UNSUPPORTED_FILES:
            mSettings.skipUnsupportedTunes = !mSettings.skipUnsupportedTunes;
            break;
        case SettingsWindow::ToggleSetting::SKIP_SUBTUNES:
            mSettings.skipSubTunes = !mSettings.skipSubTunes;
            break;
        case SettingsWindow::ToggleSetting::ALWAYS_START_FIRST_TRACK:
            mSettings.alwaysStartFirstTune = !mSettings.alwaysStartFirstTune;
            break;
        break;
    }
    mSettings.save(CONFIG_FILENAME);
}

void Osp::handleStyleChange(int style) {
    switch (style) {
        case 0: ImGui::StyleColorsDark();
            break;
        case 1: ImGui::StyleColorsLight();
            break;
        case 2: ImGui::StyleColorsClassic();
            break;
    }
    mSettings.style = style;
    mSettings.save(CONFIG_FILENAME);
}

void Osp::handleFontChange(ImFont* font, int fontIndex) {
    auto& io = ImGui::GetIO();

    io.FontDefault = font;
    mSettings.font = fontIndex;
    mSettings.save(CONFIG_FILENAME);
}

void Osp::handleMenuBarAction(const MenuBar::MenuAction action) {
    switch (action) {
        case MenuBar::MenuAction::TOGGLE_WORKSPACE_VISIBILITY:
            mShowWorkspace = !mShowWorkspace;
            break;
        case MenuBar::MenuAction::SHOW_SETTINGS:
            mSettingsWindow.setVisible(true);
            break;
        case MenuBar::MenuAction::SHOW_ABOUT:
            mAboutWindow.setVisible(true);
            break;
        case MenuBar::MenuAction::SHOW_METRICS:
            mMetricsWindow.setVisible(true);
            break;
        case MenuBar::MenuAction::QUIT:
            SDL_Event event;
            event.type = SDL_QUIT;
            SDL_PushEvent(&event);
            break;
    }
}

bool Osp::engineLoad(std::string path, std::string filename) {
    const auto file = mFileManager.getFile(path.append("/").append(filename));
    
    mLastFileSelected = filename;
    if (! mSoundEngine.load(file, !mSettings.alwaysStartFirstTune)) {
        const auto error = mSoundEngine.getError();
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s", error.c_str());
        return false;
    }

    return true;
}
