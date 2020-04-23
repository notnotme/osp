#pragma once

#include "../window.h"
#include "../../spritecatalog.h"

#include <string>
#include <glad/glad.h>

class AboutWindow : public Window {

    public:
        AboutWindow();
        virtual ~AboutWindow();

        void render(const GLuint texture, const SpriteCatalog::Frame& logoFrame);

    private:
        AboutWindow(const AboutWindow& copy);

};
