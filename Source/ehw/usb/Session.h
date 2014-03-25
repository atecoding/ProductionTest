#ifndef _SESSION_H_
#define _SESSION_H_

#define SESSION_VERSION 0x00000200

#define MAX_PHY_AUDIO_IN     	40
#define MAX_PHY_AUDIO_OUT   	40
#define MAX_LABEL_SIZE			22

#define PAN_HARD_RIGHT 100

#define SESSION_FLAG_PHANTOM_POWER_MASK	efc_flag_phantom_power

typedef struct
{
	unsigned char    nominal;		// number of bits to shift each input sample left
	unsigned char	pad;
	char    label[MAX_LABEL_SIZE];
} INPUT_SETTINGS; // 24 unsigned chars

typedef struct
{
	unsigned char	mute;
	unsigned char	solo;
	char    label[MAX_LABEL_SIZE];
} PLAYBACK_SETTINGS; // 24 unsigned chars

typedef struct
{
	unsigned char	mute;
	unsigned char	nominal;
	char    label[MAX_LABEL_SIZE];
} OUTPUT_SETTINGS; // 24 unsigned chars


#define SESSION_MUTE_BIT		1
#define SESSION_SOLO_BIT		2
#define SESSION_PAD_BIT			4


//
// Input gains
//
typedef struct tag_input_gains
{
	uint32 gains[MAX_PHY_AUDIO_IN];
} session_input_gains;

//
// The subsession is the bits of the session that need to be 
// unsigned char-swapped on the ARM and on Windows
//
typedef struct tag_session_swappable
{
	INPUT_SETTINGS		inputs[MAX_PHY_AUDIO_IN];

	unsigned char				monitorpans[MAX_PHY_AUDIO_IN][MAX_PHY_AUDIO_OUT];
	unsigned char				monitorflags[MAX_PHY_AUDIO_IN][MAX_PHY_AUDIO_OUT];

	PLAYBACK_SETTINGS	playbacks[MAX_PHY_AUDIO_OUT];

	OUTPUT_SETTINGS		outputs[MAX_PHY_AUDIO_OUT];
} subsession;


typedef struct tag_session
{
	uint32			quads;							// size of this struct in quads
	uint32			crc;							// cyclic redundancy check

	uint32			version;
	uint32			flags;							// Matt sez "you always need flags", SPDIF_MODE
													// use EFC control flag bits
	int32			mirror_channel;
	int32			digital_mode;			
	int32			clock;					
	int32			rate;	
				
	uint32			monitorgains[ MAX_PHY_AUDIO_IN ][ MAX_PHY_AUDIO_OUT ];			
	uint32			playbackgains[ MAX_PHY_AUDIO_OUT ];			
	uint32			outputgains[ MAX_PHY_AUDIO_OUT ];			
	
	subsession s;

} SESSION;																																			 


/* 

Note that the CRC checksum does not include the first two struct members.

*/

#define SESSION_CRC_START_OFFSET_BYTES	(sizeof(uint32) * 2)
#define SESSION_CRC_START_OFFSET_QUADS	(SESSION_CRC_START_OFFSET_BYTES/sizeof(uint32))

#endif // _SESSION_H_

