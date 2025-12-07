#pragma once
#include "IWindow.h"
#include "Core/utils.h"
#include "Overlay/WindowContainer/WindowContainer.h"

class WinePopupWindow : public IWindow
{
public:
    void Update();
	WinePopupWindow(const std::string& windowTitle, bool windowClosable,
		WindowContainer& windowContainer, ImGuiWindowFlags windowFlags = 0)
		: IWindow(windowTitle, windowClosable, windowFlags), m_pWindowContainer(&windowContainer) {}
	~WinePopupWindow() override = default;
protected:
	void Draw();
	WindowContainer* m_pWindowContainer = nullptr;
};

