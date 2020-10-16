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
#include <filesystem>

#include "MountPoint.h"


class LocalMountPoint :
public MountPoint
{
public:
    LocalMountPoint(std::string name, std::string root);
    virtual ~LocalMountPoint();

    virtual void setup() override;
    virtual void cleanup() override;
    virtual void navigate(std::filesystem::path path, ItemListener itemListener) override;
    virtual void getFile(std::filesystem::path path, size_t chunkBufferSize, FileListener fileListener) override;

private:
    std::string mDrive;

    LocalMountPoint(const LocalMountPoint& copy);
};
