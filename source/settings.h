#pragma once

#include <string>
#include <libconfig.h>

class Settings {

    public:
        Settings();
        virtual ~Settings();

        void putInt(const std::string key, int value) const;
        void putBool(const std::string key, bool value) const;
        int getInt(const std::string key, int defaultValue) const;
        bool getBool(const std::string key, bool defaultValue) const;

        void load(std::string filename);
        void save(std::string filename);

    private:
        config_t mConfig;

        Settings(const Settings& copy);

};