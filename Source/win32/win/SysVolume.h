#pragma once

class ehw;
class SysVolume : public ChangeBroadcaster
{
public:
	static SysVolume *create(ehw *dev);

	virtual int SetPlayGain(uint32 virtout,uint32 output,float gain);
	virtual int SetPlayMute(uint32 virtout,uint32 output,uint32 mute);

protected:
	SysVolume(ehw *dev) : _dev(dev)
	{
	}

	ehw *_dev;
};
