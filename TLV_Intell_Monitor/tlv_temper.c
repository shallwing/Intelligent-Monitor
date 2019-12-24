#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include "crc-itu-t.h"

#include "tlv_intell_monitor.h"






int packtlv_temper(char *buf, int size, float temper)
{
	unsigned short      crc16;
	int                 pack_len = TLV_FIXED_SIZE+2; /* Only 2 byte data */
	short               high_bit,small_bit ;

	if(!buf || size<TLV_MIN_SIZE )
	{
		printf("Invalid input arguments\n");
		return 0;
	}

	/* Packet head */
	buf[0] = PACK_HEADER;

	/* Tag */
	buf[1] = TAG_TEMPER;

	/* Length, this packet total 7 bytes */
	buf[2] = pack_len;

	/* Value */
	high_bit = (int) temper;
	if(temper<0)
	{
		high_bit = high_bit|0x80 ;
	}
	small_bit = (temper - high_bit)*100 ;
	buf[3] = high_bit ;
	buf[4] = small_bit ;


	/* Calc CRC16 checksum value from Packet Head(buf[0])~ Packet Value(buf[4]) */
	crc16 = crc_itu_t(MAGIC_CRC,(unsigned char*) buf, 5);

	/* Append the 2 Bytes CRC16 checksum value into the last two bytes in packet buffer */
	ushort_to_bytes((unsigned char*)&buf[5], crc16);

	return pack_len;
}


int ds18b20_get_temper(float * temp) 
{
	int             count = 0 ;
	int             flag =0 ;
	int             fd ;
	DIR *           dirp ;
	struct dirent*  direntp ;
	char            Path_name[512]= "/sys/bus/w1/devices";

	char temp_buf[256] ;
	char * str ;

	dirp = opendir(Path_name) ;                     //open dir /sys/bus/w1/devices
	if(dirp == NULL)
	{
		printf("open dir %s failed: %s\n", Path_name, strerror(errno)) ;
		return -1 ;
	}
	if( chdir(Path_name) < 0)                       //change dir to /sys/bus/w1/devices
	{
		printf("change to dirctory %s failed: %s\n", Path_name, strerror(errno)) ;
		goto cleanup ;
	}
	while( (direntp=readdir(dirp)) != NULL)         //list the file from /sys/bus/w1/devices
	{
		if(strstr(direntp->d_name,"28") != NULL)                //find file name is 28-041731f7c0ff 
		{
			count = 1;
			break ;                 
		}
	}
	if(count == 0)          //Can't find dir_name include "28-" 
	{
		printf("Can't find dir_name include '28-' \n") ;
		return -2 ;
	}
	strcat(Path_name,"/") ;
	strncat(Path_name,direntp->d_name,sizeof(Path_name)) ;          //add Pathname become /sys/bus/w1/devices/28-041731f7c0ff

	if(chdir(Path_name) < 0)                               // change to  /sys/bus/w1/devices/28-041731f7c0ff
	{
		perror("change path failed!\n") ;
		flag = 1 ;

		goto cleanup ;
	}
	fd = open("w1_slave",O_RDONLY) ;                    // open w1_slave
	if(fd < 0)
	{
		perror("open file failed!\n");
		flag =1 ;
		goto cleanup ;
	}
	if( read(fd, temp_buf, sizeof(temp_buf)) < 0 )   //read w1_slave
	{

		flag =1 ;
		goto cleanup ;
	}

	str = strstr(temp_buf, "t=") ;
	*temp = atof(str+2) /1000 ;

cleanup:
	closedir(dirp) ;
	close(fd) ;

	if(flag)
		return -3 ;
	else              
		return 0 ;

}
