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

#include <string>
#include <functional>
#include <filesystem>
#include <vector>


class MountPoint
{
public:
    // Return false from those function will stop the iteration where they are used.
    typedef std::function<bool (std::string, bool, uintmax_t)> ItemListener;
    typedef std::function<bool (const std::vector<uint8_t>&)> FileListener;

    MountPoint(std::string name, std::string scheme);
    virtual ~MountPoint();

    std::string getName();
    std::string getScheme();

    virtual void setup() = 0;
    virtual void cleanup() = 0;
    virtual void navigate(std::filesystem::path path, ItemListener itemListener) = 0;
    virtual void getFile(std::filesystem::path path, size_t chunkBufferSize, FileListener fileListener) = 0;

private:
    std::string mName;
    std::string mScheme;

    MountPoint(const MountPoint& copy);
};
