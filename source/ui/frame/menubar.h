#pragma once

#include "../../imgui/imgui.h"
#include "../../filemanager.h"
#include "../../settings.h"
#include "../frame.h"

#include <string>
#include <functional>

class MenuBar : public Frame {

    public:
        enum MenuAction {
            TOGGLE_WORKSPACE_VISIBILITY,
            SHOW_SETTINGS,
            SHOW_ABOUT,
            SHOW_METRICS,
            QUIT
        };

        struct MenuBarData {
            std::string message;
            FileManager::State fmState;
            bool itemShowWorkspaceCheked;
            Settings settings;
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
