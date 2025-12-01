#include "ControllerOverrideManager.h"

#include "dllmain.h"
#include "logger.h"
#include "Core/utils.h"
#include "Core/interfaces.h"

#include <Shellapi.h>
#include <dinput.h>
#include <mmsystem.h>
#include <joystickapi.h>
#include <dbt.h>
#include <objbase.h>

#include <array>
#include <algorithm>
#include <cctype>
#include <numeric>

namespace
{
        constexpr ULONGLONG DEVICE_REFRESH_INTERVAL_MS = 1000;
        constexpr UINT WINMM_INVALID_ID = static_cast<UINT>(-1);
        constexpr uintptr_t BBCF_PAD_SLOT0_PTR_OFFSET = 0x104FA298;

        IDirectInputDevice8W** GetBbcfPadSlot0Ptr()
        {
                auto base = reinterpret_cast<uintptr_t>(GetBbcfBaseAdress());
                return reinterpret_cast<IDirectInputDevice8W**>(base + BBCF_PAD_SLOT0_PTR_OFFSET);
        }

        void DebugLogPadSlot0()
        {
                auto ppDev = GetBbcfPadSlot0Ptr();
                IDirectInputDevice8W* dev = ppDev ? *ppDev : nullptr;
                LOG(1, "[BBCF] PadSlot0 ptr from game table = %p (slot addr=%p)\n", dev, ppDev);
        }

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

        bool IsProbablySteamInputActive()
        {
                constexpr std::array<const wchar_t*, 3> kSteamEnvVars = {
                        L"SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT",
                        L"SDL_GAMECONTROLLER_IGNORE_DEVICES",
                        L"SDL_ENABLE_STEAM_CONTROLLERS",
                };

                for (auto name : kSteamEnvVars)
                {
                        if (GetEnvironmentVariableW(name, nullptr, 0) > 0)
                        {
                                return true;
                        }
                }

                return false;
        }

        bool IsLikelySteamVirtualDevice(const ControllerDeviceInfo& info)
        {
                if (info.isKeyboard)
                {
                        return false;
                }

                std::string lowerName = info.name;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char ch) {
                        return static_cast<char>(std::tolower(ch));
                });

                return lowerName.find("steam virtual") != std::string::npos;
        }

}

std::string GuidToString(const GUID& guid)
{
wchar_t buf[64] = {};
constexpr int kBufferCount = static_cast<int>(sizeof(buf) / sizeof(buf[0]));
int written = StringFromGUID2(guid, buf, kBufferCount);
        if (written <= 0)
        {
                return {};
        }

        std::wstring wide(buf);
        int required = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (required <= 0)
        {
                return {};
        }

        std::string output(static_cast<size_t>(required), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &output[0], required, nullptr, nullptr);
        if (!output.empty() && output.back() == '\0')
        {
                output.pop_back();
        }

        return output;
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
        LOG(1, "ControllerOverrideManager::ControllerOverrideManager - initializing device list\n");
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
        LOG(1, "ControllerOverrideManager::RefreshDevices - begin (override=%d)\n", m_overrideEnabled ? 1 : 0);
        CollectDevices();
        EnsureSelectionsValid();
        m_lastRefresh = GetTickCount64();
        m_lastDeviceHash = HashDevices(m_devices);
        LOG(1, "ControllerOverrideManager::RefreshDevices - end (devices=%zu, hash=%zu)\n", m_devices.size(), m_lastDeviceHash);
}

void ControllerOverrideManager::RefreshDevicesAndReinitializeGame()
{
        LOG(1, "ControllerOverrideManager::RefreshDevicesAndReinitializeGame - begin\n");
        RefreshDevices();
        BounceTrackedDevices();
        DebugDumpTrackedDevices();
        DebugLogPadSlot0();
        SendDeviceChangeBroadcast();
        LOG(1, "ControllerOverrideManager::RefreshDevicesAndReinitializeGame - end\n");
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
                        LOG(1, "ControllerOverrideManager::TickAutoRefresh - device hash changed (devices=%zu, hash=%zu)\n", m_devices.size(), m_lastDeviceHash);
                }
        }

        m_lastRefresh = GetTickCount64();
}

