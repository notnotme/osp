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
#include "ConfigFile.h"

#include <stdexcept>


Config::Config() :
mSetting(nullptr)
{
}

Config::Config(libconfig::Setting* setting) :
mSetting(setting)
{
}

Config::~Config()
{
}

Config Config::getGroupOrCreate(std::string key)
{
    if (mSetting == nullptr)
    {
        throw std::runtime_error("Invalid Config object");
    }

    if (!mSetting->exists(key))
    {
        return Config(&mSetting->add(key, libconfig::Setting::Type::TypeGroup));
    }

    return Config(&mSetting->lookup(key));
}

int Config::get(std::string key, int defaultValue) const {
    if (mSetting == nullptr)
    {
        throw std::runtime_error("Invalid Config object");
    }

    if (!mSetting->exists(key))
    {
        return defaultValue;
    }

    return mSetting->lookup(key);
}

bool Config::get(std::string key, bool defaultValue) const {
    if (mSetting == nullptr)
    {
        throw std::runtime_error("Invalid Config object");
    }

    if (!mSetting->exists(key))
    {
        return defaultValue;
    }

    return mSetting->lookup(key);
}

std::string Config::get(std::string key, std::string defaultValue) const {
    if (mSetting == nullptr)
    {
        throw std::runtime_error("Invalid Config object");
    }

    if (!mSetting->exists(key))
    {
        return defaultValue;
    }

    return mSetting->lookup(key);
}

void Config::set(std::string key, int value) const {
    if (mSetting == nullptr)
    {
        throw std::runtime_error("Invalid Config object");
    }

    if (!mSetting->exists(key))
    {
        mSetting->add(key, libconfig::Setting::Type::TypeInt);
    }

    auto& setting = mSetting->lookup(key);
    setting = value;
}

void Config::set(std::string key, bool value) const {
    if (mSetting == nullptr)
    {
        throw std::runtime_error("Invalid Config object");
    }

    if (!mSetting->exists(key))
    {
        mSetting->add(key, libconfig::Setting::Type::TypeBoolean);
    }

    auto& setting = mSetting->lookup(key);
    setting = value;
}

void Config::set(std::string key, std::string value) const {
    if (mSetting == nullptr)
    {
        throw std::runtime_error("Invalid Config object");
    }

    if (!mSetting->exists(key))
    {
        mSetting->add(key, libconfig::Setting::Type::TypeString);
    }

    auto& setting = mSetting->lookup(key);
    setting = value;
}

ConfigFile::ConfigFile()
{
}

ConfigFile::~ConfigFile()
{
}

void ConfigFile::load(std::string filename)
{
    mConfig.readFile(filename.c_str());
}

void ConfigFile::save(std::string filename)
{
    mConfig.writeFile(filename.c_str());
}

Config ConfigFile::getGroupOrCreate(std::string key)
{
    auto& root = mConfig.getRoot();
    if (!root.exists(key))
    {
        return Config(&root.add(key, libconfig::Setting::Type::TypeGroup));
    }

    return Config(&root.lookup(key));
}
