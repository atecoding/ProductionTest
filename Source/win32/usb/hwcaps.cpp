#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <new>
#endif

#include "../../base.h"
#include "ehw.h"
#include "hwcaps.h"

void hwcaps::init(unsigned product_id)
{
	productId = product_id;
}

//void hwcaps::dump(Logger *log)
//{
//}

int32 hwcaps::numbusin()
{
	switch (productId)
	{
	case ACOUSTICIO_PRODUCT_ID:
		return 8;

    case ACOUSTICIO_M1_PRODUCT_ID:
        return 14;
            
    case ACOUSTICIO_M2_PRODUCT_ID:
        return 20;

	case ANALYZERBR:
		return 4;
	
	case ECHO2:
		return 2;

	case ECHO4:
		return 6;
	}

	jassertfalse;
	return 2;
}

int32 hwcaps::numbusout()
{
	switch (productId)
	{
	case ACOUSTICIO_PRODUCT_ID:
		return 4;

    case ACOUSTICIO_M1_PRODUCT_ID:
        return 12;
        
    case ACOUSTICIO_M2_PRODUCT_ID:
        return 20;

	case ANALYZERBR:
		return 4;

	case ECHO2:
		return 4;

	case ECHO4:
		return 6;
	}

	jassertfalse;
	return 2;
}

int32 hwcaps::numplaychan(int /*samplerate*/)
{
	switch (productId)
	{
	case ACOUSTICIO_PRODUCT_ID:
		return 4;

    case ACOUSTICIO_M1_PRODUCT_ID:
        return 12;
       
    case ACOUSTICIO_M2_PRODUCT_ID:
        return 20;
	
	case ANALYZERBR:
		return 4;

	case ECHO2:
		return 4;

	case ECHO4:
		return 6;
	}

	return 2;
}

int32 hwcaps::numrecchan(int /*samplerate*/)
{
	switch (productId)
	{
	case ACOUSTICIO_PRODUCT_ID:
		return 8;

    case ACOUSTICIO_M1_PRODUCT_ID:
        return 14;

    case ACOUSTICIO_M2_PRODUCT_ID:
        return 20;
            
	case ANALYZERBR:
		return 4;

	case ECHO2:
		return 2;

	case ECHO4:
		return 6;
	}

	return 2;
}

char const *hwcaps::BoxTypeName()
{
	switch (productId)
	{
	case ECHO2:
		return "Echo2";

	case ECHO4:
		return "Echo4";

	case ACOUSTICIO_PRODUCT_ID:
		return "Acoustic AIO";

    case ACOUSTICIO_M1_PRODUCT_ID:
        return "AIO-M1";
            
    case ACOUSTICIO_M2_PRODUCT_ID:
         return "AIO-M2";
	
	case ANALYZERBR:
		return "Analyzer BR";
	}

	return "Echo USB";
}

int hwcaps::NumMixInGroups()
{
	switch (productId)
	{
	case ACOUSTICIO_PRODUCT_ID:
		return 1;

    case ACOUSTICIO_M1_PRODUCT_ID:
        return 2;
            
    case ACOUSTICIO_M2_PRODUCT_ID:
        return 1;

	case ANALYZERBR:
		return 2;

	case ECHO2:
		return 2;

	case ECHO4:
		return 2;
	}

	return 2;
}

int hwcaps::MixInGroupSize(int /*group*/)
{
	switch (productId)
	{
	case ACOUSTICIO_PRODUCT_ID:
		return 8;

    case ACOUSTICIO_M1_PRODUCT_ID:
        {
        case 0:
            return 4;
            
        case 1:
            return 10;
        }
    
    case ACOUSTICIO_M2_PRODUCT_ID:
        return 20;

	case ANALYZERBR:
		return 2;

	case ECHO2:
		return 2;

	case ECHO4:
		return 2;
	}

	jassertfalse;
	return 2;
}

char const *hwcaps::MixInGroupName(int /*group*/)
{
	//jassert( ((uint32)group) < _caps.num_in_groups);
	//
	//switch (_caps.in_groups[group].type)
	//{
	//	case analog :
	//		return "Analog";
	//
	//	case spdif :
	//		return "S/PDIF";
	//
	//	case adat :
	//		return "ADAT";

	//	case digital :
	//		return "Digital";

	//	case guitar :
	//		return "Guitar";

	//	case piezo_guitar :
	//		return "Piezo";

	//	case guitar_string :
	//		return "String";
	//
	//	default :
	//		return "Input";
	//}
	//switch (group)
	//{
	//case 0:
	//	return "Analog";
	//case 1:
	//	return "S/PDIF";
	//}
	return "Input";

}

char const *hwcaps::MixInGroupShortName(int /*group*/)
{
	return "I";
}

int hwcaps::MixInGroup(int /*chan*/)
{
	return 0;
}

int hwcaps::MixInGroupOffset(int chan)
{
	return chan;
}

int hwcaps::MixInType(int /*chan*/)
{
	return analog;
}

