#pragma once

#include <libconfig.h>

class Settings {

    public:
        int style;
        int font;
        bool mouseEmulation;
        bool touchEnabled;
        bool skipUnsupportedTunes;
        bool alwaysStartFirstTune;
        bool skipSubTunes;

        Settings();
        virtual ~Settings();

        void save();

    private:
        Settings(const Settings& copy);

};