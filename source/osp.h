#pragma once

#include "imgui/imgui.h"
#include "filesystem/filesystem.h"
#include "ui/aboutwindow.h"
#include "ui/metricswindow.h"
#include "ui/explorerframe.h"
#include "ui/playerframe.h"
#include "ui/metadataframe.h"
#include "filemanager.h"
#include "soundengine.h"

#include <string>

class Osp {

    public:
        struct Settings {
            int mStyle;
            int mFont;
            std::string mDataPath;
        };

        Osp();
        virtual ~Osp();

        bool setup(Settings settings);
        void render();
        void cleanup();

    private:
        AboutWindow mAboutWindow;
        MetricsWindow mMetricsWindow;
        ExplorerFrame mExplorerFrame;
        PlayerFrame mPlayerFrame;
        MetaDataFrame mMetaDataFrame;
        bool mShowWorkspace;
        
        FileManager mFileManager;
        SoundEngine mSoundEngine;
        Settings mSettings;
        std::string mStatusMessage;

        Osp(const Osp& copy);

        void handlePlayerButtonClick(PlayerFrame::ButtonId button);
        void handleExplorerItemClick(FileSystem::Entry item, std::filesystem::path currentExplorerPath);
        
};
