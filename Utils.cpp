#include "stdafx.h"
#include "Utils.h"
#include <wincrypt.h>

VOID TerminateProcessById(DWORD processId)
{
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

	if (hProcess == NULL)
		return;

	TerminateProcess(hProcess, 0);
	CloseHandle(hProcess);
}

VOID GetProcessImageNameById(DWORD processId, CHAR* buffer, DWORD bufferSize)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);

	if (hProcess == NULL)
	{
		LOG_DEBUG_(MainLogger) << "OpenProcess failed with PID: " << processId << ". status code: " << GetLastError();
		return;
	}
		
	GetProcessImageFileNameA(hProcess, buffer, bufferSize);
	CloseHandle(hProcess);
}

BOOLEAN IsBtcAddress(IN wstring &text, OUT wstring *matchStr)
{
	wsmatch wideMatch;
	if (regex_search(text, wideMatch, wregex(BITCOIN_REGEX)))
		*matchStr = wideMatch.str();
		return TRUE;
	return FALSE;
}

wstring StringToWString(const std::string& s)
{
	std::wstring temp(s.length(), L' ');
	std::copy(s.begin(), s.end(), temp.begin());
	return temp;
}

wstring GenRandBtcAddress()
{
	wstring addr = L"1"; 
	WCHAR charSet[] = L"abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ123456789";
	int charSetLen = wcslen(charSet);
	HCRYPTPROV hCryptProv;
	BYTE pbData[33];
	CryptAcquireContextW(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
	CryptGenRandom(hCryptProv, 33, pbData);

	for (int i = 0; i < 33; i++)
	{
		int index = ((int)pbData[i]) % charSetLen;
		addr += charSet[index];
	}
	
	return addr; //total length: 34
}

string formatTimestamp(const time_t rawtime)
{
	struct tm dt;
	char buffer[30];
	localtime_s(&dt, &rawtime);
	strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", &dt);
	return std::string(buffer);
}