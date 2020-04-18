#pragma once

#include "../imgui/imgui.h"
#include "../filemanager.h"
#include <string>
#include <functional>

class MenuBar {

    public:
        enum MenuAction {
            TOGGLE_WORKSPACE_VISIBILITY,
            TOGGLE_MOUSE_EMULATION,
            TOGGLE_TOUCH,
            SHOW_ABOUT,
            SHOW_METRICS,
            QUIT
        };

        struct MenuBarData {
            std::string message;
            FileManager::State fmState;
            bool itemShowWorkspaceCheked;
            bool mouseEmulationEnabled;
            bool touchEnabled;
            int selectedStyle;
            int selectedFont;
        };

        MenuBar();
        virtual ~MenuBar();

        void render(const MenuBarData& menuBarData,
            std::function<void (int)> onStyleChange,
            std::function<void (ImFont*, int)> onFontChange,
            std::function<void (MenuAction)> onMenuAtion);

    private:
        MenuBar(const MenuBar& copy);

};
