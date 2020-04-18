#pragma once

#include "../imgui/imgui.h"
#include "../filemanager.h"
#include <string>
#include <functional>

class MenuBar {

    public:
        enum MenuAction {
            SHOW_ABOUT,
            SHOW_METRICS,
            QUIT
        };

        struct MenuBarData {
            std::string message;
            FileManager::State fmState;
            bool itemShowWorkspaceCheked;
            bool mouseEmulationEnabled;
            int selectedStyle;
            int selectedFont;
        };

        MenuBar();
        virtual ~MenuBar();

        void render(const MenuBarData& menuBarData,
            std::function<void (bool)> onWorkspaceCheckChange,
            std::function<void (int)> onStyleChange,
            std::function<void (ImFont*, int)> onFontChange,
            std::function<void (bool)> onMouseEmulationCheckChange,
            std::function<void (MenuAction)> onMenuAtion);

    private:
        MenuBar(const MenuBar& copy);

};
