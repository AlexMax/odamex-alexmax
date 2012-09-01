// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2012 by The Odamex Team.
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
//	Event Protocol
//
//-----------------------------------------------------------------------------


#ifndef __D_PROTOCOL_H__
#define __D_PROTOCOL_H__

#include "doomstat.h"
#include "doomtype.h"
#include "doomdef.h"
#include "m_fixed.h"
#include "farchive.h"

#define FORM_ID		(('F'<<24)|('O'<<16)|('R'<<8)|('M'))
#define ZDEM_ID		(('Z'<<24)|('D'<<16)|('E'<<8)|('M'))
#define ZDHD_ID		(('Z'<<24)|('D'<<16)|('H'<<8)|('D'))
#define VARS_ID		(('V'<<24)|('A'<<16)|('R'<<8)|('S'))
#define UINF_ID		(('U'<<24)|('I'<<16)|('N'<<8)|('F'))
#define BODY_ID		(('B'<<24)|('O'<<16)|('D'<<8)|('Y'))

#define	ANGLE2SHORT(x)	((((x)/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*360)


struct zdemoheader_s {
	byte	demovermajor;
	byte	demoverminor;
	byte	minvermajor;
	byte	minverminor;
	byte	map[8];
	unsigned int rngseed;
	byte	consoleplayer;
};

#define DEM_BAD				0
// Bad command

#define DEM_USERCMD			1
// Player movement

struct usercmd_s {
	byte	msec;			// UNUSED 
	byte	buttons;
	short	pitch;			// up/down. currently just a y-sheering amount
	short	yaw;			// left/right
	short	roll;			// UNUSED
	short	forwardmove;
	short	sidemove;
	short	upmove;
	byte	impulse;
	byte	use;			// UNUSED
	
	usercmd_s() : msec(0), buttons(0), pitch(0), yaw(0), roll(0), forwardmove(0), sidemove(0), upmove(0), impulse(0), use(0) {}
};
typedef struct usercmd_s usercmd_t;

FArchive &operator<< (FArchive &arc, usercmd_t &cmd);
FArchive &operator>> (FArchive &arc, usercmd_t &cmd);

// When transmitted, the above message is preceded by a byte
// indicating which fields are actually present in the message.
// (buttons is always sent)
#define UCMDF_BUTTONS		0x01
#define UCMDF_PITCH			0x02
#define UCMDF_YAW			0x04
#define UCMDF_FORWARDMOVE	0x08
#define UCMDF_SIDEMOVE		0x10
#define UCMDF_UPMOVE		0x20
#define UCMDF_IMPULSE		0x40
#define UCMDF_MORE			0x80	// If set, the next byte contains more flags:
// flags byte 2
#define UCMDF_ROLL			0x01
#define UCMDF_USE			0x02

#define DEM_USERCMDCLONE	2
// Use previous DEM_USERCMD for the next (x+1) commands

//#define DEM_STUFFTEXT		3
// This message is a string for the console to execute

#define DEM_MUSICCHANGE		4
// This message is a string containing the name of the new music

#define DEM_PRINT			5
// Prints a string at the top of the screen

#define DEM_CENTERPRINT		6
// Prints a string in the middle of the screen

#define DEM_STOP			7
// Stop demo playback

#define DEM_UINFCHANGED		8
// User info changed

#define DEM_SINFCHANGED		9
// Server info changed

#define DEM_GENERICCHEAT	10
// Byte 1: Cheat to apply

#define CHT_GOD				0
#define CHT_NOCLIP			1
#define CHT_NOTARGET		2
#define CHT_CHAINSAW		3
#define CHT_IDKFA			4
#define CHT_IDFA			5
#define CHT_BEHOLDV			6
#define CHT_BEHOLDS			7
#define CHT_BEHOLDI			8
#define CHT_BEHOLDR			9
#define CHT_BEHOLDA			10
#define CHT_BEHOLDL			11
#define CHT_IDDQD			12	// Same as CHT_GOD but sets health
#define CHT_MASSACRE		13
#define CHT_CHASECAM		14
#define CHT_FLY				15

#define DEM_GIVECHEAT		11
// String: arguments to give command

#define DEM_SAY				12
// Byte: who to talk to, String: message to display

#define DEM_DROPPLAYER		13
// Currently unused

#define DEM_CHANGEMAP		14
// String: map to change to

#define DEM_SUICIDE			15
// The player wants to die

#define DEM_ADDBOT			16
// Byte: playernumber, String: userinfo key/value pairs for the bot

#define DEM_KILLBOTS		17

int UnpackUserCmd (usercmd_t *ucmd, byte **stream);
int PackUserCmd (usercmd_t *ucmd, byte **stream);

struct ticcmd_t;

int ReadByte (byte **stream);
int ReadWord (byte **stream);
int ReadLong (byte **stream);
const char *ReadString (byte **stream);
void WriteByte (byte val, byte **stream);
void WriteWord (short val, byte **stream);
void WriteLong (int val, byte **stream);
void WriteString (const char *string, byte **stream);

#endif //__D_PROTOCOL_H__

