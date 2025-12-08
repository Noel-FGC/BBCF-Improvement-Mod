#include "WinePopupWindow.h"

#include "Core/interfaces.h"
#include "Core/Settings.h"
#include "Core/utils.h"

#include "Core/info.h"
#include "Overlay/imgui_utils.h"
#include <cstdlib>

void WinePopupWindow::Update()
{
    if (!m_windowOpen)
        return;

    // prevent 2 popups from showing up at once, there's probably a more scalable way of doing this, but for now its fine.
    if (m_pWindowContainer->GetWindow(WindowType_ReplayDBPopup)->IsOpen())
        return;

	BeforeDraw();

	ImGui::Begin(m_windowTitle.c_str(), &m_windowOpen, m_windowFlags);
	Draw();
	ImGui::End();

	AfterDraw();
}

void WinePopupWindow::Draw()
{
    ImVec4 black = ImVec4(0.060, 0.060, 0.060, 1);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, black);
    ImGui::OpenPopup("Are you using Wine?");

    ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    const ImVec2 buttonSize = ImVec2(120, 23);
    ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing);

    ImGui::BeginPopupModal("Are you using Wine?", NULL, ImGuiWindowFlags_AlwaysAutoResize);

        const char txt[] = "\n"
            " It has been detected that you are using software such as Wine or Proton \n"
            " to use the game on an operating system not officially supported by the  \n"
            " BBCFIM developers. There are certain features of BBCFIM that break when \n"
            " using such software, and as such they have been automatically disabled, \n"
            " if this was a mistake and you would like to re-enable them, you may do  \n"
            " so by clicking the 'Enable' button below. If you do enable this while   \n"
            " using wine or proton there is a very likely chance that your game will  \n"
            " no longer open while using BBCFIM, you can fix this easily by manually  \n"
            " changing the setting called 'EnableWineBreakingFeatures' to 0 in the    \n"
            " settings.ini file located in your game installation directory.";
        ImGui::TextUnformatted(txt);
        ImGui::Separator();
        ImGui::AlignItemHorizontalCenter(buttonSize.x);
        if (ImGui::Button("Enable##winepopup", buttonSize)) {
            Settings::changeSetting("EnableWineBreakingFeatures", "1");
            Settings::loadSettingsFile();
            ImGui::CloseCurrentPopup();
            Close();
        }
        //ImGui::SameLine();
        ImGui::AlignItemHorizontalCenter(buttonSize.x);
        if (ImGui::Button("Keep Disabled##winepopup", buttonSize)) {
            Settings::changeSetting("EnableWineBreakingFeatures", "0");
            Settings::loadSettingsFile();
            ImGui::CloseCurrentPopup();
            Close();
        }
        ImGui::EndPopup();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        
    
}
