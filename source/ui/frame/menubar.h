#pragma once

#include "../../imgui/imgui.h"
#include "../../filemanager.h"
#include "../../settings.h"
#include "../frame.h"

#include <string>
#include <functional>
#include <memory>

class MenuBar : public Frame {

    public:
        enum ItemId {
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
            std::shared_ptr<Settings> settings;
        };

        MenuBar();
        virtual ~MenuBar();

        void render(const MenuBarData& menuBarData,
            const std::function<void (int)>& onStyleChange,
            const std::function<void (ImFont*, int)>& onFontChange,
            const std::function<void (ItemId)>& onMenuAtion);

    private:
        MenuBar(const MenuBar& copy);

};
