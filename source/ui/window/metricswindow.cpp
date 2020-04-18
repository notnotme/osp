#include "metricswindow.h"

#include "../../imgui/imgui.h"
#include "../../IconsMaterialDesignIcons_c.h"
#include "../../strings.h"

MetricsWindow::MetricsWindow() :
    Window(STR_METRICS_WINDOW_TITLE) {
}

MetricsWindow::~MetricsWindow() {
}

void MetricsWindow::render() {
    if (!mVisible) {
        return;
    }
    
    ImGui::ShowMetricsWindow(&mVisible);
}
