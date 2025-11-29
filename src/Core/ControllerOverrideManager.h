#pragma once

#include <Windows.h>
#include <dinput.h>

#include <string>
#include <vector>

struct ControllerDeviceInfo
{
        GUID guid = GUID_NULL;
        std::string name;
        bool isKeyboard = false;
        bool isWinmmDevice = false;
        UINT winmmId = static_cast<UINT>(-1);
};

class ControllerOverrideManager
{
public:
        static ControllerOverrideManager& GetInstance();

        void SetOverrideEnabled(bool enabled);
        bool IsOverrideEnabled() const;

        void SetPlayerSelection(int playerIndex, const GUID& guid);
        GUID GetPlayerSelection(int playerIndex) const;

        const std::vector<ControllerDeviceInfo>& GetDevices() const;

        bool IsSteamInputLikelyActive() const { return m_steamInputLikely; }

        void RefreshDevices();
        void TickAutoRefresh();

        void ApplyOrdering(std::vector<DIDEVICEINSTANCEA>& devices) const;
        void ApplyOrdering(std::vector<DIDEVICEINSTANCEW>& devices) const;

        void OpenControllerControlPanel() const;
        bool OpenDeviceProperties(const GUID& guid) const;

private:
        ControllerOverrideManager();

        template <typename T>
        void ApplyOrderingImpl(std::vector<T>& devices) const;

        void EnsureSelectionsValid();
        bool CollectDevices();
        bool TryEnumerateDevicesA(std::vector<ControllerDeviceInfo>& outDevices);
        bool TryEnumerateDevicesW(std::vector<ControllerDeviceInfo>& outDevices);
        void TryEnumerateWinmmDevices(std::vector<ControllerDeviceInfo>& outDevices) const;
        static GUID CreateWinmmGuid(UINT winmmId);
        static bool NamesEqualIgnoreCase(const std::string& lhs, const std::string& rhs);

        static std::string WideToUtf8(const std::wstring& value);

        std::vector<ControllerDeviceInfo> m_devices;
        GUID m_playerSelections[2];
        bool m_overrideEnabled = false;
        ULONGLONG m_lastRefresh = 0;
        size_t m_lastDeviceHash = 0;
        bool m_steamInputLikely = false;
};
