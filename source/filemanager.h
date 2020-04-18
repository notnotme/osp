#pragma once

#include "filesystem/file.h"
#include "filesystem/filesystem.h"

#include <string>
#include <filesystem>
#include <list>
#include <vector>
#include <memory>
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_thread.h>

class FileManager {

    public:
        enum State {
            READY,
            LOADING,
            ERROR
        };

        FileManager();
        virtual ~FileManager();

        bool setup();
        void cleanup();
   
        std::string getLastFolder() const;
        std::filesystem::path getCurrentPath() const;
        std::vector<FileSystem::Entry> getCurrentPathEntries() const;
        bool navigate(const std::string path);
        File* getFile(const std::string path);
        State getState() const;
        std::string getError() const;
        void clearError();

    private:
        SDL_mutex* mStateMutex;
        std::string mError;
        State mState;
        SDL_Thread* mFileSystemThread;

        std::list<std::string> mCurrentPathStack;
        std::list<std::string> mLastFolder;
        std::filesystem::path mCurrentPath;
        std::vector<FileSystem::Entry> mCurrentPathEntries;
        
        std::vector<std::shared_ptr<FileSystem>> mFileSystemList;
        std::shared_ptr<FileSystem> mCurrentFileSystem;
        
        FileManager(const FileManager& copy);
  
        bool initializeFileSystems();
        void clearPath();
        void buildPath();
        bool startWorkingThread();

        static int fileSystemThreadFunc(void* userData);
};