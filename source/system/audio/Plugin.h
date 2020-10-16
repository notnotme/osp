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

#include <ECS.h>

#include "../../tools/ConfigFile.h"
#include "../../tools/LanguageFile.h"


class Plugin
{
public:
    Plugin();
    virtual ~Plugin();

    virtual std::string getName() = 0;
    virtual std::string getVersion() = 0;
    virtual std::vector<std::string> getSupportedExtensions() = 0;

    virtual void setup(Config config);
    virtual void cleanup();
    virtual void open(const std::vector<uint8_t>& buffer) = 0;
    virtual void close() = 0;
    virtual bool decode(uint8_t* stream, size_t len) = 0;

    virtual int getCurrentTrack() = 0;
    virtual int getTrackCount() = 0;
    virtual void setSubSong(int subsong) = 0;

    virtual void drawSettings(ECS::World* world, LanguageFile languageFile, float deltaTime) = 0;
    virtual void drawPlayerStats(ECS::World* world, LanguageFile languageFile, float deltaTime) = 0;
    virtual void drawMetadata(ECS::World* world, LanguageFile languageFile, float deltaTime) = 0;

protected:
    Config mConfig;
    bool beginTable(std::string label, bool scrollable, bool twoColumns = true, float firstColumnWeight = 0.4f);
    void endTable();
    void drawRow(std::string label, std::string value);

private:
    Plugin(const Plugin& copy);
};
