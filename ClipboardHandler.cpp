#include "stdafx.h"
#include "ClipboardHandler.h"
#include <sstream> 

using namespace std;

ClipboardHandler::ClipboardHandler()
{

}

ClipboardHandler::ClipboardHandler(HWND hwnd) :
	m_hwnd(hwnd),
	m_hijackerDetected(FALSE),
	m_roundCounter(0),
	m_terminated(FALSE),
	m_threadStarted(FALSE),
	m_hPollClipboardUserThread(NULL),
	m_maxRounds(INITIAL_MAX_ROUNDS),
	m_failureCounter(0)
{
	m_hTriggerEvent = CreateEventA(NULL, FALSE, FALSE, "Trigger");
}

ClipboardHandler::~ClipboardHandler()
{
}

//-------------------------------------- Get members -------------------------------------- 

BOOLEAN ClipboardHandler::HijackerDetected()
{
	return m_hijackerDetected;
}

HWND ClipboardHandler::GetWindowHandle()
{
	return m_hwnd;
}

map<HWND, INT> ClipboardHandler::GetHwndMap()
{
	return m_globalHwndMap;
}

HANDLE ClipboardHandler::GetTriggerEventHandle()
{
	return m_hTriggerEvent;
}

HANDLE ClipboardHandler::GetFinishEventHandle()
{
	return m_hFinishEvent;
}

//-------------------------------------- Set members -------------------------------------- 

VOID ClipboardHandler::SetKeyValueInHwndMap(HWND hwndKey, INT occValue)
{
	m_globalHwndMap[hwndKey] += occValue;
}

//-------------------------------------- Wrapper functions -------------------------------------- 

VOID ClipboardHandler::RemoveClipboardListener()
{
	RemoveClipboardFormatListener(m_hwnd);
}

VOID ClipboardHandler::AddClipboardListener()
{
	AddClipboardFormatListener(m_hwnd);
}

wstring ClipboardHandler::GetClipboardText()
{
	wstring clipboardText = L"";

	if (!IsClipboardFormatAvailable(CF_UNICODETEXT) || !IsClipboardFormatAvailable(CF_TEXT))
	{	
		m_failureCounter++;
		if(m_failureCounter < FAILURE_LIMIT)
			m_maxRounds++;

		LOG_WARNING_(MainLogger) << "Format not available" << " | m_maxRounds: " << m_maxRounds << " | m_failureCounter: " << m_failureCounter;
		return clipboardText;
	}
		
	if (!OpenClipboard(m_hwnd)) //hwnd
	{
		
		m_failureCounter++;
		if (m_failureCounter < FAILURE_LIMIT)
			m_maxRounds++;

		LOG_WARNING_(MainLogger) << "OpenClipboard failed in GetClipboardText(). Status code: " << GetLastError() << " | Blocker: " << GetOpenClipboardWindow() << " | m_maxRounds: " << m_maxRounds << " | m_failureCounter: " << m_failureCounter;
		return clipboardText; //cant open Clipboard
	}

	do
	{
		HANDLE hClipboardData = GetClipboardData(CF_UNICODETEXT);
		if (hClipboardData == NULL)
		{
			LOG_WARNING_(MainLogger) << "GetClipboardData failed. Status code: " << GetLastError();
			break;
		}

		WCHAR* pClipboardData = (WCHAR*)GlobalLock(hClipboardData); //locks the memory object and returns a pointer to the memory block
		if (pClipboardData == NULL)
		{
			LOG_WARNING_(MainLogger) << "pClipboardData is NULL! (from GetClipboardText) code? " << GetLastError() << ". hClipboardData: " << hClipboardData;
			break;
		}
		GlobalUnlock(hClipboardData); // unlock memory object

		clipboardText = wstring(pClipboardData);

	} while (FALSE);

	CloseClipboard();
	return clipboardText;
}


