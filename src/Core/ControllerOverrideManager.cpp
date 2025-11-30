#include "ControllerOverrideManager.h"

#include "dllmain.h"

#include <Shellapi.h>
#include <dinput.h>
#include <mmsystem.h>
#include <joystickapi.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <numeric>

namespace
{
        constexpr ULONGLONG DEVICE_REFRESH_INTERVAL_MS = 1000;
        constexpr UINT WINMM_INVALID_ID = static_cast<UINT>(-1);

        std::wstring GetPreferredSystemExecutable(const wchar_t* executableName)
        {
                wchar_t windowsDir[MAX_PATH] = {};
                UINT len = GetWindowsDirectoryW(windowsDir, MAX_PATH);

                if (len == 0 || len >= MAX_PATH)
                {
                        return std::wstring(executableName);
                }

                auto buildPath = [&](const wchar_t* subdir) {
                        std::wstring path(windowsDir, len);
                        path += L"\\";
                        path += subdir;
                        path += L"\\";
                        path += executableName;
                        return path;
                };

                // Prefer the native (64-bit) system executable when running from a 32-bit process.
                std::wstring sysnativePath = buildPath(L"Sysnative");
                if (GetFileAttributesW(sysnativePath.c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                        return sysnativePath;
                }

                std::wstring system32Path = buildPath(L"System32");
                if (GetFileAttributesW(system32Path.c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                        return system32Path;
                }

                return std::wstring(executableName);
        }

        size_t HashDevices(const std::vector<ControllerDeviceInfo>& devices)
        {
                return std::accumulate(devices.begin(), devices.end(), static_cast<size_t>(0), [](size_t acc, const ControllerDeviceInfo& info) {
                        acc ^= std::hash<std::string>{}(info.name) + 0x9e3779b97f4a7c15ULL + (acc << 6) + (acc >> 2);
                        acc ^= info.isKeyboard ? 0xfeedfaceULL : 0; // differentiate keyboard entries
                        acc ^= info.isWinmmDevice ? 0x1a2b3c4dULL : 0;
                        acc ^= static_cast<size_t>(info.winmmId) << 32;
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

bool ControllerOverrideManager::IsDeviceAllowed(const GUID& guid) const
{
        if (!m_overrideEnabled)
        {
                return true;
        }

        for (const auto& selection : m_playerSelections)
        {
                if (selection != GUID_NULL && IsEqualGUID(selection, guid))
                {
                        return true;
                }
        }

        return false;
}

void ControllerOverrideManager::OpenControllerControlPanel() const
{
    // Let the shell resolve joy.cpl the same way Win+R or Search does.
    HINSTANCE res = ShellExecuteW(nullptr, L"open", L"joy.cpl", nullptr, nullptr, SW_SHOWNORMAL);

    // Optional: fallback if this fails (res <= 32)
    if ((UINT_PTR)res <= 32)
    {
        // Old-style fallback - rarely needed, but safe to keep
        std::wstring rundllPath = GetPreferredSystemExecutable(L"rundll32.exe");
        ShellExecuteW(nullptr, L"open", rundllPath.c_str(),
            L"shell32.dll,Control_RunDLL joy.cpl",
            nullptr, SW_SHOWNORMAL);
    }
}


bool ControllerOverrideManager::OpenDeviceProperties(const GUID& guid) const
{
    // No device / keyboard / no DI - nothing to do
    if (guid == GUID_NULL || guid == GUID_SysKeyboard || !orig_DirectInput8Create)
        return false;

    IDirectInput8W* dinput = nullptr;
    HRESULT hr = orig_DirectInput8Create(GetModuleHandle(nullptr),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8W,
        (LPVOID*)&dinput,
        nullptr);
    if (FAILED(hr) || !dinput)
        return false;

    IDirectInputDevice8W* device = nullptr;
    hr = dinput->CreateDevice(guid, &device, nullptr);

    if (SUCCEEDED(hr) && device)
    {
        hr = device->RunControlPanel(nullptr, 0);
        device->Release();
    }

    dinput->Release();

    if (FAILED(hr))
    {
        // Optional: fallback - open the global Game Controllers dialog
        OpenControllerControlPanel();
        return false;
    }

    return true;
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

        std::vector<T> filtered;
        filtered.reserve(devices.size());

        for (const auto& device : devices)
        {
                if (IsDeviceAllowed(GetGuidFromInstance(device)))
                {
                        filtered.push_back(device);
                }
        }

        devices.swap(filtered);

        if (devices.empty())
        {
                return;
        }

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
        std::vector<ControllerDeviceInfo> directInputDevices;
        bool diSuccess = TryEnumerateDevicesW(directInputDevices);
        if (!diSuccess)
            diSuccess = TryEnumerateDevicesA(directInputDevices);

        std::vector<ControllerDeviceInfo> devices;
        devices.push_back({ GUID_SysKeyboard, "Keyboard", true, false, WINMM_INVALID_ID });

        std::vector<ControllerDeviceInfo> winmmDevices;
        TryEnumerateWinmmDevices(winmmDevices);

        // Optional: map WinMM IDs onto DI devices by name
        auto findWinmmIdByName = [&](const std::string& name) -> UINT {
            for (const auto& wdev : winmmDevices)
                if (NamesEqualIgnoreCase(wdev.name, name))
                    return wdev.winmmId;
            return WINMM_INVALID_ID;
            };

        for (auto& diDev : directInputDevices)
        {
            diDev.isWinmmDevice = false; // we're treating DI as primary
            diDev.winmmId = findWinmmIdByName(diDev.name);
            devices.push_back(diDev);
        }

        m_devices.swap(devices);

        return diSuccess || !winmmDevices.empty();
}

bool ControllerOverrideManager::TryEnumerateDevicesA(std::vector<ControllerDeviceInfo>& outDevices)
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

        auto callback = [](const DIDEVICEINSTANCEA* inst, VOID* ref) -> BOOL {
                auto* deviceList = reinterpret_cast<std::vector<ControllerDeviceInfo>*>(ref);
                ControllerDeviceInfo info{};
                info.guid = inst->guidInstance;
                info.isKeyboard = false;
                info.name = inst->tszProductName;
                deviceList->push_back(info);
                return DIENUM_CONTINUE;
        };

        HRESULT enumResult = dinput->EnumDevices(DI8DEVCLASS_GAMECTRL, callback, &outDevices, DIEDFL_ATTACHEDONLY | DIEDFL_INCLUDEALIASES);
        dinput->Release();

        if (FAILED(enumResult))
        {
                return false;
        }
        return true;
}

void ControllerOverrideManager::TryEnumerateWinmmDevices(std::vector<ControllerDeviceInfo>& outDevices) const
{
        UINT deviceCount = joyGetNumDevs();

        for (UINT deviceId = 0; deviceId < deviceCount; ++deviceId)
        {
                JOYCAPSW caps{};
                if (joyGetDevCapsW(deviceId, &caps, sizeof(caps)) != JOYERR_NOERROR)
                {
                        continue;
                }

                JOYINFOEX state{};
                state.dwSize = sizeof(state);
                state.dwFlags = JOY_RETURNALL;

                if (joyGetPosEx(deviceId, &state) != JOYERR_NOERROR)
                {
                        continue; // Filter out unplugged or placeholder devices
                }

                ControllerDeviceInfo deviceInfo{};
                deviceInfo.guid = CreateWinmmGuid(deviceId);
                deviceInfo.name = WideToUtf8(caps.szPname);
                deviceInfo.isKeyboard = false;
                deviceInfo.isWinmmDevice = true;
                deviceInfo.winmmId = deviceId;
                outDevices.push_back(deviceInfo);
        }
}

GUID ControllerOverrideManager::CreateWinmmGuid(UINT winmmId)
{
        GUID guid{};
        guid.Data1 = 0x77696E6D; // "winm"
        guid.Data2 = 0x6D64;     // "md"
        guid.Data3 = 0x6576;     // "ev"
        guid.Data4[0] = static_cast<unsigned char>((winmmId >> 24) & 0xFF);
        guid.Data4[1] = static_cast<unsigned char>((winmmId >> 16) & 0xFF);
        guid.Data4[2] = static_cast<unsigned char>((winmmId >> 8) & 0xFF);
        guid.Data4[3] = static_cast<unsigned char>(winmmId & 0xFF);
        return guid;
}

bool ControllerOverrideManager::NamesEqualIgnoreCase(const std::string& lhs, const std::string& rhs)
{
        if (lhs.size() != rhs.size())
        {
                return false;
        }

        for (size_t i = 0; i < lhs.size(); ++i)
        {
                if (std::tolower(static_cast<unsigned char>(lhs[i])) != std::tolower(static_cast<unsigned char>(rhs[i])))
                {
                        return false;
                }
        }

        return true;
}

bool ControllerOverrideManager::TryEnumerateDevicesW(std::vector<ControllerDeviceInfo>& outDevices)
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

        auto callback = [](const DIDEVICEINSTANCEW* inst, VOID* ref) -> BOOL {
                auto* deviceList = reinterpret_cast<std::vector<ControllerDeviceInfo>*>(ref);
                ControllerDeviceInfo info{};
                info.guid = inst->guidInstance;
                info.isKeyboard = false;
                info.name = ControllerOverrideManager::WideToUtf8(inst->tszProductName);
                deviceList->push_back(info);
                return DIENUM_CONTINUE;
        };

        HRESULT enumResult = dinput->EnumDevices(DI8DEVCLASS_GAMECTRL, callback, &outDevices, DIEDFL_ATTACHEDONLY | DIEDFL_INCLUDEALIASES);
        dinput->Release();

        if (FAILED(enumResult))
        {
                return false;
        }
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
        WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, &output[0], required, nullptr, nullptr);
        // Remove the trailing null the API writes
        if (!output.empty() && output.back() == '\0')
        {
                output.pop_back();
        }
        return output;
}
