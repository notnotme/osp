#include "window.h"

Window::Window(const std::string title) :
    Frame(),
    mTitle(title),
    mVisible(false) {
}

Window::~Window() {
}

void Window::setVisible(bool visible) {
    mVisible = visible;
}

bool Window::isVisible() const {
    return mVisible;
}

std::string Window::getTitle() const {
    return mTitle;
}
