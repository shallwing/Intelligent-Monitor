#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include <libgen.h>
#include <unistd.h>

#include "relay.h"
#include "crc-itu-t.h"
#include "tlv_intell_monitor.h"


void turn_led(int cmd)
{
	wiringPiSetup() ;
	pinMode(RELAY_PIN, OUTPUT) ;
	if(cmd == OFF)
	{
		digitalWrite ( RELAY_PIN, LOW);
	}
	else
		digitalWrite(RELAY_PIN, HIGH) ;
}

int packtlv_led_state(char *buf, int size, int cmd)
{
	unsigned short      crc16;
	int                 pack_len = TLV_FIXED_SIZE+1; /* Only 1 byte data */

	if(!buf || size<TLV_MIN_SIZE )
	{
		printf("Invalid input arguments\n");
		return 0;
	}

	/* Packet head */
	buf[0] = PACK_HEADER;

	/* Tag */
	buf[1] = TAG_LED;

	/* Length, this packet total 6 bytes */
	buf[2] = pack_len;

	/* Value */
	buf[3] = cmd ;

	/* Calc CRC16 checksum value from Packet Head(buf[0])~ Packet Value(buf[3]) */
	crc16 = crc_itu_t(MAGIC_CRC,(unsigned char*)buf, 4);

	/* Append the 2 Bytes CRC16 checksum value into the last two bytes in packet buffer */
	ushort_to_bytes((unsigned char*)&buf[4], crc16);

	return pack_len;
}
