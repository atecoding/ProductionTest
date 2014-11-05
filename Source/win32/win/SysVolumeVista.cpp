
#include <windows.h>
#include "juce.h"
#include "SysVolumeVista.h"
#include "ehw.h"
#include "hwcaps.h"


// {A54C9E5B-A49B-4546-8925-67D76C48FA31}
static const GUID SysVolumeVistaGuid = 
{ 0xa54c9e5b, 0xa49b, 0x4546, { 0x89, 0x25, 0x67, 0xd7, 0x6c, 0x48, 0xfa, 0x31 } };


HRESULT GetControllerDeviceFromEndpoint(IMMDeviceEnumerator *pEnumerator,IMMDevice* pEndpoint, IMMDevice** ppController)
{
    HRESULT                  hr;
    IDeviceTopology			 *pTopology;
    IConnector			     *pPlug;
    LPWSTR                   pwstrControllerId = NULL;

    *ppController = NULL;

    hr = pEndpoint->Activate(__uuidof(IDeviceTopology), CLSCTX_ALL, NULL, (void**)&pTopology);
	 if (0 == hr)
	 {
		// By definition, Endpoint devices only have 1 connector, representing the plug.
		 hr = pTopology->GetConnector(0, &pPlug);
		 if (0 == hr)
		 {
			 // This is the quickest way to get from an Endpoint plug to the audio controller.
			 hr = pPlug->GetDeviceIdConnectedTo(&pwstrControllerId);

			 if (0 == hr)
			 {
				 // Get IMMDevice ptr via device ID
				 hr = pEnumerator->GetDevice(pwstrControllerId, ppController);
			 }

			 pPlug->Release();
		 }
		
		 pTopology->Release();
	 }

    // Remember to free device IDs using CoTaskMemFree
    if (pwstrControllerId != NULL)
        CoTaskMemFree(pwstrControllerId);

    return hr;
}


HRESULT GetIKsControlFromEndpoint(IMMDeviceEnumerator *pEnumerator,IMMDevice* pEndpoint, IKsControl** ppKsControl)
{
    HRESULT             hr;
    IMMDevice			*pController;

    hr = GetControllerDeviceFromEndpoint(pEnumerator,pEndpoint, &pController);
	if (0 == hr)
	{
		hr = pController->Activate(__uuidof(IKsControl), CLSCTX_INPROC_SERVER, NULL, (void**)ppKsControl);
		pController->Release();
	}
    return hr;
}


SysVolumeVista::SysVolumeVista(ehw *dev) :
	SysVolume(dev)
{
	HRESULT hr;

	_map_entries = dev->getcaps()->numplaychan();
	_map = new EndpointVolumeInfo[ _map_entries ];
	memset(_map,0,_map_entries*sizeof(EndpointVolumeInfo));

	CoInitialize(NULL);

	//	
	// Enumerate all the MMDevices in the system
	//	
	hr = CoCreateInstance(	__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, 
							__uuidof(IMMDeviceEnumerator), (LPVOID *)&_enumerator);
	if (0 == hr)
	{
		BuildEndpointArray();
	}

#ifdef JUCE_DEBUG
	//DumpMap();
#endif
}


SysVolumeVista::~SysVolumeVista()
{
	FreeEndpoints();

	if (_enumerator)
		_enumerator->Release();

	CoUninitialize();

	delete[] _map;
}


void SysVolumeVista::BuildEndpointArray()
{
	HRESULT hr;
	IMMDeviceCollection *collection;	
	const ScopedLock lock(_endpoints.getLock());

	FreeEndpoints();

	hr = _enumerator->EnumAudioEndpoints( eRender, DEVICE_STATE_ACTIVE, &collection);
	if (0 == hr)
	{
		UINT count,idx;

		hr = collection->GetCount(&count);
		if (0 == hr)
		{
			for (idx = 0; idx < count; idx++)
			{
				IMMDevice *endpoint;

				hr = collection->Item(idx,&endpoint);
				if (0 == hr)
				{
					hr = StoreEndpoint(endpoint);
					if (0 != hr)
					{
						endpoint->Release();
					}
				}
			}
		}

		collection->Release();
	}
}


void SysVolumeVista::FreeEndpoints()
{
	const ScopedLock lock(_endpoints.getLock());

	DBG("SysVolumeVista::FreeEndpoints");

	for (int i = 0; i < _endpoints.size(); i++)
	{	
		FreeEndpoint(_endpoints.getReference(i));
	}

	_endpoints.clear();

	memset(_map,0,_map_entries*sizeof(EndpointVolumeInfo));
}


HRESULT SysVolumeVista::GetEchoEndpointId
(	
	IMMDevice *endpoint,
	ECHO_ENDPOINT_ID &eid
)
{
	IKsControl *ksc;
	HRESULT hr;

	hr = GetIKsControlFromEndpoint(_enumerator,endpoint,&ksc);
	if (0 == hr)
	{
		KSPROPERTY ksp;
		ULONG bytes;

		memset(&ksp,0,sizeof(ksp));
		ksp.Set = KSPROPSETID_EchoEndpointId;
		ksp.Id = KSPROPERTY_ECHO_ENDPOINT_ID;
		ksp.Flags = KSPROPERTY_TYPE_GET;
		hr = ksc->KsProperty(&ksp,sizeof(ksp),&eid,sizeof(eid),&bytes);

		DBG_PRINTF((T("hr %d  eid.SerialNum %I64x"),hr,eid.SerialNum));

		ksc->Release();
	}
	else
	{
		DBG("No ks control");
	}

	return hr;
}