void ControllerOverrideManager::RegisterCreatedDevice(IDirectInputDevice8A* device)
{
        if (!device)
        {
                return;
        }

        std::lock_guard<std::mutex> lock(m_deviceMutex);
        device->AddRef();
        m_trackedDevicesA.push_back(device);
        LOG(1, "RegisterCreatedDeviceA: device=%p (trackedA=%zu, trackedW=%zu)\n", device, m_trackedDevicesA.size(), m_trackedDevicesW.size());
}

void ControllerOverrideManager::RegisterCreatedDevice(IDirectInputDevice8W* device)
{
        if (!device)
        {
                return;
        }

        std::lock_guard<std::mutex> lock(m_deviceMutex);
        device->AddRef();
        m_trackedDevicesW.push_back(device);
        LOG(1, "RegisterCreatedDeviceW: device=%p (trackedA=%zu, trackedW=%zu)\n", device, m_trackedDevicesA.size(), m_trackedDevicesW.size());
}

void ControllerOverrideManager::BounceTrackedDevices()
{
        LOG(1, "ControllerOverrideManager::BounceTrackedDevices - begin\n");
        auto bounceCollection = [](auto& devices)
        {
                for (auto it = devices.begin(); it != devices.end();)
                {
                        auto* dev = *it;
                        if (!dev)
                        {
                                LOG(1, "BounceTrackedDevices: encountered null entry, erasing\n");
                                it = devices.erase(it);
                                continue;
                        }

                        dev->Unacquire();
                        HRESULT hr = dev->Acquire();
                        LOG(1, "BounceTrackedDevices: dev=%p Acquire hr=0x%08X\n", dev, hr);
                        if (FAILED(hr))
                        {
                                hr = dev->Acquire();
                                LOG(1, "BounceTrackedDevices: retry Acquire dev=%p hr=0x%08X\n", dev, hr);
                        }

                        if (FAILED(hr))
                        {
                                LOG(1, "BounceTrackedDevices: dev=%p failed to acquire after retries, releasing\n", dev);
                                dev->Release();
                                it = devices.erase(it);
                                continue;
                        }

                        ++it;
                }
        };

        std::lock_guard<std::mutex> lock(m_deviceMutex);
        bounceCollection(m_trackedDevicesA);
        bounceCollection(m_trackedDevicesW);
        LOG(1, "ControllerOverrideManager::BounceTrackedDevices - end (trackedA=%zu, trackedW=%zu)\n", m_trackedDevicesA.size(), m_trackedDevicesW.size());
}

void ControllerOverrideManager::DebugDumpTrackedDevices()
{
        std::lock_guard<std::mutex> lock(m_deviceMutex);

        LOG(1, "=== Tracked A devices ===\n");
        for (auto* dev : m_trackedDevicesA)
        {
                LOG(1, "  A dev=%p\n", dev);
        }

        LOG(1, "=== Tracked W devices ===\n");
        for (auto* dev : m_trackedDevicesW)
        {
                LOG(1, "  W dev=%p\n", dev);
        }
        LOG(1, "=== End tracked dump (A=%zu, W=%zu) ===\n", m_trackedDevicesA.size(), m_trackedDevicesW.size());
}

