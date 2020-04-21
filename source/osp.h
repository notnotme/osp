#pragma once

#include "platform.h"
#include "imgui/imgui.h"
#include "filesystem/filesystem.h"
#include "ui/window/aboutwindow.h"
#include "ui/window/metricswindow.h"
#include "ui/window/settingswindow.h"
#include "ui/explorerframe.h"
#include "ui/playerframe.h"
#include "ui/metadataframe.h"
#include "ui/menubar.h"
#include "filemanager.h"
#include "settings.h"
#include "soundengine.h"

#include <string>
#include <libconfig.h>

class Osp {

    public:
        const std::string CONFIG_FILENAME = "config.cfg";

        Osp();
        virtual ~Osp();

        bool setup(std::string dataPath);
        void render();
        void cleanup();

    private:
        bool mShowWorkspace;
        MenuBar mMenuBar;
        ExplorerFrame mExplorerFrame;
        PlayerFrame mPlayerFrame;
        MetaDataFrame mMetaDataFrame;

        AboutWindow mAboutWindow;
        MetricsWindow mMetricsWindow;
        SettingsWindow mSettingsWindow;
        
        FileManager mFileManager;
        SoundEngine mSoundEngine;
        Settings mSettings;
        std::string mStatusMessage;
        std::string mLastFileSelected;
        
        Osp(const Osp& copy);

        void selectNextTrack(bool skipInvalid, bool autoPlay);
        void selectPrevTrack(bool skipInvalid, bool autoPlay);
        std::string getPrevFileName() const;
        std::string getNextFileName() const;
        bool engineLoad(std::string path, std::string filename);
        
        void handlePlayerButtonClick(const PlayerFrame::ButtonId button);
        void handleExplorerItemClick(const FileSystem::Entry item, const std::filesystem::path currentExplorerPath);
        void handleSettingsChange(const SettingsWindow::ToggleSetting setting);
        void handleStyleChange(int style);
        void handleFontChange(ImFont* font, int fontIndex);
        void handleMenuBarAction(const MenuBar::MenuAction action);

};
