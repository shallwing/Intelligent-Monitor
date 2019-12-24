/*********************************************************************************
 **      Copyright:  (C) 2019 Wu Yujun<540726307@qq.com>
 **                  All rights reserved.
 **
 **       Filename:  tlv_server.c
 **    Description:  This file is tlv intell monitor server
 **                 
 **        Version:  1.0.0(2019年04月04日)
 **         Author:  Wu Yujun <540726307@qq.com>
 **      ChangeLog:  1, Release initial version on "2019年04月4日 14时04分40秒"
 **                 
 *********************************************************************************/


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>


#include "relay.h"
#include "crc-itu-t.h"
#include "tlv_intell_monitor.h"


int logon_reply(int cli_fd, char *buf, int size) ; // If 3 seconds have no replay will close client
int tlv_logon(int cli_fd ,char *buf,int size) ;//If Passwd error close client. 
int tlv_elora(int cli_fd ,char *buf,int size) ;


int     g_stop = 0 ;


void sig_handler(int sig_num)
{
	if(sig_num == SIGUSR1)
	{
		g_stop = 1 ;
	}
}


void print_usage(const char *program_name) 
{  
	printf("\n%s --1.0.0 (2019.4.4)\n", program_name);  
	printf(" Usage: %s -p <server_port>  [-h <server_use_help>]\n", program_name);  
	printf("        -p --port       the server listen port\n") ;
	printf("        -h --help       the server file how to use\n");  
	printf("        -d --daemon     the server progame running in backgruand\n");  

	return ;
}  



