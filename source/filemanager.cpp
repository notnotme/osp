#include "filemanager.h"

#include "platform.h"
#include "filesystem/localfilesystem.h"

#include <algorithm>
#include <SDL2/SDL_log.h>

FileManager::FileManager() :
    mErrorMutex(SDL_CreateMutex()),
    mStateMutex(SDL_CreateMutex()),
    mFileSystemThread(nullptr), 
    mCurrentFileSystem(nullptr) {
}

FileManager::~FileManager() {
    if (mErrorMutex != nullptr) {
        SDL_DestroyMutex(mErrorMutex);
        mErrorMutex = nullptr;
    }

    if (mStateMutex != nullptr) {
        SDL_DestroyMutex(mStateMutex);
        mStateMutex = nullptr;
    }
}

bool FileManager::setup() {
    initializeFileSystems();
    clearPath();
    buildPath();

    SDL_LockMutex(mStateMutex);
    mState = READY;
    SDL_UnlockMutex(mStateMutex);

    return true;
}

void FileManager::cleanup() {
    if (mFileSystemThread != nullptr) {
        SDL_WaitThread(mFileSystemThread, nullptr);
        mFileSystemThread = nullptr;
    }

    mCurrentPathStack.clear();
    mCurrentPathEntries.clear();
    mFileSystemList.clear();
    mLastFolder.clear();

    mCurrentFileSystem = nullptr;
    for (const auto fileSystem : mFileSystemList) {
        fileSystem->cleanup();
    }
    mFileSystemList.clear();
}

FileManager::State FileManager::getState() const {
    return mState;
}

std::string FileManager::getError() const {
    return mError;
}

void FileManager::clearError() {
    SDL_LockMutex(mStateMutex);
    mState = READY;
    SDL_UnlockMutex(mStateMutex);

    SDL_LockMutex(mErrorMutex);
    mError = "";
    SDL_UnlockMutex(mErrorMutex);
}


std::filesystem::path FileManager::getCurrentPath() const {
    return mCurrentPath;
}

std::string FileManager::getLastFolder() const {
    return mLastFolder.empty() ? "" : mLastFolder.back();
};

std::vector<FileSystem::Entry> FileManager::getCurrentPathEntries() const {
    return mCurrentPathEntries;
}

void FileManager::clearPath() {
    mCurrentFileSystem = nullptr;
    mCurrentPathStack.clear();
    mCurrentPathEntries.clear();
    mLastFolder.clear();
    for (const auto fileSystem : mFileSystemList) {
        mCurrentPathEntries.push_back({
            .folder = true,
            .name = fileSystem->getMountPoint(),
            .size = 0 
        });
    }
}

void FileManager::buildPath() {
    if (mCurrentFileSystem == nullptr) {
        mCurrentPath = "Mount points";
        return;
    }

    mCurrentPath.clear();
    for (const auto part : mCurrentPathStack) {
        mCurrentPath = mCurrentPath.append(part);
    }
}

bool FileManager::navigate(const std::string path) {
    // Cancel old navigation thread if any
    if (mState == LOADING) {
        SDL_LockMutex(mErrorMutex);
        mError = "Trying to navigate while file system thread is working.";
        SDL_UnlockMutex(mErrorMutex);
        
        return false;
    }

    // Load new path entries
    if (path == ".." && mCurrentPathStack.size() == 1) {
        // Go to mount points
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Return to mount points.\n");
        clearPath();
        buildPath();
        return true;
    }
    else {
        if (mCurrentPathStack.empty()) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Selecting mount point '%s'.\n", path.c_str());
            for (const auto fileSystem : mFileSystemList) {
                if (fileSystem->getMountPoint() == path) {
                    mCurrentFileSystem = fileSystem;
                    mLastFolder.push_back(path);
                    mCurrentPathStack.push_back(path);
                    break;
                }
            }
        }
        else {
            if (path == "..") {
                if (mLastFolder.size() > mCurrentPathStack.size()) {
                    mLastFolder.pop_back();
                }
                mCurrentPathStack.pop_back();
            } else {
                if (getLastFolder() != path) {
                    mLastFolder.push_back(path);
                }
                mCurrentPathStack.push_back(path);
            }
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Navigate to: '%s/%s'.\n", mCurrentPath == "/" ? "" : mCurrentPath.c_str(), path.c_str());
        }

        // Navigate using the current file system
        buildPath();
        return startWorkingThread();
    }
}

