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
#include "OpenmptPlugin.h"

#include <fmt/format.h>

#include <imgui/imgui.h>


OpenmptPlugin::OpenmptPlugin() :
Plugin(),
mModule(nullptr),
mMutex(SDL_CreateMutex())

{
}

OpenmptPlugin::~OpenmptPlugin()
{
    SDL_DestroyMutex(mMutex);
}

std::string OpenmptPlugin::getName()
{
    return "openmpt";
}

std::string OpenmptPlugin::getVersion()
{
    return fmt::format("{:s}.{:s}.{:s}",
        openmpt::string::get("library_version_major"),
        openmpt::string::get("library_version_minor"),
        openmpt::string::get("library_version_patch"));
}

std::vector<std::string> OpenmptPlugin::getSupportedExtensions()
{
    auto extensionsWithoutDot = openmpt::get_supported_extensions();
    auto extensionsWithDot = std::vector<std::string>();
    extensionsWithoutDot.reserve(extensionsWithoutDot.size());

    for (auto ext : extensionsWithoutDot)
        extensionsWithDot.push_back(fmt::format(".{:s}", ext));

    return extensionsWithDot;
}

void OpenmptPlugin::setup(Config config)
{
    Plugin::setup(config);
}

void OpenmptPlugin::cleanup()
{
    Plugin::cleanup();
}

void OpenmptPlugin::open(const std::vector<uint8_t>& buffer)
{
    auto amigaRessampler = mConfig.get("emulate_paula_chip", true);
    mLoopEnabled = mConfig.get("loop", false);

    mModule = new openmpt::module(buffer);
    mModule->ctl_set_boolean("render.resampler.emulate_amiga", amigaRessampler);
    mModule->ctl_set_text("play.at_end", mLoopEnabled ? "continue" : "stop");
}

int OpenmptPlugin::getCurrentTrack()
{
    return 1;
}

int OpenmptPlugin::getTrackCount()
{
    return 1;
}

void OpenmptPlugin::setSubSong(int subsong)
{
    if (mModule == nullptr)
        return;
}

void OpenmptPlugin::close()
{
    if (mModule != nullptr)
    {
        delete mModule;
        mModule = nullptr;
    }
}

bool OpenmptPlugin::decode(uint8_t* stream, size_t len)
{
    if (mModule == nullptr)
        return false;

    SDL_LockMutex(mMutex);
    auto size = (int) len / 4;
    auto reads = mModule->read_interleaved_stereo(48000, size, (int16_t*) stream);
    SDL_UnlockMutex(mMutex);

    return reads > 0 || mLoopEnabled;
}

void OpenmptPlugin::drawSettings(ECS::World* world, LanguageFile languageFile, float deltaTime)
{
    auto loop = mConfig.get("loop", false);
    if (ImGui::Checkbox(languageFile.getc("plugin.loop_song"), &loop))
        mConfig.set("loop", loop);

    auto amigaRessampler = mConfig.get("emulate_paula_chip", true);
    if (ImGui::Checkbox(languageFile.getc("plugin.emulate_paula_chip"), &amigaRessampler))
        mConfig.set("emulate_paula_chip", amigaRessampler);
}

void OpenmptPlugin::drawStats(ECS::World* world, LanguageFile languageFile, float deltaTime)
{
    if (mModule == nullptr)
        return;

    SDL_LockMutex(mMutex);
    auto type = mModule->get_metadata("type");
    auto type_long = mModule->get_metadata("type_long");
    auto original_type = mModule->get_metadata("type");
    auto original_type_long = mModule->get_metadata("type_long");
    auto artist = mModule->get_metadata("artist");
    auto tracker = mModule->get_metadata("tracker");
    auto title = mModule->get_metadata("title");
    auto date = mModule->get_metadata("date");
    auto msg = mModule->get_metadata("message");
    auto duration = (int) mModule->get_duration_seconds();
    auto position = (int) mModule->get_position_seconds();
    SDL_UnlockMutex(mMutex);

    if (Plugin::beginTable(languageFile.getc("player"), false))
    {
        Plugin::drawRow(languageFile.getc("player.title"),      title);
        Plugin::drawRow(languageFile.getc("player.track"),      "1/1");
        Plugin::drawRow(languageFile.getc("player.duration"),   fmt::format("{:d}:{:02d}",   duration / 60, duration % 60));
        Plugin::drawRow(languageFile.getc("player.position"),   fmt::format("{:d}:{:02}",    position / 60, position % 60));
        Plugin::endTable();
        ImGui::NewLine();
    }

    if (Plugin::beginTable(languageFile.getc("metadata"),  false))
    {
        Plugin::drawRow(languageFile.getc("metadata.type"),             fmt::format("{:s} ({:s})",   type_long, type));
        Plugin::drawRow(languageFile.getc("metadata.original_type"),    fmt::format("{:s} ({:s})",   original_type_long, original_type));
        Plugin::drawRow(languageFile.getc("metadata.author"),           artist);
        Plugin::drawRow(languageFile.getc("metadata.last_saved"),       date);
        Plugin::drawRow(languageFile.getc("metadata.tracker_used"),     tracker);
        Plugin::endTable();
        ImGui::NewLine();
    }

    if (msg.length() > 0)
    {
        if (Plugin::beginTable(languageFile.getc("metadata.comments_or_samples"), true, false))
        {
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", msg.c_str());
            Plugin::endTable();
        }
    }
}
