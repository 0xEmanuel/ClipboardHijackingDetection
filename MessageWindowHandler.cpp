#include "stdafx.h"
#include "MessageWindowHandler.h"
#include "ClipboardHijackingProtection.h"

MessageWindowHandler::MessageWindowHandler()
{
}


MessageWindowHandler::~MessageWindowHandler()
{
}

HWND MessageWindowHandler::RegisterMessageWindow(HINSTANCE hInst, AppData &pAppData)
{
	// Registers the window class.
	WNDCLASSEX wx = {};
	WCHAR className[] = L"MessageWindow";
	wx.cbSize = sizeof(WNDCLASSEX);
	wx.lpfnWndProc = WndProc;        // function which will handle messages
	wx.hInstance = hInst;
	wx.lpszClassName = className;

	HWND hwnd = NULL;	
	if (RegisterClassExW(&wx))
	{
		//create Message-Only Window (flag set HWND_MESSAGE) and need to use the registered className
		hwnd = CreateWindowExW(0, className, NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, NULL, &pAppData); //CreateWindowExW triggers WM_CREATE event.

		if (hwnd == NULL)
			return NULL;

		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)(&pAppData));
	}

	return hwnd;
}

AppData* GetAppData(HWND hwnd)
{
	LONG_PTR ptr = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	AppData *pAppData = reinterpret_cast<AppData*>(ptr);
	return pAppData;
}


//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_DESTROY  - post a quit message and return
//

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLIPBOARDUPDATE:
	{	
		LOG_DEBUG_(MainLogger) << "-------------------------WM_CLIPBOARDUPDATE-------------------------";
		//get the clipboardHandler from the context of the caller
		AppData *pAppData = GetAppData(hwnd);
		ClipboardHandler *pClipboardHandler = pAppData->pClipboardHandler;
		pClipboardHandler->OnClipboardChange();

		break;
	}
	case WM_DESTROY:
		break;
	default:
		return DefWindowProcW(hwnd, msg, wParam, lParam);
		break;
	}
	return 0;
}