#pragma once
#include "IWindow.h"
#include "Windows.h"
#include "Overlay/WindowContainer/WindowContainer.h"


class PatchesWindow : public IWindow
{
  public:
    PatchesWindow(const std::string& windowTitle, bool windowClosable)
      : IWindow(windowTitle, windowClosable) {}
    ~PatchesWindow() override = default;
  protected:
    void Draw() override;
};
