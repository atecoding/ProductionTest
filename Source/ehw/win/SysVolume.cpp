#include <new>
#include <windows.h>
#include "juce.h"
#include "NotificationWindow.h"
#include "SysVolumeMixerAPI.h"
#include "SysVolumeVista.h"
#include "ehw.h"
#include "hwcaps.h"

SysVolume* SysVolume::create(ehw *dev)
{
#ifdef WINDOWS_VOLUME_CONTROL

#if WINVER >= 0x600
	if (SystemStats::getOperatingSystemType() >= SystemStats::WinVista)
	{
#ifdef PCI_BUILD
		if (0 != dev->getcaps()->NumVirtualOutputs())
			return new SysVolumeVistaVirtualOuts(dev);
#endif

		return new SysVolumeVista(dev);
	}
#endif

	return new SysVolumeMixerAPI(dev);

#else

	return new SysVolume(dev);

#endif
}


#ifdef PCI_BUILD

int SysVolume::SetPlayGain(uint32 virtout,uint32 output,float gain)
{
	MIXER_MULTI_FUNCTION mmf;
	int rval,igain;
	extern const float DbToGenericScale;

	DBG("SysVolume::SetPlayGain");

	igain = roundFloatToInt(gain * DbToGenericScale);

	mmf.iCount = 1;
	mmf.MixerFunction[0].iFunction = MXF_SET_LEVEL;
	mmf.MixerFunction[0].Channel.wChannel = virtout;
	mmf.MixerFunction[0].Channel.dwType = ECHO_PIPE_OUT;
	mmf.MixerFunction[0].Data.PipeOut.wBusOut = output;
	mmf.MixerFunction[0].Data.PipeOut.Data.iLevel = igain;

	rval = _dev->DoMixerMultiFunction(&mmf,sizeof(mmf));
	if (rval != 0)
	{
		return mmf.MixerFunction[0].RtnStatus;
	}

	return 1;
}


int SysVolume::SetPlayMute(uint32 virtout,uint32 output,uint32 mute)
{
	int rval;
	MIXER_MULTI_FUNCTION mmf;

	mmf.iCount = 1;
	mmf.MixerFunction[0].iFunction = MXF_SET_MUTE;
	mmf.MixerFunction[0].Channel.wChannel = virtout;
	mmf.MixerFunction[0].Channel.dwType = ECHO_PIPE_OUT;
	mmf.MixerFunction[0].Data.PipeOut.wBusOut = output;
	mmf.MixerFunction[0].Data.PipeOut.Data.bMuteOn = mute;

	rval = _dev->DoMixerMultiFunction(&mmf,sizeof(mmf));
	if (rval != 0)
	{
		return mmf.MixerFunction[0].RtnStatus;
	}

	return 1;
}

#endif

#ifdef ECHO1394

int SysVolume::SetPlayGain(uint32 virtout,uint32 output,float gain)
{
	const ScopedLock locker(*(_dev->_lock));
	int rval;
	WDM_EFC *cmd = &(_dev->_efc);

	cmd->cmd.quads = EFC_QUADS(efc_gain);
	cmd->cmd.h.category = cat_playback_mixer;
	cmd->cmd.h.cmd = cmd_set_gain;
	cmd->cmd.u.c.gain.chan = output;
	cmd->cmd.u.c.gain.gain = _dev->DbToLin(gain);

	rval = _dev->SendEfc(	cmd,
							WDM_EFC_BYTES(efc_gain),
							cmd,
							WDM_EFR_BYTES(efr_gain));

	return rval;
}


int SysVolume::SetPlayMute(uint32 virtout,uint32 output,uint32 mute)
{
	const ScopedLock locker(*(_dev->_lock));
	int rval;
	WDM_EFC *cmd = &(_dev->_efc);

	cmd->cmd.quads = EFC_QUADS(efc_mute);
	cmd->cmd.h.category = cat_playback_mixer;
	cmd->cmd.h.cmd = cmd_set_mute;
	cmd->cmd.u.c.mute.chan = output;
	cmd->cmd.u.c.mute.mute = mute;

	rval = _dev->SendEfc(	cmd,
							WDM_EFC_BYTES(efc_mute),
							cmd,
							WDM_EFR_BYTES(efr_mute));

	return rval;
}

#endif


#ifdef ECHOUSB
int SysVolume::SetPlayGain(uint32 virtout,uint32 output,float gain)
{
	return 0;
}

int SysVolume::SetPlayMute(uint32 virtout,uint32 output,uint32 mute)
{
	return 0;
}

#endif