/*
ClipboardHijackingDetection
Author: Emanuel Durmaz
*/

#include "stdafx.h"
#include "ClipboardHijackingProtection.h"
#include "MessageWindowHandler.h"


using namespace std;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,	_In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	time_t timestamp = std::time(nullptr);
	string formattedTimestamp = formatTimestamp(timestamp);

	static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;

	string filenameMainLogger = string("ClipboardHijackingProtection_MainThreadLog_") + formattedTimestamp + string(".txt");
	string filenameThreadLogger = string("ClipboardHijackingProtection_PollThreadLog_") + formattedTimestamp + string(".txt");

	plog::init<MainLogger>(plog::debug, filenameMainLogger.c_str()).addAppender(&consoleAppender);
	plog::init<ThreadLogger>(plog::debug, filenameThreadLogger.c_str()).addAppender(&consoleAppender);

	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout); //redirect stdout to console

	LOG_DEBUG_(MainLogger) << "START";
	LOG_DEBUG_(ThreadLogger) << "START";

	AppData appData;

	MessageWindowHandler messageWindowHandler;
	HWND hwnd = messageWindowHandler.RegisterMessageWindow(hInstance, appData);

	ClipboardHandler clipboardHandler(hwnd);
	appData.pClipboardHandler = &clipboardHandler;

	clipboardHandler.AddClipboardListener();
	clipboardHandler.TriggerHijacker();


	// Main message loop:
	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return (int)msg.wParam;
}