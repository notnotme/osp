#include "settingswindow.h"

#include "../../imgui/imgui.h"
#include "../../strings.h"
#include "../../app_settings_strings.h"
#include "../../decoder/sc68/sc68_settings_strings.h"
#include "../../decoder/sidplayfp/sidplayfp_settings_strings.h"
#include "../../decoder/gme/gme_settings_strings.h"
#include "../../decoder/dumb/dumb_settings_strings.h"

#include <sc68/sc68.h>

SettingsWindow::SettingsWindow() :
    Window(STR_SETTINGS_WINDOW_TITLE) {
}

SettingsWindow::~SettingsWindow() {
}

void SettingsWindow::renderOspSettingsTab(const WindowData& windowData,
    std::function<void (ToggleAppSetting, bool value)> onToggleSetting) {

    if (ImGui::BeginTabItem(STR_APPLICATION "##applicationTab")) {
        bool mouseEmulationEnabled = windowData.settings->getBool(KEY_APP_MOUSE_EMULATION, APP_MOUSE_EMULATION_DEFAULT);
        if (ImGui::Checkbox(STR_SETTINGS_MOUSE_EMULATION, &mouseEmulationEnabled)) {
            onToggleSetting(MOUSE_EMULATION, mouseEmulationEnabled);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_MOUSE_EMULATION);
        }

        bool touchEnabled = windowData.settings->getBool(KEY_APP_TOUCH_ENABLED, APP_TOUCH_ENABLED_DEFAULT);
        if (ImGui::Checkbox(STR_SETTINGS_TOUCH_ENABLED, &touchEnabled)) {
            onToggleSetting(TOUCH_ENABLED, touchEnabled);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_TOUCH_ENABLE);
        }

        bool autoSkipUnsupported = windowData.settings->getBool(KEY_APP_SKIP_UNSUPPORTED_TUNES, APP_SKIP_UNSUPPORTED_TUNES_DEFAULT);
        if (ImGui::Checkbox(STR_SKIP_UNSUPPORTED_FILES, &autoSkipUnsupported)) {
            onToggleSetting(AUTOSKIP_UNSUPPORTED_FILES, autoSkipUnsupported);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_SKIP_UNSUPPORTED_FILES);
        }

        bool alwaysStartFirstTrack = windowData.settings->getBool(KEY_APP_ALWAYS_START_FIRST_TUNE, APP_ALWAYS_START_FIRST_TUNE_DEFAULT);
        if (ImGui::Checkbox(STR_ALWAYS_START_FIRST_TUNE, &alwaysStartFirstTrack)) {
            onToggleSetting(ALWAYS_START_FIRST_TRACK, alwaysStartFirstTrack);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_ALWAYS_START_FIRST_TUNE);
        }

        bool skipSubTunes = windowData.settings->getBool(KEY_APP_SKIP_SUBTUNES, APP_SKIP_SUBTUNES_DEFAULT);
        if (ImGui::Checkbox(STR_SKIP_SUBTUNES, &skipSubTunes)) {
            onToggleSetting(SKIP_SUBTUNES, skipSubTunes);
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
        auto loopCount = windowData.settings->getInt(KEY_SC68_LOOP_COUNT, SC68_LOOP_COUNT_DEFAULT);
        if (loopCount == -1) loopCount = 1;
        if (ImGui::Combo(STR_LOOP, &loopCount, STR_DEFAULT "\0" STR_INFINITE "\0")) {
            if (loopCount == 1) loopCount = -1;
            onDecoderIntSettingChanged(KEY_SC68_LOOP_COUNT, loopCount);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_SC68_LOOP);
        }

        auto acidifierChecked = windowData.settings->getBool(KEY_SC68_ACIDIFIER, SC68_ACIDIFIER_DEFAULT);
        if(ImGui::Checkbox(STR_ENABLE " aSIDifier", &acidifierChecked)) {
            onDecoderBoolSettingChanged(KEY_SC68_ACIDIFIER, acidifierChecked);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_SC68_ENABLE_ASIDIFIER);
        }

        if (acidifierChecked) {
            auto acidifierFlags = windowData.settings->getInt(KEY_SC68_ACIDIFIER_FLAGS, SC68_ACIDIFIER_FLAGS_DEFAULT);

            auto acidifierForce = (acidifierFlags & SC68_ASID_FORCE) == SC68_ASID_FORCE;
            if(ImGui::Checkbox(STR_FORCE " aSIDifier", &acidifierForce)) {
                if (acidifierForce) acidifierFlags |= SC68_ASID_FORCE;
                else acidifierFlags &= ~SC68_ASID_FORCE;
                onDecoderIntSettingChanged(KEY_SC68_ACIDIFIER_FLAGS, acidifierFlags);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(STR_TOOLTIP_SC68_ASIDIFIER_FORCE);
            }
            
            auto acidifierNOA = (acidifierFlags & SC68_ASID_NO_A) == SC68_ASID_NO_A;
            if(ImGui::Checkbox("No A", &acidifierNOA)) {
                if (acidifierNOA) acidifierFlags |= SC68_ASID_NO_A;
                else acidifierFlags &= ~SC68_ASID_NO_A;
                onDecoderIntSettingChanged(KEY_SC68_ACIDIFIER_FLAGS, acidifierFlags);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(STR_TOOLTIP_SC68_ASIDIFIER_NOA);
            }
            ImGui::SameLine();


            auto acidifierNOB = (acidifierFlags & SC68_ASID_NO_B) == SC68_ASID_NO_B;
            if(ImGui::Checkbox("No B", &acidifierNOB)) {
                if (acidifierNOB) acidifierFlags |= SC68_ASID_NO_B;
                else acidifierFlags &= ~SC68_ASID_NO_B;
                onDecoderIntSettingChanged(KEY_SC68_ACIDIFIER_FLAGS, acidifierFlags);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(STR_TOOLTIP_SC68_ASIDIFIER_NOB);
            }
            ImGui::SameLine();

            auto acidifierNOC = (acidifierFlags & SC68_ASID_NO_C) == SC68_ASID_NO_C;
            if(ImGui::Checkbox("No C", &acidifierNOC)) {
                if (acidifierNOC) acidifierFlags |= SC68_ASID_NO_C;
                else acidifierFlags &= ~SC68_ASID_NO_C;
                onDecoderIntSettingChanged(KEY_SC68_ACIDIFIER_FLAGS, acidifierFlags);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(STR_TOOLTIP_SC68_ASIDIFIER_NOC);
            }
        }
        ImGui::EndTabItem();
    }
}

