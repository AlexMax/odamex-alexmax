// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	System specific network interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __I_NET_H__
#define __I_NET_H__

#include "doomtype.h"
#include "huffman.h"

#include <string>

#define	MAX_UDP_PACKET	1400

#define SERVERPORT  10666
#define CLIENTPORT  10667

#define PLAYER_FULLBRIGHTFRAME 70

#define CHALLENGE 5560020  // challenge
#define LAUNCHER_CHALLENGE 777123  // csdl challenge
#define VERSION 65	// GhostlyDeath -- this should remain static from now on

extern int   localport;
extern int   msg_badread;

// network message info
struct msg_info_t
{
	int id;
	const char *msgName;
	const char *msgFormat; // 'b'=byte, 'n'=short, 'N'=long, 's'=string

	const char *getName() { return msgName ? msgName : ""; }
};

typedef class player_s player_t;

// network messages
enum svc_t
{
	svc_abort,
	svc_full,
	svc_disconnect,
	svc_reserved3,
	svc_playerinfo,			// weapons, ammo, maxammo, raisedweapon for local player
	svc_moveplayer,			// [byte] [int] [int] [int] [int] [byte]
	svc_updatelocalplayer,	// [int] [int] [int] [int] [int]
	svc_svgametic,			// [int]
	svc_updateping,			// [byte] [byte]
	svc_spawnmobj,			//
	svc_disconnectclient,
	svc_loadmap,
	svc_consoleplayer,
	svc_mobjspeedangle,
	svc_explodemissile,		// [short] - netid
	svc_removemobj,
	svc_userinfo,
	svc_movemobj,			// [short] [byte] [int] [int] [int]
	svc_spawnplayer,
	svc_damageplayer,
	svc_killmobj,
	svc_firepistol,			// [byte] - playernum
	svc_fireshotgun,		// [byte] - playernum
	svc_firessg,			// [byte] - playernum
	svc_firechaingun,		// [byte] - playernum
	svc_fireweapon,			// [byte]
	svc_sector,
	svc_print,
	svc_mobjinfo,
	svc_updatefrags,		// [byte] [short]
	svc_teampoints,
	svc_activateline,
	svc_movingsector,
	svc_startsound,
	svc_reconnect,
	svc_exitlevel,
	svc_touchspecial,
	svc_changeweapon,
	svc_reserved42,
	svc_corpse,
	svc_missedpacket,
	svc_soundorigin,
	svc_reserved46,
	svc_reserved47,
	svc_forceteam,			// [Toke] Allows server to change a clients team setting.
	svc_switch,
	svc_reserved50,
	svc_reserved51,
	svc_spawnhiddenplayer,	// [denis] when client can't see player
	svc_updatedeaths,		// [byte] [short]
	svc_ctfevent,			// [Toke - CTF] - [int]
	svc_serversettings,		// 55 [Toke] - informs clients of server settings
	svc_spectate,			// [Nes] - [byte:state], [short:playernum]
	svc_connectclient,
    svc_midprint,

	// for co-op
	svc_mobjstate = 70,
	svc_actor_movedir,
	svc_actor_target,
	svc_actor_tracer,
	svc_damagemobj,

	// for downloading
	svc_wadinfo,			// denis - [ulong:filesize]
	svc_wadchunk,			// denis - [ulong:offset], [ushort:len], [byte[]:data]
	
	// for compressed packets
	svc_compressed = 200,

	// for when launcher packets go astray
	svc_launcher_challenge = 212,
	svc_challenge = 163,
	svc_max = 255
};

