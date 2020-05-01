#pragma once

#include "platform.h"
#include "imgui/imgui.h"
#include "filesystem/filesystem.h"
#include "ui/window/aboutwindow.h"
#include "ui/window/metricswindow.h"
#include "ui/window/settingswindow.h"
#include "ui/frame/explorerframe.h"
#include "ui/frame/playerframe.h"
#include "ui/frame/metadataframe.h"
#include "ui/frame/menubar.h"
#include "spritecatalog.h"
#include "filemanager.h"
#include "settings.h"
#include "soundengine.h"

#include <string>
#include <glad/glad.h>
#include <libconfig.h>

class Osp {

    public:
        const std::string CONFIG_FILENAME = "config.cfg";

        Osp();
        virtual ~Osp();

        bool setup(const std::string dataPath);
        void render();
        void cleanup();

    private:
        bool mShowWorkspace;
        GLuint mTextureSprites;
        std::string mStatusMessage;
        std::string mLastFileSelected;
        std::shared_ptr<Settings> mSettings;
        std::shared_ptr<SpriteCatalog> mSpriteCatalog;
        std::unique_ptr<FileManager> mFileManager;
        std::unique_ptr<SoundEngine> mSoundEngine;

        MenuBar mMenuBar;
        ExplorerFrame mExplorerFrame;
        PlayerFrame mPlayerFrame;
        MetaDataFrame mMetaDataFrame;

        AboutWindow mAboutWindow;
        MetricsWindow mMetricsWindow;
        SettingsWindow mSettingsWindow;

        Osp(const Osp& copy);

        void selectNextTrack(bool skipInvalid, bool autoPlay);
        void selectPrevTrack(bool skipInvalid, bool autoPlay);
        std::string getPrevFileName() const;
        std::string getNextFileName() const;
        bool engineLoad(std::string path, std::string filename);
        
        void handlePlayerButtonClick(const PlayerFrame::ButtonId button);
        void handleExplorerItemClick(const FileSystem::Entry item, const std::filesystem::path currentExplorerPath);
        void handleAppSettingsChange(const SettingsWindow::AppSetting setting, bool value);
        void handleStyleChange(int style);
        void handleFontChange(ImFont* font, int fontIndex);
        void handleMenuBarAction(const MenuBar::ItemId action);

};
