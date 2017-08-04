/*
    mpg321 - a fully free clone of mpg123.
    Copyright (C) 2001 Joe Drew
    
    Network code based heavily upon:
    plaympeg - Sample MPEG player using the SMPEG library
    Copyright (C) 1999 Loki Entertainment Software
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>

#include <limits.h>

#include "mpg321.h"

#define MAX_HOSTNAME 256

#define IFVERB if(options.opt & MPG321_VERBOSE_PLAY)

int proxy_enable = 0;
char *proxy_server;
int auth_enable = 0;
int auth_enable_var = 0;
extern char authstring[80];
int http_file_length = 0;

int shoutcast(int);

int is_address_multicast(unsigned long address)
{
    if ((address & 255) >= 224 && (address & 255) <= 239)
        return (1);
    return (0);
}

#ifndef USE_IPV6

/* =================== IPV4 ================== */

int tcp_open(char *address, int port)
{
    struct sockaddr_in stAddr;
    struct hostent *host;
    int sock;
    struct linger l;

    memset(&stAddr, 0, sizeof(stAddr));
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(port);

    if ((host = gethostbyname(address)) == NULL)
        return (0);

    stAddr.sin_addr = *((struct in_addr *)host->h_addr_list[0]);

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        return (0);

    l.l_onoff = 1;
    l.l_linger = 5;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&l, sizeof(l)) < 0)
        return (0);

    if (connect(sock, (struct sockaddr *)&stAddr, sizeof(stAddr)) < 0)
        return (0);

    return (sock);
}

int udp_open(char *address, int port)
{
    int enable = 1L;
    struct sockaddr_in stAddr;
    struct sockaddr_in stLclAddr;
    struct ip_mreq stMreq;
    struct hostent *host;
    int sock;

    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons(port);

    if ((host = gethostbyname(address)) == NULL)
        return (0);

    stAddr.sin_addr = *((struct in_addr *)host->h_addr_list[0]);

    /* Create a UDP socket */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return (0);

    /* Allow multiple instance of the client to share the same address and port */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&enable, sizeof(unsigned long int)) < 0)
        return (0);

    /* If the address is multicast, register to the multicast group */
    if (is_address_multicast(stAddr.sin_addr.s_addr))
    {
        /* Bind the socket to port */
        stLclAddr.sin_family = AF_INET;
        stLclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        stLclAddr.sin_port = stAddr.sin_port;
        if (bind(sock, (struct sockaddr *)&stLclAddr, sizeof(stLclAddr)) < 0)
            return (0);

        /* Register to a multicast address */
        stMreq.imr_multiaddr.s_addr = stAddr.sin_addr.s_addr;
        stMreq.imr_interface.s_addr = INADDR_ANY;
        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq)) < 0)
            return (0);
    }
    else
    {
        /* Bind the socket to port */
        stLclAddr.sin_family = AF_INET;
        stLclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        stLclAddr.sin_port = htons(0);
        if (bind(sock, (struct sockaddr *)&stLclAddr, sizeof(stLclAddr)) < 0)
            return (0);
    }

    return (sock);
}

#else

/* =================== IPV6 ================== */

int tcp_open(char *address, int port)
{
    struct addrinfo hints, *res, *res_tmp;
    int sock;
    struct linger l;
    char ipstring[MAX_HOSTNAME];
    char portstring[MAX_HOSTNAME];
    
    snprintf(portstring, sizeof(portstring), "%d", port);

    memset(&hints, 0, sizeof(hints));
    /*
     * hints.ai_protocol  = 0;
     * hints.ai_addrlen   = 0;
     * hints.ai_canonname = NULL;
     * hints.ai_addr      = NULL;
     * hints.ai_next      = NULL;
     */
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;



    if ( getaddrinfo(address, portstring, &hints, &res) != 0)
	    return(0);
    
    for (res_tmp = res; res_tmp != NULL; res_tmp = res_tmp->ai_next) {	     
	    if ( (res_tmp->ai_family != AF_INET) &&
			    (res_tmp->ai_family != AF_INET6) )
		    continue;
	    if ( (sock = socket(res_tmp->ai_family, res_tmp->ai_socktype,
					    res_tmp->ai_protocol)) == -1) {
		sock = 0;
		continue;
	    }
	    
	    l.l_onoff = 1;
	    l.l_linger = 5;
	    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&l, sizeof(l)) < 0) {
		    /* return (0); */
		    sock = 0;
		    continue;
	    }
	    
	    if ( connect(sock, res_tmp->ai_addr, res_tmp->ai_addrlen)
			    == -1 )
	    {
		    close(sock);
		    sock = 0;
		    continue;
	    }

	    ipstring[0] = 0;
	    getnameinfo(res_tmp->ai_addr, res_tmp->ai_addrlen, ipstring,
			    sizeof(ipstring), NULL, 0, NI_NUMERICHOST);

	    if (ipstring == NULL)
		    sock = 0;
	    
	    break;
    }
    
    freeaddrinfo(res);
    return sock;
}

