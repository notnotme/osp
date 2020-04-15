#include "metadataframe.h"

#include "../imgui/imgui.h"

MetaDataFrame::MetaDataFrame() {
}

MetaDataFrame::~MetaDataFrame() {
}

void MetaDataFrame::renderDiskInformation(const FrameData& frameData) {
    if (!ImGui::BeginChild(ImGui::GetID("diskMetaData"), ImVec2(0,ImGui::GetContentRegionAvail().y * 0.25f), false)) {
        ImGui::EndChild();
    }

    char temp[32];

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::TextUnformatted("Disk information");
    ImGui::Separator();

    if (!frameData.metaData.diskInformation.title.empty()) {
        ImGui::LabelText(frameData.metaData.diskInformation.title.c_str(), "Title");
    }

    if (!frameData.metaData.diskInformation.ripper.empty()) {
        ImGui::LabelText(frameData.metaData.diskInformation.ripper.c_str(), "Ripper");
    }

    if (!frameData.metaData.diskInformation.converter.empty()) {
        ImGui::LabelText(frameData.metaData.diskInformation.converter.c_str(), "Converter");
    }

    if (!frameData.metaData.diskInformation.copyright.empty()) {
        ImGui::LabelText(frameData.metaData.diskInformation.copyright.c_str(), "Copyright");
    }

    if (frameData.metaData.diskInformation.trackCount > 1) {
        sprintf(temp, "%d", frameData.metaData.diskInformation.trackCount);    
        ImGui::LabelText(temp, "Track count");
    }

    if (frameData.metaData.diskInformation.duration > 0) {
        sprintf(temp, "%d:%02d", frameData.metaData.diskInformation.duration / 60, frameData.metaData.diskInformation.duration % 60);    
        ImGui::LabelText(temp, "Total duration");
    }
    
    ImGui::EndChild();
}

void MetaDataFrame::renderTrackInformation(const FrameData& frameData) {
    if (!ImGui::BeginChild(ImGui::GetID("trackMetaData"), ImVec2(0,ImGui::GetContentRegionAvail().y), false)) {
        ImGui::EndChild();
    }

    char temp[32];

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::TextUnformatted("Track information");
    ImGui::Separator();

    if (!frameData.metaData.trackInformation.title.empty()) {
        ImGui::LabelText(frameData.metaData.trackInformation.title.c_str(), "Title");
    }

    if (!frameData.metaData.trackInformation.author.empty()) {
        ImGui::LabelText(frameData.metaData.trackInformation.author.c_str(), "Author");
    }

    if (!frameData.metaData.trackInformation.copyright.empty()) {
        ImGui::LabelText(frameData.metaData.trackInformation.copyright.c_str(), "Copyright");
    }

    if (frameData.metaData.diskInformation.trackCount > 1) {
        sprintf(temp, "%d/%d", frameData.metaData.trackInformation.trackNumber, frameData.metaData.diskInformation.trackCount);    
        ImGui::LabelText(temp, "Track number");
    }

    if (frameData.metaData.trackInformation.position > -1) {
        sprintf(temp, "%d:%02d", frameData.metaData.trackInformation.position / 60, frameData.metaData.trackInformation.position % 60);    
        ImGui::LabelText(temp, "Play time");
    } 

    if (frameData.metaData.trackInformation.duration > 0) {
        sprintf(temp, "%d:%02d", frameData.metaData.trackInformation.duration / 60, frameData.metaData.trackInformation.duration % 60);    
        ImGui::LabelText(temp, "Duration");
    } 

    if (!frameData.metaData.trackInformation.comment.empty()) {
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::TextUnformatted("Comments");
        ImGui::TextWrapped("%s", frameData.metaData.trackInformation.comment.c_str());
    }

    ImGui::EndChild();
}

void MetaDataFrame::render(const FrameData& frameData) {
    const auto disabled = frameData.state != SoundEngine::State::PAUSED
        && frameData.state != SoundEngine::State::STARTED;

    if (disabled) {
        // Don't draw anything
        return;
    }

    // Disable frame padding so labels are packed together vertically
    // otherwise there is a gap due to our style settings
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
    if (frameData.metaData.hasDiskInformation) {
        renderDiskInformation(frameData);
    }
    renderTrackInformation(frameData);
    ImGui::PopStyleVar();
}