BOOLEAN ClipboardHandler::SetClipboardText(wstring &text)
{
	BOOLEAN status = FALSE;
	if (!OpenClipboard(m_hwnd))
	{
		m_failureCounter++;
		if (m_failureCounter < FAILURE_LIMIT)
			m_maxRounds++;

		LOG_WARNING_(MainLogger) << "OpenClipboard failed in SetClipboardText(). Status code: " << GetLastError() <<  " | Blocker: " << GetOpenClipboardWindow() << " | m_maxRounds: " << m_maxRounds << " | m_failureCounter: " << m_failureCounter;
		return status;
	}
		
	EmptyClipboard();

	size_t size = (text.length() + 1) * sizeof(WCHAR); // + 1 for (wide) NUL-terminator
	HANDLE hClipboardData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, size); //returns a handle to a memory object
	
	WCHAR* pClipboardData = (WCHAR*)GlobalLock(hClipboardData); //locks the memory object and returns a pointer to the memory block
	if (pClipboardData == NULL)
	{
		LOG_WARNING_(MainLogger) << "pClipboardData is NULL! (from SetClipboardText). Error code: " << GetLastError();
	}

	CopyMemory(pClipboardData, text.c_str(), size); //copy the data from the local variable to the global memory.
	GlobalUnlock(hClipboardData); // unlock the memory

	HANDLE h = SetClipboardData(CF_UNICODETEXT, hClipboardData);
	if(h == NULL)
	{
		LOG_WARNING_(MainLogger) << "SetClipboardData failed!";
		GlobalFree(hClipboardData);
	}
	else
	{//since SetClipboardData succeeded, dont free pClipboardData (SetClipboardData does not copy the data)
		status = TRUE;
	}
	
	CloseClipboard();
	return status;
}

// -------------------------------------- Helper functions -------------------------------------- 
BOOLEAN ClipboardHandler::TriggerHijacker()
{
	LOG_DEBUG_(MainLogger) << "------------------------- TRIGGER -------------------------";

	m_hPollClipboardUserThread = CreateThread(NULL, 0, PollClipboardUserThread, this, 0, NULL); //set an observing thread to detect the possible hijacker PID
	if (m_hPollClipboardUserThread != NULL)
		SetThreadPriority(m_hPollClipboardUserThread, THREAD_PRIORITY_TIME_CRITICAL);
		

	wstring newText = GenRandBtcAddress();
	m_baitAddresses.insert(newText);

	BOOLEAN succeeded = SetClipboardText(newText); //triggers event

	if ((!succeeded) && (m_hPollClipboardUserThread != NULL)) //Trigger failed and thread creation was successful.
		TerminatePollThread();
		
	return succeeded;
}

VOID ClipboardHandler::TerminatePollThread()
{
	SetEvent(m_hTriggerEvent); //just stop immediately the poll, since our trigger failed
	WaitForSingleObject(m_hPollClipboardUserThread, INFINITE);
	m_hPollClipboardUserThread = NULL;
	LOG_DEBUG_(MainLogger) << "Terminate thread ...";
}

VOID ClipboardHandler::CleanupClipboard()
{
	if (!OpenClipboard(m_hwnd))
		return;

	EmptyClipboard();
	CloseClipboard();
}


// -------------------------------------- Thread functions --------------------------------------

//can only determine hijacker if he did not open clipboard via OpenClipboard(NULL).
DWORD WINAPI PollClipboardUserThread(LPVOID lpParam)
{
	LOG_DEBUG_(ThreadLogger) << "-----------------------START THREAD----------------------";

	ClipboardHandler* pClipboardHandler = (ClipboardHandler*)lpParam;
	HWND ownHwnd = pClipboardHandler->GetWindowHandle();
	HWND blockingHwnd = NULL;

	map<HWND,INT> hwndMap;

	for(INT i = 0;;i++)
	{
		blockingHwnd = GetOpenClipboardWindow();
		int statusCode = GetLastError();
		if (statusCode != ERROR_SUCCESS)
			LOG_DEBUG_(ThreadLogger) << "GetOpenClipboardWindow() failed. Error code: " << statusCode;
		LOG_DEBUG_(ThreadLogger) << "blockingHwnd: " << blockingHwnd;

		if ((blockingHwnd != NULL) && (blockingHwnd != ownHwnd))
			hwndMap[blockingHwnd]++; // save blockingHwnd as key and increcment value (integer) -> counting occurrence
	
		if (WaitForSingleObject(pClipboardHandler->GetTriggerEventHandle(), 0) == WAIT_OBJECT_0)
		{
			LOG_DEBUG_(ThreadLogger) << "Received terminate event ... Break loop at i = " << i;
			break;
		}
	}

	for (pair<HWND, INT> const& pair : hwndMap)
	{
		pClipboardHandler->SetKeyValueInHwndMap((HWND)pair.first, (INT)pair.second); //add value (to already exisiting value to) associated key to member map
		LOG_DEBUG_(ThreadLogger) << "HWND: " << (HWND)pair.first << " OCCURRENCE: " << (INT)pair.second;
	}
		

	LOG_DEBUG_(ThreadLogger) << "SIZE: " << hwndMap.size();

	LOG_DEBUG_(ThreadLogger) << "-----------------------END THREAD----------------------";
	return 0;
}

