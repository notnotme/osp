#pragma once

#include <string>
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

        void load(std::string filename);
        void save(std::string filename);

};