int udp_open(char *address, int port)
{
  int enable = 1L;
  struct addrinfo hints, *res, *res_tmp;
  int sock;
  struct linger l;
  char ipstring[MAX_HOSTNAME];
  char portstring[MAX_HOSTNAME];
  struct ipv6_mreq imr6;
  struct ip_mreq imr;

  snprintf(portstring, sizeof(portstring), "%d", port);

  memset(&hints, 0, sizeof(hints));
  /*
   * hints.ai_protocol  = 0;
   * hints.ai_addrlen   = 0;
   * hints.ai_canonname = NULL;
   * hints.ai_addr      = NULL;
   * hints.ai_next      = NULL;
   */
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;

  if ( getaddrinfo(address, portstring, &hints, &res) != 0)
    return(0);
    
  for (res_tmp = res; res_tmp != NULL; res_tmp = res_tmp->ai_next) {	     
    if ( (res_tmp->ai_family != AF_INET) &&
	   (res_tmp->ai_family != AF_INET6) )
	continue;
    if ( (sock = socket(res_tmp->ai_family, res_tmp->ai_socktype,
				res_tmp->ai_protocol)) < 0) {
	sock = 0;
	continue;
    }
    /* Allow multiple instance of the client to share the same address and port */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			 (char *)&enable, sizeof(unsigned long int)) < 0) {
	sock = 0;
	continue;
    }

    /* If the address is multicast, register to the multicast group */
    if ( (res_tmp->ai_family == AF_INET6) && 
	   IN6_IS_ADDR_MULTICAST(&(((struct sockaddr_in6 *)res_tmp->ai_addr)->
					   sin6_addr) ) ) {
	  /* Bind the socket to port */
	  if ( bind(sock, res_tmp->ai_addr, res_tmp->ai_addrlen) < 0) {
	    close(sock);
	    sock = 0;
	    continue;
	  }
  
	  imr6.ipv6mr_multiaddr =
	    ((struct sockaddr_in6 *)res_tmp->ai_addr)->
	    sin6_addr;
	  imr6.ipv6mr_interface = INADDR_ANY;
	  if ( setsockopt(
				sock,
				IPPROTO_IPV6,
				IPV6_ADD_MEMBERSHIP,
				(char *) &imr6,
				sizeof(struct ipv6_mreq))
		 < 0)
	    return(0);
    }
    else if ( (res_tmp->ai_family == AF_INET) &&
		  IN_MULTICAST(ntohl((((struct sockaddr_in *)res_tmp->
					     ai_addr)->sin_addr.s_addr)) ) ){
	/* Bind the socket to port */
	if ( bind(sock, res_tmp->ai_addr, res_tmp->ai_addrlen) < 0) {
	  close(sock);
	  sock = 0;
	  continue;
	}
	
	imr.imr_multiaddr =
	  ((struct sockaddr_in *)res_tmp->ai_addr)->
	  sin_addr;
	imr.imr_interface.s_addr = INADDR_ANY;
	
	if ( setsockopt(sock,
			    IPPROTO_IP,
			    IP_ADD_MEMBERSHIP,
			    (char *) &imr,
			    sizeof(struct ip_mreq))
	     < 0)
	  return(0);
    } 
    else {
	/* Bind the socket to port */
	if ( bind(sock, res_tmp->ai_addr, res_tmp->ai_addrlen) < 0) {
	  close(sock);
	  sock = 0;
	  continue;
	}
    }
  }

    freeaddrinfo(res);
    return (sock);
}

#endif

int raw_open(char *arg)
{
    char *host;
    int port;
    int sock;

    /* Check for URL syntax */
    if (strncmp(arg, "raw://", strlen("raw://")))
        return (0);

    /* Parse URL */
    port = 0;
    host = arg + strlen("raw://");
    if (strchr(host, ':') != NULL)  /* port is specified */
    {
        port = atoi(strchr(host, ':') + 1);
        *strchr(host, ':') = 0;
    }

    /* Open a UDP socket */
    if (!(sock = udp_open(host, port)))
        perror("raw_open");

    return (sock);
}

/**
 * Read a http line header up to and excluding '\r\n'.
 * This function read character by character.
 * @param tcp_sock the socket use to read the stream
 * @param buf a buffer to receive the data
 * @param size size of the buffer
 * @return the size of the stream read or -1 if an error occured
 */