// -------------------------------------- Main Logic -------------------------------------- 
VOID ClipboardHandler::Terminate()
{
	LOG_DEBUG_(MainLogger) << "-----------------------Termination----------------------";
	RemoveClipboardListener();

	INT highestOccurrence = 0;
	string mostOccurredProcessPath = "";
	DWORD mostOccuredPid = 0;
	
	for (pair<HWND, INT> const& pair : m_globalHwndMap)
	{
		HWND hwnd = pair.first;
		INT occurrence = pair.second;

		DWORD processId = 0;
		DWORD threadIdCreator = GetWindowThreadProcessId(hwnd, &processId);
		LOG_DEBUG_(MainLogger) << "processId: " << processId << " | hwnd: " << hwnd << " | threadIdCreator: " << threadIdCreator;

		if (processId == 0)
			LOG_DEBUG_(MainLogger) << " something went wrong with GetWindowThreadProcessId() ... error code: " << GetLastError();

		CHAR buffer[FILENAME_MAX] = { 0 };
		GetProcessImageNameById(processId, buffer, sizeof(buffer));
		string processPath = string(buffer);

		if (occurrence > highestOccurrence)
		{
			highestOccurrence = occurrence;
			mostOccurredProcessPath = processPath;
			mostOccuredPid = processId;
		}

		LOG_DEBUG_(MainLogger) << processPath << " : " << processId << " : " << hwnd << " : " << occurrence;
	}

	LOG_DEBUG_(MainLogger) << "-----------------------END: Termination----------------------";

	if (m_hijackerDetected)
	{
		wstring msg = L"Crypto Clipboard Hijacker detected with PID: " + to_wstring(mostOccuredPid) + L"\nProcessImagePath: " + StringToWString(mostOccurredProcessPath);
		LOG_DEBUG_(MainLogger) << msg;

		//MessageBox(NULL, msg.c_str(), L"Attention: Malware found!", MB_ICONWARNING | MB_OK);
	}
	else
	{
		//MessageBox(NULL, L"NO DETECTION", L"No Detection", MB_OK);
		LOG_DEBUG_(MainLogger) << "No Detection";
	}
		

	//More debug infos
	wstringstream  debugInfos;
	debugInfos << L"More Debug Infos: " << endl << endl << "Used bait addresses:" << endl;
	for (wstring baitAddress : m_baitAddresses)
		debugInfos << baitAddress << endl;

	debugInfos << endl << "detected addresses:" << endl;
	for (wstring detectedAddress : m_detectedAddresses)
		debugInfos << detectedAddress << endl;

	debugInfos << endl << "m_globalHwndMap:" << endl;
	for (pair<HWND, INT> const& pair : m_globalHwndMap)
		debugInfos << "hwnd: " << (HWND)pair.first << " | occurrence: " << (INT)pair.second << endl;

	LOG_DEBUG_(MainLogger) << endl << debugInfos.str();;

	exit(11);
}

VOID ClipboardHandler::OnClipboardChange()
{
	if (m_terminated)
	{
		CleanupClipboard(); //make sure to cleanup def. the last clipboard (delayed) event
		return;
	}
		
	if (m_hPollClipboardUserThread != NULL)
		TerminatePollThread();

	m_roundCounter++;
	LOG_DEBUG_(MainLogger) << "m_roundCounter: " << m_roundCounter;
	if (m_hijackerDetected || (m_roundCounter >= m_maxRounds))
	{
		m_terminated = TRUE;
		Terminate();
	}

	do
	{
		wstring clipboardText = GetClipboardText();
		if (clipboardText.empty())
			break;
			
		LOG_DEBUG_(MainLogger) << L"ClipboardContent:" << endl << clipboardText;

		if (m_baitAddresses.find(clipboardText) != m_baitAddresses.end()) //Ignore our triggered Cliboard events
		{
			LOG_DEBUG_(MainLogger) << L"its just my bait...";
			break;
		}

		//some malware families poll every X time intervall, therefore no time-based detection anymore
		wstring btcMatch(L"");
		if (IsBtcAddress(clipboardText, &btcMatch))  //we only care about btc addresses, since most malware hijacks at least btc addresses
		{
			m_detectedAddresses.insert(btcMatch);

			LOG_DEBUG_(MainLogger) << "Found unknown address! ";
			m_hijackerDetected = TRUE;
		}
	} while (FALSE);

	TriggerHijacker();
}

