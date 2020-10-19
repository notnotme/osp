/*
 * This file is part of OSP (https://github.com/notnotme/osp).
 * Copyright (c) 2020 Romain Graillot
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "FileSystem.h"

#include <algorithm>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "file/LocalMountPoint.h"
#include "../config.h"

#define FILE_CHUNK_SIZE 16384 // Size of read buffer when opening a file from a mount point


FileSystem::FileSystem(Config config, LanguageFile languageFile) :
ECS::EntitySystem(),
mConfig(config),
mLanguageFile(languageFile),
mWorkerThreadMutex(SDL_CreateMutex()),
mThreadParams
({
    {.thread = nullptr, .status = IDLE, .path = {}},
    {.thread = nullptr, .status = IDLE, .path = {}}
})
{
}

FileSystem::~FileSystem()
{
    SDL_DestroyMutex(mWorkerThreadMutex);
}

void FileSystem::configure(ECS::World* world)
{
    TRACE(">>>");

    // Add mount point
    mMountPoints.push_back(new LocalMountPoint(mLanguageFile.getc("mount_points.default_filesystem"), DEFAULT_MOUNTPOINT));

    // Initialize all mount point
    for (auto* mountPoint : mMountPoints)
    {
        mountPoint->setup();
    }

    // Subscribe for events
    world->subscribe<FileSystemLoadTaskEvent>(this);
    world->subscribe<FileSystemCancelTaskEvent>(this);

    // Tells everyone what is mounted
    listMountPoints(world);
}

void FileSystem::unconfigure(ECS::World* world)
{
    TRACE(">>>");

    // Unubscribe for events
    world->unsubscribe<FileSystemLoadTaskEvent>(this);
    world->unsubscribe<FileSystemCancelTaskEvent>(this);

    // If we are working stop right now
    cancelFileThread();
    cancelDirectoryThread();

    // Release any resources used by MountPoints
    for (auto* mountPoint : mMountPoints)
    {
        mountPoint->cleanup();
        delete mountPoint;
    }
}

void FileSystem::tick(ECS::World* world, float deltaTime)
{
    // Check for pending event probably fired by one of the worker thread
    SDL_LockMutex(mWorkerThreadMutex);
    if (!mPendingFileSystemBusyEvent.empty())
    {
        for (auto event : mPendingFileSystemBusyEvent)
        {
            world->emit(event);
        }
        mPendingFileSystemBusyEvent.clear();
    }

    if (!mPendingFileSystemErrorEvent.empty())
    {
        for (auto event : mPendingFileSystemErrorEvent)
        {
            world->emit(event);
        }
        mPendingFileSystemErrorEvent.clear();
    }

    if (mPendingFileLoadedEvent.has_value())
    {
        world->emit(mPendingFileLoadedEvent.value());
        mPendingFileLoadedEvent.reset();
    }

    if (mPendingDirectoryLoadedEvent.has_value())
    {
        world->emit(mPendingDirectoryLoadedEvent.value());
        mPendingDirectoryLoadedEvent.reset();
    }
    SDL_UnlockMutex(mWorkerThreadMutex);
}

void FileSystem::listMountPoints(ECS::World* world)
{
    auto items = std::vector<DirectoryLoadedEvent::Item>();
    for (auto* mountPoint : mMountPoints)
    {
        items.push_back
        ({
            .isFolder = true,
            .name = mountPoint->getName(),
            .size = 0
        });
    }

    world->emit<DirectoryLoadedEvent>
    ({
        .path = "",
        .items = items
    });
}

void FileSystem::receive(ECS::World* world, const FileSystemLoadTaskEvent& event)
{
    TRACE("Received FileSystemLoadTaskEvent type {:d}, {:s}", event.type, event.path);

    // Cancel any work
    ThreadParams* target;
    switch (event.type)
    {
        case FileSystemLoadTaskEvent::LOAD_DIRECTORY:
            cancelDirectoryThread();
            target = &mThreadParams[DIRECTORY];
        break;
        case FileSystemLoadTaskEvent::LOAD_FILE:
            cancelFileThread();
            target = &mThreadParams[FILE];
        break;
    }

    // Build path to navigate
    target->path.clear();
    auto path = std::filesystem::path(event.path);
    for (auto elm : path)
    {
        target->path.push_back(elm);
    }

    // Replace the mount point name by his scheme if present
    for (auto* mountPoint : mMountPoints)
    {
        if (target->path[0] == mountPoint->getName())
        {
            target->path[0] = mountPoint->getScheme();
            break;
        }
    }

    if (event.type == FileSystemLoadTaskEvent::LOAD_DIRECTORY)
    {
        // Catch if we request the mount point listing
        if (target->path.size() == 2 && target->path[1] == "..")
        {
            listMountPoints(world);
            return;
        }

        // Navigate to path
        target->thread = SDL_CreateThread(workerThreadFuncDirectory, "OSPFILE", this);
    }
    else if (event.type == FileSystemLoadTaskEvent::LOAD_FILE)
    {
        // Get the file stored in path
       target->thread = SDL_CreateThread(workerThreadFuncFile, "OSPDIR", this);
    }
}

void FileSystem::receive(ECS::World* world, const FileSystemCancelTaskEvent& event)
{
    TRACE("Received FileSystemCancelTaskEvent.");
    switch (event.type)
    {
        case FileSystemCancelTaskEvent::LOAD_FILE:
            cancelFileThread();
        break;
        case FileSystemCancelTaskEvent::LOAD_DIRECTORY:
            cancelDirectoryThread();
        break;
    }
}

int FileSystem::workerThreadFuncDirectory(void* thiz)
{
    TRACE("Directory thread alive.");
    if (SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW) != 0)
    {
        TRACE("Set SDL_THREAD_PRIORITY_LOW failed");
    }

    auto* fileSystem = (FileSystem*) thiz;
    auto* threadParams = &fileSystem->mThreadParams[DIRECTORY];

    // Tells everyone we are working
    threadParams->status = WORKING;
    SDL_LockMutex(fileSystem->mWorkerThreadMutex);
    fileSystem->mPendingFileSystemBusyEvent.push_back(
    (FileSystemBusyEvent) {
        .isLoading = true,
        .type = FileSystemBusyEvent::DIRECTORY
    });
    SDL_UnlockMutex(fileSystem->mWorkerThreadMutex);

    // Select the mount point to use
    auto* selectedMountPoint = (MountPoint*) nullptr;
    for (auto* mountPoint : fileSystem->mMountPoints)
    {
        if (threadParams->path[0] == mountPoint->getScheme())
        {
            selectedMountPoint = mountPoint;
            break;
        }
    }

    if (selectedMountPoint == nullptr)
    {
        SDL_LockMutex(fileSystem->mWorkerThreadMutex);
        fileSystem->mPendingFileSystemErrorEvent.push_back(
        (FileSystemErrorEvent) {
            .message = fmt::format("No mountpoint available to open {:s}", threadParams->path.back())
        });

        fileSystem->mPendingFileSystemBusyEvent.push_back(
        (FileSystemBusyEvent) {
            .isLoading = true,
            .type = FileSystemBusyEvent::DIRECTORY
        });
        SDL_UnlockMutex(fileSystem->mWorkerThreadMutex);
        return 0;
    }

    if (threadParams->path.back() == "..")
    {
        // We want to go up
        threadParams->path.pop_back();
        threadParams->path.pop_back();
    }

    auto path = std::filesystem::path();
    for (auto elm : threadParams->path)
    {
        path /= elm;
    }

    auto items = std::vector<DirectoryLoadedEvent::Item>();
    try
    {
        selectedMountPoint->navigate(
            path,
            [&](std::string name, bool isFolder, uintmax_t size)
            {
                items.push_back
                ({
                    .isFolder = isFolder,
                    .name = name,
                    .size = size
                });
                return threadParams->status != CANCELING;
            });
    }
    catch(const std::exception& e)
    {
        auto error = e.what();

        TRACE("{:s}.", error);

        // Send a notification event if something goes wrong
        threadParams->status = CANCELING;
        SDL_LockMutex(fileSystem->mWorkerThreadMutex);
        fileSystem->mPendingFileSystemErrorEvent.push_back(
        (FileSystemErrorEvent) {
            .message = error
        });
        SDL_UnlockMutex(fileSystem->mWorkerThreadMutex);
    }

    // If not cancelled sort by folder and filename asc then add back navigation
    if (threadParams->status != CANCELING)
    {
        std::sort(items.begin(), items.end(),
        [](auto a, auto b)
        {
            if (a.isFolder != b.isFolder)
            {
                return a.isFolder;
            }

            std::transform(a.name.begin(), a.name.end(), a.name.begin(), ::tolower);
            std::transform(b.name.begin(), b.name.end(), b.name.begin(), ::tolower);
            return a.name < b.name;
        });

        items.insert(items.begin(),
        {
            .isFolder = true,
            .name = "..",
            .size = 0
        });
    }

    if (threadParams->status != CANCELING)
    {
        // Tells to everyone what was in the directory and we stopped working
        SDL_LockMutex(fileSystem->mWorkerThreadMutex);
        fileSystem->mPendingDirectoryLoadedEvent.emplace(
        (DirectoryLoadedEvent) {
            .path = path,
            .items = items
        });

        fileSystem->mPendingFileSystemBusyEvent.push_back(
        (FileSystemBusyEvent) {
            .isLoading = false,
            .type = FileSystemBusyEvent::DIRECTORY
        });
        SDL_UnlockMutex(fileSystem->mWorkerThreadMutex);
    }
    else
    {
        // Tells to everyone that we are finished working
        SDL_LockMutex(fileSystem->mWorkerThreadMutex);
        fileSystem->mPendingFileSystemBusyEvent.push_back(
        (FileSystemBusyEvent) {
            .isLoading = false,
            .type = FileSystemBusyEvent::DIRECTORY
        });

        // Restore old path
        SDL_UnlockMutex(fileSystem->mWorkerThreadMutex);
    }

    threadParams->status = IDLE;
    return 0;
}

int FileSystem::workerThreadFuncFile(void* thiz)
{
    TRACE("File thread alive.");
    if (SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW) != 0)
    {
        TRACE("Set SDL_THREAD_PRIORITY_LOW failed");
    }

    auto fileSystem = (FileSystem*) thiz;
    auto* threadParams = &fileSystem->mThreadParams[FILE];

    // Tells everyone we are working
    threadParams->status = WORKING;
    SDL_LockMutex(fileSystem->mWorkerThreadMutex);
    fileSystem->mPendingFileSystemBusyEvent.push_back(
    (FileSystemBusyEvent) {
        .isLoading = true,
        .type = FileSystemBusyEvent::FILE
    });
    SDL_UnlockMutex(fileSystem->mWorkerThreadMutex);

    // Select mountpoint to use
    auto* selectedMountPoint = (MountPoint*) nullptr;
    for (auto* mountPoint : fileSystem->mMountPoints)
    {
        if (threadParams->path[0] == mountPoint->getScheme())
        {
            selectedMountPoint = mountPoint;
            break;
        }
    }

    if (selectedMountPoint == nullptr)
    {
        auto error = fmt::format("No mountpoint available to open {:s}", threadParams->path.back());
        TRACE("{:s}.", error);

        // Send a notification event if something goes wrong
        threadParams->status = CANCELING;
        SDL_LockMutex(fileSystem->mWorkerThreadMutex);
        fileSystem->mPendingFileSystemErrorEvent.push_back(
        (FileSystemErrorEvent) {
            .message = error
        });
        SDL_UnlockMutex(fileSystem->mWorkerThreadMutex);
        return 0;
    }

    auto path = std::filesystem::path();
    for (auto elm : threadParams->path)
    {
        path /= elm;
    }

    auto fileBuffer = std::vector<uint8_t>();
    try
    {
        selectedMountPoint->getFile(
            path,
            FILE_CHUNK_SIZE,
            [&](const std::vector<uint8_t>& chunkBuffer)
            {
                fileBuffer.insert(fileBuffer.end(), chunkBuffer.begin(), chunkBuffer.end());
                return threadParams->status != CANCELING;
            });
    }
    catch(const std::exception& e)
    {
        auto error = e.what();

        TRACE("{:s}.", error);
        threadParams->status = CANCELING;

        // Send a notification event if something goes wrong
        SDL_LockMutex(fileSystem->mWorkerThreadMutex);
        fileSystem->mPendingFileSystemErrorEvent.push_back(
        (FileSystemErrorEvent) {
            .message = error
        });
        SDL_UnlockMutex(fileSystem->mWorkerThreadMutex);
    }

    if (threadParams->status != CANCELING)
    {
        // If not canceled tells to everyone that a file was read and we are now not working
        SDL_LockMutex(fileSystem->mWorkerThreadMutex);
        fileSystem->mPendingFileLoadedEvent.emplace(
        (FileLoadedEvent) {
            .path = path,
            .buffer = fileBuffer
        });

        fileSystem->mPendingFileSystemBusyEvent.push_back(
        (FileSystemBusyEvent) {
            .isLoading = false,
            .type = FileSystemBusyEvent::FILE
        });
        SDL_UnlockMutex(fileSystem->mWorkerThreadMutex);
    }
    else
    {
        // If  canceled tells to everyone we are now not working
        SDL_LockMutex(fileSystem->mWorkerThreadMutex);
        fileSystem->mPendingFileSystemBusyEvent.push_back(
        (FileSystemBusyEvent) {
            .isLoading = false,
            .type = FileSystemBusyEvent::FILE
        });
        SDL_UnlockMutex(fileSystem->mWorkerThreadMutex);
    }

    threadParams->status = IDLE;
    return 0;
}

void FileSystem::cancelFileThread()
{
    if (mThreadParams[FILE].thread != nullptr)
    {
        mThreadParams[FILE].status = CANCELING;
        SDL_WaitThread(mThreadParams[FILE].thread, nullptr);
        TRACE("Waiting file worker thread to finish...");
        mThreadParams[FILE].thread = nullptr;
    }
}

void FileSystem::cancelDirectoryThread()
{
    if (mThreadParams[DIRECTORY].thread != nullptr)
    {
        mThreadParams[DIRECTORY].status = CANCELING;
        SDL_WaitThread(mThreadParams[DIRECTORY].thread, nullptr);
        TRACE("Waiting directory worker thread to finish...");
        mThreadParams[DIRECTORY].thread = nullptr;
    }
}
