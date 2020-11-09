#pragma once
#include "Utils.h"

#define INITIAL_MAX_ROUNDS 160
#define FAILURE_LIMIT 60

using namespace std;

class ClipboardHandler
{
public:
	ClipboardHandler();
	ClipboardHandler(HWND hwnd);
	~ClipboardHandler();

	wstring GetClipboardText();
	BOOLEAN SetClipboardText(wstring &text);
	VOID OnClipboardChange();

	BOOLEAN TriggerHijacker();

	BOOLEAN HijackerDetected();
	VOID CleanupClipboard();

	VOID RemoveClipboardListener();
	VOID AddClipboardListener();

	HWND GetWindowHandle();

	VOID SetKeyValueInHwndMap(HWND hwndKey, INT occValue);
	map<HWND, INT> GetHwndMap();
	
	HANDLE GetTriggerEventHandle();
	HANDLE GetFinishEventHandle();

	VOID Terminate();
	VOID TerminatePollThread();

private:
	BOOLEAN m_threadStarted;

	BOOLEAN m_hijackerDetected;
	HWND m_hwnd;

	vector<DWORD> m_hijackerPIDs;

	DWORD m_maxRounds;
	DWORD m_roundCounter;
	DWORD m_failureCounter;

	HANDLE m_hTriggerEvent;
	HANDLE m_hFinishEvent;

	BOOLEAN m_terminated;

	HANDLE m_hPollClipboardUserThread;

	set<wstring> m_baitAddresses;
	set<wstring> m_detectedAddresses;

	map<HWND, INT> m_globalHwndMap; //this map contains all occurrences of hwnd that were blocking the clipboard
};

DWORD WINAPI PollClipboardUserThread(LPVOID lpParam);