int main(int argc, char **argv)
{
	char                	*program_name ;
	char                	*ip = NULL ;
	int                 	port = -1 ;
	int                 	opt = -1 ;
	int                 	daemon_run = 0 ;
	int                 	listen_fd = -1 ;
	int                 	cli_fd = -1 ;
	int                 	nready = 0 ;
	int			rv = -1 ;
	int			ep_fd ;
	struct epoll_event 	ep_ev ;           
	struct epoll_event	ev_arr[MAX_EV] ; //have already even happened save in ev_arr
	int                 	i ;
	int			sock_fd ;
	char			buf[BUFSIZE] ;
	int 			bytes = 0;
	char			true_tlv[BUFSIZE] ;
	int			true_size ;
	char			logon_buf[BUFSIZE] ;
	int 			logon_bytes = 0 ;
	struct sigaction    	sa;



	program_name = basename(argv[0]) ;

	const char *short_opts = "i:p:hd";                    
	const struct option long_opts[] =   {  
		{"help", no_argument, NULL, 'h'},    
		{"ip", required_argument, NULL, 'i'},
		{"daemon", no_argument, NULL, 'd'},  
		{ "port", required_argument, NULL, 'p'},  
		{0, 0, 0, 0} 
	}; 
	while ((opt= getopt_long(argc, argv, short_opts, long_opts,NULL)) != -1) 
	{
		switch (opt) 
		{
			case 'i':
				ip = optarg ;
				break ;
			case 'p':
				port = atoi(optarg) ;
				break ;
			case 'd':
				daemon_run = 1 ;
				break ;
			case 'h':
				print_usage(program_name) ;
				return 0 ;
		}
	}

	if( port < 0 )
	{
		print_usage(program_name) ;
		return -1 ;
	}
	/*  program running in backgrund    */
	if(daemon_run == 1)                      
	{
		daemon(0,0) ;
	}

	/*	program run till get sinal "SIGUSR1"	*/
	signal(SIGUSR1, sig_handler);
	/*   if client close,still write to client will  have sigpipe and kill process  */
	sa.sa_handler = SIG_IGN;
	sigaction( SIGPIPE, &sa, 0 );

	listen_fd = create_socket(ip, port) ;
	if(listen_fd < 0)
	{
		printf("CreatSocket failed\n");
		return -1 ;
	}


	ep_fd = epoll_create(1024) ;
	if(ep_fd < 0)
	{
		printf("epoll_create() failed: %s\n", strerror(errno)) ;
		close(listen_fd) ;
		return -2 ;
	}
	printf("epoll_create sucessful, epfd[%d]\n", ep_fd) ;
	memset(&ep_ev, 0, sizeof(ep_ev)) ;
	ep_ev.data.fd = listen_fd ;
	ep_ev.events = EPOLLIN ;
	if( epoll_ctl(ep_fd, EPOLL_CTL_ADD, listen_fd, &ep_ev ) < 0)
	{
		printf("epoll_ctl() add listen_fd failed:%s\n", strerror(errno)) ;
		return -3 ;
	}
	memset(buf,0,sizeof(buf)) ;

	while(!g_stop)
	{
		/*       timeout set -1 causes epoll_wait() to block indefinitely        */
		nready = epoll_wait(ep_fd, ev_arr, MAX_EV, -1); 
		if(nready < 0)
		{
			printf("epoll_wait failed: %s\n", strerror(errno)) ;
			continue ;
		}
		printf("the %d of file descriptors ready for the requested I/O\n", nready) ;
		for(i=0 ; i<nready; i++ )
		{
			if(ev_arr[i].events & EPOLLERR || ev_arr[i].events & EPOLLRDHUP )
			{
				printf("fd[%d] found error event: %s\n",ev_arr[i].data.fd ,strerror(errno)) ;
				epoll_ctl( ep_fd, EPOLL_CTL_DEL ,ev_arr[i].data.fd, NULL ) ;
				close(ev_arr[i].data.fd) ;
			}
			else if(ev_arr[i].data.fd == listen_fd) //have client want to connect 
			{
				true_size = 0 ;
				memset(true_tlv, 0 ,sizeof(true_tlv)) ;
				printf("Have client want to connect!\n") ;
				cli_fd = accept_client(listen_fd) ;
				if( cli_fd < 0)
				{
					printf("Accept error\n") ;
					continue ;
				}
				/*	If 3 seconds have on reply, will close client	*/
				if((rv =  logon_reply(cli_fd, &logon_buf[logon_bytes], BUFSIZE-logon_bytes)) < 0) 
				{
					printf("Logon reply error,and close client\n") ;
					close(cli_fd) ;
					continue ;
				}
				logon_bytes = rv + logon_bytes ;
				logon_bytes = unpack_tlv(logon_buf,logon_bytes, true_tlv, &true_size) ;
				/*	If passwd is true EPOLL_CTL_ADD, else close client	*/
				if( tlv_logon(cli_fd,true_tlv,true_size) == LOGON_OK)
				{
					ep_ev.data.fd = cli_fd ;
					ep_ev.events= EPOLLIN ;
					if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, cli_fd, &ep_ev) < 0)
					{
						printf("epoll_ctl add client[%d] failure: %s, will close client\n", 
								cli_fd, strerror(errno)) ;
						close(cli_fd) ;
						continue ;
					}
					printf("\nLOGON OK!!!!!\n") ;
				}
				/*	passwd is error		*/
				else{
					printf("\nLogon failed\n") ;
					close(cli_fd) ;
					continue ;
				}
			}
			else //Already connected client have events come
			{
				printf("Already connect client have event come!\n") ;
				sock_fd = ev_arr[i].data.fd ;
				true_size = 0 ;
				memset(true_tlv, 0 ,sizeof(true_tlv)) ;
				rv = read(sock_fd, &buf[bytes], BUFSIZE - bytes);
				if(rv <= 0)
				{
					printf("Read() error:%s\n", strerror(errno)) ;
					epoll_ctl(ep_fd, EPOLL_CTL_DEL, sock_fd, NULL) ;
					close(sock_fd) ;
					continue ;
				}
				bytes = rv + bytes ;
				bytes = unpack_tlv(buf,bytes, true_tlv, &true_size) ;
				if(true_size == 0)
				{
					printf("\nERROR CMD\n") ;
					continue ;
				}
				print_tlv("TRUE_TLV",true_tlv, true_size) ;

				if(tlv_elora(sock_fd, true_tlv,true_size) < 0)
				{
					printf("tlv_elora() failed\n") ;
					epoll_ctl(ep_fd, EPOLL_CTL_DEL, sock_fd, NULL) ;
					close(sock_fd) ;
					continue ;
				}
			}

		} //for(i=0 ; i<nready; i++ ), Traversal all happend events.
	}//while(!g_stop),Program run till get signal SIGUSR1.

	printf("Program is exit...\n") ;
	close(listen_fd) ;
	return 0 ;
}