static int http_read_header(int tcp_sock, char *buf, int size)
{
  int offset = -1;
  
  do
    {
      offset++;
      if (read(tcp_sock, buf + offset, 1) < 0)
	return -1;
    }
  while (offset<size-1 && !(offset>0 && buf[offset-1]=='\r' && buf[offset]=='\n'));
  
  buf[offset-1] = 0;
  buf[offset] = 0;
  return offset-1;
}

/**
 * Read a line from the tcp receive buffer up to and excluding '\n'
 * This function read character by character.
 * @param tcp_sock the socket use to read the stream
 * @param buf a buffer to receive the data
 * @param size size of the buffer
 * @return the size of the stream read or -1 if an error occured
 */
static int http_read_content_line(int tcp_sock, char *buf, int size)
{
  int offset = -1;
  
  do
    {
      offset++;
      if (read(tcp_sock, buf + offset, 1) < 0)
	return -1;
    }
  while (offset<size-1 && !(offset>=0 && buf[offset]=='\n'));
  
  buf[offset] = 0;
  return offset;
}

int http_open(char *arg)
{
  char *host;
  int port;
  char *request;
  int tcp_sock;
  char http_request[PATH_MAX];
  char http_response[PATH_MAX];
  char path[PATH_MAX];
  int len;
  int ofs;
 
  char phost[16];
  int pport;
    
  /* Check for URL syntax */
  if (strncmp(arg, "http://", 7))
    return (0);
  
  /* Parse URL */
  port = 80;
  host = arg + 7;
  path[0] = 0;
  if ((request = strchr(host, '/')))
    {
      *request++ = 0;
      snprintf(path, sizeof(path) - strlen(host) - 75, "%s", request);
    }
  
  if (strchr(host, ':') != NULL)  /* port is specified */
    {
      port = atoi(strchr(host, ':') + 1);
      *strchr(host, ':') = 0;
    }
  
  /* Open a TCP socket */
  if(proxy_enable)
  {
	  char *t = strchr(proxy_server,':');
	  pport = atoi(strchr(proxy_server,':')+1);
	  snprintf(phost,(int)(t-proxy_server)+1,"%s",proxy_server);
	              
	  if (!(tcp_sock = tcp_open(phost,pport)))
	  {
		  perror("http_open");
		  return (0);
	  }
  }else{

	  if (!(tcp_sock = tcp_open(host, port)))
      	  {
		  perror("http_open");
	    	  return (0);
      	  }
  }
  
  /* Send HTTP GET request */
  /* Please don't use a Agent know by shoutcast (Lynx, Mozilla) seems to be reconized and print
   * a html page and not the stream */
  /* ml> that's right, but it's just a playlist, so you've got to extract the URLs! */
  if(!proxy_enable)
  {
	  

	  snprintf(http_request, sizeof(http_request),
	   "GET /%s HTTP/1.0\r\n"
	   "Pragma: no-cache\r\n"
	   "Host: %s\r\n"
	   "User-Agent: xmms/1.2.7\r\n"  /* to make ShoutCast happy */
	   "Accept: * / *\r\n"
	   "\r\n", path, host);
  }else{
	  if(auth_enable || auth_enable_var)
	  {
		  do_basicauth();
                  snprintf(http_request, sizeof(http_request), "GET http://%s/%s HTTP/1.0\r\nProxy-authorization: Basic %s \r\n"
				  "Pragma: no-cache\r\nUser-Agent: xmms/1.2.7\r\n\r\n",host,path,authstring);
	  }else{
		  snprintf(http_request, sizeof(http_request), "GET http://%s/%s HTTP/1.0\r\nnPragma: no-cache\r\nUser-Agent: xmms/1.2.7\r\n\r\n",host,path);
	  }
  }
  
  send(tcp_sock, http_request, strlen(http_request), 0);
  
  /* Parse server reply */
  len = http_read_header(tcp_sock, http_response, PATH_MAX);
  if (len == -1)
    {
      fprintf(stderr, "http_open: %s\n", strerror(errno));
      return 0;
    }
  IFVERB printf( "HTTP header: >%s<\n", http_response );
  if(!strncmp(http_response, "HTTP/1.0 ", 9) || !strncmp(http_response, "HTTP/1.1 ", 9))
    {
      IFVERB printf( "this is an HTTP/1.x header\n" );
      ofs = 9;
      if(!strncmp(http_response+ofs, "200 ", 4))
	{
	  do  /* check Content-Type header */
	    {
	      len = http_read_header(tcp_sock, http_response, PATH_MAX);
	      if (len == -1)
		{
		  fprintf(stderr, "http_open: %s\n", strerror(errno));
		  return 0;
		}
	      IFVERB printf( "HTTP header: >%s<\n", http_response );
	      if(!strncmp(http_response, "Content-Length: ", 16))
	      {
		            char http_length[20];
			    sprintf(http_length,"%s",http_response+16);
			    http_file_length = atoi(http_length);

	      }
	      if(!strncmp(http_response, "Content-Type: ", 14))
		{
		  IFVERB printf( "Content-Type: >%s<\n", http_response+14 );
		  if(!strncmp(http_response+14, "audio/x-scpls", 13))
		    {
		      return(shoutcast(tcp_sock));
		    }
		  else  /* any other content types */
		    {
		      return(tcp_sock);
		    }
		}
	    }
	  while(len>0);
	  fprintf(stderr, "http_open: 200 contains no Content-Type header.\n");
	  return 0;
	}
      else if (!strncmp(http_response+ofs, "302 ", 4))  /* redirect */
	{
	  do
	    {
	      len = http_read_header(tcp_sock, http_response, PATH_MAX);
	      if (len == -1)
		{
		  fprintf(stderr, "http_open: %s\n", strerror(errno));
		  return 0;
		}
	      IFVERB printf( "HTTP header: >%s<\n", http_response );
	      if(!strncmp(http_response, "Location: ", 10))
		{
		  IFVERB printf( "Redirecting to:>%s<\n", http_response+10 );
		  close(tcp_sock);
		  return http_open(http_response+10);
		}
	    }
	  while(len>0);	   
	  fprintf(stderr, "http_open: 302 contains no Location header.\n");
	  return 0;
	}
      else  /* it's neither 302 nor 200 */
	{
	  fprintf(stderr, "http_open: %s\n", http_response);
	  return 0;
	}
    }
  else if(!strncmp(http_response, "ICY ", 4))
    {
      /* This is icecast streaming */
      IFVERB printf( "this is an ICY header\n" );
      do
	{
	  len = http_read_header(tcp_sock, http_response, PATH_MAX);
	  if (len == -1)
	    {
	      fprintf(stderr, "http_open: %s\n", strerror(errno));
	      return 0;
	    }
	  IFVERB printf( "ICY header: >%s<\n", http_response );
	}
      while(len>0);
      return(tcp_sock);
    }
  else  /* neither HTTP nor ICY */
    {
      fprintf(stderr, "http_open: %s\n", http_response);
      return 0;
    }
}


