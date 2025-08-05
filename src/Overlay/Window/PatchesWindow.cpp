#include "PatchesWindow.h"
#include "IWindow.h"
#include "Core/Interfaces.h"
#include "PatchesWindow.h"
#include "Overlay/imgui_utils.h"
#include "Overlay/WindowManager.h"
#include "Hooks/HookManager.h"

void RemoveODFilter()
{
  DWORD Address = (DWORD)0x1d88c1;
  char* patchedBytes = "\xEB";
  int byteLength = 1;

  HookManager::OverWriteBytesAtRVA(Address, patchedBytes, byteLength);
}

void RestoreODFilter()
{
  DWORD Address = (DWORD)0x1d88c1;
  char* originalBytes = "\x71";
  int byteLength = 1;
  HookManager::OverWriteBytesAtRVA(Address, originalBytes, byteLength);
}

void EnableAlwaysDoODFilter()
{
  DWORD Address = (DWORD)0x1d88c1;
  char* originalBytes = "\x90\x90";
  int byteLength = 2;

  HookManager::OverWriteBytesAtRVA(Address, originalBytes, byteLength);
}

void DisableAlwaysDoODFilter()
{
  DWORD Address = (DWORD)0x1d88c1;
  char* originalBytes = "\x74\x71";
  int byteLength = 2;

  HookManager::OverWriteBytesAtRVA(Address, originalBytes, byteLength);
}

static std::vector<Patch> patches = {
  { "Disable OD Distortion BG Filter",
    "Disables The Red Filter That Displays Over The Distortion Drive Stage Background While In Overdrive", false,
    RemoveODFilter, RestoreODFilter },
  { "Always Do OD Distortion BG Filter",
    "Always Display The Red Filter That Displays Over The Distortion Drive Stage Background, Even When Not In Overdrive", false,
    EnableAlwaysDoODFilter, DisableAlwaysDoODFilter }
};

void PatchesWindow::Draw()
{
  for (auto& patch : patches)
  {
    ImGui::Checkbox(patch.name, &patch.enabled);
		if (ImGui::IsItemHovered())
    {
      ImGui::SetTooltip(patch.tooltip);
    }
  }

  if (ImGui::Button("Apply"))
    ApplyPatches();
}

void PatchesWindow::ApplyPatches()
{
  for (auto& patch : patches)
  {
    patch.disable();
  }
  for (auto& patch : patches)
  {
    if (patch.enabled)
    {
      patch.enable();
    }
  }
};

