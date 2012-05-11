// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62)
// Copyright (C) 2006-2012 by The Odamex Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Master Server communicator
//
//-----------------------------------------------------------------------------


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

#include "i_net.h"

#ifndef WIN32
typedef int SOCKET;
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define closesocket close
#define ioctlsocket ioctl
#define Sleep(x) usleep(x * 1000)
#else
typedef int socklen_t;
#endif


int net_socket;
int localport;
netadr_t net_from;   // address of who sent the packet

buf_t net_message(MAX_UDP_PACKET);
size_t msg_readcount;
int msg_badread;


//
// UDPsocket
//
SOCKET UDPsocket(void)
{
	SOCKET s;

	// allocate a socket
	s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET)
     	printf("can't create socket\n");

	return s;
}

//
// BindToLocalPort
//
void BindToLocalPort(SOCKET s, u_short port)
{
	int v;
	struct sockaddr_in address;

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	v = bind(s, (sockaddr *)&address, sizeof(address));
	if (v == SOCKET_ERROR)
    	   printf("BindToPort: error\n");
}


void CloseNetwork(void)
{
	closesocket(net_socket);
#ifdef WIN32
	WSACleanup();
#endif
}


// this is from Quake source code :)

void SockadrToNetadr(struct sockaddr_in *s, netadr_t *a)
{
	 memcpy(&(a->ip), &(s->sin_addr), sizeof(struct in_addr));
     a->port = s->sin_port;
}

void NetadrToSockadr(netadr_t *a, struct sockaddr_in *s)
{
     memset(s, 0, sizeof(*s));
     s->sin_family = AF_INET;

	 memcpy(&(s->sin_addr), &(a->ip), sizeof(struct in_addr));
     s->sin_port = a->port;
}

