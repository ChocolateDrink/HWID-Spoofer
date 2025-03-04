#pragma once

#include <Windows.h>
#include <string>

namespace registry {
	inline bool get(const std::wstring& key, const std::wstring& value, std::wstring& returned) {
		HKEY handle = NULL;

		LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key.c_str(), NULL, KEY_READ, &handle);
		if (result != ERROR_SUCCESS)
			return FALSE;

		DWORD type = NULL;
		DWORD size = 0;

		result = RegQueryValueEx(handle, value.c_str(), NULL, &type, NULL, &size);
		if (result != ERROR_SUCCESS || type != REG_SZ) {
			RegCloseKey(handle);
			return FALSE;
		}

		WCHAR* buffer = new WCHAR[size / sizeof(WCHAR)];

		result = RegQueryValueEx(handle, value.c_str(), NULL, &type, (LPBYTE)buffer, &size);
		if (result == ERROR_SUCCESS) {
			returned = buffer;
			delete[] buffer;
		}
		else {
			delete[] buffer;
			RegCloseKey(handle);
			return FALSE;
		}

		RegCloseKey(handle);
		return TRUE;
	}

	inline bool set(const std::wstring& key, const std::wstring& value, std::wstring& newValue) {
		HKEY handle = NULL;

		LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key.c_str(), NULL, KEY_SET_VALUE, &handle);
		if (result != ERROR_SUCCESS)
			return FALSE;

		result = RegSetValueEx(
			handle, value.c_str(), NULL, REG_SZ,
			reinterpret_cast<const BYTE*>(newValue.c_str()),
			static_cast<DWORD>((newValue.length() + 1) * sizeof(wchar_t))
		);

		if (result != ERROR_SUCCESS) {
			RegCloseKey(handle);
			return FALSE;
		}

		RegCloseKey(handle);
		return TRUE;
	}

	inline bool setIdentifierProtection() {
		HKEY handle = NULL;

		LONG result = RegOpenKeyExW(
			HKEY_CURRENT_USER,
			L"Software\\Microsoft\\DeviceManagement\\SecurityProviders",
			NULL, KEY_WRITE, &handle
		);

		if (result != ERROR_SUCCESS)
			return FALSE;

		DWORD byte = 1;
		result = RegSetValueExW(
			handle, L"EnableMemoryProtection", NULL, REG_DWORD,
			reinterpret_cast<const BYTE*>(&byte), sizeof(byte)
		);

		if (result != ERROR_SUCCESS) {
			RegCloseKey(handle);
			return FALSE;
		}

		RegCloseKey(handle);
		return TRUE;
	}
}