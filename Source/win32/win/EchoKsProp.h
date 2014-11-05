#ifndef _ksprop_echo_id_h_
#define _ksprop_echo_id_h_

// {B06EF68F-A505-4b0c-B293-39AF7565A188}

#define STATIC_KSPROPSETID_EchoEndpointId \
	0xb06ef68fL, 0xa505, 0x4b0c, 0xb2, 0x93, 0x39, 0xaf, 0x75, 0x65, 0xa1, 0x88
DEFINE_GUIDSTRUCT("B06EF68F-A505-4b0c-B293-39AF7565A188", KSPROPSETID_EchoEndpointId);
#define KSPROPSETID_EchoEndpointId DEFINE_GUIDNAMED(KSPROPSETID_EchoEndpointId)

enum
{
	KSPROPERTY_ECHO_ENDPOINT_ID
};

typedef struct 
{
	ULONGLONG	SerialNum;
	int			StartChannel;
	int			ChannelWidth;
} ECHO_ENDPOINT_ID;

#endif
