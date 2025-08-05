#include "PatchesWindow.h"
#include "IWindow.h"
#include "Core/Interfaces.h"
#include "PatchesWindow.h"
#include "Overlay/imgui_utils.h"
#include "Overlay/WindowManager.h"
#include "Hooks/Patches.h"

void PatchesWindow::Draw()
{
  for (Patch& patch : PatchManager::GetPatches())
  {
    ImGui::Checkbox(patch.name, patch.enabledSetting);
		if (ImGui::IsItemHovered())
    {
      ImGui::SetTooltip(patch.tooltip);
    }
  }

  if (ImGui::Button("Apply"))
    PatchManager::ApplyPatches();
}
