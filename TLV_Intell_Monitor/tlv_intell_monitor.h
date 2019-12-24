
#ifndef TLV_PACK_H
#define TLV_PACK_H

#define PACK_HEADER        0xFD
/*  TLV packet fixed segement size, 1B Head, 1B Tag, 1B length, 2B CRC16, total 5B. */
#define TLV_FIXED_SIZE     5

/*  TLV packet Minimum size is fixed bytes + 1 byte data */
#define TLV_MIN_SIZE       (TLV_FIXED_SIZE+1)

#define TLV_MAX_SIZE       32

#endif /*	TLV_PACK_H	*/

/*  Tag definition */
enum
{
	TAG_LOGON=1,
	TAG_CAMERA,
	TAG_LED,
	TAG_TEMPER,
};


#define		ERROR		-1
#define		OK		0

#define		BACK_LOG	13
#define 	MAX_EV      	256

#define		BUFSIZE		256

#define 	LOGON_OK	1
#define		LOGON_ERROR	-1

#define 	ON		1
#define		OFF		0





int  unpack_tlv(char *buf, int bytes, char *true_tlv,int *true_size) ;
void print_tlv(char* type,char *buf, int len) ;
int ds18b20_get_temper(float * temp) ;
int packtlv_temper(char *buf, int size, float temper) ;
int create_socket(char *lis_addr, int port) ;
int accept_client(int listen_fd); //accept new client
int tlv_elora(int cli_fd ,char *buf,int size) ;
int logon_reply(int cli_fd, char *buf, int size) ;
int packtlv_led_state(char *buf, int size, int cmd) ;
void turn_camera(int cmd) ;
int packtlv_camera(char *buf, int size, int cmd) ;
