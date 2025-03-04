#pragma once

#include <Windows.h>

namespace perms {
	inline bool isAdmin() {
		BOOL adminBuffer = FALSE;
		PSID adminGroupBuffer = NULL;
		SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;

		BOOL success = AllocateAndInitializeSid(
			&authority, 2,
			SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS,
			NULL, NULL, NULL,
			NULL, NULL, NULL,
			&adminGroupBuffer
		);

		if (success) {
			CheckTokenMembership(NULL, adminGroupBuffer, &adminBuffer);
			FreeSid(adminGroupBuffer);
		}

		return adminBuffer == TRUE;
	}

	inline bool promptAdmin() {
		CHAR path[MAX_PATH];
		GetModuleFileNameA(NULL, path, MAX_PATH);

		WCHAR pathW[MAX_PATH];
		MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, MAX_PATH);

		SHELLEXECUTEINFO info = { 0 };
		info.cbSize = sizeof(info);
		info.fMask = SEE_MASK_DEFAULT;
		info.hwnd = NULL;
		info.lpVerb = L"runas";
		info.lpFile = pathW;
		info.lpParameters = L"";
		info.nShow = SW_NORMAL;

		BOOL success = ShellExecuteEx(&info);
		return success == TRUE;
	}
}