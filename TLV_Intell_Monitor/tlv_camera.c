#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include "tlv_intell_monitor.h"
#include "crc-itu-t.h"



int packtlv_camera(char *buf, int size, int cmd)
{
	unsigned short      crc16;
	int                 pack_len = TLV_FIXED_SIZE+1; /* Only 1 byte data */

	if(!buf || size<TLV_MIN_SIZE )
	{
		printf("Invalid input arguments:%s\n",__FUNCTION__);
		return 0;
	}

	/* Packet head */
	buf[0] = PACK_HEADER;

	/* Tag */
	buf[1] = TAG_CAMERA;

	/* Length, this packet total 6 bytes */
	buf[2] = pack_len;

	/* Value */
	buf[3] = (OFF==cmd) ? 0x00 : 0x01;

	/* Calc CRC16 checksum value from Packet Head(buf[0])~ Packet Value(buf[3]) */
	crc16 = crc_itu_t(MAGIC_CRC, (unsigned char*)buf, 4);

	/* Append the 2 Bytes CRC16 checksum value into the last two bytes in packet buffer */
	ushort_to_bytes((unsigned char*)&buf[4], crc16);

	return pack_len;
}



void turn_camera(int cmd)
{
	switch(cmd)
	{
		case ON:
			printf("Trun on camera\n") ;
			system("sudo mjpg_streamer -i \"input_uvc.so -d /dev/video0 -r 1920x1080\" -o \"output_http.so -p 8090 -w /usr/local/share/mjpg-streamer/www/\" -b") ;
			break ;
	
		case OFF:
			printf("Trun off camera\n") ;
			system("sudo killall -SIGINT mjpg_streamer") ;
			break ;
	
	
	}


}
