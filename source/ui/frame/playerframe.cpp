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
    const auto playSprite = frameData.catalog->getFrame("play");
    const auto pauseSprite = frameData.catalog->getFrame("pause");
    const auto stopSprite = frameData.catalog->getFrame("stop");
    const auto nextSprite = frameData.catalog->getFrame("next");
    const auto prevSprite = frameData.catalog->getFrame("prev");    
    const auto playButtonFrame = frameData.state == SoundEngine::State::STARTED ? pauseSprite : playSprite;
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
    if (ImGui::ImageButton((ImTextureID)(intptr_t) frameData.texture, stopSprite.size, stopSprite.uv0, stopSprite.uv1)) {
        onButtonClick(STOP);
    }
    ImGui::PopID();
    
    ImGui::SameLine();
    
    ImGui::PushID(ImGui::GetID("playerButtonPrev"));
    if (ImGui::ImageButton((ImTextureID)(intptr_t) frameData.texture, prevSprite.size, prevSprite.uv0, prevSprite.uv1)) {
        onButtonClick(PREV);
    }
    ImGui::PopID();
    
    ImGui::SameLine();
    
    ImGui::PushID(ImGui::GetID("playerButtonNext"));
    if (ImGui::ImageButton((ImTextureID)(intptr_t) frameData.texture, nextSprite.size, nextSprite.uv0, nextSprite.uv1)) {
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
