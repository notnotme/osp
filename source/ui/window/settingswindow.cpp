#include "settingswindow.h"

#include "../../imgui/imgui.h"
#include "../../strings.h"

SettingsWindow::SettingsWindow() :
    Window(STR_SETTINGS_WINDOW_TITLE) {
}

SettingsWindow::~SettingsWindow() {
}

void SettingsWindow::renderOspSettingsTab(const WindowData& windowData,
    std::function<void (ToggleSetting)> onToggleSetting) {

    if (ImGui::BeginTabItem(STR_APPLICATION "##applicationTab")) {
        ImGui::NewLine();
        ImGui::TextUnformatted(STR_GENERAL);
        ImGui::Separator();
        
        bool mouseEmulationEnabled = windowData.mouseEmulationEnabled;
        if (ImGui::Checkbox(STR_SETTINGS_MOUSE_EMULATION, &mouseEmulationEnabled)) {
            onToggleSetting(MOUSE_EMULATION);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_MOUSE_EMULATION);
        }

        bool touchEnabled = windowData.touchEnabled;
        if (ImGui::Checkbox(STR_SETTINGS_TOUCH_ENABLED, &touchEnabled)) {
            onToggleSetting(TOUCH_ENABLED);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_TOUCH_ENABLE);
        }

        ImGui::NewLine();
        ImGui::TextUnformatted(STR_AUDIO);
        ImGui::Separator();

        bool autoSkipUnsupported = windowData.skipUnsupportedTunes;
        if (ImGui::Checkbox(STR_SKIP_UNSUPPORTED_FILES, &autoSkipUnsupported)) {
            onToggleSetting(AUTOSKIP_UNSUPPORTED_FILES);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_SKIP_UNSUPPORTED_FILES);
        }

        bool alwaysStartFirstTrack = windowData.alwaysStartFirstTune;
        if (ImGui::Checkbox(STR_ALWAYS_START_FIRST_TUNE, &alwaysStartFirstTrack)) {
            onToggleSetting(ALWAYS_START_FIRST_TRACK);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_ALWAYS_START_FIRST_TUNE);
        }

        bool skipSubTunes = windowData.skipSubTunes;
        if (ImGui::Checkbox(STR_SKIP_SUBTUNES, &skipSubTunes)) {
            onToggleSetting(SKIP_SUBTUNES);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_SKIP_SUBTUNES);
        }

        ImGui::EndTabItem();
    }
}

void SettingsWindow::renderSc68DecoderTab() {
    if (ImGui::BeginTabItem("SC68##sc68Tab")) {
        ImGui::TextUnformatted("TODO");
        ImGui::EndTabItem();
    }
}

void SettingsWindow::renderSidplayDecoderTab() {
    if (ImGui::BeginTabItem("Sidplayfp##sidplayTab")) {
        ImGui::TextUnformatted("TODO");
        ImGui::EndTabItem();
    }
}

void SettingsWindow::renderGmeDecoderTab() {
    if (ImGui::BeginTabItem("Gme##gmeTab")) {
        ImGui::TextUnformatted("TODO");
        ImGui::EndTabItem();
    }
}

void SettingsWindow::renderDumbDecoderTab() {
    if (ImGui::BeginTabItem("Dumb##dumbTab")) {
        ImGui::TextUnformatted("TODO");
        ImGui::EndTabItem();
    }
}

void SettingsWindow::render(const WindowData& windowData,
    std::function<void (ToggleSetting)> onToggleSetting) {

    if (!mVisible) {
        return;
    }

    const auto io = ImGui::GetIO();
    const auto windowFlags = ImGuiWindowFlags_None;
    const auto style = ImGui::GetStyle();

    ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x/2, io.DisplaySize.y/2), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (!ImGui::Begin(STR_SETTINGS_WINDOW_TITLE, &mVisible, windowFlags)) {
        ImGui::End();
        return;
    }

    auto tabBarFlags = ImGuiTabBarFlags_NoTooltip;
    if (ImGui::BeginTabBar("ospSettingsTab", tabBarFlags)) {
        renderOspSettingsTab(windowData, onToggleSetting);
        renderSc68DecoderTab();
        renderSidplayDecoderTab();
        renderGmeDecoderTab();
        renderDumbDecoderTab();
        ImGui::EndTabBar();
    }

    const auto spaceAvail = ImGui::GetContentRegionAvail();
    const auto closeButtonTextSize = ImGui::CalcTextSize(STR_CLOSE);

    ImGui::Dummy(ImVec2(0, spaceAvail.y - closeButtonTextSize.y - style.WindowPadding.y - (style.ItemInnerSpacing.y*2) - (style.ItemSpacing.y*4)));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SetCursorPosX(spaceAvail.x - style.WindowPadding.x - closeButtonTextSize.x);
    if (ImGui::Button(STR_CLOSE "##closeSettings")) {
        mVisible = false;
    }

    ImGui::End();
}
