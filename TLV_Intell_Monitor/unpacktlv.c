#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "crc-itu-t.h"
#include "tlv_intell_monitor.h"

int  unpack_tlv(char *buf, int bytes, char *true_tlv,int *true_size)
{
	int                         i ;
	char                        *tlv_ptr = NULL;
	int                         tlv_len = 0 ;
	unsigned short              crc16,val ;


	if(!buf||!true_tlv)
	{
		printf("Invailed input!\n") ;
		return 0 ;
	}

again:
	if(bytes < TLV_MIN_SIZE)
	{
		//printf("tlv packet too short\n") ;
		return bytes ;
	}

	for(i=0; i< bytes; i++)
	{
		if((unsigned char)buf[i]==PACK_HEADER)
		{
			if(bytes -i <= 2)
			{
				printf("tlv_packet is too short so incomplete\n") ;
				memmove(buf, &buf[i], bytes - i) ;
				return bytes -i ;
			}
			tlv_ptr = &buf[i] ;
			tlv_len = tlv_ptr[2] ;

			if(tlv_len < TLV_MIN_SIZE||tlv_len>TLV_MAX_SIZE )
			{
				memmove(buf, &tlv_ptr[2], bytes-i-2) ;
				bytes = bytes -i -2 ;
				goto again;
			}
			if(tlv_len > bytes - i)
			{
				memmove(buf, tlv_ptr, bytes-i) ;
				printf("tlv_packet is incomplete\n") ;
				return bytes - i ;
			}
			crc16 = crc_itu_t(MAGIC_CRC,(unsigned char*)tlv_ptr, tlv_len-2);
			val = bytes_to_ushort((unsigned char*)&tlv_ptr[tlv_len-2],2) ;
			//printf("crc: %d, val: %d\n",crc16, val) ;
			if(val != crc16)
			{
				printf("CRC check error\n") ;
				memmove(buf, &tlv_ptr[2], bytes-i-2) ;
				bytes = bytes -i -2 ;
				goto again ;
			}
			//printf("CRC check true\n") ;
			memmove(&true_tlv[*true_size], tlv_ptr, tlv_len) ;
			*true_size += tlv_len ; 
			memmove(buf,&tlv_ptr[tlv_len], bytes-i-tlv_len) ;
			bytes = bytes -i -tlv_len ;
			goto again ;
		}//if((unsigned char)buf[i] == PACK_HEADER)
	}//for(i=0;i<bytes;i++)
	return 0 ;
}



void print_tlv(char* type,char *buf, int len) 
{
	int i ;

	if(!buf)
	{
		printf("Invaild input\n") ;
		return ;
	}
	if(type)
	{
		printf("\n%s:\n",type) ;
	}
	for(i=0;i<len;i++)
	{
		if((unsigned char)buf[i]==PACK_HEADER)
		{
			printf("\n") ;
		}
		printf("0x%02x  ",(unsigned char)buf[i]) ;
	}
	printf("\n") ;
}
