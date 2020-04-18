#include "playerframe.h"

#include "../imgui/imgui.h"
#include "../IconsMaterialDesignIcons_c.h"
#include "../strings.h"

PlayerFrame::PlayerFrame() {
}

PlayerFrame::~PlayerFrame() {
}

void PlayerFrame::renderTitleAndSeekBar(const FrameData& frameData) {
    auto title = frameData.metaData.trackInformation.title.empty()
        ? frameData.metaData.diskInformation.title.empty()
            ? STR_TRACK_NO_TITLE
            : frameData.metaData.diskInformation.title.c_str()
        : frameData.metaData.trackInformation.title.c_str();

    switch (frameData.state) {
        case SoundEngine::State::STARTED:
        case SoundEngine::State::PAUSED:
            ImGui::Text(STR_PLAYING_S, title);
            break;
        default:
            ImGui::TextUnformatted(ICON_MDI_MUSIC " Playing: None.");
            break;
    }
    ImGui::Spacing();
}

void PlayerFrame::renderButtonBar(const FrameData& frameData, bool disabled, std::function<void (ButtonId)> onButtonClick) {
    const auto& style = ImGui::GetStyle();
    
    const auto numberOfButton = 4;
    const auto totalSpacing = (numberOfButton-1) * style.ItemSpacing.x;
    const auto buttonWidth = (ImGui::GetContentRegionAvailWidth()-totalSpacing) / numberOfButton;
    
    if (ImGui::Button(frameData.state == SoundEngine::State::STARTED ? STR_BUTTON_PAUSE : STR_BUTTON_PLAY, ImVec2(buttonWidth, 64))) {
        onButtonClick(PLAY);
    }
    ImGui::SameLine();
    if (ImGui::Button(STR_BUTTON_STOP, ImVec2(buttonWidth, 64))) {
        onButtonClick(STOP);
    }
    ImGui::SameLine();
    if (ImGui::Button(STR_BUTTON_BACK, ImVec2(buttonWidth, 64))) {
        onButtonClick(PREV);
    }
    ImGui::SameLine();
    if (ImGui::Button(STR_BUTTON_NEXT, ImVec2(buttonWidth, 64))) {
        onButtonClick(NEXT);
    }
}

void PlayerFrame::render(const FrameData& frameData, std::function<void (ButtonId)> onButtonClick) {
    const auto disabled = frameData.state != SoundEngine::State::PAUSED
        && frameData.state != SoundEngine::State::STARTED;
    
    renderTitleAndSeekBar(frameData);
    renderButtonBar(frameData, disabled, onButtonClick);
}
