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
#define DEFAULT_MOUNTPOINT "/"

// Silence log if we are not in DEBUG mode
#ifndef DEBUG
#define TRACE(fmtt, ...) ((void)0)
#else
#include <fmt/core.h>
#define TRACE(fmtt, ...) fmt::print("{:s} {:s}: " fmtt "\n", __FILE__, __FUNCTION__, ## __VA_ARGS__)
// Meh
#endif