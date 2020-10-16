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
#include "Plugin.h"

#include <filesystem>
#include <SDL2/SDL.h>
#include <fmt/format.h>

#include <imgui/imgui.h>


Plugin::Plugin()
{
}

Plugin::~Plugin()
{
}

void Plugin::setup(Config config)
{
    mConfig = config.getGroupOrCreate(getName());
}

void Plugin::cleanup()
{
}

bool Plugin::beginTable(std::string id, bool scrollable, bool twoColumns, float firstColumnWeight)
{
    auto tableFlags = ImGuiTableFlags_RowBg
        | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV;

    if (scrollable)
        tableFlags |= ImGuiTableFlags_ScrollY;

    if (!ImGui::BeginTable(id.c_str(), twoColumns ? 2 : 1, tableFlags))
        return false;

    if (scrollable)
        ImGui::TableSetupScrollFreeze(0, 1);

    if (twoColumns)
    {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_None, firstColumnWeight);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_None);
    }
    else
    {
        ImGui::TableSetupColumn(id.c_str(), ImGuiTableColumnFlags_None);
        ImGui::TableHeadersRow();
    }

    return true;
}

void Plugin::endTable()
{
    ImGui::EndTable();
}

void Plugin::drawRow(std::string label, std::string value)
{
    ImGui::TableNextColumn(); ImGui::TextUnformatted(label.c_str());
    ImGui::TableNextColumn();
    if (value.length() > 0)
        ImGui::TextUnformatted(value.c_str());
    else
        ImGui::TextDisabled("--");
}
