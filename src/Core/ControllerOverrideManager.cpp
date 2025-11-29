#include "ControllerOverrideManager.h"

#include "dllmain.h"

#include <Shellapi.h>
#include <dinput.h>

#include <algorithm>
#include <numeric>

namespace
{
        constexpr ULONGLONG DEVICE_REFRESH_INTERVAL_MS = 1000;

        size_t HashDevices(const std::vector<ControllerDeviceInfo>& devices)
        {
                return std::accumulate(devices.begin(), devices.end(), static_cast<size_t>(0), [](size_t acc, const ControllerDeviceInfo& info) {
                        acc ^= std::hash<std::string>{}(info.name) + 0x9e3779b97f4a7c15ULL + (acc << 6) + (acc >> 2);
                        acc ^= info.isKeyboard ? 0xfeedfaceULL : 0; // differentiate keyboard entries
                        acc ^= std::hash<unsigned long>{}(info.guid.Data1);
                        return acc;
                });
        }

        template <typename InstanceType>
        GUID GetGuidFromInstance(const InstanceType& instance)
        {
                return instance.guidInstance;
        }

        template <>
        GUID GetGuidFromInstance<DIDEVICEINSTANCEW>(const DIDEVICEINSTANCEW& instance)
        {
                return instance.guidInstance;
        }
}

ControllerOverrideManager& ControllerOverrideManager::GetInstance()
{
        static ControllerOverrideManager instance;
        return instance;
}

ControllerOverrideManager::ControllerOverrideManager()
{
        m_playerSelections[0] = GUID_NULL;
        m_playerSelections[1] = GUID_NULL;
        RefreshDevices();
}

void ControllerOverrideManager::SetOverrideEnabled(bool enabled)
{
        m_overrideEnabled = enabled;
}

bool ControllerOverrideManager::IsOverrideEnabled() const
{
        return m_overrideEnabled;
}

void ControllerOverrideManager::SetPlayerSelection(int playerIndex, const GUID& guid)
{
        if (playerIndex < 0 || playerIndex > 1)
                return;

        m_playerSelections[playerIndex] = guid;
        EnsureSelectionsValid();
}

GUID ControllerOverrideManager::GetPlayerSelection(int playerIndex) const
{
        if (playerIndex < 0 || playerIndex > 1)
                return GUID_NULL;

        return m_playerSelections[playerIndex];
}

const std::vector<ControllerDeviceInfo>& ControllerOverrideManager::GetDevices() const
{
        return m_devices;
}

void ControllerOverrideManager::RefreshDevices()
{
        CollectDevices();
        EnsureSelectionsValid();
        m_lastRefresh = GetTickCount64();
        m_lastDeviceHash = HashDevices(m_devices);
}

void ControllerOverrideManager::TickAutoRefresh()
{
        if (GetTickCount64() - m_lastRefresh < DEVICE_REFRESH_INTERVAL_MS)
        {
                return;
        }

        if (CollectDevices())
        {
                size_t newHash = HashDevices(m_devices);
                if (newHash != m_lastDeviceHash)
                {
                        EnsureSelectionsValid();
                        m_lastDeviceHash = newHash;
                }
        }

        m_lastRefresh = GetTickCount64();
}

void ControllerOverrideManager::ApplyOrdering(std::vector<DIDEVICEINSTANCEA>& devices) const
{
        ApplyOrderingImpl(devices);
}

void ControllerOverrideManager::ApplyOrdering(std::vector<DIDEVICEINSTANCEW>& devices) const
{
        ApplyOrderingImpl(devices);
}

void ControllerOverrideManager::OpenControllerControlPanel() const
{
        ShellExecute(nullptr, TEXT("open"), TEXT("rundll32.exe"), TEXT("shell32.dll,Control_RunDLL joy.cpl"), nullptr, SW_SHOWNORMAL);
}

template <typename T>
void ControllerOverrideManager::ApplyOrderingImpl(std::vector<T>& devices) const
{
        if (!m_overrideEnabled || devices.empty())
        {
                return;
        }

        auto guidForIndex = [this](int idx) {
                if (idx < 0 || idx > 1)
                        return GUID_NULL;
                return m_playerSelections[idx];
        };

        std::vector<bool> consumed(devices.size(), false);
        std::vector<T> ordered;
        ordered.reserve(devices.size());

        auto appendByGuid = [&](const GUID& guid) {
                if (guid == GUID_NULL)
                        return;

                for (size_t i = 0; i < devices.size(); ++i)
                {
                        if (consumed[i])
                                continue;

                        if (IsEqualGUID(GetGuidFromInstance(devices[i]), guid))
                        {
                                ordered.push_back(devices[i]);
                                consumed[i] = true;
                                break;
                        }
                }
        };

        appendByGuid(guidForIndex(0));
        appendByGuid(guidForIndex(1));

        for (size_t i = 0; i < devices.size(); ++i)
        {
                        if (!consumed[i])
                        {
                                ordered.push_back(devices[i]);
                        }
        }

        devices.swap(ordered);
}

