#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "juce.h"
#include "NotificationWindow.h"

static char classname[] = "notification msg window";

static Win32NotificationWindow *gInstance = NULL;

Win32NotificationWindow *Win32NotificationWindow::instance()
{
	if (NULL == gInstance)
		gInstance = new Win32NotificationWindow;

	return gInstance;
}

Win32NotificationWindow::Win32NotificationWindow() :
	Thread("win32 notification thread"),
	hWindow(NULL)
{
	startThread();
}

Win32NotificationWindow::~Win32NotificationWindow()
{
	if (handle())
	{
		PostMessage(hWindow,WM_CLOSE,0,0);
		waitForThreadToExit(2000);
	}
}

LRESULT CALLBACK Win32NotificationWindow::WndProc
(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
)
{
	LONG_PTR context;
	Win32NotificationWindow *that;
	int i;

	context = GetWindowLongPtrA(hwnd,GWLP_USERDATA);
	that = (Win32NotificationWindow *) context;
	if (that)
	{
		ScopedLock sl(that->cs);

		//DBG(String:: formatted ("Win32NotificationWindow::WndProc %x",uMsg));

		for (i = 0; i < that->listeners.size(); i++)
		{
			that->listeners[i]->Win32MessageReceived(uMsg,wParam,lParam);
		}
	}

 	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}

void Win32NotificationWindow::run()
{
	MSG Msg;
	int rval;

	//
	// register the window class
	//
	WNDCLASSEXA wcex;

	wcex.cbSize				= sizeof(WNDCLASSEX);
	wcex.style				= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc		= Win32NotificationWindow::WndProc;
	wcex.cbClsExtra			= 0;
	wcex.cbWndExtra			= 0;
	wcex.hInstance			= GetModuleHandle(NULL);
	wcex.hIcon				= 0;
	wcex.hCursor			= 0;
	wcex.hbrBackground		= (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName		= 0;
	wcex.lpszClassName		= classname;
	wcex.hIconSm			= 0;

	RegisterClassExA(&wcex);

	//
	// make the window
	//
	hWindow = CreateWindowExA(	0,
								classname,
								NULL,
								0,
								0,0,0,0,
								HWND_MESSAGE,NULL,
								GetModuleHandle(NULL), NULL);
	if (NULL == hWindow)
		return;

	//
	// Save the "this" pointer in the window
	//
	SetWindowLongPtrA(hWindow,(int) GWLP_USERDATA,(__int3264) (LONG_PTR) this);

	//
	// listen to the message loop
	//
	while ( (rval = GetMessage(&Msg, hWindow, 0, 0)) != 0)
	{
		if (-1 == rval)
			return;

		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
}

void Win32NotificationWindow::addListener(Win32MessageListener *wml)
{
	Win32NotificationWindow *wnw = Win32NotificationWindow::instance();

	if (wnw)
	{
		ScopedLock sl(wnw->cs);

		wnw->listeners.addIfNotAlreadyThere(wml);
	}
}

void Win32NotificationWindow::removeListener(Win32MessageListener *wml)
{
	if (gInstance)
	{
		ScopedLock sl(gInstance->cs);
		gInstance->listeners.removeAllInstancesOf(wml);
	}
}

HWND Win32NotificationWindow::handle()
{
	Win32NotificationWindow *wnw = Win32NotificationWindow::instance();

	while (wnw->isThreadRunning() && (NULL == wnw->hWindow))
	{
		Sleep(10);
	}

	return wnw->hWindow;
}

#endif