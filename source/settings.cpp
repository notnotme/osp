#include "settings.h"

#include "SDL2/SDL_log.h"

Settings::Settings() :
    style(0),
    font(0),
    mouseEmulation(true),
    touchEnabled(true),
    skipUnsupportedTunes(true),
    alwaysStartFirstTune(false),
    skipSubTunes(false) {
}

Settings::~Settings() {
}

void Settings::load(std::string filename) {
    config_t cfg;
    config_init(&cfg);
    
    if (! config_read_file(&cfg, filename.c_str())) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unable to load %s\n", filename.c_str());
        config_destroy(&cfg);
        return;
    }

    config_lookup_int(&cfg, "style", &style);
    config_lookup_int(&cfg, "font", &font);

    int value;
    config_lookup_bool(&cfg, "mouseEmulation", &value);
    mouseEmulation = value == 1;

    config_lookup_bool(&cfg, "touchEnabled", &value);
    touchEnabled = value == 1;

    config_lookup_bool(&cfg, "skipUnsupportedTunes", &value);
    skipUnsupportedTunes = value == 1;

    config_lookup_bool(&cfg, "alwaysStartFirstTune", &value);
    alwaysStartFirstTune = value == 1;

    config_lookup_bool(&cfg, "skipSubTunes", &value);
    skipSubTunes = value == 1;

    config_destroy(&cfg);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Config loaded %s\n", filename.c_str());
}

void Settings::save(std::string filename) {
    config_t cfg;
    config_init(&cfg);
    const auto root = config_root_setting(&cfg);

    config_setting_set_int(config_setting_add(root, "style", CONFIG_TYPE_INT), style);
    config_setting_set_int(config_setting_add(root, "font", CONFIG_TYPE_INT), font);
    config_setting_set_bool(config_setting_add(root, "mouseEmulation", CONFIG_TYPE_BOOL), mouseEmulation);
    config_setting_set_bool(config_setting_add(root, "touchEnabled", CONFIG_TYPE_BOOL), touchEnabled);
    config_setting_set_bool(config_setting_add(root, "skipUnsupportedTunes", CONFIG_TYPE_BOOL), skipUnsupportedTunes);
    config_setting_set_bool(config_setting_add(root, "alwaysStartFirstTune", CONFIG_TYPE_BOOL), alwaysStartFirstTune);
    config_setting_set_bool(config_setting_add(root, "skipSubTunes", CONFIG_TYPE_BOOL), skipSubTunes);
    
    if(! config_write_file(&cfg, filename.c_str())) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Unable to save %s\n", filename.c_str());
    } else {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Config saved %s\n", filename.c_str());
    }
    config_destroy(&cfg);
}