void ControllerOverrideManager::EnsureSelectionsValid()
{
        auto containsGuid = [this](const GUID& guid) {
                return std::any_of(m_devices.begin(), m_devices.end(), [&](const ControllerDeviceInfo& info) {
                        return IsEqualGUID(info.guid, guid);
                });
        };

        for (int i = 0; i < 2; ++i)
        {
                if (m_playerSelections[i] == GUID_NULL || containsGuid(m_playerSelections[i]))
                        continue;

                if (!m_devices.empty())
                {
                        m_playerSelections[i] = m_devices.front().guid;
                }
                else
                {
                        m_playerSelections[i] = GUID_NULL;
                }
        }
}

bool ControllerOverrideManager::CollectDevices()
{
        std::vector<ControllerDeviceInfo> devices;
        devices.push_back({ GUID_SysKeyboard, "Keyboard", true });

        bool success = TryEnumerateDevicesA();

        if (!success)
        {
                success = TryEnumerateDevicesW();
        }

        if (!success)
        {
                m_devices = devices;
                return false;
        }

        // TryEnumerateDevices* store results into m_devices temporarily. Move them into devices list.
        devices.insert(devices.end(), m_devices.begin(), m_devices.end());
        m_devices.swap(devices);
        return true;
}

bool ControllerOverrideManager::TryEnumerateDevicesA()
{
        if (!orig_DirectInput8Create)
        {
                return false;
        }

        IDirectInput8A* dinput = nullptr;
        if (FAILED(orig_DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8A, (LPVOID*)&dinput, nullptr)))
        {
                return false;
        }

        std::vector<ControllerDeviceInfo> devices;

        auto callback = [](const DIDEVICEINSTANCEA* inst, VOID* ref) -> BOOL {
                auto* deviceList = reinterpret_cast<std::vector<ControllerDeviceInfo>*>(ref);
                ControllerDeviceInfo info{};
                info.guid = inst->guidInstance;
                info.isKeyboard = false;
                info.name = inst->tszProductName;
                deviceList->push_back(info);
                return DIENUM_CONTINUE;
        };

        HRESULT enumResult = dinput->EnumDevices(DI8DEVCLASS_GAMECTRL, callback, &devices, DIEDFL_ATTACHEDONLY);
        dinput->Release();

        if (FAILED(enumResult))
        {
                return false;
        }

        m_devices.swap(devices);
        return true;
}

bool ControllerOverrideManager::TryEnumerateDevicesW()
{
        if (!orig_DirectInput8Create)
        {
                return false;
        }

        IDirectInput8W* dinput = nullptr;
        if (FAILED(orig_DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W, (LPVOID*)&dinput, nullptr)))
        {
                return false;
        }

        std::vector<ControllerDeviceInfo> devices;

        auto callback = [](const DIDEVICEINSTANCEW* inst, VOID* ref) -> BOOL {
                auto* deviceList = reinterpret_cast<std::vector<ControllerDeviceInfo>*>(ref);
                ControllerDeviceInfo info{};
                info.guid = inst->guidInstance;
                info.isKeyboard = false;
                info.name = ControllerOverrideManager::WideToUtf8(inst->tszProductName);
                deviceList->push_back(info);
                return DIENUM_CONTINUE;
        };

        HRESULT enumResult = dinput->EnumDevices(DI8DEVCLASS_GAMECTRL, callback, &devices, DIEDFL_ATTACHEDONLY);
        dinput->Release();

        if (FAILED(enumResult))
        {
                return false;
        }

        m_devices.swap(devices);
        return true;
}

std::string ControllerOverrideManager::WideToUtf8(const std::wstring& value)
{
        if (value.empty())
                return {};

        int required = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);

        if (required <= 0)
                return {};

        std::string output(static_cast<size_t>(required), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, output.data(), required, nullptr, nullptr);
        // Remove the trailing null the API writes
        if (!output.empty() && output.back() == '\0')
        {
                output.pop_back();
        }
        return output;
}
