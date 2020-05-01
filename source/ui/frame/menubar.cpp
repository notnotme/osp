#include "menubar.h"

#include "../../imgui/imgui_impl_sdl.h"
#include "../../strings.h"
#include "../../app_settings_strings.h"

MenuBar::MenuBar() : Frame() {
}

MenuBar::~MenuBar() {
}

void MenuBar::render(const MenuBarData& menuBarData,
    const std::function<void (int)>& onStyleChange,
    const std::function<void (ImFont*, int)>& onFontChange,
    const std::function<void (ItemId)>& onMenuAtion) {

    const auto& style = ImGui::GetStyle();
    auto& io = ImGui::GetIO();

    ImGui::BeginMenuBar();

    if (ImGui::BeginMenu(STR_APPLICATION)) {
        if (ImGui::MenuItem(STR_MENU_ITEM_SHOW_WORKSPACE, nullptr, menuBarData.itemShowWorkspaceCheked, true)) {
            onMenuAtion(TOGGLE_WORKSPACE_VISIBILITY);
        }
        if (ImGui::BeginMenu(STR_MENU_ITEM_THEME)) {

            auto style = menuBarData.settings->getInt(KEY_APP_STYLE, APP_STYLE_DEFAULT);
            if (ImGui::Combo(STR_MENU_ITEM_STYLE, &style, "Dark\0Light\0Classic\0")) {
                onStyleChange(style);
            }

            const auto defaultFont = io.Fonts->Fonts[menuBarData.settings->getInt(KEY_APP_FONT, APP_FONT_DEFAULT)];
            if (ImGui::BeginCombo(STR_MENU_ITEM_FONT, defaultFont->GetDebugName())) {
                for (auto n=0; n<io.Fonts->Fonts.Size; n++) {
                    const auto font = io.Fonts->Fonts[n];
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

        if (ImGui::MenuItem(STR_SETTINGS_WINDOW_TITLE, nullptr, false, true)) {
            onMenuAtion(SHOW_SETTINGS);
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
    ImGui::Text("%s : %s", STR_STATUS, menuBarData.message.c_str());      

    char temp[32];
    sprintf(temp, "%.1f FPS", io.Framerate);
    ImGui::SameLine((ImGui::GetWindowContentRegionWidth() - ImGui::CalcTextSize(temp).x) - style.WindowPadding.x);
    ImGui::Separator();
    ImGui::TextColored(style.Colors[ImGuiCol_PlotHistogramHovered], "%s", temp);  

    ImGui::EndMenuBar();
}