void ControllerOverrideManager::SendDeviceChangeBroadcast() const
{
        if (!g_gameProc.hWndGameWindow)
        {
                return;
        }

        SendMessageTimeout(g_gameProc.hWndGameWindow, WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 0, SMTO_ABORTIFHUNG, 50, nullptr);
        LOG(1, "ControllerOverrideManager::SendDeviceChangeBroadcast - sent WM_DEVICECHANGE to %p\n", g_gameProc.hWndGameWindow);
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
        LOG(1, "ControllerOverrideManager::CollectDevices - begin\n");
        m_steamInputLikely = IsProbablySteamInputActive();

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

        LOG(1, "ControllerOverrideManager::CollectDevices - steamInputLikely=%d diSuccess=%d diCount=%zu winmmCount=%zu total=%zu\n",
                m_steamInputLikely ? 1 : 0, diSuccess ? 1 : 0, directInputDevices.size(), winmmDevices.size(), m_devices.size());

        for (size_t i = 0; i < m_devices.size(); ++i)
        {
                const auto& device = m_devices[i];
                LOG(1, "  Device[%zu]: name='%s' guid=%s keyboard=%d winmm=%d winmmId=%u\n", i, device.name.c_str(), GuidToString(device.guid).c_str(),
                        device.isKeyboard ? 1 : 0, device.isWinmmDevice ? 1 : 0, device.winmmId);
        }

        bool anyListedGamepad = false;
        bool anySteamVirtualPad = false;
        bool anyNonSteamVirtualPad = false;
        for (const auto& device : m_devices)
        {
                if (device.isKeyboard)
                {
                        continue;
                }

                anyListedGamepad = true;
                if (IsLikelySteamVirtualDevice(device))
                {
                        anySteamVirtualPad = true;
                }
                else
                {
                        anyNonSteamVirtualPad = true;
                }
        }

        m_steamInputLikely = m_steamInputLikely && anyListedGamepad && anySteamVirtualPad && !anyNonSteamVirtualPad;

        return diSuccess || !winmmDevices.empty();
}

bool ControllerOverrideManager::TryEnumerateDevicesA(std::vector<ControllerDeviceInfo>& outDevices)
{
        LOG(1, "ControllerOverrideManager::TryEnumerateDevicesA - begin\n");
        if (!orig_DirectInput8Create)
        {
                LOG(1, "ControllerOverrideManager::TryEnumerateDevicesA - orig_DirectInput8Create missing\n");
                return false;
        }

        IDirectInput8A* dinput = nullptr;
        if (FAILED(orig_DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8A, (LPVOID*)&dinput, nullptr)))
        {
                LOG(1, "ControllerOverrideManager::TryEnumerateDevicesA - DirectInput8Create failed\n");
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
                LOG(1, "ControllerOverrideManager::TryEnumerateDevicesA - EnumDevices failed hr=0x%08X\n", enumResult);
                return false;
        }
        LOG(1, "ControllerOverrideManager::TryEnumerateDevicesA - success count=%zu\n", outDevices.size());
        for (size_t i = 0; i < outDevices.size(); ++i)
        {
                LOG(1, "  [A] Device[%zu]: name='%s' guid=%s\n", i, outDevices[i].name.c_str(), GuidToString(outDevices[i].guid).c_str());
        }
        return true;
}

void ControllerOverrideManager::TryEnumerateWinmmDevices(std::vector<ControllerDeviceInfo>& outDevices) const
{
        UINT deviceCount = joyGetNumDevs();
        LOG(1, "ControllerOverrideManager::TryEnumerateWinmmDevices - begin count=%u\n", deviceCount);

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
                LOG(1, "  [WINMM] Device id=%u name='%s' guid=%s\n", deviceId, deviceInfo.name.c_str(), GuidToString(deviceInfo.guid).c_str());
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
        LOG(1, "ControllerOverrideManager::TryEnumerateDevicesW - begin\n");
        if (!orig_DirectInput8Create)
        {
                LOG(1, "ControllerOverrideManager::TryEnumerateDevicesW - orig_DirectInput8Create missing\n");
                return false;
        }

        IDirectInput8W* dinput = nullptr;
        if (FAILED(orig_DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W, (LPVOID*)&dinput, nullptr)))
        {
                LOG(1, "ControllerOverrideManager::TryEnumerateDevicesW - DirectInput8Create failed\n");
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
                LOG(1, "ControllerOverrideManager::TryEnumerateDevicesW - EnumDevices failed hr=0x%08X\n", enumResult);
                return false;
        }
        LOG(1, "ControllerOverrideManager::TryEnumerateDevicesW - success count=%zu\n", outDevices.size());
        for (size_t i = 0; i < outDevices.size(); ++i)
        {
                LOG(1, "  [W] Device[%zu]: name='%s' guid=%s\n", i, outDevices[i].name.c_str(), GuidToString(outDevices[i].guid).c_str());
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