int shoutcast(int tcp_sock)
{
  int len;
  int noe, en;
  int j;
  char ** pl;
  
  char http_response[PATH_MAX];
  
  do
    {
      len = http_read_header(tcp_sock, http_response, PATH_MAX);
      if (len == -1)
	{
	  fprintf(stderr, "http_open: %s\n", strerror(errno));
	  return 0;
	}
      IFVERB printf( "Shoutcast: >%s<\n", http_response );
    }
  while(len>0);
  
  len = http_read_content_line(tcp_sock, http_response, PATH_MAX);
  if (len == -1)
    {
      fprintf(stderr, "http_open: %s\n", strerror(errno));
      return 0;
    }
  IFVERB printf( "Shoutcast: >%s<\n", http_response );
  if (strncmp(http_response, "[playlist]", 10) != 0)
    {
      fprintf(stderr, "Server response does not conform to audio/x-scpls MIME type format\n" );
      return 0;
    }
  
  len = http_read_content_line(tcp_sock, http_response, PATH_MAX);
  if (len == -1)
    {
      fprintf(stderr, "http_open: %s\n", strerror(errno));
      return 0;
    }
  IFVERB printf( "Shoutcast: >%s<\n", http_response );
  if (strncmp(http_response, "numberofentries=", 16) != 0)
    {
      fprintf(stderr, "Server response does not conform to audio/x-scpls MIME type format\n" );
      return 0;
    }
  
  sscanf( http_response+16, "%d", &noe );
  if(noe<1)
    {
      printf( "The ShoutCast playlist contains no entries.\n" );
      return(0);
    }
  
  printf( "The ShoutCast playlist contains %d entries:\n", noe );
  pl = (char**)calloc(noe,sizeof(char*));
  j = 0;
  while(j<noe)
    {
      len = http_read_content_line(tcp_sock, http_response, PATH_MAX);
      if (len == -1)
	{
	  fprintf(stderr, "http_open: %s\n", strerror(errno));
	  return 0;
	}
      if (strncmp(http_response, "File", 4) == 0)
	{
	  char * equ;
	  equ = strchr(http_response, '=')+1;
	  pl[j] = (char*)calloc(strlen(equ),sizeof(char));
	  strcpy(pl[j],equ);
	  printf( "[%2d] %s\n", ++j, equ );
	}
    }
  en = 1;
  if(noe>1)
    {
      do
	{
	  printf( "Please choose from the list: " );
	  scanf( "%d", &en );
	}
      while(en<1 || en>noe);
    }
  
  close(tcp_sock);
  return http_open(pl[en-1]);
}