char *NET_AdrToString(netadr_t a, bool displayport)
{
     static char s[64];

	 if (displayport)
		sprintf(s, "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));
	 else
		sprintf(s, "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);

     return s;
}

bool NET_StringToAdr(char *s, netadr_t *a)
{
     struct hostent  *h;
     struct sockaddr_in sadr;
     char *colon;
     char copy[128];


     memset(&sadr, 0, sizeof(sadr));
     sadr.sin_family = AF_INET;

     sadr.sin_port = 0;

     strcpy(copy, s);
     // strip off a trailing :port if present
     for (colon = copy ; *colon ; colon++)
          if (*colon == ':')
          {
             *colon = 0;
             sadr.sin_port = htons(atoi(colon + 1));
          }

     if (copy[0] >= '0' && copy[0] <= '9')
     {
          *(int *)&sadr.sin_addr = inet_addr(copy);
     }
     else
     {
          if (!(h = gethostbyname(copy)))
                return 0;
          *(int *)&sadr.sin_addr = *(int *)h->h_addr_list[0];
     }

     SockadrToNetadr(&sadr, a);

     return true;
}

bool NET_CompareAdr(netadr_t a, netadr_t b)
{
    if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
        return true;

	return false;
}



int NET_GetPacket(void)
{
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);

    int ret = recvfrom(net_socket, (char *)net_message.data, net_message.maxsize, 0, (struct sockaddr *)&from, &fromlen);

    if (ret == -1)
    {
#ifdef WIN32
		errno = WSAGetLastError();

		if (errno == WSAEWOULDBLOCK)
			return false;
		if (errno == WSAECONNRESET)
			return false;

		if (errno == WSAEMSGSIZE)
		{
			printf("Warning: Oversize packet from %s\n", NET_AdrToString(net_from, true));
			return false;
		}

		printf("NET_GetPacket: %s", strerror(errno));
		return false;
#else
        if (errno == EWOULDBLOCK)
            return false;
        if (errno == ECONNREFUSED)
            return false;

        printf("NET_GetPacket: %s\n", strerror(errno));
        return false;
#endif
    }
    net_message.cursize = ret;
    SockadrToNetadr(&from, &net_from);

    msg_readcount = 0;
    msg_badread = false;

    return ret;
}


void NET_SendPacket(int length, byte *data, netadr_t to)
{
    int ret;
    struct sockaddr_in addr;

    NetadrToSockadr(&to, &addr);

    ret = sendto(net_socket, (const char*)data, length, 0, (struct sockaddr *)&addr, sizeof(addr) );

    if (ret == -1)
    {
#ifdef WIN32
		  int err = WSAGetLastError();

		  if (err == WSAEWOULDBLOCK)
			  return;
#else
          if (errno == EWOULDBLOCK)
              return;
          if (errno == ECONNREFUSED)
              return;
          printf("NET_SendPacket: %s\n", strerror(errno));
#endif
    }
}


void SZ_Clear(buf_t *buf)
{
    buf->clear();
}


byte *SZ_GetSpace(buf_t *b, int length)
{
	byte *data;

	if (b->cursize + length >= b->maxsize)
	{
       SZ_Clear(b);
	   b->overflowed = true;
       printf("SZ_GetSpace: overflow\n");
	}

	data = b->data + b->cursize;
	b->cursize += length;

	return data;
}

void SZ_Write(buf_t *b, void *data, int length)
{
    memcpy(SZ_GetSpace(b, length), data, length);
}


void MSG_WriteByte(buf_t *b, int c)
{
    byte *buf;

    buf = SZ_GetSpace(b, 1);
    buf[0] = c;
}

void MSG_WriteShort(buf_t *b, int c)
{
    byte *buf;

    buf = SZ_GetSpace(b, 2);
    buf[0] = c & 0xff;
    buf[1] = c >> 8;
}


void MSG_WriteLong(buf_t *b, int c)
{
    byte *buf;

    buf = SZ_GetSpace(b, 4);
    buf[0] = c & 0xff;
    buf[1] = (c >> 8) & 0xff;
    buf[2] = (c >> 16) & 0xff;
    buf[3] = c >> 24;
}


void MSG_WriteString(buf_t *b, char *s)
{
    if (!s)
        MSG_WriteByte(b, 0);
    else
	{
        SZ_Write(b, s, strlen(s));
        MSG_WriteByte(b, 0);
	}
}



int MSG_ReadByte(void)
{
    int c;

    if (msg_readcount + 1 > net_message.cursize)
    {
        msg_badread = true;
        return -1;
    }

    c = (unsigned char)net_message.data[msg_readcount];
    msg_readcount++;

    return c;
}

int MSG_ReadShort(void)
{
    int c;

    if (msg_readcount + 2 > net_message.cursize)
    {
        msg_badread = true;
        return -1;
    }

    c = (short)(net_message.data[msg_readcount] + (net_message.data[msg_readcount + 1] << 8));
    msg_readcount += 2;

    return c;
}

int MSG_BytesLeft(void)
{
	if(net_message.cursize < msg_readcount)
		return 0;

    return net_message.cursize - msg_readcount;
}

int MSG_ReadLong(void)
{
    int c;

	if (msg_readcount + 4 > net_message.cursize)
    {
        msg_badread = true;
        return -1;
    }

    c = net_message.data[msg_readcount] + (net_message.data[msg_readcount + 1] << 8) + (net_message.data[msg_readcount + 2] << 16) + (net_message.data[msg_readcount + 3] << 24);
    msg_readcount += 4;

    return c;
}

char *MSG_ReadString(void)
{
    static char string[2048];
    signed char c;
    unsigned int l;

    l = 0;

	if(MSG_BytesLeft())
    do
    {
       c = (signed char)MSG_ReadByte();
       if (c == 0)
           break;
       string[l] = c;
       l++;
	} while (l < sizeof(string) - 1);

    string[l] = 0;

    return string;
}


//
// InitNetCommon
//
void InitNetCommon(void)
{
   unsigned long _true = true;

#ifdef WIN32
   WSADATA wsad;
   WSAStartup(0x0101, &wsad);
#endif

   net_socket = UDPsocket();
   BindToLocalPort(net_socket, localport);
   if (ioctlsocket(net_socket, FIONBIO, &_true) == -1)
       printf("UDPsocket: ioctl FIONBIO: %s", strerror(errno));

   SZ_Clear(&net_message);
}


void I_SetPort(netadr_t &addr, int port)
{
   addr.port = htons(port);
}
