#pragma once

struct Patch
{
  const char* name;
  const char* tooltip;
  bool* enabledSetting;
  void (*enable)();
  void (*disable)();
  std::string settingName;
  bool enabled = false;
};


class PatchManager
{
  public:
    static void ApplyPatches();
    static std::vector<Patch> GetPatches();
};