void SettingsWindow::renderSidplayDecoderTab(const WindowData& windowData,
    std::function<void (std::string key, int value)> onDecoderIntSettingChanged,
    std::function<void (std::string key, bool value)> onDecoderBoolSettingChanged) {

    if (ImGui::BeginTabItem("Sidplayfp##sidplayTab")) {
        auto samplingMethod = windowData.settings->getInt(KEY_SIDPLAYFP_SAMPLING_METHOD, SIDPLAYFP_SAMPLING_METHOD_DEFAULT);
        if (ImGui::Combo(STR_SAMPLING_MODE, &samplingMethod, STR_INTERPOLATE "\0" STR_RESAMPLE_INTERPOLATE "\0")) {
            onDecoderIntSettingChanged(KEY_SIDPLAYFP_SAMPLING_METHOD, samplingMethod);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_SIDPLAY_SAMPLING_MODE);
        }
        
        auto sidEmulation = windowData.settings->getInt(KEY_SIDPLAYFP_SID_EMULATION, SIDPLAYFP_SID_EMULATION_DEFAULT);
        if (ImGui::Combo(STR_SID_EMULATION, &sidEmulation, "ReSIDfp\0ReSID\0")) {
            onDecoderIntSettingChanged(KEY_SIDPLAYFP_SID_EMULATION, sidEmulation);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_SIDPLAY_EMULATION);
        }

        auto fastSampling = windowData.settings->getBool(KEY_SIDPLAYFP_FAST_SAMPLING, SIDPLAYFP_FAST_SAMPLING_DEFAULT);
        if (ImGui::Checkbox(STR_FAST_SAMPLING, &fastSampling)) {
            onDecoderBoolSettingChanged(KEY_SIDPLAYFP_FAST_SAMPLING, fastSampling);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_SIDPLAY_FAST_SAMPLING);
        }

        auto digiBoost = windowData.settings->getBool(KEY_SIDPLAYFP_ENABLE_DIGIBOOST, SIDPLAYFP_ENABLE_DIGIBOOST_DEFAULT);
        if (ImGui::Checkbox("8580 Digiboost", &digiBoost)) {
            onDecoderBoolSettingChanged(KEY_SIDPLAYFP_ENABLE_DIGIBOOST, digiBoost);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_SIDPLAY_DIGIBOOST);
        }

        // todo settings for resid builders

        ImGui::EndTabItem();
    }
}

