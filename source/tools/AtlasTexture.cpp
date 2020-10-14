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
#include "AtlasTexture.h"

#include <stdexcept>
#include <filesystem>
#include <jansson.h>

#include <SDL2/SDL_image.h>


AtlasTexture::AtlasTexture() :
mTextureId(0)
{
}

AtlasTexture::~AtlasTexture()
{
}

void AtlasTexture::setup(std::string filename)
{
    // Load JSON atlas
    json_error_t error;
    json_t *root, *size, *frame, *value, *meta, *frames;
    int catalogWidth, catalogHeight;
    std::string associatedImage;

    root = json_load_file(filename.c_str(), 0, &error);
    if (root == nullptr)
        throw std::runtime_error("json_load_file failed");

    // META ----------------------
    meta = json_object_get(root, "meta");
    frames = json_object_get(root, "frames");
    if (meta == nullptr || frames == nullptr)
    {
        json_decref(root);
        throw std::runtime_error("Malformed sprite catalog");
    }

    size = json_object_get(meta, "size");
    if (size == nullptr)
    {
        json_decref(root);
        throw std::runtime_error("Malformed sprite catalog");
    }

    value = json_object_get(size, "w");
    if (value != nullptr)
        catalogWidth = json_integer_value(value);
    else
    {
        json_decref(root);
        throw std::runtime_error("Malformed sprite catalog");
    }

    value = json_object_get(size, "h");
    if (value != nullptr)
        catalogHeight = json_integer_value(value);
    else
    {
        json_decref(root);
        throw std::runtime_error("Malformed sprite catalog");
    }

    value = json_object_get(meta, "image");
    if (value != nullptr)
        associatedImage = json_string_value(value);
    else
    {
        json_decref(root);
        throw std::runtime_error("Malformed sprite catalog");
    }

    // FRAMES ----------------------
    const char *key;
    json_object_foreach(frames, key, value)
    {
        int x,y,w,h;
        frame = json_object_get(value, "frame");

        value = json_object_get(frame, "x");
        x = json_integer_value(value);

        value = json_object_get(frame, "y");
        y = json_integer_value(value);

        value = json_object_get(frame, "w");
        w = json_integer_value(value);

        value = json_object_get(frame, "h");
        h = json_integer_value(value);

        mCatalog.insert(std::pair<std::string, AtlasRect>(key,
        (AtlasRect) {
            .width = w,
            .height =  h,
            .left = (float) x / (float) catalogWidth,
            .top = (float) y / (float) catalogHeight,
            .right = (float) (x + w) / (float) catalogWidth,
            .bottom = (float) (y + h) / (float) catalogHeight
        }));
    }

    // Free json data
    json_decref(root);

    // Creating the OpenGL texture to store image data in graphic memory
    glGenTextures(1, &mTextureId);
    if (mTextureId < 0)
        throw std::runtime_error("Can't create texture for atlas");

    // Load image data into the texture
    auto imageFilename = std::filesystem::path(filename).replace_filename(associatedImage);
    auto* image = IMG_Load(imageFilename.c_str());
    if (image == nullptr)
        throw std::runtime_error("Unable to load the atlas image");

    glBindTexture(GL_TEXTURE_2D, mTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);

    SDL_FreeSurface(image);
}

void AtlasTexture::cleanup()
{
    mCatalog.clear();

    // Deletet texture from graphic memory
    if (mTextureId >= 0)
        glDeleteTextures(1, &mTextureId);
}

GLuint AtlasTexture::getTextureId() const
{
    return mTextureId;
}

AtlasTexture::AtlasRect AtlasTexture::getAtlasRect(std::string name)
{
    if (mCatalog.find(name) != mCatalog.end())
        return mCatalog[name];

    throw std::runtime_error("AtlasRect not found");
}
