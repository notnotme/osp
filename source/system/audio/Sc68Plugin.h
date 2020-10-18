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
#include <string>

#include <sc68/sc68.h>
#include <ECS.h>

#include "Plugin.h"
#include "../../tools/ConfigFile.h"
#include "../../tools/LanguageFile.h"


class Sc68Plugin :
public Plugin
{
public:
    Sc68Plugin();
    virtual ~Sc68Plugin();

    virtual std::string getName() override;
    virtual std::string getVersion() override;
    virtual std::vector<std::string> getSupportedExtensions() override;

    virtual void setup(Config config) override;
    virtual void cleanup() override;
    virtual void open(const std::vector<uint8_t>& buffer) override;
    virtual void close() override;
    virtual bool decode(uint8_t* stream, size_t len) override;

    virtual int getCurrentTrack();
    virtual int getTrackCount();
    virtual void setSubSong(int subsong);

    virtual void drawSettings(ECS::World* world, LanguageFile languageFile, float deltaTime) override;
    virtual void drawPlayerStats(ECS::World* world, LanguageFile languageFile, float deltaTime) override;
    virtual void drawMetadata(ECS::World* world, LanguageFile languageFile, float deltaTime) override;

private:
    sc68_create_t mSC68Config;
    sc68_t* mSC68;
    int mCurrentTrack;
    int mTrackCount;

    Sc68Plugin(const Sc68Plugin& copy);
};
