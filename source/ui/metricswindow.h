#pragma once

#include "window.h"

#include <string>

class MetricsWindow : public Window {

    public:
        MetricsWindow();
        virtual ~MetricsWindow();

        void render();

    private:
        MetricsWindow(const MetricsWindow& copy);

};
