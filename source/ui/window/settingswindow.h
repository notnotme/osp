#pragma once

#include "../window.h"
#include "../../settings.h"

#include <functional>
#include <memory>

class SettingsWindow : public Window {

    public:
        enum ToggleAppSetting {
            MOUSE_EMULATION,
            TOUCH_ENABLED,
            AUTOSKIP_UNSUPPORTED_FILES,
            SKIP_SUBTUNES,
            ALWAYS_START_FIRST_TRACK
        };

        struct WindowData {
            std::shared_ptr<Settings> settings;
        };

        SettingsWindow();
        virtual ~SettingsWindow();

        void render(const WindowData& windowData,
            std::function<void (ToggleAppSetting)> onToggleSetting,
            std::function<void (std::string key, int value)> onDecoderIntSettingChanged,
            std::function<void (std::string key, bool value)> onDecoderBoolSettingChanged);

    private:
        SettingsWindow(const SettingsWindow& copy);

        void renderOspSettingsTab(const WindowData& windowData,
            std::function<void (ToggleAppSetting)> onToggleSetting);

        void renderSc68DecoderTab(const WindowData& windowData,
            std::function<void (std::string key, int value)> onDecoderIntSettingChanged,
            std::function<void (std::string key, bool value)> onDecoderBoolSettingChanged);

        void renderSidplayDecoderTab();
        void renderGmeDecoderTab();
        void renderDumbDecoderTab();

};
