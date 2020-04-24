#pragma once

#include "../window.h"
#include "../../spritecatalog.h"

#include <string>
#include <memory>
#include <glad/glad.h>

class AboutWindow : public Window {

    public:
        AboutWindow();
        virtual ~AboutWindow();

        void render(const GLuint texture, std::shared_ptr<SpriteCatalog> catalog);

    private:
        AboutWindow(const AboutWindow& copy);

};
