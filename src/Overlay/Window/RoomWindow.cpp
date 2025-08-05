#include "RoomWindow.h"

#include "Core/interfaces.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Overlay/imgui_utils.h"
#include "Overlay/WindowManager.h"
#include "Overlay/Widget/ActiveGameModeWidget.h"
#include "Overlay/Widget/GameModeSelectWidget.h"
#include "Overlay/Widget/StageSelectWidget.h"
#include "Overlay/Window/PaletteEditorWindow.h"

void RoomWindow::BeforeDraw()
{
	ImGui::SetWindowPos(m_windowTitle.c_str(), ImVec2(200, 200), ImGuiCond_FirstUseEver);

	//ImVec2 windowSizeConstraints;
	//switch (Settings::settingsIni.menusize)
	//{
	//case 1:
	//	windowSizeConstraints = ImVec2(250, 190);
	//	break;
	//case 3:
	//	windowSizeConstraints = ImVec2(400, 230);
	//	break;
	//default:
	//	windowSizeConstraints = ImVec2(330, 230);
	//}

	ImVec2 windowSizeConstraints = ImVec2(300, 190);

	ImGui::SetNextWindowSizeConstraints(windowSizeConstraints, ImVec2(1000, 1000));
}

void RoomWindow::Draw()
{
	if (!g_interfaces.pRoomManager->IsRoomFunctional())
	{
		ImGui::TextDisabled("YOU ARE NOT IN A ROOM OR ONLINE MATCH!");
		ImGui::NewLine();

		ImGui::Text("Online Input Delay");
		ImGui::SameLine();
		ImGui::SliderInt("##Rollback Delay", &g_gameVals.onlineDelay, 1, 5);
		
    if (isInMatch)
    {
      ImGui::VerticalSpacing(10);
      WindowManager::GetInstance().GetWindowContainer()->
        GetWindow<PaletteEditorWindow>(WindowType_PaletteEditor)->ShowAllPaletteSelections("Main");

      ImGui::VerticalSpacing(10);
      ImGui::HorizontalSpacing();
      WindowManager::GetInstance().GetWindowContainer()->
        GetWindow<PaletteEditorWindow>(WindowType_PaletteEditor)->ShowReloadAllPalettesButton();
    }



		m_windowTitle = m_origWindowTitle;

		return;
	}

	std::string roomTypeName = g_interfaces.pRoomManager->GetRoomTypeName();
	SetWindowTitleRoomType(roomTypeName);

	ImGui::Text("Online type: %s", roomTypeName.c_str());

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

	if (isInMenu() || isOnCharacterSelectionScreen())
	{
		ImGui::NewLine();

		ImGui::Text("Online Input Delay");
		ImGui::SameLine();
		ImGui::SliderInt("##Rollback Delay", &g_gameVals.onlineDelay, 1, 5);
	}

	if (isInMatch())
	{
		ImGui::NewLine();
		ImGui::Text("Online Input Delay: %d", g_gameVals.onlineDelay);
	}

	if (isStageSelectorEnabledInCurrentState())
	{
		ImGui::VerticalSpacing(10);
		StageSelectWidget();
	}

	if (isOnCharacterSelectionScreen() || isOnVersusScreen() || isInMatch())
	{
		ImGui::VerticalSpacing(10);
		ActiveGameModeWidget();
	}

	if (isGameModeSelectorEnabledInCurrentState())
	{
		bool isThisPlayerSpectator = g_interfaces.pRoomManager->IsRoomFunctional() && g_interfaces.pRoomManager->IsThisPlayerSpectator();

		if (!isThisPlayerSpectator)
		{
			GameModeSelectWidget();
		}
	}

	if (isInMatch())
	{
		ImGui::VerticalSpacing(10);
		WindowManager::GetInstance().GetWindowContainer()->
			GetWindow<PaletteEditorWindow>(WindowType_PaletteEditor)->ShowAllPaletteSelections("Room");
	}

  DrawRematchSetting();

	if (isInMenu())
	{
		ImGui::VerticalSpacing(10);
		DrawRoomImPlayers();
	}

	if (isOnCharacterSelectionScreen() || isOnVersusScreen() || isInMatch())
	{
		ImGui::VerticalSpacing(10);
		DrawMatchImPlayers();
	}

	ImGui::PopStyleVar();

}

