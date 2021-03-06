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
#pragma once

#include <string>
#include <vector>
#include <deque>
#include <optional>

#include <SDL2/SDL.h>
#include <ECS.h>

#include "../event/file/DirectoryLoadedEvent.h"
#include "../event/file/FileLoadedEvent.h"
#include "../event/file/FileSystemBusyEvent.h"
#include "../event/file/FileSystemErrorEvent.h"
#include "../event/audio/AudioSystemConfiguredEvent.h"
#include "../event/audio/AudioSystemPlayEvent.h"
#include "../event/audio/AudioSystemErrorEvent.h"
#include "../tools/AtlasTexture.h"
#include "../tools/ConfigFile.h"
#include "../tools/LanguageFile.h"


class UiSystem :
public ECS::EntitySystem,
public ECS::EventSubscriber<SDL_Event>,
public ECS::EventSubscriber<FileSystemBusyEvent>,
public ECS::EventSubscriber<FileSystemErrorEvent>,
public ECS::EventSubscriber<DirectoryLoadedEvent>,
public ECS::EventSubscriber<FileLoadedEvent>,
public ECS::EventSubscriber<AudioSystemConfiguredEvent>,
public ECS::EventSubscriber<AudioSystemPlayEvent>,
public ECS::EventSubscriber<AudioSystemErrorEvent>
{
public:
    UiSystem(Config config, LanguageFile languageFile, SDL_Window* window);
    virtual ~UiSystem();

    virtual void configure(ECS::World* world) override;
    virtual void unconfigure(ECS::World* world) override;
    virtual void tick(ECS::World* world, float deltaTime) override;

    virtual void receive(ECS::World* world, const SDL_Event& event) override;
    virtual void receive(ECS::World* world, const FileSystemBusyEvent& event) override;
    virtual void receive(ECS::World* world, const FileSystemErrorEvent& event) override;
    virtual void receive(ECS::World* world, const DirectoryLoadedEvent& event) override;
    virtual void receive(ECS::World* world, const FileLoadedEvent& event) override;
    virtual void receive(ECS::World* world, const AudioSystemConfiguredEvent& event) override;
    virtual void receive(ECS::World* world, const AudioSystemPlayEvent& event) override;
    virtual void receive(ECS::World* world, const AudioSystemErrorEvent& event) override;

private:
    struct Notification
    {
        enum Type
        {
            INFO, ERROR
        };

        Type type;
        std::string message;
        float displayTimeMs;
    };

    struct Playlist
    {
        std::vector<std::string> paths;
        int index;
        bool inUse;
        bool loop;
    };

    struct LoadFileParams
    {
        bool forceStart;
        bool isGoingBack;
        int playlistIndex;
    };

    struct LoadDirectoryParams
    {
        bool addToPlaylist;
    };

    enum AudioSystemStatus
    {
        PLAYING,
        PAUSED,
        STOPPED
    };

    SDL_Window* mWindow;
    Config mConfig;
    LanguageFile mLanguageFile;
    AtlasTexture mIconAtlas;
    AudioSystemStatus mAudioSystemStatus;
    Playlist mPlaylist;
    LoadFileParams mLoadFileParams;
    LoadDirectoryParams mLoadDirectoryParams;

    bool mShowWorkSpace;
    bool mShowDemoWindow;
    bool mShowMetricsWindow;
    bool mShowSettingsWindow;
    bool mShowAboutWindow;
    bool mIsLoadingDirectory;
    bool mIsLoadingFile;
    float mNotificationDisplayTimeMs;

    std::string mStatusMessage;
    std::string mCurrentPath;
    std::vector<DirectoryLoadedEvent::Item> mCurrentPathItems;
    std::vector<AudioSystemConfiguredEvent::PluginInformation> mPluginInformations;
    std::optional<AudioSystemConfiguredEvent::PluginInformation> mCurrentPluginUsed;
    std::deque<Notification> mNotifications;

    UiSystem(const UiSystem& copy);

    void pushNotification(Notification::Type type, std::string message);
    bool isFileSupported(std::string path);
    void processFileItemSelection(ECS::World* world, DirectoryLoadedEvent::Item item, bool addToPlaylist);
    void processPlaylistItemSelection(ECS::World* world, int selectedIndex, bool stayPaused, bool goingBackward);

    void resetPlaylist(bool eraseAllPaths);
    void removeItemFromPlaylist(int index);
    void processNextPlaylistItem(ECS::World* world);
    void processPrevPlaylistItem(ECS::World* world);
};
