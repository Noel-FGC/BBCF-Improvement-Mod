#pragma once
#include "IWindow.h"
#include "Windows.h"
#include "Overlay/WindowContainer/WindowContainer.h"

struct Patch
{
  const char* name;
  const char* tooltip;
  bool enabled;
  void (*enable)();
  void (*disable)();
};

class PatchesWindow : public IWindow
{
  public:
    PatchesWindow(const std::string& windowTitle, bool windowClosable)
      : IWindow(windowTitle, windowClosable) {}
    ~PatchesWindow() override = default;
  protected:
    void Draw() override;
  private:
    void ApplyPatches();
};
