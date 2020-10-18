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
#include "AudioSystem.h"

#include <string>
#include <vector>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "audio/OpenmptPlugin.h"
#include "audio/GmePlugin.h"
#include "audio/SidplayfpPlugin.h"
#include "audio/Sc68Plugin.h"

#include "../event/audio/AudioSystemConfiguredEvent.h"
#include "../tools/LanguageFile.h"
#include "../config.h"

AudioSystem::AudioSystem(Config config) :
ECS::EntitySystem(),
mConfig(config),
mMutex(SDL_CreateMutex()),
mCurrentPlugin(nullptr),
mPlayStatus(NO_FILE)
{
}

AudioSystem::~AudioSystem()
{
    SDL_DestroyMutex(mMutex);
}

void AudioSystem::configure(ECS::World* world)
{
    TRACE(">>>");

    // All plugin are setup in a way that they are outputting 48000Hz 16bits PCM stereo samples
    SDL_AudioSpec obtainedAudioSpec;
    SDL_AudioSpec wantedAudioSpec;
    wantedAudioSpec.callback = AudioSystem::audioCallback;
    wantedAudioSpec.userdata = this;
    wantedAudioSpec.samples = 2048; // 2048 for better latency
    wantedAudioSpec.channels = 2;
    wantedAudioSpec.format = AUDIO_S16SYS;
    wantedAudioSpec.freq = 48000;

    // Check if the audio device can handle the requested format (open device allowing format change then close it)
    mAudioDevice = SDL_OpenAudioDevice(nullptr, 0, &wantedAudioSpec, &obtainedAudioSpec, SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (mAudioDevice <= 0)
    {
        throw std::runtime_error(SDL_GetError());
    }

    // Reopen the device but force our format to be used (SDL will ressample)
    SDL_CloseAudioDevice(mAudioDevice);
    if (wantedAudioSpec.channels != obtainedAudioSpec.channels
        || wantedAudioSpec.freq != obtainedAudioSpec.freq
        || wantedAudioSpec.format != obtainedAudioSpec.format)
    {
        TRACE("SDL will ressample sound data from {:d} channels {:d}Hz (0x{:X}) to {:d} channels {:d}Hz (0x{:X})",
            wantedAudioSpec.channels, wantedAudioSpec.freq, wantedAudioSpec.format,
            obtainedAudioSpec.channels, obtainedAudioSpec.freq, obtainedAudioSpec.format);
    }

    // Reopen the device but force using our format this time
    mAudioDevice = SDL_OpenAudioDevice(nullptr, 0, &wantedAudioSpec, &obtainedAudioSpec, 0);
    if (mAudioDevice <= 0)
    {
        throw std::runtime_error(SDL_GetError());
    }

    TRACE("Current driver: {:s} {:d} channels {:d}Hz (0x{:X}), buffer size: {:d}",
        SDL_GetCurrentAudioDriver(), obtainedAudioSpec.channels, obtainedAudioSpec.freq, obtainedAudioSpec.format, obtainedAudioSpec.samples);

    // Dope
    mPlugins.push_back(new OpenmptPlugin());
    mPlugins.push_back(new GmePlugin());
    mPlugins.push_back(new SidplayfpPlugin());
    mPlugins.push_back(new Sc68Plugin());

    // Setup each plugin and create data for other systems
    auto pluginInformations = std::vector<AudioSystemConfiguredEvent::PluginInformation>();
    for (auto* plugin : mPlugins)
    {
        plugin->setup(mConfig);
        auto name = plugin->getName();
        auto version = plugin->getVersion();
        auto extensions = plugin->getSupportedExtensions();

        pluginInformations.push_back
        ({
            .name = name,
            .version = version,
            .supportedExtensions = extensions,
            .drawSettings =
                [plugin](ECS::World* world, LanguageFile languageFile, float deltaTime)
                {
                    plugin->drawSettings(world, languageFile, deltaTime);
                },
            .drawPlayerStats =
                [plugin](ECS::World* world, LanguageFile languageFile, float deltaTime)
                {
                    plugin->drawPlayerStats(world, languageFile, deltaTime);
                },
            .drawMetadata =
                [plugin](ECS::World* world, LanguageFile languageFile, float deltaTime)
                {
                    plugin->drawMetadata(world, languageFile, deltaTime);
                }
        });

        TRACE("Plugin {:s} {}", name, extensions);
    }

    // Subcribe for events
    world->subscribe<AudioSystemLoadFileEvent>(this);
    world->subscribe<AudioSystemPlayTaskEvent>(this);

    // Tells everyone we are configured
    world->emit<AudioSystemConfiguredEvent>({.pluginInformations = pluginInformations});
}

void AudioSystem::unconfigure(ECS::World* world)
{
    TRACE(">>>");

     // Unubscribe for events
    world->unsubscribe<AudioSystemLoadFileEvent>(this);
    world->unsubscribe<AudioSystemPlayTaskEvent>(this);

    // Release SDL resources used for audio
    if (mPlayStatus != NO_FILE)
    {
        stopAudio(world, true, false);
    }

    SDL_ClearQueuedAudio(mAudioDevice);
    SDL_CloseAudioDevice(mAudioDevice);

    // Release resources used by plugins
    for (auto* plugin : mPlugins)
    {
        plugin->cleanup();
        delete plugin;
    }
}

void AudioSystem::tick(ECS::World* world, float deltaTime)
{
    // Check for pending event probably sent by the audio calback
    SDL_LockMutex(mMutex);
    if (mPendingAudioSystemErrorEvent.has_value())
    {
        world->emit(mPendingAudioSystemErrorEvent.value());
        mPendingAudioSystemErrorEvent.reset();
    }
    if (mPendingAudioSystemPlayEvent.has_value())
    {
        world->emit(mPendingAudioSystemPlayEvent.value());
        mPendingAudioSystemPlayEvent.reset();
    }
    SDL_UnlockMutex(mMutex);
}

void AudioSystem::stopAudio(ECS::World* world, bool userStop, bool sendEvent)
{
    TRACE("Stop audio playback.");

    // Pause SDL audio and clear current plugin and file
    SDL_PauseAudioDevice(mAudioDevice, true);

    // Prepare an event to tell everyone we stopped playback
    auto event =
    (AudioSystemPlayEvent) {
        .type = userStop ? AudioSystemPlayEvent::STOPPED_BY_USER : AudioSystemPlayEvent::STOPPED,
        .pluginName = mCurrentPlugin->getName(),
        .filename = mCurrentFileLoaded,
        .trackNumber = mCurrentPlugin->getCurrentTrack(),
        .trackCount =  mCurrentPlugin->getTrackCount()
    };

    mCurrentPlugin->close();
    mCurrentPlugin = nullptr;

    mPlayStatus = NO_FILE;
    mCurrentFileLoaded = "";

    if (sendEvent)
    {
        if (world != nullptr)
        {
            world->emit<AudioSystemPlayEvent>(event);
        }
        else
        {
            // If World is null place the event in the optional, this was probably called from a thread (audio callback)
            SDL_LockMutex(mMutex);
            mPendingAudioSystemPlayEvent.emplace(event);
            SDL_UnlockMutex(mMutex);
        }
    }
}

void AudioSystem::audioCallback(void* thiz, uint8_t* stream, int len)
{
    auto* audioSystem = (AudioSystem*) thiz;
    memset(stream, 0, len);

    try
    {
        // Decode some frames of sound using the current decoder
        if (! audioSystem->mCurrentPlugin->decode(stream, len))
        {
            audioSystem->stopAudio(nullptr, false, true);
            return;
        }
    }
    catch(const std::exception& e)
    {
        auto error = e.what();
        TRACE("Audio callback error: {:s}", error);

        // Something bad happened, stop audio and send a notification about it
        audioSystem->stopAudio(nullptr, true, true);

        SDL_LockMutex(audioSystem->mMutex);
        audioSystem->mPendingAudioSystemErrorEvent.emplace(
        (AudioSystemErrorEvent) {
            .message = std::string("AudioSystem error: ").append(error)
        });
        SDL_UnlockMutex(audioSystem->mMutex);
    }
}

void AudioSystem::receive(ECS::World* world, const AudioSystemLoadFileEvent& event)
{
    TRACE("Received AudioSystemLoadFileEvent: {:d} {:s} ({:d} Kb), track: {:d}.", event.type, event.path, (uint32_t) event.buffer.size() / 1024, event.startTrack);

    // Stop playback but keep trace of what we were doing
    if (mPlayStatus != NO_FILE)
    {
        stopAudio(world, false, false);
    }

    std::string fileExtension = std::filesystem::path(event.path).extension();
    std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

    for (auto* plugin : mPlugins)
    {
        for (auto extension : plugin->getSupportedExtensions())
        {
            if (extension == fileExtension)
            {
                TRACE("Selecting {:s} plugin.", plugin->getName());
                mCurrentPlugin = plugin;
                break;
            }
        }
    }

    if (mCurrentPlugin == nullptr)
    {
        TRACE("Unsupported file extension: {:s}", fileExtension);
        // We should never reach this code because checks are done before (FileSystem)
        return;
    }

    try
    {
        mCurrentPlugin->open(event.buffer);
        mCurrentPlugin->setSubSong(event.startTrack);
    }
    catch(const std::exception& e)
    {
        // Something bad happened, tells everyone and close the plugin that was in use
        stopAudio(world, true, true);

        world->emit<AudioSystemErrorEvent>
        ({
            .message = fmt::format("AudioSystem error: {:s}", e.what())
        });

        return;
    }

    // Start playing right now and tells everyone
    mCurrentFileLoaded = event.path;
    TRACE("File loaded.");

    auto audioEvent =
    (AudioSystemPlayEvent) {
        .pluginName = mCurrentPlugin->getName(),
        .filename = mCurrentFileLoaded,
        .trackNumber = mCurrentPlugin->getCurrentTrack(),
        .trackCount = mCurrentPlugin->getTrackCount()
    };

    switch (event.type)
    {
        case AudioSystemLoadFileEvent::LOAD_AND_PLAY:
            // If before receiveing this event we were already playing or if no file was loaded
            mPlayStatus = PLAYING;
            audioEvent.type = AudioSystemPlayEvent::PLAYING;
            SDL_PauseAudioDevice(mAudioDevice, false);
            TRACE("Playback started...");
            break;
        case AudioSystemLoadFileEvent::LOAD_AND_PAUSE:
            // If we were paused, stay paused.
            mPlayStatus = PAUSED;
            audioEvent.type = AudioSystemPlayEvent::PAUSED;
            TRACE("Playback paused...");
        break;
    }

    world->emit<AudioSystemPlayEvent>(audioEvent);
}

void AudioSystem::receive(ECS::World* world, const AudioSystemPlayTaskEvent& event)
{
    TRACE("Received AudioSystemPlayTaskEvent: {:d}.", event.type);
    if (mPlayStatus == NO_FILE)
    {
        return;
    }

    switch (event.type)
    {
        case AudioSystemPlayTaskEvent::PLAY:
            if (mPlayStatus == PAUSED)
            {
                SDL_PauseAudioDevice(mAudioDevice, false);
                mPlayStatus = PLAYING;
                world->emit<AudioSystemPlayEvent>
                ({
                    .type = AudioSystemPlayEvent::PLAYING,
                    .pluginName = mCurrentPlugin->getName(),
                    .filename = mCurrentFileLoaded,
                    .trackNumber = mCurrentPlugin->getCurrentTrack(),
                    .trackCount = mCurrentPlugin->getTrackCount()
                });
            }
        break;
        case AudioSystemPlayTaskEvent::PAUSE:
            if (mPlayStatus == PLAYING)
            {
                SDL_PauseAudioDevice(mAudioDevice, true);
                mPlayStatus = PAUSED;
                world->emit<AudioSystemPlayEvent>
                ({
                    .type = AudioSystemPlayEvent::PAUSED,
                    .pluginName = mCurrentPlugin->getName(),
                    .filename = mCurrentFileLoaded,
                    .trackNumber = mCurrentPlugin->getCurrentTrack(),
                    .trackCount = mCurrentPlugin->getTrackCount()
                });
            }
        break;
        case AudioSystemPlayTaskEvent::PREV_SUBSONG:
            if (mCurrentPlugin->getTrackCount() <= 1
                || mCurrentPlugin->getCurrentTrack() <= 1)
            {
                world->emit<AudioSystemPlayEvent>
                ({
                    .type = AudioSystemPlayEvent::NO_PREV_SUBSONG,
                    .pluginName = mCurrentPlugin->getName(),
                    .filename = mCurrentFileLoaded,
                    .trackNumber = mCurrentPlugin->getCurrentTrack(),
                    .trackCount = mCurrentPlugin->getTrackCount()
                });
                break;
            }

            SDL_PauseAudioDevice(mAudioDevice, true);
            if (mCurrentPlugin->getCurrentTrack() > 1)
            {
                mCurrentPlugin->setSubSong(mCurrentPlugin->getCurrentTrack()-1);
            }

            if (mPlayStatus != PAUSED)
            {
                SDL_PauseAudioDevice(mAudioDevice, false);
                world->emit<AudioSystemPlayEvent>
                ({
                    .type = AudioSystemPlayEvent::PLAYING,
                    .pluginName = mCurrentPlugin->getName(),
                    .filename = mCurrentFileLoaded,
                    .trackNumber = mCurrentPlugin->getCurrentTrack(),
                    .trackCount = mCurrentPlugin->getTrackCount()
                });
            }
        break;
        case AudioSystemPlayTaskEvent::NEXT_SUBSONG:
            if (mCurrentPlugin->getTrackCount() <= 1
                || mCurrentPlugin->getCurrentTrack() >= mCurrentPlugin->getTrackCount())
            {
                world->emit<AudioSystemPlayEvent>
                ({
                    .type = AudioSystemPlayEvent::NO_NEXT_SUBSONG,
                    .pluginName = mCurrentPlugin->getName(),
                    .filename = mCurrentFileLoaded,
                    .trackNumber = mCurrentPlugin->getCurrentTrack(),
                    .trackCount = mCurrentPlugin->getTrackCount()
                });
                break;
            }

            SDL_PauseAudioDevice(mAudioDevice, true);
            if (mCurrentPlugin->getCurrentTrack() < mCurrentPlugin->getTrackCount())
            {
                mCurrentPlugin->setSubSong(mCurrentPlugin->getCurrentTrack()+1);
            }

            if (mPlayStatus != PAUSED)
            {
                SDL_PauseAudioDevice(mAudioDevice, false);
                world->emit<AudioSystemPlayEvent>
                ({
                    .type = AudioSystemPlayEvent::PLAYING,
                    .pluginName = mCurrentPlugin->getName(),
                    .filename = mCurrentFileLoaded,
                    .trackNumber = mCurrentPlugin->getCurrentTrack(),
                    .trackCount = mCurrentPlugin->getTrackCount()
                });
            }
        break;
        case AudioSystemPlayTaskEvent::STOP:
            if (mPlayStatus != NO_FILE)
            {
                // If the user request a stop, then stop and send an event.
                stopAudio(world, true, true);
            }
        break;
    }
}