int ftp_get_reply(int tcp_sock)
{
    int i;
    char c;
    char answer[1024];

    do
    {
        /* Read a line */
        for (i = 0, c = 0; i < 1023 && c != '\n'; i++)
        {
            read(tcp_sock, &c, sizeof(char));
            answer[i] = c;
        }
        answer[i] = 0;
        fprintf(stderr, "%s", answer + 4);
    }
    while (answer[3] == '-');

    answer[3] = 0;

    return (atoi(answer));
}

int ftp_open(char *arg)
{
    char *host;
    int port;
    char *dir;
    char *file;
    int tcp_sock;
    int data_sock;
    char ftp_request[PATH_MAX];
    struct sockaddr_in stLclAddr;
    socklen_t namelen;
    int i;

    /* Check for URL syntax */
    if (strncmp(arg, "ftp://", strlen("ftp://")))
        return (0);

    /* Parse URL */
    port = 21;
    host = arg + strlen("ftp://");
    if ((dir = strchr(host, '/')) == NULL)
        return (0);
    *dir++ = 0;
    if ((file = strrchr(dir, '/')) == NULL)
    {
        file = dir;
        dir = NULL;
    }
    else
        *file++ = 0;

    if (strchr(host, ':') != NULL)  /* port is specified */
    {
        port = atoi(strchr(host, ':') + 1);
        *strchr(host, ':') = 0;
    }

    /* Open a TCP socket */
    if (!(tcp_sock = tcp_open(host, port)))
    {
        perror("ftp_open");
        return (0);
    }

    /* Send FTP USER and PASS request */
    ftp_get_reply(tcp_sock);
    snprintf(ftp_request, sizeof(ftp_request), "USER anonymous\r\n");
    send(tcp_sock, ftp_request, strlen(ftp_request), 0);
    if (ftp_get_reply(tcp_sock) != 331)
        return (0);
    snprintf(ftp_request, sizeof(ftp_request), "PASS smpeguser@\r\n");
    send(tcp_sock, ftp_request, strlen(ftp_request), 0);
    if (ftp_get_reply(tcp_sock) != 230)
        return (0);
    snprintf(ftp_request, sizeof(ftp_request), "TYPE I\r\n");
    send(tcp_sock, ftp_request, strlen(ftp_request), 0);
    if (ftp_get_reply(tcp_sock) != 200)
        return (0);
    if (dir != NULL)
    {
        snprintf(ftp_request, sizeof(ftp_request), "CWD %s\r\n", dir);
        send(tcp_sock, ftp_request, strlen(ftp_request), 0);
        if (ftp_get_reply(tcp_sock) != 250)
            return (0);
    }

    /* Get interface address */
    namelen = sizeof(stLclAddr);
    if (getsockname(tcp_sock, (struct sockaddr *)&stLclAddr, &namelen) < 0)
        return (0);

    /* Open data socket */
    if ((data_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        return (0);

    stLclAddr.sin_family = AF_INET;

    /* Get the first free port */
    for (i = 0; i < 0xC000; i++)
    {
        stLclAddr.sin_port = htons(0x4000 + i);
        if (bind(data_sock, (struct sockaddr *)&stLclAddr, sizeof(stLclAddr)) >= 0)
            break;
    }
    port = 0x4000 + i;

    if (listen(data_sock, 1) < 0)
        return (0);

    i = ntohl(stLclAddr.sin_addr.s_addr);
    snprintf(ftp_request, sizeof(ftp_request), "PORT %d,%d,%d,%d,%d,%d\r\n",
            (i >> 24) & 0xFF, (i >> 16) & 0xFF,
            (i >> 8) & 0xFF, i & 0xFF, (port >> 8) & 0xFF, port & 0xFF);
    send(tcp_sock, ftp_request, strlen(ftp_request), 0);
    if (ftp_get_reply(tcp_sock) != 200)
        return (0);

    snprintf(ftp_request, sizeof(ftp_request), "RETR %s\r\n", file);
    send(tcp_sock, ftp_request, strlen(ftp_request), 0);
    if (ftp_get_reply(tcp_sock) != 150)
        return (0);

    return (accept(data_sock, NULL, NULL));
}
