#include "metricswindow.h"

#include "../imgui/imgui.h"
#include "../IconsMaterialDesignIcons_c.h"

MetricsWindow::MetricsWindow() :
    Window(ICON_MDI_CHART_BAR " Dear ImGui Metrics") {
}

MetricsWindow::~MetricsWindow() {
}

void MetricsWindow::render() {
    if (!mVisible) {
        return;
    }
    
    ImGui::ShowMetricsWindow(&mVisible);
}
