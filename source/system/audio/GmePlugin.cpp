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
#include "GmePlugin.h"

#include <stdexcept>

// Need to undef check because it's causing issue with fmt
#undef check
#include <fmt/format.h>

#include <imgui/imgui.h>


GmePlugin::GmePlugin() :
Plugin(),
mMusicEmu(nullptr),
mMutex(SDL_CreateMutex()),
mCurrentTrack(0),
mTrackCount(0)
{
}

GmePlugin::~GmePlugin()
{
    SDL_DestroyMutex(mMutex);
}

std::string GmePlugin::getName()
{
    return "gme";
}

std::string GmePlugin::getVersion()
{
    return "0.6.3";
}

std::vector<std::string> GmePlugin::getSupportedExtensions()
{
    return
    {
        ".ay", ".gbs", ".gym", ".hes", ".kss", ".nsf", ".nsfe",
        ".sap", ".spc", ".vgm", ".vgz"
    };
}

void GmePlugin::setup(Config config)
{
    Plugin::setup(config);
}

void GmePlugin::cleanup()
{
    Plugin::cleanup();
}

void GmePlugin::open(const std::vector<uint8_t>& buffer)
{
    auto header = gme_identify_header(buffer.data());
    if (header[0] == '\0')
        throw std::runtime_error(gme_wrong_file_type);

    auto error = gme_open_data(buffer.data(), buffer.size(), &mMusicEmu, 48000);
    if (error != nullptr)
        throw std::runtime_error(error);

    auto enableAccuracy = mConfig.get("enable_accuracy", true);
    auto loadPlaybackLimit = mConfig.get("autoload_playback_limit", true);
    auto ignoreSilence = mConfig.get("ignore_silence", false);

    mCurrentTrack = 0;
    mTrackCount = gme_track_count(mMusicEmu);

    gme_enable_accuracy(mMusicEmu, enableAccuracy);
    gme_set_autoload_playback_limit(mMusicEmu, loadPlaybackLimit);
    gme_ignore_silence(mMusicEmu, ignoreSilence);
    gme_start_track(mMusicEmu, mCurrentTrack);
}

int GmePlugin::getCurrentTrack()
{
    return mCurrentTrack+1;
}

int GmePlugin::getTrackCount()
{
    return mTrackCount;
}

void GmePlugin::setSubSong(int subsong)
{
    if (mMusicEmu == nullptr)
        return;

    if (subsong > 0 && subsong <= mTrackCount)
    {
        mCurrentTrack = subsong-1;
        gme_start_track(mMusicEmu, mCurrentTrack);
    }
}

void GmePlugin::close()
{
    if (mMusicEmu != nullptr)
    {
        gme_delete(mMusicEmu);
        mMusicEmu = nullptr;
    }

    mCurrentTrack = 0;
    mTrackCount = 0;
}

bool GmePlugin::decode(uint8_t* stream, size_t len)
{
    if (mMusicEmu == nullptr)
        return false;

    SDL_LockMutex(mMutex);
    auto size = (int) len / 2;
    auto error = gme_play(mMusicEmu, size, (short*) stream);
    bool ended = gme_track_ended(mMusicEmu);
    SDL_UnlockMutex(mMutex);

    if (error != nullptr)
        throw std::runtime_error(error);

    return !ended;
}

void GmePlugin::drawSettings(ECS::World* world, LanguageFile languageFile, float deltaTime)
{
    auto enableAccuracy = mConfig.get("enable_accuracy", true);
    if (ImGui::Checkbox(languageFile.getc("plugin.enable_accuracy"), &enableAccuracy))
        mConfig.set("enable_accuracy", enableAccuracy);

    auto loadPlaybackLimit = mConfig.get("autoload_playback_limit", true);
    if (ImGui::Checkbox(languageFile.getc("plugin.autoload_playback_limit"), &loadPlaybackLimit))
        mConfig.set("autoload_playback_limit", loadPlaybackLimit);

    auto ignoreSilence = mConfig.get("ignore_silence", false);
    if (ImGui::Checkbox(languageFile.getc("plugin.ignore_silence"), &ignoreSilence))
        mConfig.set("ignore_silence", ignoreSilence);
}

void GmePlugin::drawStats(ECS::World* world, LanguageFile languageFile, float deltaTime)
{
    if (mMusicEmu == nullptr)
        return;

    gme_info_t* info;
    SDL_LockMutex(mMutex);
    gme_track_info(mMusicEmu, &info, mCurrentTrack);
    SDL_UnlockMutex(mMutex);

    auto duration = (info->length > 0 ? info->length : info->play_length) / 1000;
    auto position = (gme_tell_samples(mMusicEmu) / 48000) / 2;

    if (Plugin::beginTable(languageFile.getc("player"), false))
    {
        Plugin::drawRow(languageFile.getc("player.title"),        info->song);
        Plugin::drawRow(languageFile.getc("player.track"),        fmt::format("{:d}/{:d}",     mCurrentTrack+1, mTrackCount));
        Plugin::drawRow(languageFile.getc("player.duration"),     fmt::format("{:d}:{:02d}",   duration / 60, duration % 60));
        Plugin::drawRow(languageFile.getc("player.position"),     fmt::format("{:d}:{:02}",    position / 60, position % 60));
        Plugin::endTable();
        ImGui::NewLine();
    }

    if (Plugin::beginTable(languageFile.getc("metadata"), false))
    {
        Plugin::drawRow(languageFile.getc("metadata.game"),         info->game);
        Plugin::drawRow(languageFile.getc("metadata.author"),       info->author);
        Plugin::drawRow(languageFile.getc("metadata.copyright"),    info->copyright);
        Plugin::drawRow(languageFile.getc("metadata.system"),       info->system);
        Plugin::drawRow(languageFile.getc("metadata.dumper"),       info->dumper);
        ImGui::EndTable();
        ImGui::NewLine();
    }

    if (strlen(info->comment) > 0)
    {
        if (Plugin::beginTable(languageFile.getc("metadata.comments"), true, false))
        {
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", info->comment);
            Plugin::endTable();
        }
    }

    gme_free_info(info);
}