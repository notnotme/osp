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
#include "LocalMountPoint.h"

#include <filesystem>
#include <fstream>


LocalMountPoint::LocalMountPoint(std::string name, std::string root) :
MountPoint(name, root)
{
}

LocalMountPoint::~LocalMountPoint()
{
}

void LocalMountPoint::setup()
{
}

void LocalMountPoint::cleanup()
{
}

void LocalMountPoint::navigate(std::filesystem::path path, ItemListener itemListener)
{
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
    {
        throw std::runtime_error("Failed to open the requested directory");
    }

    auto iterator = std::filesystem::directory_iterator(path);
    for (auto& p : iterator)
    {
        // Hide hidden file, maybe an user option
        auto filename = std::string(p.path().filename());
        if (filename[0] != '.')
        {
            auto isDirectory = p.is_directory();
            auto isFile = p.is_regular_file();
            if (isFile || isDirectory)
            {
                auto doContinue = itemListener(filename, isDirectory, isFile ? p.file_size() : 0);
                if (!doContinue)
                {
                    return; // Listener tell us to stop
                }
            }
        }
    }
}

void LocalMountPoint::getFile(std::filesystem::path path, size_t chunkBufferSize, FileListener fileListener)
{
    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
    {
        throw std::runtime_error("Failed to open the requested file");
    }

    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    if (!ifs.good())
    {
        ifs.close();
        throw std::runtime_error("The requested file is not readable");
    }

    auto readBuffer = std::vector<uint8_t>(chunkBufferSize, 0);
    while (!ifs.eof())
    {
        // !!!
        ifs.read((char*) readBuffer.data(), readBuffer.size());

        size_t readCount = ifs.gcount();
        if (readCount != chunkBufferSize)
        {
            readBuffer.resize(readCount);
        }

        auto doContinue = fileListener(readBuffer);
        if (!doContinue)
        {
            break; // Listener tells us to stop
        }
    }
    ifs.close();
}
