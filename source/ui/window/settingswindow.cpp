#include "settingswindow.h"

#include "../../imgui/imgui.h"
#include "../../strings.h"
#include "../../decoder/sc68/sc68_settings_strings.h"

#include <sc68/sc68.h>

SettingsWindow::SettingsWindow() :
    Window(STR_SETTINGS_WINDOW_TITLE) {
}

SettingsWindow::~SettingsWindow() {
}

void SettingsWindow::renderOspSettingsTab(const WindowData& windowData,
    std::function<void (ToggleAppSetting)> onToggleSetting) {

    if (ImGui::BeginTabItem(STR_APPLICATION "##applicationTab")) {
        ImGui::TextUnformatted(STR_GENERAL);
        ImGui::Separator();
        
        bool mouseEmulationEnabled = windowData.settings->getBool("app-mouseEmulation", true);
        if (ImGui::Checkbox(STR_SETTINGS_MOUSE_EMULATION, &mouseEmulationEnabled)) {
            onToggleSetting(MOUSE_EMULATION);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_MOUSE_EMULATION);
        }

        bool touchEnabled = windowData.settings->getBool("app-touchEnabled", true);
        if (ImGui::Checkbox(STR_SETTINGS_TOUCH_ENABLED, &touchEnabled)) {
            onToggleSetting(TOUCH_ENABLED);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_TOUCH_ENABLE);
        }

        ImGui::NewLine();
        ImGui::TextUnformatted(STR_AUDIO);
        ImGui::Separator();

        bool autoSkipUnsupported = windowData.settings->getBool("app-skipUnsupportedTunes", true);
        if (ImGui::Checkbox(STR_SKIP_UNSUPPORTED_FILES, &autoSkipUnsupported)) {
            onToggleSetting(AUTOSKIP_UNSUPPORTED_FILES);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_SKIP_UNSUPPORTED_FILES);
        }

        bool alwaysStartFirstTrack = windowData.settings->getBool("app-alwaysStartFirstTune", false);
        if (ImGui::Checkbox(STR_ALWAYS_START_FIRST_TUNE, &alwaysStartFirstTrack)) {
            onToggleSetting(ALWAYS_START_FIRST_TRACK);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_ALWAYS_START_FIRST_TUNE);
        }

        bool skipSubTunes = windowData.settings->getBool("app-skipSubTunes", false);
        if (ImGui::Checkbox(STR_SKIP_SUBTUNES, &skipSubTunes)) {
            onToggleSetting(SKIP_SUBTUNES);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_SKIP_SUBTUNES);
        }

        ImGui::EndTabItem();
    }
}

void SettingsWindow::renderSc68DecoderTab(const WindowData& windowData,
    std::function<void (std::string key, int value)> onDecoderIntSettingChanged,
    std::function<void (std::string key, bool value)> onDecoderBoolSettingChanged) {

    if (ImGui::BeginTabItem("SC68##sc68Tab")) {
        auto loopCount = windowData.settings->getInt(KEY_LOOP_COUNT, LOOP_COUNT_DEFAULT);
        if (loopCount == -1) loopCount = 1;
        if (ImGui::Combo("Loop mode", &loopCount, "Default\0Infinite\0""2\0""3\0""4\0")) {
            if (loopCount == 1) loopCount = -1;
            onDecoderIntSettingChanged(KEY_LOOP_COUNT, loopCount);
        }

        auto acidifierChecked = windowData.settings->getBool(KEY_ACIDIFIER, ACIDIFIER_DEFAULT);
        if(ImGui::Checkbox(STR_ENABLE" aSIDifier##sc68Acidifier", &acidifierChecked)) {
            onDecoderBoolSettingChanged(KEY_ACIDIFIER, acidifierChecked);
        }

        if (acidifierChecked) {
            ImGui::TextUnformatted("aSIDifier mode");
            ImGui::Separator();
            auto acidifierFlags = windowData.settings->getInt(KEY_ACIDIFIER_FLAGS, ACIDIFIER_FLAGS_DEFAULT);

            auto acidifierForce = (acidifierFlags & SC68_ASID_FORCE) == SC68_ASID_FORCE;
            if(ImGui::Checkbox("Force##sc68AcidifierForce", &acidifierForce)) {
                if (acidifierForce) acidifierFlags |= SC68_ASID_FORCE;
                else acidifierFlags &= ~SC68_ASID_FORCE;
                onDecoderIntSettingChanged(KEY_ACIDIFIER_FLAGS, acidifierFlags);
            }

            auto acidifierNOA = (acidifierFlags & SC68_ASID_NO_A) == SC68_ASID_NO_A;
            if(ImGui::Checkbox("No A##sc68AcidifierNOA", &acidifierNOA)) {
                if (acidifierNOA) acidifierFlags |= SC68_ASID_NO_A;
                else acidifierFlags &= ~SC68_ASID_NO_A;
                onDecoderIntSettingChanged(KEY_ACIDIFIER_FLAGS, acidifierFlags);
            }

            auto acidifierNOB = (acidifierFlags & SC68_ASID_NO_B) == SC68_ASID_NO_B;
            if(ImGui::Checkbox("No B##sc68AcidifierNOB", &acidifierNOB)) {
                if (acidifierNOB) acidifierFlags |= SC68_ASID_NO_B;
                else acidifierFlags &= ~SC68_ASID_NO_B;
                onDecoderIntSettingChanged(KEY_ACIDIFIER_FLAGS, acidifierFlags);
            }

            auto acidifierNOC = (acidifierFlags & SC68_ASID_NO_C) == SC68_ASID_NO_C;
            if(ImGui::Checkbox("No C##sc68AcidifierNOC", &acidifierNOC)) {
                if (acidifierNOC) acidifierFlags |= SC68_ASID_NO_C;
                else acidifierFlags &= ~SC68_ASID_NO_C;
                onDecoderIntSettingChanged(KEY_ACIDIFIER_FLAGS, acidifierFlags);
            }
        }
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
    std::function<void (ToggleAppSetting)> onToggleSetting,
    std::function<void (std::string key, int value)> onDecoderIntSettingChanged,
    std::function<void (std::string key, bool value)> onDecoderBoolSettingChanged) {

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
        renderSc68DecoderTab(windowData, onDecoderIntSettingChanged, onDecoderBoolSettingChanged);
        //renderSidplayDecoderTab();
        //renderGmeDecoderTab();
        //renderDumbDecoderTab();
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
