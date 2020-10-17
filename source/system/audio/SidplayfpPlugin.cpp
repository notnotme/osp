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
#include "SidplayfpPlugin.h"

#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <fmt/format.h>

#include <SDL2/SDL.h>
#include <sidplayfp/SidConfig.h>
#include <sidplayfp/SidInfo.h>
#include <sidplayfp/SidTuneInfo.h>
#include <sidplayfp/builders/residfp.h>

#include <imgui/imgui.h>

#include "../../config.h"


SidplayfpPlugin::SidplayfpPlugin() :
Plugin(),
mPlayer(nullptr),
mBuilder(nullptr),
mTune(nullptr),
mMutex(SDL_CreateMutex()),
mCurrentTrack(0),
mTrackCount(0)
{
}

SidplayfpPlugin::~SidplayfpPlugin()
{
    SDL_DestroyMutex(mMutex);
}

std::string SidplayfpPlugin::getName()
{
    return "sidplayfp";
}

std::string SidplayfpPlugin::getVersion()
{
    return fmt::format("{:d}.{:d}.{:d}", LIBSIDPLAYFP_VERSION_MAJ, LIBSIDPLAYFP_VERSION_MIN, LIBSIDPLAYFP_VERSION_LEV);
}

std::vector<std::string> SidplayfpPlugin::getSupportedExtensions()
{
    return
    {
        ".sid", ".psid", ".rsid", ".mus"
    };
}

void SidplayfpPlugin::setup(Config config)
{
    Plugin::setup(config);

    mPlayer = new sidplayfp();

    try
    {
        mKernalRom = loadRom(fmt::format("{:s}c64roms/kernal", DATAPATH));
        mBasicRom = loadRom(fmt::format("{:s}c64roms/basic", DATAPATH));
        mChargenRom = loadRom(fmt::format("{:s}c64roms/chargen", DATAPATH));
        mPlayer->setRoms(mKernalRom.data(), mBasicRom.data(), mChargenRom.data());
    }
    catch(const std::exception& e)
    {
        TRACE("meh meh");
    }

    mBuilder = new ReSIDfpBuilder("OSP");
    mBuilder->create(mPlayer->info().maxsids());
    if (!mBuilder->getStatus())
    {
        throw std::runtime_error(mBuilder->error());
    }
}

void SidplayfpPlugin::cleanup()
{
    if (mPlayer != nullptr)
    {
        if (mPlayer->isPlaying())
        {
            mPlayer->stop();
        }

        delete mPlayer;
    }

    if (mBuilder != nullptr)
    {
        delete mBuilder;
    }

    if (mTune != nullptr)
    {
        delete mTune;
    }

    Plugin::cleanup();
}

void SidplayfpPlugin::open(const std::vector<uint8_t>& buffer)
{
    auto digiBoost = mConfig.get("enable_digiboost", false);
    auto fastSampling = mConfig.get("enable_fast_sampling", false);
    auto samplingMethod = mConfig.get("sampling_method", SidConfig::RESAMPLE_INTERPOLATE);
    bool alwaysStartFirstTrack = mConfig.get("first_track_always", true);

    SidConfig cfg;
    cfg.frequency = 48000;
    cfg.samplingMethod = (SidConfig::sampling_method_t) samplingMethod;
    cfg.fastSampling = fastSampling;
    cfg.digiBoost = digiBoost;
    cfg.playback = SidConfig::STEREO;
    cfg.sidEmulation = mBuilder;

    if (!mPlayer->config(cfg))
    {
        throw std::runtime_error(mPlayer->error());
    }

    mTune = new SidTune(buffer.data(), buffer.size());
    if (mTune->getStatus() == false)
    {
        throw std::runtime_error(mTune->statusString());
    }

    // Load tune into engine
    auto musicInfo = mTune->getInfo();
    mTrackCount = musicInfo->songs();

    mCurrentTrack = alwaysStartFirstTrack ? 1 : 0;
    mTune->selectSong(mCurrentTrack);
    if (!mPlayer->load(mTune))
    {
        throw std::runtime_error(mPlayer->error());
    }
}