int logon_reply(int cli_fd, char *buf, int size)
{
	fd_set                  rset ;
	struct timeval 		timeout; 
	int                     rv;

	if(!buf)
	{
		printf("Invaild input\n") ;
		return -1 ;
	}
	timeout.tv_sec = 3;
	timeout.tv_usec = 0; 
	FD_ZERO(&rset) ;
	FD_SET(cli_fd, &rset) ;
	rv = select(cli_fd+1, &rset, NULL, NULL, &timeout) ;
	if(rv < 0) //select error.
	{
		printf("select() failed: %s\n", strerror(errno)) ;
		return -2 ;
	}
	if(rv== 0) //select time out.
	{
		printf("Time out,client no send passwd\n") ;
		return -3 ;
	}
	rv = read(cli_fd, buf, size) ;
	if(rv <= 0)//read from client failed 
	{
		printf("Read from buf error or disconnect:%s\n", strerror(errno)) ;
		return -4 ;
	}
	return rv ;
}


int tlv_logon(int cli_fd ,char *buf,int size)
{
	int 	i ;
	char 	passwd[32] ;
	int 	tlv_len ;

	if(!buf||size < TLV_MIN_SIZE)
	{
		printf("Invailed input!\n") ;
		return LOGON_ERROR ;
	}

	memset(passwd, 0, sizeof(passwd)) ;
	for(i=0;i<size;i++)
	{
		if((unsigned char)buf[i] == PACK_HEADER)
		{	
			if(buf[i+1] == TAG_LOGON )
			{
				tlv_len = buf[i+2] ;
				snprintf(passwd,tlv_len-4,"%s",&buf[i+3]) ;
				if(!strcmp(passwd,"iot@yun"))	
				{
					return LOGON_OK ;
				}
				else
				{
					return LOGON_ERROR ;
				}
			}
		}
	}
	return LOGON_ERROR ;
}



int tlv_elora(int cli_fd ,char *buf,int size)
{
	int		send_len ;
	char		sbuf[128] ;
	float 		temper;
	int 		i ;

	if(!buf||size<TLV_MIN_SIZE)
	{
		printf("\nUnkown cmd\n") ;
		return 0 ;
	}
	memset(sbuf, 0, sizeof(sbuf)) ;
	for(i=0;i<size;i++) 
	{
		if((unsigned char)buf[i]==PACK_HEADER)
		{
			switch((unsigned char)buf[i+1])
			{
				case TAG_CAMERA:
					printf("\nTurn CAMERA:");
					if((unsigned char)buf[i+3]==ON)
					{
						printf("ON\n") ;
						turn_camera(ON) ;
						send_len = packtlv_camera(sbuf,sizeof(sbuf),ON) ;
					}
					else if((unsigned char)buf[i+3] == OFF)
					{
						printf("OFF\n") ;
						turn_camera(OFF) ;
						send_len = packtlv_camera(sbuf, sizeof(sbuf), OFF) ;
					}
					if( write(cli_fd, sbuf, send_len) < 0)
					{
						printf("write camera state to client failed\n") ;
						return -1 ;
					}
					break ;
				case TAG_TEMPER:
					printf("\nGet TEMPER\n") ;
					if( ds18b20_get_temper(&temper) < 0)
					{
						printf("ds18b20_get_temper() failed\n") ;
						return -2 ;	
					}
					printf("Temperature: %fC\n",temper) ;
					send_len = packtlv_temper(sbuf, sizeof(sbuf), temper) ;
					if(send_len <= 0)
					{
						printf("packtlv_temper() failed\n") ;
						return -3 ;
					}
					if( write(cli_fd, sbuf, send_len) < 0)
					{
						printf("write temper to client failed\n") ;
						return -4 ;
					}
					break ;

				case TAG_LED:
					printf("\nTurn LED:");
					if((unsigned char)buf[i+3]==ON)
					{
						printf("ON\n") ;
						turn_led(ON) ;
						send_len = packtlv_led_state(sbuf, sizeof(sbuf), ON) ;
						if(send_len <= 0)
						{
							printf("packtlv_led_state() failed\n") ;
							return -5 ;
						}
					}
					else if((unsigned char)buf[i+3]==OFF)
					{
						printf("OFF\n") ;
						turn_led(OFF) ;
						send_len = packtlv_led_state(sbuf, sizeof(sbuf), OFF) ;
						if(send_len <= 0)
						{
							printf("packtlv_led_state() failed\n") ;
							return -6 ;
						}

					}
					if( write(cli_fd, sbuf, send_len) < 0)
					{
						printf("write led state to client failed\n") ;
						return -7 ;
					}
					break ;

				default:
					printf("Unknow tlv\n") ;
					break ;
			}
		}
	}
	return 0 ;
}

