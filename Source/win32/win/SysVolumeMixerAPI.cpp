#ifdef WINDOWS_VOLUME_CONTROL

#include <new>
#include <windows.h>
#include "juce.h"
#include "ehw.h"
#include "NotificationWindow.h"
#include "SysVolumeMixerAPI.h"


#define GET_MUTEX			WaitForSingleObject(_dev->m_hMutex,INFINITE)
#define RELEASE_MUTEX		ReleaseMutex(_dev->m_hMutex)

SysVolumeMixerAPI::SysVolumeMixerAPI(ehw *dev) :
	SysVolume(dev)
{
	UINT numdevs,i;
	MIXERCAPS mc;
	MMRESULT mmr;
	HWND hwnd;
	String uniquename = dev->GetUniqueName();

	hwnd = Win32NotificationWindow::handle();

    numdevs = mixerGetNumDevs();
	for (i = 0; i < numdevs; i++)
	{
		mmr = mixerGetDevCaps(i,&mc,sizeof(mc));
		if (MMSYSERR_NOERROR == mmr)
		{
			String name(mc.szPname);

			if (name.startsWith(uniquename) && (name.containsIgnoreCase(T("out")) || name.containsIgnoreCase(T("playback"))))
			{
				HMIXER hm;

				DBG_PRINTF((T("Opening mixer %S"),mc.szPname));

				mmr = mixerOpen( &hm,
								i,
								(DWORD_PTR) hwnd,
								NULL,
								CALLBACK_WINDOW);
				if (MMSYSERR_NOERROR == mmr)
				{
					_mixers.add(hm);
				}
			}

		}
	}

	Win32NotificationWindow::addListener(this);
}

SysVolumeMixerAPI::~SysVolumeMixerAPI(void)
{
	int i;
	
	Win32NotificationWindow::removeListener(this);

	for (i = 0; i < _mixers.size(); i++)
	{
		mixerClose(_mixers[i]);
	}
}


void SysVolumeMixerAPI::Win32MessageReceived(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if (MM_MIXM_CONTROL_CHANGE == uMsg)
	{
		sendChangeMessage();
	}
}

#endif


