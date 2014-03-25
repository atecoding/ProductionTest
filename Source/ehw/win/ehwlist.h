/********************************************************************************
 *
 * ehwlist.h
 *
 * A class that builds a list of Echo hardware objects
 *
 ********************************************************************************/

#ifndef _EhwList_h_
#define _EhwList_h_

#include <setupapi.h>
#include "ehw.h"
#include "cdevchange.h"

class ehw;
class ehwlist
{
protected:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ehwlist);

	Array<uint32>			_vendor_ids;
	ehw::ptr					_hwlist;
	int32						_numdevices;

	BOOL GetInterface
	(
		HDEVINFO hdi,
		PSP_DEVICE_INTERFACE_DATA did,
		MemoryBlock &mb,
		DWORD index
	);

	CDevChangeNotify			*_devchange;
	FileLogger					*_logger;

	void SortList();

public:
	ehwlist(int num_vendor_ids,uint32 *vendor_ids,CriticalSection *lock = NULL,FileLogger *logger = NULL);
	~ehwlist();

	ehw *BuildDeviceList(char *pnpid);
	ehw *RemoveDevice(const char *pnpid);
	ehw *FindDevice(char *pnpid);

	int32 GetNumDevices()
	{
		return _numdevices;
	}

	ehw *GetNthDevice(int32 index);
	
	void Cleanup(ehw *pDeviceToKeep = NULL);
	
	void RegisterMessageListener(MessageListener *listener);
	void UnregisterMessageListener(MessageListener *listener);
	
	CriticalSection	*_cs;
	bool localLock;

	enum
	{
		MATCH_ANY_VENDOR = 0
	};
};

#pragma comment(lib, "setupapi.lib")

#endif // _EhwList_h_
