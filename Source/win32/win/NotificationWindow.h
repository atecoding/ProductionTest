#if JUCE_WIN32

#ifndef _notification_window_h_
#define _notification_window_h_

class Win32MessageListener
{
protected:
	virtual void Win32MessageReceived(UINT uMsg,WPARAM wParam,LPARAM lParam) = 0;

	friend class Win32NotificationWindow;
};
    

class Win32NotificationWindow : 
	public Thread,
	public DeletedAtShutdown
{
public:
	virtual ~Win32NotificationWindow();

	static HWND handle();

	static void addListener(Win32MessageListener *wml);
	static void removeListener(Win32MessageListener *wml);

protected:
	Win32NotificationWindow();
	
	static Win32NotificationWindow *instance();
	static DWORD WINAPI WndThread(LPVOID context);
	static LRESULT CALLBACK WndProc(HWND hwnd,
									UINT uMsg,
									WPARAM wParam,
									LPARAM lParam );

	virtual void run();
	
	HWND		hWindow;

	Array<Win32MessageListener *> listeners;
	CriticalSection cs;
};

#endif

#endif