void RoomWindow::SetWindowTitleRoomType(const std::string& roomTypeName)
{
	m_windowTitle = "Online - " + roomTypeName + "###Room";
}

void RoomWindow::ShowClickableSteamUser(const char* playerName, const CSteamID& steamId) const
{
	ImGui::TextUnformatted(playerName);
	ImGui::HoverTooltip("Click to open Steam profile");
	if (ImGui::IsItemClicked())
	{
		g_interfaces.pSteamFriendsWrapper->ActivateGameOverlayToUser("steamid", steamId);
	}
}

void RoomWindow::DrawRoomImPlayers()
{
	ImGui::BeginGroup();
	ImGui::TextUnformatted("Improvement Mod users in Room:");
	ImGui::BeginChild("RoomImUsers", ImVec2(230, 150), true);

	for (const IMPlayer& imPlayer : g_interfaces.pRoomManager->GetIMPlayersInCurrentRoom())
	{
		ShowClickableSteamUser(imPlayer.steamName.c_str(), imPlayer.steamID);
		ImGui::NextColumn();
	}

	ImGui::EndChild();
	ImGui::EndGroup();
}

void RoomWindow::DrawMatchImPlayers()
{
	ImGui::BeginGroup();
	ImGui::TextUnformatted("Improvement Mod users in match:");
	ImGui::BeginChild("MatchImUsers", ImVec2(230, 150), true);

	if (g_interfaces.pRoomManager->IsThisPlayerInMatch())
	{
		ImGui::Columns(2);
		for (const IMPlayer& imPlayer : g_interfaces.pRoomManager->GetIMPlayersInCurrentMatch())
		{
			uint16_t matchPlayerIndex = g_interfaces.pRoomManager->GetPlayerMatchPlayerIndexByRoomMemberIndex(imPlayer.roomMemberIndex);
			std::string playerType;

			if (matchPlayerIndex == 0)
				playerType = "Player 1";
			else if (matchPlayerIndex == 1)
				playerType = "Player 2";
			else
				playerType = "Spectator";

			ShowClickableSteamUser(imPlayer.steamName.c_str(), imPlayer.steamID);
			ImGui::NextColumn();

			ImGui::TextUnformatted(playerType.c_str());
			ImGui::NextColumn();
		}
	}

	ImGui::EndChild();
	ImGui::EndGroup();
}

void RoomWindow::DrawRematchSetting()
{
    const char* items[] = { "No Rematch", "No limit", "FT2", "FT3", "FT5", "FT10"};
    static int currentItem = 0;

    if (!g_gameVals.pRoom || g_gameVals.pRoom->roomStatus == RoomStatus_Unavailable 
        || !(RoomManager::GetRoomSettingsStaticBaseAdress()))
    {
        ImGui::TextUnformatted("Room is not available!");
        return;
    }
    //sets the selected value in the dropdown the actual value
    switch (g_gameVals.pRoom->rematch) {
    case RoomRematch::RematchType_Disabled:
        currentItem = 0;
        break;
    case RoomRematch::RematchType_Unlimited:
        currentItem = 1;
        break;
    case RoomRematch::RematchType_Ft2:
        currentItem = 2;
        break;
    case RoomRematch::RematchType_Ft3:
        currentItem = 3;
        break;
    case RoomRematch::RematchType_Ft5:
        currentItem = 4;
        break;
    case RoomRematch::RematchType_Ft10:
        currentItem = 5;
        break;
    default:
        break;
    }

    ImGui::Text("Rematch Settings");
    ImGui::SameLine();
    if (ImGui::Combo("##dropdown", &currentItem, items, IM_ARRAYSIZE(items)))
    {
        switch (currentItem) {
        case 0:
            g_interfaces.pRoomManager->ChangeRematchAmnt(0);
            break;

        case 1:
            g_interfaces.pRoomManager->ChangeRematchAmnt(-1);
            break;

        case 2:
            g_interfaces.pRoomManager->ChangeRematchAmnt(2);
            break;

        case 3:
            g_interfaces.pRoomManager->ChangeRematchAmnt(3);
            break;

        case 4:
            g_interfaces.pRoomManager->ChangeRematchAmnt(5);
            break;

        case 5:
            g_interfaces.pRoomManager->ChangeRematchAmnt(10);
            break;



        default:
            break;
        }

    };

}
