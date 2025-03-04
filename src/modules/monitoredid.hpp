#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <random>
#include <ctime>
#include <iomanip>
#include <sstream>

inline std::vector<std::wstring> getDevices() {
	HKEY handle = NULL;
	std::vector<std::wstring> paths;

	LONG result = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY",
		NULL, KEY_READ, &handle
	);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(handle);
		return paths;
	}

	wchar_t keyName[255] = {};
	DWORD index = 0;
	DWORD keyNameLen = sizeof(keyName) / sizeof(wchar_t);

	result = RegEnumKeyEx(
		handle, index++, keyName, &keyNameLen,
		NULL, NULL, NULL, NULL
	);

	while (result == ERROR_SUCCESS) {
		std::wstring manufacturer = keyName;
		HKEY manufacturerKey = NULL;

		result = RegOpenKeyEx(
			handle, keyName, NULL,
			KEY_READ, &manufacturerKey
		);

		if (result == ERROR_SUCCESS) {
			DWORD manufacturerIndex = 0;
			result = RegEnumKeyEx(
				manufacturerKey, manufacturerIndex++,
				keyName, &keyNameLen, NULL, NULL, NULL, NULL
			);

			while (result == ERROR_SUCCESS) {
				std::wstring manufacturerPath = L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\" + manufacturer + L"\\" + keyName;
				paths.push_back(manufacturerPath);
			}
		}

		RegCloseKey(handle);
		return paths;
	}
}

inline bool setEDID(const std::wstring& path) {
	HKEY handle = NULL;
	std::wstring paramsPath = path + L"\\Device Parameters";

	LONG result = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE, paramsPath.c_str(),
		NULL, KEY_READ | KEY_WRITE, &handle
	);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(handle);
		return FALSE;
	}

	BYTE edid[128] = { 0 };
	DWORD size = sizeof(edid);

	result = RegQueryValueEx(
		handle, L"EDID", NULL,
		NULL, edid, &size
	);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(handle);
		return FALSE;
	}

	if (size < 128) {
		RegCloseKey(handle);
		return FALSE;
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 255);

	BYTE newSerial[4] = {
		static_cast<BYTE>(dis(gen)),
		static_cast<BYTE>(dis(gen)),
		static_cast<BYTE>(dis(gen)),
		static_cast<BYTE>(dis(gen))
	};

	edid[12] = newSerial[0];
	edid[13] = newSerial[1];
	edid[14] = newSerial[2];
	edid[15] = newSerial[3];

	BYTE checksum = 0;
	for (int i = 0; i < 127; ++i) {
		checksum += edid[i];
	}
	checksum = (256 - checksum) % 256;
	edid[127] = checksum;

	result = RegSetValueEx(
		handle, L"EDID", NULL,
		REG_BINARY, edid, sizeof(edid)
	);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(handle);
		return FALSE;
	}

	RegCloseKey(handle);
	return TRUE;
}

inline bool initInterceptConfig(int count) {
	HKEY handle = NULL;

	std::wstring prefix = L"H3v9Bf1XlWz8Zq2Mk5Y";
	std::wstring path = L"Software\\Microsoft\\DeviceManagement\\Display";
	LONG result = RegCreateKeyEx(
		HKEY_CURRENT_USER, path.c_str(),
		NULL, NULL, NULL, KEY_WRITE, NULL,
		&handle, NULL
	);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(handle);
		throw std::runtime_error("could not open registry key");
	}

	DWORD displayCount = count;
	result = RegSetValueEx(
		handle, L"ModifiedDisplayCount", NULL,
		REG_DWORD, (const BYTE*)&displayCount, sizeof(displayCount)
	);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(handle);
		throw std::runtime_error("could not set display count");
	}

	std::time_t now = std::time(nullptr);
	struct tm timeinfo;

	localtime_s(&timeinfo, &now);

	std::wstringstream ss;
	ss << std::put_time(&timeinfo, L"%FT%T%z");
	std::wstring timeStr = ss.str();

	result = RegSetValueEx(
		handle, L"LastModifiedTime", NULL,
		REG_SZ, (const BYTE*)timeStr.c_str(),
		static_cast<DWORD>((timeStr.size() + 1) * sizeof(wchar_t))
	);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(handle);
		throw std::runtime_error("could not set last modified time");
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

	if (!registry::setIdentifierProtection())
		throw std::runtime_error("could not set identifier protection");

	return TRUE;
}

namespace monitoredid {
	inline std::string init() {
		std::vector<std::wstring> paths = getDevices();
		if (paths.empty())
			return "failed: could not get device paths";

		int count = 0;
		for (const auto& path : paths) {
			if (setEDID(path)) {
				++count;
			}
		}

		try {
			initInterceptConfig(count);
		}
		catch (const std::exception& e) {
			return "failed: " + std::string(e.what());
		}

		return "success: monitor EDID has been spoofed";
	}
}