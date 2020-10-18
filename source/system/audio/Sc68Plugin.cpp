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
#include "Sc68Plugin.h"

#include <stdexcept>
#include <fmt/format.h>
#include <sc68/file68.h>

#include <imgui/imgui.h>

#include "../../config.h"


Sc68Plugin::Sc68Plugin() :
Plugin(),
mSC68(nullptr),
mCurrentTrack(0),
mTrackCount(0)
{
}

Sc68Plugin::~Sc68Plugin()
{
}

std::string Sc68Plugin::getName()
{
    return "sc68";
}

std::string Sc68Plugin::getVersion()
{
    return "3.0.0";
}

std::vector<std::string> Sc68Plugin::getSupportedExtensions()
{
    return
    {
        ".snd", ".sndh", ".sc68"
    };
}

void Sc68Plugin::setup(Config config)
{
    Plugin::setup(config);

    if (sc68_init(nullptr))
    {
        throw std::runtime_error("sc68_init failed");
    }

    mSC68Config = {0};
    mSC68Config.sampling_rate = 48000;
    mSC68 = sc68_create(&mSC68Config);
    if (mSC68 == nullptr)
    {
        throw std::runtime_error(sc68_error(mSC68));
    }
}

void Sc68Plugin::cleanup()
{
    if (mSC68 != nullptr)
    {
        sc68_destroy(mSC68);
        sc68_shutdown();
    }

    Plugin::cleanup();
}

void Sc68Plugin::open(const std::vector<uint8_t>& buffer)
{
    auto aSIDifierEnabled = mConfig.get("enable_asidifier", false);
    auto loop = mConfig.get("loop", false);
    bool alwaysStartFirstTrack = mConfig.get("first_track_always", true);

    if (sc68_load_mem(mSC68, buffer.data(), buffer.size()) != 0)
    {
        throw std::runtime_error(sc68_error(mSC68));
    }

    sc68_cntl(mSC68, SC68_SET_ASID, aSIDifierEnabled ? SC68_ASID_ON : SC68_ASID_OFF);
    if (sc68_play(mSC68, alwaysStartFirstTrack ? 1 : SC68_DEF_TRACK, loop ? SC68_INF_LOOP : SC68_DEF_LOOP) < 0)
    {
        throw std::runtime_error(sc68_error(mSC68));
    }

    sc68_music_info_t diskInfo;
    sc68_process(mSC68, nullptr, 0);
    sc68_music_info(mSC68, &diskInfo, -1, 0);
    mTrackCount = diskInfo.tracks;
    mCurrentTrack = sc68_cntl(mSC68, SC68_GET_TRACK);
}

int Sc68Plugin::getCurrentTrack()
{
    return mCurrentTrack;
}

int Sc68Plugin::getTrackCount()
{
    return mTrackCount;
}

void Sc68Plugin::setSubSong(int subsong)
{
    if (mSC68 == nullptr)
    {
        return;
    }

    if (subsong > 0 && subsong <= mTrackCount)
    {
        mCurrentTrack = subsong;
        auto loop = mConfig.get("loop", false);
        sc68_stop(mSC68);
        sc68_play(mSC68, subsong, loop ? SC68_INF_LOOP : SC68_DEF_LOOP);
        sc68_process(mSC68, nullptr, 0);
    }
}

void Sc68Plugin::close()
{
    if (mSC68 != nullptr)
    {
        sc68_stop(mSC68);
        sc68_close(mSC68);
    }

    mCurrentTrack = 0;
    mTrackCount = 0;
}

bool Sc68Plugin::decode(uint8_t *stream, size_t len)
{
    if (mSC68 == nullptr)
    {
        return false;
    }

    auto amount = (int) len / 4;
    auto retCode = sc68_process(mSC68, stream, &amount);

    if (retCode == SC68_ERROR)
    {
        throw std::runtime_error(sc68_error(mSC68));
    }

    return !(retCode & SC68_END);
}

