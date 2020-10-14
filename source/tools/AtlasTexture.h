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

#include <glad/glad.h>
#include <map>
#include <string>


class AtlasTexture
{
public:
    struct AtlasRect
    {
        // Size fo the inner rect inside the atlas
        int width;
        int height;

        // Those are in texture coordinates (0.0f..1.0f)
        float left;
        float top;
        float right;
        float bottom;
    };

    AtlasTexture();
    virtual ~AtlasTexture();

    void setup(std::string filename);
    void cleanup();
    GLuint getTextureId() const;
    AtlasRect getAtlasRect(std::string name);

private:
    GLuint mTextureId;
    std::map<std::string, AtlasRect> mCatalog;

    AtlasTexture(const AtlasTexture& copy);
};
