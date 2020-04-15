#include "explorerframe.h"

#include "../imgui/imgui.h"
#include "../IconsMaterialDesignIcons_c.h"

ExplorerFrame::ExplorerFrame() {
}

ExplorerFrame::~ExplorerFrame() {
}

void ExplorerFrame::renderPath(const FrameData& frameData) {
    ImGui::TextUnformatted(ICON_MDI_FOLDER_OPEN_OUTLINE);
    ImGui::SameLine();
    ImGui::TextUnformatted(frameData.currentPath.c_str());
    ImGui::Spacing();
}

void ExplorerFrame::renderExplorer(const FrameData& frameData, std::function<void (FileSystem::Entry)> onItemClick) {
    const auto tableSize = ImVec2(0, 0);
    const auto tableFlags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollFreezeTopRow | ImGuiTableFlags_RowBg
     | ImGuiTableFlags_BordersHOuter | ImGuiTableFlags_BordersVOuter | ImGuiTableFlags_BordersVInner
     | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersVFullHeight;
    
    if (ImGui::BeginTable("##fileTable", 2, tableFlags, tableSize)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 0.80f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_None, 0.20f);
        ImGui::TableAutoHeaders();

        ImGuiListClipper clipper;
        clipper.Begin(frameData.listing.size());
        while (clipper.Step()) {
            char temp[256];
            for (auto row=clipper.DisplayStart; row<clipper.DisplayEnd; row++) {
                const auto item = frameData.listing[row];
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                sprintf(temp, "%s %s", item.folder ? ICON_MDI_FOLDER_OPEN : ICON_MDI_FILE, item.name.c_str());
                if (ImGui::Selectable(temp, false, ImGuiSelectableFlags_SpanAllColumns)) {
                    onItemClick(item);
                }

                ImGui::TableSetColumnIndex(1);
                sprintf(temp, "%10u Kb", (unsigned int) item.size / 1024);
                ImGui::TextUnformatted(temp);
            }
        }
        ImGui::EndTable();
    }
}

void ExplorerFrame::render(const FrameData& frameData, std::function<void (FileSystem::Entry)> onItemClick) {
    renderPath(frameData);
    if (frameData.isWorking) {
        // Using a little hack to make font bigger
        auto& io = ImGui::GetIO();
        const auto savedScale = io.FontDefault->Scale;
        io.FontDefault->Scale = 2.0f;
        ImGui::PushFont(io.FontDefault);

        // Add a little rotating bar to loading text
        auto strLoading = std::string(ICON_MDI_TIMER_SAND).append(" Loading ");
        strLoading.push_back("|/-\\"[(int)(ImGui::GetTime() / 0.05f) & 3]);
        
        // Center in space
        const auto spaceAvail = ImGui::GetContentRegionAvail();
        const auto textSize = ImGui::CalcTextSize(strLoading.c_str());
        ImGui::SetCursorPosX((spaceAvail.x/2) - textSize.x/2);
        ImGui::SetCursorPosY((spaceAvail.y/2) - textSize.y/2);
        ImGui::Text("%s", strLoading.c_str());

        io.FontDefault->Scale = savedScale;
        ImGui::PopFont();
    } else {
        renderExplorer(frameData, onItemClick);
    }
}