void Sc68Plugin::drawSettings(ECS::World *world, LanguageFile languageFile, float deltaTime)
{
    bool alwaysStartFirstTrack = mConfig.get("first_track_always", true);
    if (ImGui::Checkbox(languageFile.getc("plugin.always_start_first_track"), &alwaysStartFirstTrack))
    {
        mConfig.set("first_track_always", alwaysStartFirstTrack);
    }

    auto loop = mConfig.get("loop", false);
    if (ImGui::Checkbox(languageFile.getc("plugin.loop_song"), &loop))
    {
        mConfig.set("loop", loop);
    }

    bool aSIDifierEnabled = mConfig.get("enable_asidifier", false);
    if (ImGui::Checkbox(languageFile.getc("plugin.enable_asidifier"), &aSIDifierEnabled))
    {
        mConfig.set("enable_asidifier", aSIDifierEnabled);
    }
}

void Sc68Plugin::drawPlayerStats(ECS::World* world, LanguageFile languageFile, float deltaTime)
{
    if (mSC68 == nullptr)
    {
        return;
    }

    sc68_music_info_t trackInfo;

    mCurrentTrack = sc68_cntl(mSC68, SC68_GET_TRACK);
    int trackResult = sc68_music_info(mSC68, &trackInfo, mCurrentTrack, 0);
    auto position = sc68_cntl(mSC68, SC68_GET_POS) / 1000;
    auto duration = trackInfo.trk.time_ms / 1000;

    if (trackResult != 0)
    {
        return;
    }

    if (Plugin::beginTable(languageFile.getc("player"), false))
    {
        Plugin::drawRow(languageFile.getc("player.title"),      trackInfo.title);
        Plugin::drawRow(languageFile.getc("player.track"),      fmt::format("{:d}/{:d}",     mCurrentTrack, mTrackCount));
        Plugin::drawRow(languageFile.getc("player.duration"),   fmt::format("{:d}:{:02d}",   duration / 60, duration % 60));
        Plugin::drawRow(languageFile.getc("player.position"),   fmt::format("{:d}:{:02}",    position / 60, position % 60));
        Plugin::endTable();
    }
}

void Sc68Plugin::drawMetadata(ECS::World* world, LanguageFile languageFile, float deltaTime)
{
    if (mSC68 == nullptr)
    {
        return;
    }

    sc68_music_info_t diskInfo;
    sc68_music_info_t trackInfo;

    int diskResult = sc68_music_info(mSC68, &diskInfo, -1, 0);
    int trackResult = sc68_music_info(mSC68, &trackInfo, mCurrentTrack, 0);

    if (diskResult != 0 || trackResult != 0)
    {
        return;
    }

    if (Plugin::beginTable(languageFile.getc("metadata"), false))
    {
        Plugin::drawRow(languageFile.getc("metadata.album"),        diskInfo.album);
        Plugin::drawRow(languageFile.getc("metadata.author"),       trackInfo.artist);
        Plugin::drawRow(languageFile.getc("metadata.genre"),        trackInfo.genre);
        Plugin::drawRow(languageFile.getc("metadata.copyright"),    trackInfo.year);
        Plugin::drawRow(languageFile.getc("metadata.ripper"),       diskInfo.ripper);
        Plugin::drawRow(languageFile.getc("metadata.converter"),    diskInfo.converter);
        Plugin::drawRow(languageFile.getc("metadata.replay"),       diskInfo.replay);
        Plugin::drawRow(languageFile.getc("metadata.hardware"),     diskInfo.trk.hw);
        if (diskInfo.trk.asid)
        {
            auto& style = ImGui::GetStyle();
            auto textSize = ImGui::CalcTextSize("[ASID]");
            auto textOffset = (ImGui::GetContentRegionAvailWidth() + style.FramePadding.x * 2) - textSize.x;
            ImGui::SameLine(textOffset, 0);
            ImGui::Text("[ASID]");
        }
        Plugin::endTable();
    }
}
