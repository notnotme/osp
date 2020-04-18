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

    if (ImGui::BeginTabItem(STR_APPLICATION)) {
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
        ImGui::EndTabItem();
    }
}

void SettingsWindow::renderSc68DecoderTab() {
    if (ImGui::BeginTabItem("SC68")) {
        ImGui::TextUnformatted("TODO");
        ImGui::EndTabItem();
    }
}

void SettingsWindow::renderSidplayDecoderTab() {
    if (ImGui::BeginTabItem("Sidplayfp")) {
        ImGui::TextUnformatted("TODO");
        ImGui::EndTabItem();
    }
}

void SettingsWindow::renderGmeDecoderTab() {
    if (ImGui::BeginTabItem("Gme")) {
        ImGui::TextUnformatted("TODO");
        ImGui::EndTabItem();
    }
}

void SettingsWindow::renderDumbDecoderTab() {
    if (ImGui::BeginTabItem("Dumb")) {
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

    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x/3, io.DisplaySize.y/2), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x/2, io.DisplaySize.y/2), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (!ImGui::Begin(STR_SETTINGS_WINDOW_TITLE, &mVisible, windowFlags)) {
        ImGui::End();
        return;
    }

    auto tabBarFlags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("ospSettingsTab", tabBarFlags)) {
        renderOspSettingsTab(windowData, onToggleSetting);
        renderSc68DecoderTab();
        renderSidplayDecoderTab();
        renderGmeDecoderTab();
        renderDumbDecoderTab();
        ImGui::EndTabBar();
    }

    ImGui::End();
}
