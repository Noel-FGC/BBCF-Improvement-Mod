#include "Core/Interfaces.h"
#include "Core/Settings.h"
#include "Hooks/HookManager.h"
#include "Patches.h"

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
    "Disables The Red Filter That Displays Over The Distortion Drive Stage Background While In Overdrive", &Settings::settingsIni.DisableODDDBGPatch,
    RemoveODFilter, RestoreODFilter, "DisableODDDBGPatch" },
  { "Always Do OD Distortion BG Filter",
    "Always Display The Red Filter That Displays Over The Distortion Drive Stage Background, Even When Not In Overdrive", &Settings::settingsIni.AlwaysODDDBGPatch,
    EnableAlwaysDoODFilter, DisableAlwaysDoODFilter, "AlwaysODDDBGPatch" }
};

void PatchManager::ApplyPatches()
{
  // these should remain separate, otherwise a patches disable function 
  // could overwrite bytes changed by a previous patch's enable function
  // if they're both toggled at once.
  // this will probably still be atleast a little error prone once more
  // patches are added, but for right now its fine
  for (auto& patch : patches)
  {
    if (patch.enabled && !*patch.enabledSetting) // disable all enabled patches that aren't marked to be enabled.
      patch.disable();
  }
  for (auto& patch : patches)
  {
    if (*patch.enabledSetting && !patch.enabled) // enable all disabled patches marked to be enabled
      patch.enable();
    if (patch.enabled != *patch.enabledSetting)
    {
      Settings::changeSetting(patch.settingName, std::to_string(*patch.enabledSetting));
      patch.enabled = *patch.enabledSetting;
    }
  }
};

std::vector<Patch> PatchManager::GetPatches()
{
  return patches;
}
