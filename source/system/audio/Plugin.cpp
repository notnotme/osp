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

bool Plugin::beginTable(std::string label, bool scrollable, bool twoColumns)
{
    auto tableFlags = ImGuiTableFlags_RowBg
        | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_BordersInnerV;

    if (scrollable)
        tableFlags |= ImGuiTableFlags_ScrollY;

    auto labelC = label.c_str();
    if (!ImGui::BeginTable(labelC, twoColumns ? 2 : 1, tableFlags))
        return false;

    if (scrollable)
        ImGui::TableSetupScrollFreeze(0, 1);

    if (twoColumns)
    {
        ImGui::TableSetupColumn(labelC, ImGuiTableColumnFlags_NoHide, 0.4f);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_NoHide);
    }
    else
    {
        ImGui::TableSetupColumn(labelC, ImGuiTableColumnFlags_NoHide);
    }

    ImGui::TableHeadersRow();
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