// network messages
enum clc_t
{
	clc_abort,
	clc_reserved1,
	clc_disconnect,
	clc_say,
	clc_move,			// send cmds
	clc_userinfo,		// send userinfo
	clc_svgametic,
	clc_rate,
	clc_ack,
	clc_rcon,
	clc_rcon_password,
	clc_changeteam,		// [NightFang] - Change your team [Toke - Teams] Made this actualy work
	clc_ctfcommand,
	clc_spectate,			// denis - [byte:state]
	clc_wantwad,			// denis - string:name, string:hash
	clc_kill,				// denis - suicide
	clc_cheat,				// denis - god, pumpkins, etc
    clc_cheatpulse,         // Russell - one off cheats (idkfa, idfa etc)

	// for when launcher packets go astray
	clc_launcher_challenge = 212,
	clc_challenge = 163,
	clc_max = 255
};

extern msg_info_t clc_info[clc_max];
extern msg_info_t svc_info[svc_max];

enum svc_compressed_masks
{
	adaptive_mask = 1,
	adaptive_select_mask = 2,
	adaptive_record_mask = 4,
	minilzo_mask = 8
};

typedef struct
{
   byte    ip[4];
   unsigned short  port;
   unsigned short  pad;
} netadr_t;

extern  netadr_t  net_from;  // address of who sent the packet


class buf_t
{
public:
	byte	*data;
	size_t	allocsize, cursize, readpos;
	bool	overflowed;  // set to true if the buffer size failed

public:

	void WriteByte(byte b)
	{
		byte *buf = SZ_GetSpace(sizeof(b));
		
		if(!overflowed)
		{
			*buf = b;
		}
	}

	void WriteShort(short s)
	{
		byte *buf = SZ_GetSpace(sizeof(s));
		
		if(!overflowed)
		{
			buf[0] = s&0xff;
			buf[1] = s>>8;
		}
	}

	void WriteLong(int l)
	{
		byte *buf = SZ_GetSpace(sizeof(l));
		
		if(!overflowed)
		{
			buf[0] = l&0xff;
			buf[1] = (l>>8)&0xff;
			buf[2] = (l>>16)&0xff;
			buf[3] = l>>24;
		}
	}

	void WriteString(const char *c)
	{
		if(c && *c)
		{
			size_t l = strlen(c);
			byte *buf = SZ_GetSpace(l + 1);

			if(!overflowed)
			{
				memcpy(buf, c, l + 1);
			}
		}
		else
			WriteByte(0);
	}

	void WriteChunk(const char *c, unsigned l, int startpos = 0)
	{
		byte *buf = SZ_GetSpace(l);

		if(!overflowed)
		{
			memcpy(buf, c + startpos, l);
		}
	}

	int ReadByte()
	{
		if(readpos+1 > cursize)
		{
			overflowed = true;
			return -1;
		}
		return (unsigned char)data[readpos++];
	}

	int NextByte()
	{
		if(readpos+1 > cursize)
		{
			overflowed = true;
			return -1;
		}
		return (unsigned char)data[readpos];
	}

	byte *ReadChunk(size_t size)
	{
		if(readpos+size > cursize)
		{
			overflowed = true;
			return NULL;
		}
		size_t oldpos = readpos;
		readpos += size;
		return data+oldpos;
	}

	int ReadShort()
	{
		if(readpos+2 > cursize)
		{
			overflowed = true;
			return -1;
		}
		size_t oldpos = readpos;
		readpos += 2;
		return (short)(data[oldpos] + (data[oldpos+1]<<8));
	}

	int ReadLong()
	{
		if(readpos+4 > cursize)
		{
			overflowed = true;
			return -1;
		}
		size_t oldpos = readpos;
		readpos += 4;
		return data[oldpos] +
				(data[oldpos+1]<<8) +
				(data[oldpos+2]<<16)+
				(data[oldpos+3]<<24);
	}

	const char *ReadString()
	{
		byte *begin = data + readpos;

		while(ReadByte() > 0);

		if(overflowed)
		{
			return "";
		}

		return (const char *)begin;
	}

	size_t BytesLeftToRead()
	{
		return overflowed || cursize < readpos ? 0 : cursize - readpos;
	}

	size_t BytesRead()
	{
		return readpos;
	}