File* FileManager::getFile(const std::string path) {
    if (mCurrentFileSystem != nullptr) {
        return mCurrentFileSystem->getFile(path);
    }

    SDL_LockMutex(mErrorMutex);
    mError = "No file system to work with";
    SDL_UnlockMutex(mErrorMutex);
    
    return nullptr;
}

bool FileManager::initializeFileSystems() {
    if (const auto fileSystem = std::shared_ptr<FileSystem>(new LocalFileSystem(DEFAULT_LOCAL_FS_PATH));
        fileSystem->setup() == false) {

        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot load LocalFileSystem (%s).\n", DEFAULT_LOCAL_FS_PATH);
        fileSystem->cleanup();
    } else {
        mFileSystemList.push_back(fileSystem);
    }
    
    return true;
}

bool FileManager::startWorkingThread() {
    if (mFileSystemThread != nullptr) {
        // We must wait
        SDL_WaitThread(mFileSystemThread, nullptr);
        mFileSystemThread = nullptr;
    }

    if (mFileSystemThread = SDL_CreateThread(FileManager::fileSystemThreadFunc, "OSP-FM-Thread", this);
        mFileSystemThread == nullptr) {
            
        SDL_LockMutex(mStateMutex);
        mState = ERROR;
        SDL_UnlockMutex(mStateMutex);

        SDL_LockMutex(mErrorMutex);
        mError = std::string("Unable to start file system thread : ").append(SDL_GetError());
        SDL_UnlockMutex(mErrorMutex);
        
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", mError.c_str());
        return false;
    }
    
    return true;
}

int FileManager::fileSystemThreadFunc(void* userData) {
    const auto fileManager = static_cast<FileManager*>(userData);

    SDL_LockMutex(fileManager->mStateMutex);
    fileManager->mState = LOADING;
    SDL_UnlockMutex(fileManager->mStateMutex);

    // SDL_Delay(1000); 

    // Insert back navigation
    fileManager->mCurrentPathEntries.clear();
    fileManager->mCurrentPathEntries.insert(fileManager->mCurrentPathEntries.begin(), {
        .folder = true,
        .name = "..",
        .size = 0
    });

    // Get listing from filesystem
    std::vector<FileSystem::Entry> list;
    if (!fileManager->mCurrentFileSystem->navigate(fileManager->mCurrentPath, list)) {
        SDL_LockMutex(fileManager->mStateMutex);
        fileManager->mState = ERROR;
        SDL_UnlockMutex(fileManager->mStateMutex);

        SDL_LockMutex(fileManager->mErrorMutex);
        fileManager->mError = fileManager->mCurrentFileSystem->getError();
        SDL_UnlockMutex(fileManager->mErrorMutex);
        return 0;
    }

    // sort by folder and filename asc
    std::sort(list.begin(), list.end(), [](FileSystem::Entry a, FileSystem::Entry b) {
        if (a.folder != b.folder) return a.folder;
        std::transform(a.name.begin(), a.name.end(), a.name.begin(), ::tolower);
        std::transform(b.name.begin(), b.name.end(), b.name.begin(), ::tolower);
        return a.name < b.name;
    });

    fileManager->mCurrentPathEntries.insert(fileManager->mCurrentPathEntries.end(), list.begin(), list.end());

    SDL_LockMutex(fileManager->mStateMutex);
    fileManager->mState = READY;
    SDL_UnlockMutex(fileManager->mStateMutex);

    return 0;
}
