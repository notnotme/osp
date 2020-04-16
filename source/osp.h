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
        std::string mLastFileSelected;
        
        Osp(const Osp& copy);

        void selectNextTrack(bool skipInvalid, bool autoPlay);
        void selectPrevTrack(bool skipInvalid, bool autoPlay);
        std::string getPrevFileName() const;
        std::string getNextFileName() const;
        bool engineLoad(std::string path, std::string filename);
        void handlePlayerButtonClick(const PlayerFrame::ButtonId button);
        void handleExplorerItemClick(const FileSystem::Entry item, const std::filesystem::path currentExplorerPath);
        
};
