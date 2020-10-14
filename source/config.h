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

// Silence log if we are in Release mode
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