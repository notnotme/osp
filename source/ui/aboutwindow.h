#pragma once

#include "window.h"
#include <string>
#include <SDL2/SDL.h>

class AboutWindow : public Window {

    public:
        AboutWindow();
        virtual ~AboutWindow();

        void render();

    private:
        AboutWindow(const AboutWindow& copy);

};