int SidplayfpPlugin::getCurrentTrack()
{
    return mCurrentTrack;
}

int SidplayfpPlugin::getTrackCount()
{
    return mTrackCount;
}

void SidplayfpPlugin::setSubSong(int subsong)
{
    if (mPlayer == nullptr || mTune == nullptr)
    {
        return;
    }

    if (subsong > 0 && subsong <= mTrackCount)
    {
        SDL_LockMutex(mMutex);
        mPlayer->load(nullptr);
        mTune->selectSong(subsong);
        mPlayer->load(mTune);
        SDL_UnlockMutex(mMutex);
    }
}

void SidplayfpPlugin::close()
{
    if (mPlayer != nullptr)
    {
        mPlayer->stop();
        mPlayer->load(nullptr);
    }

    if (mTune != nullptr)
    {
        delete mTune;
        mTune = nullptr;
    }

    mCurrentTrack = 0;
    mTrackCount = 0;
}

bool SidplayfpPlugin::decode(uint8_t* stream, size_t len)
{
    if (mPlayer == nullptr || mTune == nullptr)
    {
        return false;
    }

    SDL_LockMutex(mMutex);
    auto size = (uint_least32_t) len / 2;
    auto played = mPlayer->play((short*) stream, size);
    SDL_UnlockMutex(mMutex);

    if (played < size && mPlayer->isPlaying())
    {
        throw std::runtime_error(mPlayer->error());
    }

    return true;
}

void SidplayfpPlugin::drawSettings(ECS::World* world, LanguageFile languageFile, float deltaTime)
{
    bool alwaysStartFirstTrack = mConfig.get("first_track_always", true);
    if (ImGui::Checkbox(languageFile.getc("plugin.always_start_first_track"), &alwaysStartFirstTrack))
    {
        mConfig.set("first_track_always", alwaysStartFirstTrack);
    }

    auto digiBoost = mConfig.get("enable_digiboost", false);
    if (ImGui::Checkbox(languageFile.getc("plugin.enable_digiboost"), &digiBoost))
    {
        mConfig.set("enable_digiboost", digiBoost);
    }

    auto fastSampling = mConfig.get("enable_fast_sampling", false);
    if (ImGui::Checkbox(languageFile.getc("plugin.enable_fast_sampling"), &fastSampling))
    {
        mConfig.set("enable_fast_sampling", fastSampling);
    }

    auto samplingMethod = mConfig.get("sampling_method", SidConfig::RESAMPLE_INTERPOLATE);
    if (ImGui::Combo(languageFile.getc("plugin.sampling_method"), &samplingMethod, "Interpolate\0Resample interpolate\0"))
    {
        mConfig.set("sampling_method", samplingMethod);
    }
}

void SidplayfpPlugin::drawPlayerStats(ECS::World* world, LanguageFile languageFile, float deltaTime)
{
    if (mTune == nullptr)
    {
        return;
    }

    SDL_LockMutex(mMutex);
    auto musicInfo = mTune->getInfo();
    auto title = musicInfo->infoString(0);
    mCurrentTrack = musicInfo->currentSong(); // Never change while playing ?
    SDL_UnlockMutex(mMutex);

    if (Plugin::beginTable(languageFile.getc("player"), false))
    {
        Plugin::drawRow(languageFile.getc("player.title"),    title);
        Plugin::drawRow(languageFile.getc("player.track"),    fmt::format("{:d}/{:d}", mCurrentTrack, mTrackCount));
        Plugin::endTable();
    }
}

