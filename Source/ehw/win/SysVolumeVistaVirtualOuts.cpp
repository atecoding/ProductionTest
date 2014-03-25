#ifdef PCI_BUILD

#include <windows.h>
#include "juce.h"
#include "SysVolumeVista.h"
#include "ehw.h"
#include "hwcaps.h"


SysVolumeVistaVirtualOuts::SysVolumeVistaVirtualOuts(ehw *dev) :
	SysVolumeVista(dev)
{
}


SysVolumeVistaVirtualOuts::~SysVolumeVistaVirtualOuts()
{
}



int SysVolumeVistaVirtualOuts::SetPlayGain(uint32 virtout,uint32 output,float gain)
{
	DBG("SysVolumeVistaVirtualOuts::SetPlayGain");
	if (0 == output)
		return SysVolumeVista::SetPlayGain(virtout,virtout,gain);

	return SysVolume::SetPlayGain(virtout,output,gain);
}



int SysVolumeVistaVirtualOuts::SetPlayMute(uint32 virtout,uint32 output,uint32 mute)
{
	DBG_PRINTF((T("SysVolumeVistaVirtualOuts::SetPlayMute vout %d   out %d   mute %d"),virtout,output,mute));

	if (0 == output)
	{
	//	virtout &= ~1;	
		return SysVolumeVista::SetPlayMute(virtout,virtout,mute);
	}

	return SysVolume::SetPlayMute(virtout,output,mute);
}

#endif
