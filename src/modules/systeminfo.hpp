#pragma once

#include <Windows.h>
#include <string>
#include <stdexcept>

#include "../../src/utils/registry.hpp"

inline std::wstring getHWID() {
	std::wstring key = L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation";
	std::wstring value = L"ComputerHardwareId";
	std::wstring hwid = L"";

	if (registry::get(key, value, hwid)) {
		return hwid;
	}
	else {
		return L"";
	}
}

inline std::wstring genUUID() {
	GUID guid;
	HRESULT result = CoCreateGuid(&guid);

	if (FAILED(result))
		return L"{4c44eed4-3aee-4138-84cb-afb92bd9c597}";

	wchar_t buffer[37];
	swprintf(buffer, sizeof(buffer) / sizeof(wchar_t),
		L"{%08x-%04x-%04x-%04x-%012llx}",
		guid.Data1, guid.Data2, guid.Data3,
		(guid.Data4[0] << 8) | guid.Data4[1],
		((unsigned long long)guid.Data4[2] << 40) |
		((unsigned long long)guid.Data4[3] << 32) |
		((unsigned long long)guid.Data4[4] << 24) |
		((unsigned long long)guid.Data4[5] << 16) |
		((unsigned long long)guid.Data4[6] << 8) |
		(unsigned long long)guid.Data4[7]
	);

	return std::wstring(buffer);
}

inline bool setHWID(std::wstring& guid) {
	std::wstring key = L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation";
	std::wstring value = L"ComputerHardwareId";

	if (registry::set(key, value, guid)) {
		return true;
	}
	else {
		return false;
	}
}

inline bool initInterceptConfig(const std::wstring& original, const std::wstring& replacement) {
	HKEY handle = NULL;

	LPCWSTR securityKey = L"Software\\Microsoft\\DeviceManagement\\SecurityProviders";
	LPCWSTR key = L"Software\\Microsoft\\DeviceManagement\\SystemIdentifiers";
	std::wstring prefix = L"D4gH9kL2Z7mQwT1rP8vX";

	LONG result = RegOpenKeyExW(
		HKEY_CURRENT_USER, key,
		NULL, KEY_WRITE, &handle
	);

	if (result != ERROR_SUCCESS) {
		result = RegCreateKeyExW(
			HKEY_CURRENT_USER, key, NULL, NULL,
			REG_OPTION_NON_VOLATILE, KEY_WRITE,
			NULL, &handle, NULL
		);

		if (result != ERROR_SUCCESS)
			throw std::runtime_error("could not open registry key");
	}

	result = RegSetValueExW(
		handle, L"OriginalIdentifier", NULL, REG_SZ,
		reinterpret_cast<CONST BYTE*>(original.c_str()),
		static_cast<DWORD>((original.size() + 1) * sizeof(wchar_t))
	);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(handle);
		throw std::runtime_error("could not set original identifier");
	}

	result = RegSetValueExW(
		handle, L"SystemIdentifier", NULL, REG_SZ,
		reinterpret_cast<const BYTE*>(replacement.c_str()),
		static_cast<DWORD>((replacement.size() + 1) * sizeof(wchar_t))
	);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(handle);
		throw std::runtime_error("could not set system identifier");
	}

	result = RegSetValueExW(
		handle, L"IdentifierPrefix", NULL, REG_SZ,
		reinterpret_cast<const BYTE*>(prefix.c_str()),
		static_cast<DWORD>((prefix.size() + 1) * sizeof(wchar_t))
	);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(handle);
		throw std::runtime_error("could not set identifier prefix");
	}

	RegCloseKey(handle);

	if (!registry::setIdentifierProtection())
		throw std::runtime_error("could not set identifier protection");

	return TRUE;
}

namespace systeminfo {
	inline std::string init() {
		std::wstring hwid = getHWID();
		if (hwid.empty())
			return "failed: could not get hwid";

		std::wstring guid = genUUID();

		if (!setHWID(guid))
			return "failed: could not set new hwid";

		try {
			initInterceptConfig(hwid, guid);
		}
		catch (const std::exception& e) {
			return "failed: " + std::string(e.what());
		}

		return "success: system info has been spoofed";
	}
}