void SettingsWindow::renderGmeDecoderTab(const WindowData& windowData,
            std::function<void (std::string key, int value)> onDecoderIntSettingChanged,
            std::function<void (std::string key, bool value)> onDecoderBoolSettingChanged) {

    if (ImGui::BeginTabItem("Gme##gmeTab")) {
        auto enableAccuracy = windowData.settings->getBool(KEY_GME_ENABLE_ACCURACY, GME_ENABLE_ACCURACY_DEFAULT);
        if (ImGui::Checkbox(STR_ENABLE_ACCURACY, &enableAccuracy)) {
            onDecoderBoolSettingChanged(KEY_GME_ENABLE_ACCURACY, enableAccuracy);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_GME_ACCURACY);
        }

        auto autoloadPlaybackLimit = windowData.settings->getBool(KEY_GME_AUTOLOAD_PLAYBACK_LIMIT, GME_AUTOLOAD_PLAYBACK_LIMIT_DEFAULT);
        if (ImGui::Checkbox(STR_LOAD_PLAYBACK_LIMIT, &autoloadPlaybackLimit)) {
            onDecoderBoolSettingChanged(KEY_GME_AUTOLOAD_PLAYBACK_LIMIT, autoloadPlaybackLimit);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_GME_PLAYBACK_LIMIT);
        }

        auto ignoreSilence = windowData.settings->getBool(KEY_GME_IGNORE_SILENCE, GME_GME_IGNORE_SILENCE_DEFAULT);
        if (ImGui::Checkbox(STR_IGNORE_SILENCE, &ignoreSilence)) {
            onDecoderBoolSettingChanged(KEY_GME_IGNORE_SILENCE, ignoreSilence);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_GME_IGNORE_SILENCE);
        }

        ImGui::EndTabItem();
    }
}

void SettingsWindow::renderDumbDecoderTab(const WindowData& windowData,
            std::function<void (std::string key, int value)> onDecoderIntSettingChanged,
            std::function<void (std::string key, bool value)> onDecoderBoolSettingChanged) {

    if (ImGui::BeginTabItem("Dumb##dumbTab")) {
        auto maxToMix = windowData.settings->getInt(KEY_DUMB_MAX_TO_MIX, DUMB_MAX_TO_MIX_DEFAULT);
        if (ImGui::Combo(STR_MAX_TO_MIX, &maxToMix, "64\0""128\0""256\0""512\0")) {
            onDecoderIntSettingChanged(KEY_DUMB_MAX_TO_MIX, maxToMix);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(STR_TOOLTIP_DUMB_MAX_TO_MIX);
        }
        ImGui::EndTabItem();
    }
}

void SettingsWindow::render(const WindowData& windowData,
    std::function<void (ToggleAppSetting, bool value)> onToggleSetting,
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
        renderSidplayDecoderTab(windowData, onDecoderIntSettingChanged, onDecoderBoolSettingChanged);
        renderGmeDecoderTab(windowData, onDecoderIntSettingChanged, onDecoderBoolSettingChanged);
        renderDumbDecoderTab(windowData, onDecoderIntSettingChanged, onDecoderBoolSettingChanged);
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
