#include "playerframe.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../IconsMaterialDesignIcons_c.h"

PlayerFrame::PlayerFrame() {
}

PlayerFrame::~PlayerFrame() {
}

void PlayerFrame::renderTitleAndSeekBar(const FrameData& frameData) {
    auto title = frameData.metaData.trackInformation.title.empty()
        ? frameData.metaData.diskInformation.title.empty()
            ? "n/a (No title provided)"
            : frameData.metaData.diskInformation.title.c_str()
        : frameData.metaData.trackInformation.title.c_str();

    switch (frameData.state) {
        case SoundEngine::State::STARTED:
        case SoundEngine::State::PAUSED:
            ImGui::Text(ICON_MDI_MUSIC " Playing: %s", title);
            break;
        default:
            ImGui::TextUnformatted(ICON_MDI_MUSIC " Playing: None.");
            break;
    }
}

void PlayerFrame::renderButtonBar(const FrameData& frameData, bool disabled, std::function<void (ButtonId)> onButtonClick) {
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Columns(3, "playerColumns", false);
    ImGui::NextColumn();
    if (ImGui::Button(frameData.state == SoundEngine::State::STARTED ? ICON_MDI_PAUSE : ICON_MDI_PLAY)) {
        onButtonClick(PLAY);
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_MDI_STOP)) {
        onButtonClick(STOP);
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_MDI_SKIP_PREVIOUS)) {
        onButtonClick(PREV);
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_MDI_SKIP_NEXT)) {
        onButtonClick(NEXT);
    }
    ImGui::NextColumn();
    ImGui::Columns(1);
}

void PlayerFrame::render(const FrameData& frameData, std::function<void (ButtonId)> onButtonClick) {
    const auto disabled = frameData.state != SoundEngine::State::PAUSED
        && frameData.state != SoundEngine::State::STARTED;
    
    ImGui::BeginChild(ImGui::GetID("player"), ImVec2(0,ImGui::GetContentRegionAvail().y * 0.2f), false);
    renderTitleAndSeekBar(frameData);
    renderButtonBar(frameData, disabled, onButtonClick);
    ImGui::EndChild();
}
