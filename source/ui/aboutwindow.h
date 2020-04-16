#pragma once

#include "window.h"

#include <string>

class AboutWindow : public Window {

    public:
        AboutWindow();
        virtual ~AboutWindow();

        void render();

    private:
        AboutWindow(const AboutWindow& copy);

};
