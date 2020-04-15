#pragma once

#include <string>
#include <SDL2/SDL.h>

class Window {

    public:
        Window(const std::string title);
        virtual ~Window();

        virtual std::string getTitle();
        void setVisible(bool visible);
        bool isVisible();
        
    protected:
        std::string mTitle;
        bool mVisible;

    private:
        Window(const Window& copy);

};