HRESULT SysVolumeVista::StoreEndpoint(IMMDevice *endpoint)
{
	int idx,i;
	ECHO_ENDPOINT_ID eid;
	EndpointInfo info;
	HRESULT hr;

	hr = GetEchoEndpointId(endpoint,eid);
	if (0 == hr)
	{
		if (_dev->GetSerialNumber() == eid.SerialNum)
		{
			hr = endpoint->Activate(__uuidof(IAudioEndpointVolume), 
									CLSCTX_INPROC_SERVER, 
									NULL, 
									(void**)&info.aevolume);
			if (0 == hr)
			{
				info.volume_notify = new CVolumeNotification(this);
				hr = info.aevolume->RegisterControlChangeNotify(info.volume_notify);
				if (0 == hr)
				{
					//
					// store this endpoint in an empty slot
					//
					const ScopedLock lock(_endpoints.getLock());

					for (idx = 0; idx < _endpoints.size(); idx++)
					{
						if (NULL == _endpoints[idx].mmdevice)
							break;
					}

					info.mmdevice = endpoint;
					info.StartChannel = eid.StartChannel;
					info.ChannelWidth = eid.ChannelWidth;
					_endpoints.set(idx,info);

					//
					// Add this endpoint to the channel map
					//
					EndpointVolumeInfo evi = { info.aevolume, 0 };
					
					idx = eid.StartChannel;
					for (i = 0; i < eid.ChannelWidth; i++)
					{
						DBG_PRINTF((T("%d %p %d"),idx,evi.aevolume,evi.offset));
						_map[idx] = evi;
						evi.offset++;
						idx++;
					}
				}
				else
				{
					info.volume_notify->Release();
				}
			}
		}
		else
		{
			hr = S_FALSE;
		}
	}
	
	return hr;
}


void SysVolumeVista::FreeEndpoint(EndpointInfo &info)
{
	if (info.aevolume)
	{
		if (info.volume_notify)
		{
			info.aevolume->UnregisterControlChangeNotify(info.volume_notify);
			info.volume_notify->Release();
		}

		info.aevolume->Release();
	}

	if (info.mmdevice)
		info.mmdevice->Release();

	memset(&info,0,sizeof(info));
}
	

STDMETHODIMP CVolumeNotification::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA NotificationData)
{
	if (NotificationData->guidEventContext != SysVolumeVistaGuid)
		_parent->sendChangeMessage();

	return S_OK;
}



int SysVolumeVista::SetPlayGain(uint32 virtout,uint32 output,float gain)
{
	const ScopedLock lock(_endpoints.getLock());
	IAudioEndpointVolume *aev;
	int rval = 1;
	DBG("SysVolumeVista::SetPlayGain");

	if (0 == _endpoints.size())
	{
		DBG_PRINTF((T("SysVolumeVista::SetPlayGain  BuildEndpointArray")));

		BuildEndpointArray();
	}

	aev = _map[output].aevolume;

	DBG_PRINTF((T("%d %p %d"),output,aev,_map[output].offset));
	if (aev)
	{
		rval = aev->SetChannelVolumeLevel(_map[output].offset,gain,&SysVolumeVistaGuid);
		if (S_FALSE == rval) // SetChannelVolumeLevel returns S_FALSE if the value isn't changing
			rval = S_OK;
		if (S_OK != rval)
		{
			DBG_PRINTF((T("SetChannelVolumeLevel  failed  rval:%x"),rval));
			FreeEndpoints();
			rval = -1;
		}
	}
	else
	{
		rval = SysVolume::SetPlayGain(virtout,output,gain);
	}

	return rval;
}


int SysVolumeVista::SetPlayMute(uint32 virtout,uint32 output,uint32 mute)
{
	const ScopedLock lock(_endpoints.getLock());
	IAudioEndpointVolume *aev;
	int rval = 1;
	DBG_PRINTF((T("SysVolumeVista::SetPlayMute virtout:%d output:%d mute:%d"),virtout,output,mute));

	if (0 == _endpoints.size())
	{
		DBG("SysVolumeVista::SetPlayMute  No endpoints");
		BuildEndpointArray();
	}

	/*

	Vista does not support muting per individual channels; you can only mute
	the entire device at once.

	To work around this, pass null for the reference GUID.  This means that the console
	will receive the change notification callback and automatically update all the mutes
	choose one.

	The practical result of this is that the gang button will not matter for playback mutes;
	the mutes will always be ganged, regardless of the gang button setting.
	
	*/

	aev = _map[output].aevolume;
	if (aev)
	{
		DBG_PRINTF((T("%d %p %d"),output,aev,_map[output].offset));

		rval = aev->SetMute(mute,NULL);
		if (S_FALSE == rval) // SetMute returns S_FALSE if the value isn't changing
			rval = S_OK;
		if (S_OK != rval)
		{
			DBG_PRINTF((T("SetMute failed  rval:%x"),rval));

			FreeEndpoints();
			rval = -1;
		}
	}
	else
	{
		DBG("AEV is null");
		rval = SysVolume::SetPlayMute(virtout,output,mute);
	}

	return rval;
}


#ifdef JUCE_DEBUG

void SysVolumeVista::DumpMap()
{
	int i;
	const ScopedLock lock(_endpoints.getLock());
	
	DBG("\nSysVolumeVista::DumpMap()");
	for (i = 0; i < _dev->getcaps()->numplaychan(); i++)
	{
		DBG_PRINTF((T("%d %p %d"),i,_map[i].aevolume,_map[i].offset));
	}
}

#endif
