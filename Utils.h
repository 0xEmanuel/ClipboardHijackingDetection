#pragma once

#define BITCOIN_REGEX L"[13][a-km-zA-HJ-NP-Z0-9]{26,33}"

// [13][a-km-zA-HJ-NP-Z0-9]{26,33}$ matches: " 1F1tAaz5x1HUXrCNLbtMDqcw6o5GNn4xqX" or "	1F1tAaz5x1HUXrCNLbtMDqcw6o5GNn4xqX" (TAB)
//^[13][a-km-zA-HJ-NP-Z0-9]{26,33} matches: "1F1tAaz5x1HUXrCNLbtMDqcw6o5GNn4xqX " or "1F1tAaz5x1HUXrCNLbtMDqcw6o5GNn4xqX	" (TAB)
//^[13][a-km-zA-HJ-NP-Z0-9]{26,33}$ matches: "1F1tAaz5x1HUXrCNLbtMDqcw6o5GNn4xqX"
// [13][a-km-zA-HJ-NP-Z0-9]{26,33} matches: "    1F1tAaz5x1HUXrCNLbtMDqcw6o5GNn4xqX        "

#define  LOG_DEBUG_COUT(instance, msg) LOG_DEBUG_(instance) << msg; cout << msg

#include "stdafx.h"
#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>

enum // Define log instances. Default is 0 and is omitted from this enum.
{
	MainLogger = 0,
	ThreadLogger = 1
};

using namespace std;

VOID TerminateProcessById(DWORD processId); //TODO: or by handle? GetProcessHandleFromHwnd 
BOOLEAN IsBtcAddress(IN wstring &text, OUT wstring *matchStr);

VOID GetProcessImageNameById(DWORD processId, CHAR* buffer, DWORD bufferSize);

wstring StringToWString(const std::string& s);

wstring GenRandBtcAddress();

string formatTimestamp(const time_t rawtime);