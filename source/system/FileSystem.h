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
#pragma once

#include <vector>
#include <string>
#include <optional>

#include <SDL2/SDL.h>
#include <ECS.h>

#include "file/MountPoint.h"
#include "../event/file/FileSystemLoadTaskEvent.h"
#include "../event/file/DirectoryLoadedEvent.h"
#include "../event/file/FileLoadedEvent.h"
#include "../event/file/FileSystemBusyEvent.h"
#include "../event/file/FileSystemCancelTaskEvent.h"
#include "../event/file/FileSystemErrorEvent.h"
#include "../tools/ConfigFile.h"
#include "../tools/LanguageFile.h"


class FileSystem :
public ECS::EntitySystem,
public ECS::EventSubscriber<FileSystemLoadTaskEvent>,
public ECS::EventSubscriber<FileSystemCancelTaskEvent>
{
public:
    FileSystem(Config config, LanguageFile languageFile);
    virtual ~FileSystem();

    virtual void configure(ECS::World* world) override;
    virtual void unconfigure(ECS::World* world) override;
    virtual void tick(ECS::World* world, float deltaTime) override;

    virtual void receive(ECS::World* world, const FileSystemLoadTaskEvent& event) override;
    virtual void receive(ECS::World* world, const FileSystemCancelTaskEvent& event) override;

private:
    enum WorkThreadStatus
    {
        IDLE,
        WORKING,
        CANCELING
    };

    enum Thread
    {
        FILE,
        DIRECTORY
    };

    struct ThreadParams
    {
        SDL_Thread* thread;
        WorkThreadStatus status;
        std::vector<std::string> path;
    };

    Config mConfig;
    LanguageFile mLanguageFile;
    SDL_mutex* mWorkerThreadMutex;
    ThreadParams mThreadParams[2];

    std::vector<MountPoint*> mMountPoints;
    std::vector<FileSystemBusyEvent> mPendingFileSystemBusyEvent;
    std::vector<FileSystemErrorEvent> mPendingFileSystemErrorEvent;
    std::optional<DirectoryLoadedEvent> mPendingDirectoryLoadedEvent;
    std::optional<FileLoadedEvent> mPendingFileLoadedEvent;

    FileSystem(const FileSystem& copy);

    void cancelFileThread();
    void cancelDirectoryThread();
    void listMountPoints(ECS::World* world);
    static int workerThreadFuncDirectory(void* thiz);
    static int workerThreadFuncFile(void* thiz);
};
