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

#include <vector>
#include <optional>
#include <string>

#include <SDL2/SDL.h>
#include <ECS.h>

#include "audio/Plugin.h"
#include "../event/file/FileLoadedEvent.h"
#include "../event/audio/AudioSystemPlayTaskEvent.h"
#include "../event/audio/AudioSystemPlayEvent.h"
#include "../event/app/NotificationMessageEvent.h"
#include "../tools/ConfigFile.h"


class AudioSystem :
public ECS::EntitySystem,
public ECS::EventSubscriber<FileLoadedEvent>,
public ECS::EventSubscriber<AudioSystemPlayTaskEvent>
{
public:
    AudioSystem(Config config);
    virtual ~AudioSystem();

    virtual void configure(ECS::World* world) override;
    virtual void unconfigure(ECS::World* world) override;
    virtual void tick(ECS::World* world, float deltaTime) override;

    virtual void receive(ECS::World* world, const FileLoadedEvent& event) override;
    virtual void receive(ECS::World* world, const AudioSystemPlayTaskEvent& event) override;

private:
    enum AudioSystemStatus
    {
        NO_FILE,
        PLAYING,
        PAUSED
    };

    Config mConfig;
    SDL_AudioDeviceID mAudioDevice;
    SDL_mutex* mMutex;
    Plugin* mCurrentPlugin;
    AudioSystemStatus mPlayStatus;

    std::string mCurrentFileLoaded;
    std::vector<Plugin*> mPlugins;
    std::optional<NotificationMessageEvent> mPendingNotificationMessageEvent;
    std::optional<AudioSystemPlayEvent> mPendingAudioSystemPlayEvent;

    AudioSystem(const AudioSystem& copy);

    void stopAudio(ECS::World* world, bool userStop, bool sendEvent);
    static void audioCallback(void* thiz, uint8_t* stream, int len);
};
