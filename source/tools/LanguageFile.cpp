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
#include "LanguageFile.h"

#include <stdexcept>
#include <filesystem>
#include <jansson.h>
#include <fmt/format.h>


// SHared members between all instance of LanguageFile
std::map<std::string, std::string> LanguageFile::mCatalog;
std::string LanguageFile::mFolder;
LanguageFile::Language LanguageFile::mLanguage;

LanguageFile::LanguageFile()
{
}

LanguageFile::~LanguageFile()
{
}

void LanguageFile::load(std::string folder, Language language)
{
    mFolder = folder;
    mLanguage = language;
    reload(language);
}

void LanguageFile::reload(Language language)
{
    // Delete old catalog if exists
    mCatalog.clear();
    mLanguage = language;

    json_error_t error;
    json_t *root;
    auto filename = getFilename();
    root = json_load_file(filename.c_str(), 0, &error);
    if (root == nullptr)
    {
        throw std::runtime_error("json_load_file failed");
    }

    const char *key;
    json_t *value;
    json_object_foreach(root, key, value)
    {
        if (value->type != JSON_STRING)
        {
            continue;
        }

        mCatalog.insert(std::pair<std::string, std::string>(key, json_string_value(value)));
    }

    // Free json data
    json_decref(root);
}

std::string LanguageFile::getFilename()
{
    std::string langStr;
    switch (mLanguage)
    {
        default:
        case ENGLISH:
            langStr = "en";
            break;
        case FRENCH:
            langStr = "fr";
            break;
    }

    return fmt::format("{:s}/{:s}.json", mFolder, langStr);
}

std::string LanguageFile::get(std::string key)
{
    if (mCatalog.find(key) != mCatalog.end())
    {
        return mCatalog.at(key);
    }

    return key;
}

const char* LanguageFile::getc(std::string key)
{
    if (mCatalog.find(key) != mCatalog.end())
    {
        return mCatalog.at(key).c_str();
    }

    return key.c_str();
}