int hwcaps::NumMixOutGroups()
{
	switch (productId)
	{
	case ACOUSTICIO_PRODUCT_ID:
		return 1;

    case ACOUSTICIO_M1_PRODUCT_ID:
        return 2;
    
    case ACOUSTICIO_M2_PRODUCT_ID:
        return 1;

	case ANALYZERBR:
		return 2;

	case ECHO2:
		return 2;

	case ECHO4:
		return 2;
	}

	return 1;
}

int hwcaps::MixOutGroupSize(int /*group*/)
{
	return 2;
}

char const *hwcaps::MixOutGroupName(int /*group*/)
{
	return "Output";
}

char const *hwcaps::MixOutGroupShortName(int /*group*/)
{
	return "Out";
}

int hwcaps::MixOutGroup(int chan)
{
	switch (productId)
	{
	case ACOUSTICIO_PRODUCT_ID:
		return chan >> 1;

    case ACOUSTICIO_M1_PRODUCT_ID:
        {
            if (chan < 2)
                return 0;
            
            return 1;
        }
    
    case ACOUSTICIO_M2_PRODUCT_ID:
        return 0;

	case ANALYZERBR:
		return chan >> 1;

	case ECHO2:
		return chan >> 1;

	case ECHO4:
		return chan >> 1;
	}
	return 0;
}

int hwcaps::MixOutGroupOffset(int chan)
{
	switch (productId)
	{
	case ACOUSTICIO_PRODUCT_ID:
		return chan % 8;

    case ACOUSTICIO_M1_PRODUCT_ID:
        {
            if (chan < 2)
                return chan;
            
            return chan - 2;
        }
    
    case ACOUSTICIO_M2_PRODUCT_ID:
        return chan;

	case ANALYZERBR:
		return chan % 2;
	
	case ECHO2:
		return chan % 2;

	case ECHO4:
		return chan % 6;
	}
	return chan;
}

int hwcaps::MixOutType(int /*chan*/)
{
	return analog;
}

int hwcaps::MixPlayType(int chan)
{
	return hwcaps::MixOutType(chan);
}

int hwcaps::MinSampleRate()
{
	switch (productId)
	{
	case ACOUSTICIO_PRODUCT_ID:
    case ACOUSTICIO_M1_PRODUCT_ID:
    case ACOUSTICIO_M2_PRODUCT_ID:
		return 48000;

	case ANALYZERBR:
		return 44100;

	case ECHO2:
		return 32000;

	case ECHO4:
		return 44100;
	}
	return 48000;
}

int hwcaps::MaxSampleRate()
{
	switch (productId)
	{
	case ACOUSTICIO_PRODUCT_ID:
    case ACOUSTICIO_M1_PRODUCT_ID:
		return 192000;
            
    case ACOUSTICIO_M2_PRODUCT_ID:
        return 48000;

	case ECHO2:
		return 192000;

	case ECHO4:
		return 192000;
	}

	return 48000;
}

bool hwcaps::HasAnalogMirror()
{
	return false;
}

bool hwcaps::HasDigitalModeSwitch()
{
	return false;
}

bool hwcaps::HasSpdifOut()
{
	return true;
}

bool hwcaps::HasSpdifIn()
{
	return true;
}

bool hwcaps::ExternalSync()
{
	return false;
}

bool hwcaps::ClockSupported(int clock)
{
	return internal_clock == clock;
}

void hwcaps::GetClockName(int clock,String &name)
{
	static char const *clocknames[ num_clock_sources ] =
	{
		"Internal",
		"FireWire",
		"Word",
		"S/PDIF",
		"ADAT",
		"ADAT B",
		"Variable"
	};

	name = clocknames[clock];
}

bool hwcaps::HasDsp()
{
	return false;
}

uint32 hwcaps::DspVersion()
{
	return 0;
}

uint32 hwcaps::ArmVersion()
{
	return 0;
}

bool hwcaps::HasFpga()
{
	return false;
}

uint32 hwcaps::FpgaVersion()
{
	return 0;
}

bool hwcaps::HasPhantomPower()
{
	return false;
}

bool hwcaps::HasSoftClip()
{
	return false;
}

bool hwcaps::HasIsocAudioMapping()
{
	return false;
}

bool hwcaps::HasOutputBussing()
{
	return false;
}

void PrintFirmwareVersion(String &str,uint32 ver)
{
	uint32 major,minor,rev;

	major = ((ver >> 8) & 0xf) | (10 * ((ver >> 12) & 0xf));
	minor = (ver >> 4) & 0xf;
	rev = ver & 0xf;

	str = String::formatted("%d.%d",major,minor);

	if (0 != rev)
	{
		str += ".";
		str += String( (int) rev);
	}
}

int32 hwcaps::NumMidiIn()
{
	return 0;
}

int32 hwcaps::NumMidiOut()
{
	return 0;
}

bool hwcaps::HasOutputNominal()
{
	return false;
}

bool hwcaps::HasInputNominal(int /*inbus*/)
{
	return false;
}

bool hwcaps::SupportsRobotGuitar()
{
	return false;
}

bool hwcaps::SupportsGuitarCharging()
{
	return false;
}