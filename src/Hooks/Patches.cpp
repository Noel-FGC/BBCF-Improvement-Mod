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
  char* originalBytes = "\x74";
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

void DisableODStageFilter()
{
  DWORD Address = (DWORD)0x161C6F;
  char* patchedBytes = "\x90\x90\x90\x90\x90\x90"; // nop nop nop nop nop nop
  int byteLength = 6;

  HookManager::OverWriteBytesAtRVA(Address, patchedBytes, byteLength);
}

void RestoreODStageFilter()
{

  DWORD Address = (DWORD)0x161C6F;
  char* originalBytes = "\x89\x81\x50\x01\x00\x00"; // mov [ecx+00000150],eax
  int byteLength = 6;

  HookManager::OverWriteBytesAtRVA(Address, originalBytes, byteLength);
}

void DisableDistortionBG()
{
  DWORD Addr1 = (DWORD)0xC9E56;
  char* patchedBytes1 =  "\x90\x90\x90";
  int byteLength1 = 3;
  DWORD Addr2 = (DWORD)0x1CD266;
  char* patchedBytes2 =  "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90";
  int byteLength2 = 11;
  DWORD Addr3 = (DWORD)0x166C27;
  char* patchedBytes3 =  "\x90\x90\x90\x90\x90\x90\x90";
  int byteLength3 = 7;
  DWORD Addr4 = (DWORD)0x166C3B;
  char* originalBytes4 = "\xC7\x46\x30\x01\x00\x00\x00";
  char* patchedBytes4 = "\x90\x90\x90\x90\x90\x90\x90";
  int byteLength4 = 7;
  DWORD Addr5 = (DWORD)0x15B3F9;
  char* originalBytes5 = "\x89\xB7\xF0\x2D\x06\x00";
  char* patchedBytes5 =  "\x90\x90\x90\x90\x90\x90";
  int byteLength5 = 6;

  HookManager::OverWriteBytesAtRVA(Addr1, patchedBytes1, byteLength1);
  HookManager::OverWriteBytesAtRVA(Addr2, patchedBytes2, byteLength2);
  HookManager::OverWriteBytesAtRVA(Addr3, patchedBytes3, byteLength3);
  HookManager::OverWriteBytesAtRVA(Addr4, patchedBytes4, byteLength4);
  HookManager::OverWriteBytesAtRVA(Addr5, patchedBytes5, byteLength5);
}

void RestoreDistortionBG()
{
  DWORD Addr1 = (DWORD)0xC9E56;
  char* originalBytes1 = "\x89\x70\x1C";
  int byteLength1 = 3;
  DWORD Addr2 = (DWORD)0x1CD266;
  char* originalBytes2 = "\xC7\x84\xB0\xD0\xB5\x01\x00\x1E\x00\x00\x00";
  int byteLength2 = 11;
  DWORD Addr3 = (DWORD)0x166C27;
  char* originalBytes3 = "\xC7\x46\x2C\x01\x00\x00\x00";
  int byteLength3 = 7;
  DWORD Addr4 = (DWORD)0x166C3B;
  char* originalBytes4 = "\xC7\x46\x30\x01\x00\x00\x00";
  int byteLength4 = 7;
  DWORD Addr5 = (DWORD)0x15B3F9;
  char* originalBytes5 = "\x89\xB7\xF0\x2D\x06\x00";
  int byteLength5 = 6;

  HookManager::OverWriteBytesAtRVA(Addr1, originalBytes1, byteLength1);
  HookManager::OverWriteBytesAtRVA(Addr2, originalBytes2, byteLength2);
  HookManager::OverWriteBytesAtRVA(Addr3, originalBytes3, byteLength3);
  HookManager::OverWriteBytesAtRVA(Addr4, originalBytes4, byteLength4);
  HookManager::OverWriteBytesAtRVA(Addr5, originalBytes5, byteLength5);
}


void EnableInstantRestart()
{
    DWORD Address = (DWORD)0x160FEA;
    char* ModdedBytes = "\x90\x90"; // nop nop
    int byteLength = 2;

    HookManager::OverWriteBytesAtRVA(Address, ModdedBytes, byteLength);
}

void DisableInstantRestart()
{

    DWORD Address = (DWORD)0x160FEA;
    char* VanillaBytes = "\x75\x0D"; // jne 00560FF9
    int byteLength = 2;

    HookManager::OverWriteBytesAtRVA(Address, VanillaBytes, byteLength);
}

static std::vector<Patch> patches {
  { "Disable OD Distortion BG Filter",
    "Disables The Red Filter That Displays Over The Distortion Drive Stage Background While In Overdrive, Mainly Used With A Replaced Distortion Stage", &Settings::settingsIni.DisableODDDBGPatch,
    RemoveODFilter, RestoreODFilter, "DisableODDDBGPatch" },
  { "Always Do OD Distortion BG Filter",
    "Always Display The Red Filter That Displays Over The Distortion Drive Stage Background, Even When Not In Overdrive", &Settings::settingsIni.AlwaysODDDBGPatch,
    EnableAlwaysDoODFilter, DisableAlwaysDoODFilter, "AlwaysODDDBGPatch" },
  { "Disable OD Stage Filter",
    "Disable The Brown Filter That Display's Over The Stage During OverDrive", &Settings::settingsIni.DisableODFilterPatch,
    DisableODStageFilter, RestoreODStageFilter, "DisableODFilterPatch" },
  { "Disable DD Stage Background",
    "Disable the stage change when someone does a distortion drive, mainly used with a green screen stage as its not clean", &Settings::settingsIni.DisableDDStagePatch,
    DisableDistortionBG, RestoreDistortionBG, "DisableDDStagePatch" },
  {
      "Enable Instant Restart",
      "Skips The Long Fade To Black Before Resetting Positions In Training Mode", &Settings::settingsIni.EnableInstantRestartPatch,
      EnableInstantRestart, DisableInstantRestart, "EnableInstantRestartPatch"
  }
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
