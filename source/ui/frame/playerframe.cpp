#include "playerframe.h"

#include "../../imgui/imgui.h"
#include "../../strings.h"

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

void PlayerFrame::renderButtonBar(const FrameData& frameData, bool disabled,
    std::function<void (ButtonId)> onButtonClick) {
    
    const auto& style = ImGui::GetStyle();
    const auto playButtonFrame = frameData.state == SoundEngine::State::STARTED ? frameData.pauseFrame : frameData.playFrame;
    const auto startX = (ImGui::GetContentRegionAvailWidth()/2 - (playButtonFrame.size.x * 4)/2 - style.ItemSpacing.x * 6);

    ImGui::NewLine();
    ImGui::NewLine();
    ImGui::SameLine(startX);
    ImGui::PushID(ImGui::GetID("playerButtonPlay"));
    if (ImGui::ImageButton((ImTextureID)(intptr_t) frameData.texture, playButtonFrame.size, playButtonFrame.uv0, playButtonFrame.uv1)) {
        onButtonClick(PLAY);
    }
    ImGui::PopID();

    ImGui::SameLine();
    
    ImGui::PushID(ImGui::GetID("playerButtonStop"));
    if (ImGui::ImageButton((ImTextureID)(intptr_t) frameData.texture, frameData.stopFrame.size, frameData.stopFrame.uv0, frameData.stopFrame.uv1)) {
        onButtonClick(STOP);
    }
    ImGui::PopID();
    
    ImGui::SameLine();
    
    ImGui::PushID(ImGui::GetID("playerButtonPrev"));
    if (ImGui::ImageButton((ImTextureID)(intptr_t) frameData.texture, frameData.prevFrame.size, frameData.prevFrame.uv0, frameData.prevFrame.uv1)) {
        onButtonClick(PREV);
    }
    ImGui::PopID();
    
    ImGui::SameLine();
    
    ImGui::PushID(ImGui::GetID("playerButtonNext"));
    if (ImGui::ImageButton((ImTextureID)(intptr_t) frameData.texture, frameData.nextFrame.size, frameData.nextFrame.uv0, frameData.nextFrame.uv1)) {
        onButtonClick(NEXT);
    }
    ImGui::PopID();
}

void PlayerFrame::render(const FrameData& frameData,
    std::function<void (ButtonId)> onButtonClick) {
    
    const auto disabled = frameData.state != SoundEngine::State::PAUSED
        && frameData.state != SoundEngine::State::STARTED;
    
    renderTitleAndSeekBar(frameData);
    renderButtonBar(frameData, disabled, onButtonClick);
}
