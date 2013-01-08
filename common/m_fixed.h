// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Fixed point arithemtics, implementation.
//
//-----------------------------------------------------------------------------


#ifndef __M_FIXED__
#define __M_FIXED__

#include <stdlib.h>
#include "doomtype.h"

//
// Fixed point, 32bit as 16.16.
//
#define FRACBITS				16
#define FRACUNIT				(1<<FRACBITS)

typedef int fixed_t;				// fixed 16.16
typedef unsigned int dsfixed_t;	// fixedpt used by span drawer

//
// Fixed Point Multiplication
//
inline static fixed_t FixedMul(fixed_t a, fixed_t b)
{
	return (fixed_t)(((int64_t)a * b) >> FRACBITS);
}

//
// Fixed Point Division
//
inline static fixed_t FixedDiv(fixed_t a, fixed_t b)
{
	return (abs(a) >> 14) >= abs(b) ? ((a ^ b) >> 31) ^ MAXINT :
		(fixed_t)(((int64_t)a << FRACBITS) / b);
}

#endif


