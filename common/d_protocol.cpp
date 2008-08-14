// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2008 by The Odamex Team.
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


#include <stdio.h>
#include <stdlib.h>

#include "m_alloc.h"
#include "i_system.h"
#include "d_protocol.h"
#include "d_ticcmd.h"
#include "d_net.h"
#include "doomdef.h"
#include "doomstat.h"
#include "cmdlib.h"
#include "z_zone.h"


const char *ReadString (byte **stream)
{
	char *string = *((char **)stream);

	*stream += strlen (string) + 1;
	return string;
}

int ReadByte (byte **stream)
{
	byte v = **stream;
	*stream += 1;
	return v;
}

int ReadWord (byte **stream)
{
	short v = (((*stream)[0]) << 8) | (((*stream)[1]));
	*stream += 2;
	return v;
}

int ReadLong (byte **stream)
{
	int v = (((*stream)[0]) << 24) | (((*stream)[1]) << 16) | (((*stream)[2]) << 8) | (((*stream)[3]));
	*stream += 4;
	return v;
}

void WriteString (const char *string, byte **stream)
{
	char *p = *((char **)stream);

	while (*string) {
		*p++ = *string++;
	}

	*p++ = 0;
	*stream = (byte *)p;
}


void WriteByte (byte v, byte **stream)
{
	**stream = v;
	*stream += 1;
}

void WriteWord (short v, byte **stream)
{
	(*stream)[0] = v >> 8;
	(*stream)[1] = v & 255;
	*stream += 2;
}

void WriteLong (int v, byte **stream)
{
	(*stream)[0] = v >> 24;
	(*stream)[1] = (v >> 16) & 255;
	(*stream)[2] = (v >> 8) & 255;
	(*stream)[3] = v & 255;
	*stream += 4;
}

// Returns the number of bytes read
int UnpackUserCmd (usercmd_t *ucmd, byte **stream)
{
	byte *start = *stream;
	byte flags;
	byte flags2;

	// make sure the ucmd is empty
	memset (ucmd, 0, sizeof(usercmd_t));

	flags = ReadByte (stream);
	if (flags & UCMDF_MORE)
		flags2 = ReadByte (stream);
	else
		flags2 = 0;

	if (flags) {
		if (flags & UCMDF_BUTTONS)
			ucmd->buttons = ReadByte (stream);
		if (flags & UCMDF_PITCH)
			ucmd->pitch = ReadWord (stream);
		if (flags & UCMDF_YAW)
			ucmd->yaw = ReadWord (stream);
		if (flags & UCMDF_FORWARDMOVE)
			ucmd->forwardmove = ReadWord (stream);
		if (flags & UCMDF_SIDEMOVE)
			ucmd->sidemove = ReadWord (stream);
		if (flags & UCMDF_UPMOVE)
			ucmd->upmove = ReadWord (stream);
		if (flags & UCMDF_IMPULSE)
			ucmd->impulse = ReadByte (stream);

		if (flags2) {
			if (flags2 & UCMDF_ROLL)
				ucmd->roll = ReadWord (stream);
			if (flags2 & UCMDF_USE)
				ucmd->use = ReadByte (stream);
		}
	}

	return *stream - start;
}

// Returns the number of bytes written
int PackUserCmd (usercmd_t *ucmd, byte **stream)
{
	byte flags = 0;
	byte flags2 = 0;
	byte *temp = *stream;
	byte *start = *stream;

	WriteByte (0, stream);			// Make room for the packing bits
	if (ucmd->roll || ucmd->use) {
		flags |= UCMDF_MORE;
		WriteByte (0, stream);		// Make room for more packing bits
	}

	if (ucmd->buttons) {
		flags |= UCMDF_BUTTONS;
		WriteByte (ucmd->buttons, stream);
	}
	if (ucmd->pitch) {
		flags |= UCMDF_PITCH;
		WriteWord (ucmd->pitch, stream);
	}
	if (ucmd->yaw) {
		flags |= UCMDF_YAW;
		WriteWord (ucmd->yaw, stream);
	}
	if (ucmd->forwardmove) {
		flags |= UCMDF_FORWARDMOVE;
		WriteWord (ucmd->forwardmove, stream);
	}
	if (ucmd->sidemove) {
		flags |= UCMDF_SIDEMOVE;
		WriteWord (ucmd->sidemove, stream);
	}
	if (ucmd->upmove) {
		flags |= UCMDF_UPMOVE;
		WriteWord (ucmd->upmove, stream);
	}
	if (ucmd->impulse) {
		flags |= UCMDF_IMPULSE;
		WriteByte (ucmd->impulse, stream);
	}

	if (ucmd->roll) {
		flags2 |= UCMDF_ROLL;
		WriteWord (ucmd->roll, stream);
	}
	if (ucmd->use) {
		flags2 |= UCMDF_USE;
		WriteByte (ucmd->use, stream);
	}

	// Write the packing bits
	WriteByte (flags, &temp);
	if (flags2)
		WriteByte (flags2, &temp);

	return *stream - start;
}

FArchive &operator<< (FArchive &arc, usercmd_t &cmd)
{
	byte bytes[256];
	byte *stream = bytes;
	int len = PackUserCmd (&cmd, &stream);
	arc << (BYTE)len;
	arc.Write (bytes, len);
	return arc;
}

FArchive &operator>> (FArchive &arc, usercmd_t &cmd)
{
	byte bytes[256];
	byte *stream = bytes;
	BYTE len;
	arc >> len;
	arc.Read (bytes, len);
	UnpackUserCmd (&cmd, &stream);
	return arc;
}


VERSION_CONTROL (d_protocol_cpp, "$Id$")

