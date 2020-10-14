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

#define APP_CONFIG_NAME "settings.cfg"

#if defined(__SWITCH__)
#define DATAPATH "romfs:/"
#else
#define DATAPATH "romfs/"
#endif

// Default data access path for LocalFileSystem.
#if defined(__SWITCH__)
#define DEFAULT_MOUNTPOINT "sdmc:/"
#else
#define DEFAULT_MOUNTPOINT "/"
#endif

// Silence log if we are not in DEBUG mode
#ifndef DEBUG
#define TRACE(fmtt,...) ((void)0)
#else
#include <fmt/core.h>
#define TRACE(fmtt,...) fmt::print( \
        std::string(__FILE__).append(" ") \
        .append(__FUNCTION__/*__PRETTY_FUNCTION__*/).append(": ") \
        .append(fmtt) \
        .append("\n"), \
        ## __VA_ARGS__)
// Meh
#endif