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
#include "MountPoint.h"


MountPoint::MountPoint(std::string name, std::string scheme) :
mName(name),
mScheme(scheme)
{
}

MountPoint::~MountPoint()
{
}

std::string MountPoint::getName()
{
    return mName;
}

std::string MountPoint::getScheme()
{
    return mScheme;
}
