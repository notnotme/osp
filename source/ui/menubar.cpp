#include "menubar.h"

#include "../imgui/imgui_impl_sdl.h"
#include "../IconsMaterialDesignIcons_c.h"
#include "../strings.h"

MenuBar::MenuBar() {
}

MenuBar::~MenuBar() {
}

void MenuBar::render(const MenuBarData& menuBarData,
            std::function<void (int)> onStyleChange,
            std::function<void (ImFont*, int)> onFontChange,
            std::function<void (MenuAction)> onMenuAtion) {

    const auto& style = ImGui::GetStyle();
    auto& io = ImGui::GetIO();

    ImGui::BeginMenuBar();

    if (ImGui::BeginMenu(STR_MENU_ITEM_APPLICATION)) {
        if (ImGui::MenuItem(STR_MENU_ITEM_SHOW_WORKSPACE, nullptr, menuBarData.itemShowWorkspaceCheked, true)) {
            onMenuAtion(TOGGLE_WORKSPACE_VISIBILITY);
        }
        if (ImGui::BeginMenu(STR_MENU_ITEM_THEME)) {

            auto style = menuBarData.selectedStyle;
            if (ImGui::Combo(STR_MENU_ITEM_STYLE, &style, "Dark\0Light\0Classic\0")) {
                onStyleChange(style);
            }

            const auto defaultFont = io.Fonts->Fonts[menuBarData.selectedFont];
            if (ImGui::BeginCombo(STR_MENU_ITEM_FONT, defaultFont->GetDebugName())) {
                for (auto n=0; n<io.Fonts->Fonts.Size; n++) {
                    ImFont* font = io.Fonts->Fonts[n];
                    ImGui::PushID((void*) font);
                    if (ImGui::Selectable(font->GetDebugName(), font == defaultFont)) {
                        onFontChange(font, n);
                    }
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(STR_MENU_ITEM_CONFIGURATION, true)) {
            if (ImGui::MenuItem(STR_MENU_ITEM_MOUSE_EMULATION, nullptr, menuBarData.mouseEmulationEnabled, true)) {
                onMenuAtion(TOGGLE_MOUSE_EMULATION);
            }
            if (ImGui::MenuItem(STR_MENU_ITEM_TOUCH_ENABLED, nullptr, menuBarData.touchEnabled, true)) {
                onMenuAtion(TOGGLE_TOUCH);
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem(STR_MENU_ITEM_QUIT, nullptr, false, menuBarData.fmState == FileManager::State::READY)) {
            onMenuAtion(QUIT);
        }
        ImGui::EndMenu();
    }
        
    if (ImGui::BeginMenu(STR_MENU_ITEM_HELP)) {
        if (ImGui::MenuItem(STR_METRICS_WINDOW_TITLE, nullptr, false, true)) {
            onMenuAtion(SHOW_METRICS);
        }
        if (ImGui::MenuItem(STR_ABOUT_WINDOW_TITLE)) {
            onMenuAtion(SHOW_ABOUT);
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();
    ImGui::Text("Status: %s", menuBarData.message.c_str());      

    char temp[32];
    sprintf(temp, "%.1f FPS", io.Framerate);
    ImGui::SameLine((ImGui::GetWindowContentRegionWidth() - ImGui::CalcTextSize(temp).x) - style.WindowPadding.x);
    ImGui::Separator();
    ImGui::TextColored(style.Colors[ImGuiCol_PlotHistogramHovered], "%s", temp);  

    ImGui::EndMenuBar();
}
