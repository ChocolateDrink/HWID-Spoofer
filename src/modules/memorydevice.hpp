#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <random>
#include <stdexcept>

#include "../../src/utils/registry.hpp"

inline std::vector<std::wstring> genSerials(int count) {
	std::vector<std::wstring> serials;
	serials.reserve(count);

	const std::vector<std::wstring> prefixes = { L"KHX", L"CMK", L"BLS", L"TF", L"CT", L"HX", L"F4", L"TD" };

	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> dist(0, static_cast<int>(prefixes.size()) - 1);
	std::uniform_int_distribution<> charDist(0, 61);

	for (int i = 0; i < count; ++i) {
		std::wstring serial = prefixes[dist(rng)];

		for (int j = 0; j < 12; ++j) {
			char c = (charDist(rng) < 26) ? ('A' + charDist(rng)) : ('0' + charDist(rng) - 26);
			serial.push_back(c);
		}

		serials.push_back(serial);
	}

	return serials;
}

inline bool initDeviceSpoofer(const std::vector<std::wstring>& serials) {
	HKEY handle = NULL;

	LPCWSTR securityKey = L"Software\\Microsoft\\DeviceManagement\\SecurityProviders";
	LPCWSTR key = L"Software\\Microsoft\\DeviceManagement\\Memory";
	std::wstring prefix = L"F5s7rQz9xLp8Nw3D2yT0";

	LONG result = RegCreateKeyExW(
		HKEY_CURRENT_USER, key, NULL, NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE, NULL, &handle, NULL
	);

	if (result != ERROR_SUCCESS)
		throw std::runtime_error("could not create open registry key");

	DWORD deviceCount = static_cast<DWORD>(serials.size());
	result = RegSetValueExW(
		handle, L"DeviceCount", NULL, REG_DWORD,
		reinterpret_cast<CONST BYTE*>(&deviceCount),
		sizeof(deviceCount)
	);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(handle);
		throw std::runtime_error("could not set device count");
	}

	for (int i = 0; i < serials.size(); ++i) {
		result = RegSetValueExW(
			handle, (L"Device" + std::to_wstring(i) + L"_Serial").c_str(),
			NULL, REG_SZ, reinterpret_cast<CONST BYTE*>(serials[i].c_str()),
			static_cast<DWORD>((serials[i].size() + 1) * sizeof(wchar_t))
		);

		if (result != ERROR_SUCCESS) {
			RegCloseKey(handle);
			throw std::runtime_error("could not set device serial");
		}
	}

	result = RegSetValueExW(
		handle, L"MemoryDevicePrefix", NULL, REG_SZ,
		reinterpret_cast<CONST BYTE*>(prefix.c_str()),
		static_cast<DWORD>((prefix.size() + 1) * sizeof(wchar_t))
	);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(handle);
		throw std::runtime_error("could not set device prefix");
	}

	RegCloseKey(handle);

	if (!registry::setIdentifierProtection())
		throw std::runtime_error("could not set identifier protection");

	return TRUE;
}

inline bool initDeviceInfo(const std::vector<std::wstring>& serials) {
	HKEY handle = NULL;

	LPCWSTR key = L"Software\\Microsoft\\DeviceManagement\\MemoryInfo";

	LONG result = RegCreateKeyExW(
		HKEY_CURRENT_USER, key, NULL,
		NULL,REG_OPTION_NON_VOLATILE,
		KEY_WRITE, NULL, &handle, NULL
	);

	if (result != ERROR_SUCCESS)
		throw std::runtime_error("could not open registry key");

	const std::vector<std::wstring> manufacturers = { L"Kingston", L"Corsair", L"G.Skill", L"Crucial", L"HyperX" };
	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> dist(0, static_cast<int>(manufacturers.size()) - 1);
	std::uniform_int_distribution<> sizeDist(8, 32);

	for (int i = 0; i < serials.size(); ++i) {
		HKEY device = NULL;

		result = RegCreateKeyExW(
			handle, (L"Device" + std::to_wstring(i)).c_str(),
			NULL, NULL, REG_OPTION_NON_VOLATILE,
			KEY_WRITE, NULL, &device, NULL
		);

		if (result != ERROR_SUCCESS) {
			RegCloseKey(handle);
			throw std::runtime_error("could not set device key");
		}

		const std::wstring manufacturer = manufacturers[dist(rng)];
		const int size = sizeDist(rng);

		result = RegSetValueExW(
			device, L"SerialNumber", NULL, REG_SZ,
			reinterpret_cast<CONST BYTE*>(serials[i].c_str()),
			static_cast<DWORD>((serials[i].size() + 1) * sizeof(wchar_t))
		);

		if (result != ERROR_SUCCESS) {
			RegCloseKey(device);
			throw std::runtime_error("could not set serial number");
		}

		result = RegSetValueExW(
			device, L"Manufacturer", NULL, REG_SZ,
			reinterpret_cast<CONST BYTE*>(manufacturer.c_str()),
			static_cast<DWORD>((manufacturer.size() + 1) * sizeof(wchar_t))
		);

		if (result != ERROR_SUCCESS) {
			RegCloseKey(device);
			throw std::runtime_error("could not set manufacturer");
		}

		std::wstring capacity = std::to_wstring(size) + L" GB";
		result = RegSetValueExW(
			device, L"Capacity", NULL, REG_SZ,
			reinterpret_cast<CONST BYTE*>(capacity.c_str()),
			static_cast<DWORD>((capacity.size() + 1) * sizeof(wchar_t))
		);

		if (result != ERROR_SUCCESS) {
			RegCloseKey(device);
			throw std::runtime_error("could not set capacity");
		}

		std::wstring speed = std::to_wstring(1600 + (rng() % 2400)) + L" MHz";
		result = RegSetValueExW(
			device, L"Speed", NULL, REG_SZ,
			reinterpret_cast<CONST BYTE*>(speed.c_str()),
			static_cast<DWORD>((speed.size() + 1) * sizeof(wchar_t))
		);

		if (result != ERROR_SUCCESS) {
			RegCloseKey(device);
			throw std::runtime_error("could not set speed");
		}

		std::wstring locator = L"DIMM" + std::to_wstring(i);
		result = RegSetValueExW(
			device, L"DeviceLocator", NULL, REG_SZ,
			reinterpret_cast<CONST BYTE*>(locator.c_str()),
			static_cast<DWORD>((locator.size() + 1) * sizeof(wchar_t))
		);

		if (result != ERROR_SUCCESS) {
			RegCloseKey(device);
			throw std::runtime_error("could not set locator");
		}

		RegCloseKey(device);
	}

	RegCloseKey(handle);
	return TRUE;
}

namespace memorydevice {
	inline std::string init() {
		auto serials = genSerials(4);

		try {
			initDeviceSpoofer(serials);
			initDeviceInfo(serials);
		}
		catch (const std::exception& e) {
			return "failed: " + std::string(e.what());
		}

		return "success: memory device has been spoofed";
	}
}
