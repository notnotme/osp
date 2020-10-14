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
#include <functional>

#include <ECS.h>

#include "../../tools/LanguageFile.h"


struct AudioSystemConfiguredEvent
{
    struct PluginInformation
    {
        typedef std::function<void (ECS::World*, LanguageFile, float)> DrawSettings;
        typedef std::function<void (ECS::World*, LanguageFile, float)> DrawStats;

        std::string name;
        std::string version;
        std::vector<std::string> supportedExtensions;
        DrawSettings drawSettings;
        DrawStats drawStats;
    };

    std::vector<PluginInformation> pluginInformations;
};
