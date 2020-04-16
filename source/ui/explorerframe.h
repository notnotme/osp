#pragma once

#include "../filesystem/filesystem.h"

#include <filesystem>
#include <vector>
#include <functional>

class ExplorerFrame {

    public:
        struct FrameData {
            std::filesystem::path currentPath;
            std::vector<FileSystem::Entry> listing;
            bool isWorking;
        };

        ExplorerFrame();
        virtual ~ExplorerFrame();

        void render(const FrameData& frameData, std::function<void (FileSystem::Entry)> onItemClick);

    private:
        ExplorerFrame(const ExplorerFrame& copy);

        void renderPath(const FrameData& frameData);
        void renderExplorer(const FrameData& frameData, std::function<void (FileSystem::Entry)> onItemClick);
        
};
