#pragma once

#include "SysVolume.h"

class SysVolumeMixerAPI : public Win32MessageListener, public SysVolume
{
public:
	SysVolumeMixerAPI(ehw *dev);
	~SysVolumeMixerAPI();

protected:
	virtual void Win32MessageReceived(UINT uMsg,WPARAM wParam,LPARAM lParam);

	Array<HMIXER> _mixers;
};
