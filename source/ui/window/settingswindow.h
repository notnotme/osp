#pragma once

#include "window.h"

#include <functional>

class SettingsWindow : public Window {

    public:
        enum ToggleSetting {
            MOUSE_EMULATION,
            TOUCH_ENABLED
        };

        struct WindowData {
            bool mouseEmulationEnabled;
            bool touchEnabled;
        };

        SettingsWindow();
        virtual ~SettingsWindow();

        void render(const WindowData& windowData,
            std::function<void (ToggleSetting)> onToggleSetting);

    private:
        SettingsWindow(const SettingsWindow& copy);

        void renderOspSettingsTab(const WindowData& windowData,
            std::function<void (ToggleSetting)> onToggleSetting);

        void renderSc68DecoderTab();
        void renderSidplayDecoderTab();
        void renderGmeDecoderTab();
        void renderDumbDecoderTab();
        
};
