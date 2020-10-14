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
#include <libconfig.h++>


class Config
{
public:
    virtual ~Config();

    Config getGroupOrCreate(std::string key);
    int get(std::string key, int defaultValue) const;
    bool get(std::string key, bool defaultValue) const;
    std::string get(const std::string key, std::string defaultValue) const;

    void set(std::string key, int value) const;
    void set(std::string key, bool value) const;
    void set(std::string key, std::string value) const;

private:
    friend class ConfigFile;
    friend class Plugin;

    libconfig::Setting* mSetting;

    Config();
    Config(libconfig::Setting* setting);
};


class ConfigFile
{
public:
    ConfigFile();
    virtual ~ConfigFile();

    void load(std::string filename);
    void save(std::string filename);
    Config getGroupOrCreate(std::string key);

private:
    libconfig::Config mConfig;

    ConfigFile(const ConfigFile& copy);
};
