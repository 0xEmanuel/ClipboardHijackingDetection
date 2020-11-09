#pragma once
#include "ClipboardHandler.h"
#include "ClipboardHijackingProtection.h"


class MessageWindowHandler
{
public:
	MessageWindowHandler();
	~MessageWindowHandler();

	HWND RegisterMessageWindow(HINSTANCE hInstance, AppData &pAppData);

private:
	
	

};

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);