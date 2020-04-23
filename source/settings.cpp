#include "settings.h"

#include "SDL2/SDL_log.h"

Settings::Settings() {
    config_init(&mConfig);
}

Settings::~Settings() {
    config_destroy(&mConfig);
}

void Settings::load(std::string filename) {
    if (! config_read_file(&mConfig, filename.c_str())) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unable to load %s\n", filename.c_str());
    } else {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Config loaded %s\n", filename.c_str());
    }
}

void Settings::save(std::string filename) {
    if(! config_write_file(&mConfig, filename.c_str())) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unable to save %s\n", filename.c_str());
    } else {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Config saved %s\n", filename.c_str());
    }
}

void Settings::putInt(const std::string key, int value) const {
    auto setting = config_setting_get_member(mConfig.root, key.c_str());
    if (setting == nullptr) {
        setting = config_setting_add(mConfig.root, key.c_str(), CONFIG_TYPE_INT);
    }

    config_setting_set_int(setting, value);
}

int Settings::getInt(const std::string key, int defaultValue) const {
    auto setting = config_setting_get_member(mConfig.root, key.c_str());
    if (setting == nullptr) {
        return defaultValue;
    }

    return config_setting_get_int(setting);
}

void Settings::putBool(const std::string key, bool value) const {
    auto setting = config_setting_get_member(mConfig.root, key.c_str());
    if (setting == nullptr) {
        setting = config_setting_add(mConfig.root, key.c_str(), CONFIG_TYPE_BOOL);
    }

    config_setting_set_bool(setting, value);
}

bool Settings::getBool(const std::string key, bool defaultValue) const {
    auto setting = config_setting_get_member(mConfig.root, key.c_str());
    if (setting == nullptr) {
        return defaultValue;
    }

    return config_setting_get_bool(setting) == 1;
}