	byte *ptr()
	{
		return data;
	}

	size_t size()
	{
		return cursize;
	}
	
	size_t maxsize()
	{
		return allocsize;
	}

	void setcursize(size_t len)
	{
		cursize = len > allocsize ? allocsize : len;
	}

	void clear()
	{
		cursize = 0;
		readpos = 0;
		overflowed = false;
	}

	void resize(size_t len)
	{
		delete[] data;
		
		data = new byte[len];
		allocsize = len;
		cursize = 0;
		readpos = 0;
		overflowed = false;
	}

	byte *SZ_GetSpace(size_t length)
	{
		if (cursize + length >= allocsize)
		{
			clear();
			overflowed = true;
			Printf (PRINT_HIGH, "SZ_GetSpace: overflow\n");
		}

		byte *ret = data + cursize;
		cursize += length;

		return ret;
	}

	buf_t &operator =(const buf_t &other)
	{
		delete[] data;
		
		data = new byte[other.allocsize];
		allocsize = other.allocsize;
		cursize = other.cursize;
		overflowed = other.overflowed;
		readpos = other.readpos;

		if(!overflowed)
			for(size_t i = 0; i < cursize; i++)
				data[i] = other.data[i];

		return *this;
	}
	
	buf_t()
		: data(0), allocsize(0), cursize(0), readpos(0), overflowed(false)
	{
	}
	buf_t(size_t len)
		: data(new byte[len]), allocsize(len), cursize(0), readpos(0), overflowed(false)
	{
	}
	buf_t(const buf_t &other)
		: data(new byte[other.allocsize]), allocsize(other.allocsize), cursize(other.cursize), readpos(other.readpos), overflowed(other.overflowed)
		
	{
		if(!overflowed)
			for(size_t i = 0; i < cursize; i++)
				data[i] = other.data[i];
	}
	~buf_t()
	{
		delete[] data;
		data = NULL;
	}
};

extern buf_t net_message;

void CloseNetwork (void);
void InitNetCommon(void);
void I_SetPort(netadr_t &addr, int port);
bool NetWaitOrTimeout(size_t ms);

char *NET_AdrToString (netadr_t a);
bool NET_StringToAdr (const char *s, netadr_t *a);
bool NET_CompareAdr (netadr_t a, netadr_t b);
int  NET_GetPacket (void);
void NET_SendPacket (buf_t &buf, netadr_t &to);
void NET_GetLocalAddress (void);

void SZ_Clear (buf_t *buf);
void SZ_Write (buf_t *b, const void *data, int length);
void SZ_Write (buf_t *b, const byte *data, int startpos, int length);

void MSG_WriteByte (buf_t *b, byte c);
void MSG_WriteMarker (player_t &Player, buf_t *b, svc_t c, const size_t Size);
void MSG_WriteMarker (netadr_t &To, buf_t *b, clc_t c, const size_t Size);
void MSG_WriteShort (buf_t *b, short c);
void MSG_WriteLong (buf_t *b, int c);
void MSG_WriteBool(buf_t *b, bool);
void MSG_WriteFloat(buf_t *b, float);
void MSG_WriteString (buf_t *b, const char *s);
void MSG_WriteChunk (buf_t *b, const void *p, unsigned l);

int MSG_BytesLeft(void);
int MSG_NextByte (void);

int MSG_ReadByte (void);
void *MSG_ReadChunk (const size_t &size);
int MSG_ReadShort (void);
int MSG_ReadLong (void);
bool MSG_ReadBool(void);
float MSG_ReadFloat(void);
const char *MSG_ReadString (void);

bool MSG_DecompressMinilzo ();
bool MSG_CompressMinilzo (buf_t &buf, size_t start_offset, size_t write_gap);

bool MSG_DecompressAdaptive (huffman &huff);
bool MSG_CompressAdaptive (huffman &huff, buf_t &buf, size_t start_offset, size_t write_gap);

#endif



