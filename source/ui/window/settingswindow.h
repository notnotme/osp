#pragma once

#include "../window.h"
#include "../../settings.h"

#include <functional>
#include <memory>

class SettingsWindow : public Window {

    public:
        enum AppSetting {
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
            const std::function<void (AppSetting, bool value)>& onToggleSetting,
            const std::function<void (std::string key, int value)>& onDecoderIntSettingChanged,
            const std::function<void (std::string key, bool value)>& onDecoderBoolSettingChanged);

    private:
        SettingsWindow(const SettingsWindow& copy);

        void renderOspSettingsTab(const WindowData& windowData,
            const std::function<void (AppSetting, bool value)>& onToggleSetting);

        void renderSc68DecoderTab(const WindowData& windowData,
            const std::function<void (std::string key, int value)>& onDecoderIntSettingChanged,
            const std::function<void (std::string key, bool value)>& onDecoderBoolSettingChanged);

        void renderSidplayDecoderTab(const WindowData& windowData,
            const std::function<void (std::string key, int value)>& onDecoderIntSettingChanged,
            const std::function<void (std::string key, bool value)>& onDecoderBoolSettingChanged);

        void renderGmeDecoderTab(const WindowData& windowData,
            const std::function<void (std::string key, int value)>& onDecoderIntSettingChanged,
            const std::function<void (std::string key, bool value)>& onDecoderBoolSettingChanged);

        void renderDumbDecoderTab(const WindowData& windowData,
            const std::function<void (std::string key, int value)>& onDecoderIntSettingChanged,
            const std::function<void (std::string key, bool value)>& onDecoderBoolSettingChanged);

};
