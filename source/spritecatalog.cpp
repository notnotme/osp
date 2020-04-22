#include "spritecatalog.h"
#include <jansson.h>

#include "SDL2/SDL_log.h"

SpriteCatalog::SpriteCatalog() {
}

SpriteCatalog::~SpriteCatalog() {
}

bool SpriteCatalog::setup(const std::string filename) {
    json_error_t error;
    json_t *root, *size, *frame, *value, *meta, *frames;
    int catalogWidth, catalogHeight;

    root = json_load_file(filename.c_str(), 0, &error);
    if (root == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "json error on line %d: %s\n", error.line, error.text);
        mError = std::string("Cannot load sprite catalog : ").append(filename);
        return false;
    }

    // META ----------------------
    meta = json_object_get(root, "meta");
    frames = json_object_get(root, "frames");
    if (meta == nullptr || frames == nullptr) {
        mError = std::string("Malformed sprite catalog : ").append(filename);
        goto _fail;
    }
    
    size = json_object_get(meta, "size");
    if (size == nullptr) {
        mError = std::string("Malformed sprite catalog : ").append(filename);
        goto _fail;
    }

    value = json_object_get(size, "w");
    if (value == nullptr) {
        mError = std::string("Malformed sprite catalog : ").append(filename);
        goto _fail;
    }
    catalogWidth = json_integer_value(value);

    value = json_object_get(size, "h");
    if (value == nullptr) {
        mError = std::string("Malformed sprite catalog : ").append(filename);
        goto _fail;
    }
    catalogHeight = json_integer_value(value);

    // FRAMES ----------------------
    const char *key;
    json_object_foreach(frames, key, value) {
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

        mCatalog.insert(std::pair<std::string, Frame>(key, (Frame) {
            .size = glm::vec2(w, h),
            .uv0 = glm::vec2((float) x / (float) catalogWidth, (float) y / (float) catalogHeight),
            .uv1 = glm::vec2((float) (x + w) / (float) catalogWidth, (float) (y + h) / (float) catalogHeight)
        }));
    }

    json_decref(root);
    return true;

_fail:
    json_decref(root);
    return false;
}

void SpriteCatalog::cleanup() {
    mCatalog.clear();
}

const SpriteCatalog::Frame SpriteCatalog::getFrame(const std::string name) {
    if (mCatalog.find(name) == mCatalog.end()) {
        mError = std::string("Frame not found : ").append(name.c_str());
        return Frame();
    }
    
    return mCatalog[name];
}

std::string SpriteCatalog::getError() const {
    return mError;
}
