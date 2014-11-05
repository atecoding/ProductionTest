#pragma once

#if WINVER >= 0x600
#include <mmdeviceapi.h>
#include <endpointvolume.h>

#include "SysVolume.h"
#include "EchoKsProp.h"

//-----------------------------------------------
//
// SysVolumeVista
//
class CVolumeNotification;
class CEndpointNotification;
class SysVolumeVista :
	public SysVolume
{
public:
	SysVolumeVista(ehw *dev);
	virtual ~SysVolumeVista();

	virtual int SetPlayGain(uint32 virtout,uint32 output,float gain);
	virtual int SetPlayMute(uint32 virtout,uint32 output,uint32 mute);

protected:
	struct EndpointInfo
	{
		IMMDevice				*mmdevice;
		IAudioEndpointVolume	*aevolume;
		CVolumeNotification		*volume_notify;
		int						StartChannel;
		int						ChannelWidth;
	};

	struct EndpointVolumeInfo
	{
		IAudioEndpointVolume	*aevolume;
		int						offset;
	};



	Array<EndpointInfo,CriticalSection> _endpoints;
	EndpointVolumeInfo *_map;
	int _map_entries;

	IMMDeviceEnumerator *_enumerator;

	void BuildEndpointArray();
	void FreeEndpoints();
	HRESULT GetEchoEndpointId(	IMMDevice *endpoint,
								ECHO_ENDPOINT_ID &eid);
	HRESULT StoreEndpoint(IMMDevice *endpoint);
	void FreeEndpoint(EndpointInfo &info);

#ifdef JUCE_DEBUG
	void DumpMap();
#endif

	friend CEndpointNotification;
};



//-----------------------------------------------
//
// CVolumeNotification
//
class ehw;
class SysVolumeVista;
class CVolumeNotification : public IAudioEndpointVolumeCallback 
{ 
    LONG m_RefCount; 
	SysVolumeVista *_parent;

    ~CVolumeNotification(void) {}; 
public: 
    CVolumeNotification(SysVolumeVista *sv) : m_RefCount(1) 
    { 
		_parent = sv;
    } 
    STDMETHODIMP_(ULONG)AddRef() { return InterlockedIncrement(&m_RefCount); } 
    STDMETHODIMP_(ULONG)Release()  
    { 
        LONG ref = InterlockedDecrement(&m_RefCount);  
        if (ref == 0) 
            delete this; 
        return ref; 
    } 
    STDMETHODIMP QueryInterface(REFIID IID, void **ReturnValue) 
    { 
        if (IID == IID_IUnknown || IID== __uuidof(IAudioEndpointVolumeCallback))  
        { 
            *ReturnValue = static_cast<IUnknown*>(this); 
            AddRef(); 
            return S_OK; 
        } 
        *ReturnValue = NULL; 
        return E_NOINTERFACE; 
    } 

    STDMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA NotificationData);
}; 



//-----------------------------------------------
//
// CMMDeviceId
//
class CMMDeviceId
{
public :
	CMMDeviceId(IMMDevice *mmdev)
	{
		mmdev->GetId(&id);
	}

	~CMMDeviceId()
	{
		CoTaskMemFree(id);
	}

	bool same(LPCWSTR other)
	{
		return 0 == wcscmp(other,id);
	}

protected :
	WCHAR *id;
};


//-----------------------------------------------
//
// SysVolumeVistaVirtualOuts
//

#ifdef PCI_BUILD

class SysVolumeVistaVirtualOuts :
	public SysVolumeVista
{
public:
	SysVolumeVistaVirtualOuts(ehw *dev);
	virtual ~SysVolumeVistaVirtualOuts();

	virtual int SetPlayGain(uint32 virtout,uint32 output,float gain);
	virtual int SetPlayMute(uint32 virtout,uint32 output,uint32 mute);
};

#endif

#endif