void SidplayfpPlugin::drawMetadata(ECS::World* world, LanguageFile languageFile, float deltaTime)
{
    if (mTune == nullptr)
    {
        return;
    }

    SDL_LockMutex(mMutex);
    auto musicInfo = mTune->getInfo();
    auto author = musicInfo->infoString(1);
    auto copyright =  musicInfo->infoString(2);
    auto sidModel = musicInfo->sidModel(0);
    auto clockSpeed = musicInfo->clockSpeed();
    auto compatibility = musicInfo->compatibility();
    SDL_UnlockMutex(mMutex);

    if (Plugin::beginTable(languageFile.getc("metadata"), false))
    {
        Plugin::drawRow(languageFile.getc("metadata.author"),               author);
        Plugin::drawRow(languageFile.getc("metadata.copyright"),            copyright);

        ImGui::TableNextColumn(); ImGui::TextUnformatted(languageFile.getc("metadata.sid_model"));
        ImGui::TableNextColumn();
        if (sidModel == SidTuneInfo::SIDMODEL_6581)         ImGui::TextUnformatted("6581");
        else if (sidModel == SidTuneInfo::SIDMODEL_8580)    ImGui::TextUnformatted("8580");
        else if (sidModel == SidTuneInfo::SIDMODEL_ANY)     ImGui::Text("%s", languageFile.getc("ANY"));
        else if (sidModel == SidTuneInfo::SIDMODEL_UNKNOWN) ImGui::Text("%s", languageFile.getc("UNKNOWN"));

        ImGui::TableNextColumn(); ImGui::TextUnformatted(languageFile.getc("metadata.clock_speed"));
        ImGui::TableNextColumn();
        if (clockSpeed == SidTuneInfo::CLOCK_NTSC)          ImGui::TextUnformatted("NTSC");
        else if (clockSpeed == SidTuneInfo::CLOCK_PAL)      ImGui::TextUnformatted("PAL");
        else if (clockSpeed == SidTuneInfo::CLOCK_ANY)      ImGui::Text("%s", languageFile.getc("ANY"));
        else if (clockSpeed == SidTuneInfo::CLOCK_UNKNOWN)  ImGui::Text("%s", languageFile.getc("UNKNOWN"));

        ImGui::TableNextColumn(); ImGui::TextUnformatted(languageFile.getc("metadata.sid_compatibility"));
        ImGui::TableNextColumn();
        if (compatibility == SidTuneInfo::COMPATIBILITY_BASIC)      ImGui::TextUnformatted("BASIC");
        else if (compatibility == SidTuneInfo::COMPATIBILITY_C64)   ImGui::TextUnformatted("C64");
        else if (compatibility == SidTuneInfo::COMPATIBILITY_PSID)  ImGui::TextUnformatted("PSID");
        else if (compatibility == SidTuneInfo::COMPATIBILITY_R64)   ImGui::TextUnformatted("R64");

        Plugin::endTable();
    }

    auto commentCount = musicInfo->numberOfCommentStrings();
    if (commentCount > 0)
    {
        ImGui::Spacing();
        if (Plugin::beginTable(languageFile.getc("metadata.comments"), true, false))
        {
            auto& io = ImGui::GetIO();
            ImGui::PushFont(io.Fonts->Fonts[1]);
            ImGui::TableNextColumn();
            for (unsigned int i=0; i<commentCount; ++i)
            {
                ImGui::TextUnformatted("%s\n", musicInfo->commentString(i));
            }
            ImGui::PopFont();
            Plugin::endTable();
        }
    }
}

std::vector<uint8_t> SidplayfpPlugin::loadRom(const std::string path)
{
    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
    {
        throw std::runtime_error("Failed to open the requested file");
    }

    std::ifstream ifs(path, std::ios::binary | std::ios::in | std::ios::ate);
    auto romSize = ifs.tellg();

    auto buffer = std::vector<uint8_t>(romSize, 0);
    ifs.seekg(0, std::ios::beg);
    if (!ifs.good())
    {
        ifs.close();
        throw std::runtime_error("The requested file not readable");
    }

    ifs.read((char*) buffer.data(), buffer.size());
    ifs.close();

    return buffer;
}
