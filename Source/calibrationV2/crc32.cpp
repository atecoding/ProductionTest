#include "../base.h"
#include "crc32.h"

#if 0
/*
*
* Copied from the XMOS xs1.h header file - mimics the crc32 XS1 instruction
*
* Incorporate a word into a Cyclic Redundancy Checksum.The calculation performed
* is
* \code
* for (int i = 0; i < 32; i++) {
*int xorBit = (crc & 1);
*
*   checksum = (checksum >> 1) | ((data & 1) << 31);
*data = data >> 1;
*
*   if (xorBit)
*     checksum = checksum ^ poly;
*
}
*\endcode
* \param[in, out] checksum The initial value of the checksum, which is
*                         updated with the new checksum.
* \param data The data to compute the CRC over.
* \param poly The polynomial to use when computing the CRC.
* \sa chkct
* \sa testct

*/
#endif

unsigned int CRC32Block(unsigned int const *message, size_t count, unsigned int poly)
{
	unsigned int checksum = 0xffffffff;
	while (count != 0)
	{
		unsigned int data = *message;

		for (int i = 0; i < 32; i++) 
		{
			int xorBit = (checksum & 1);
			
			checksum = (checksum >> 1) | ((data & 1) << 31);
			data = data >> 1;
			
			 if (xorBit)
				 checksum = checksum ^ poly;
		}

		count--;
		message++;
	}

	return checksum;
}
