#pragma once

#include <string>

class Window {

    public:
        Window(const std::string title);
        virtual ~Window();

        virtual std::string getTitle() const;
        void setVisible(bool visible);
        bool isVisible() const;
        
    protected:
        std::string mTitle;
        bool mVisible;

    private:
        Window(const Window& copy);

};
