#include "aboutwindow.h"

#include "../imgui/imgui.h"
#include "../IconsMaterialDesignIcons_c.h"

#include <sc68/sc68.h>
#include <dumb.h>
#include <sidplayfp/sidplayfp.h>
#include <SDL2/SDL.h>

AboutWindow::AboutWindow() :
    Window(ICON_MDI_INFORMATION " About") {
}

AboutWindow::~AboutWindow() {
}

void AboutWindow::render() {
    if (!mVisible) return;

    const auto imguiIO = ImGui::GetIO();
    const auto imguiStyle = ImGui::GetStyle();
    const auto windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

    if (!ImGui::IsPopupOpen(mTitle.c_str())) {
        ImGui::OpenPopup(mTitle.c_str());
    }
    ImGui::SetNextWindowPos(ImVec2(imguiIO.DisplaySize.x/2, imguiIO.DisplaySize.y/2), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(mTitle.c_str(), &mVisible, windowFlags)) {
        ImGui::TextUnformatted("OSP stand for Old School Player :)");
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::TextUnformatted("It is a chiptune player that can handle several\n"
                               "old sound format produced in the early years of\n"
                               "computer sound and hacking until now.");
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::TextUnformatted("This program make use of:");
        ImGui::BulletText("SDL %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
        ImGui::BulletText("Dear ImGui %s", ImGui::GetVersion());
        ImGui::BulletText("libdumb %s", DUMB_VERSION_STR);
        ImGui::BulletText("libgme 0.5.5");
        ImGui::BulletText("%s", sc68_versionstr());
        ImGui::BulletText("libsidplayfp %d.%d.%d", LIBSIDPLAYFP_VERSION_MAJ, LIBSIDPLAYFP_VERSION_MIN, LIBSIDPLAYFP_VERSION_LEV);
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::TextUnformatted("Embeded font:");
        ImGui::BulletText("Atari ST 8x16 System");
        ImGui::BulletText("ProggyClean");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextUnformatted("Version: " GIT_VERSION " (" GIT_COMMIT ")");
        ImGui::TextUnformatted("Build date: " BUILD_DATE);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - imguiStyle.FramePadding.x - ImGui::CalcTextSize("Close").x);
        if (ImGui::Button("Close")) {
            mVisible = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
