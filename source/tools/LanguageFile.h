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
#include <map>

/**
 * Helper to manage languages
 * Probably very unsafe and memory hungry...
 */
class LanguageFile
{
public:
    enum Language
    {
        ENGLISH,
        FRENCH,
        MAX_LANG
    };

    LanguageFile();
    virtual ~LanguageFile();

    void load(std::string folder, Language language);
    void reload(Language language);
    std::string getFilename();

    std::string get(std::string key);
    const char* getc(std::string key);

private:
    // All instance of a LanguageFile will share members.
    // so if any LanguageFile call ::load()/::reload(), all instance created will be affected.
    static std::string mFolder;
    static Language mLanguage;
    static std::map<std::string, std::string> mCatalog;
};
