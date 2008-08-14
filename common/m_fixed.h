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

typedef int fixed_t;		// fixed 16.16
typedef unsigned int dsfixed_t;	// fixedpt used by span drawer

extern "C" fixed_t FixedMul_ASM				(fixed_t a, fixed_t b);
extern "C" fixed_t STACK_ARGS FixedDiv_ASM	(fixed_t a, fixed_t b);
fixed_t FixedMul_C				(fixed_t a, fixed_t b);
fixed_t FixedDiv_C				(fixed_t a, fixed_t b);

#ifdef ALPHA
inline fixed_t FixedMul (fixed_t a, fixed_t b)
{
    return (fixed_t)(((long)a * (long)b) >> 16);
}

inline fixed_t FixedDiv (fixed_t a, fixed_t b)
{
    if (abs(a) >> 14 >= abs(b))
	    return (a^b)<0 ? MININT : MAXINT;
	return (fixed_t)((((long)a) << 16) / b);
}

#else

#ifdef USEASM

#if defined(_MSC_VER)

__inline fixed_t FixedMul (fixed_t a, fixed_t b)
{
	fixed_t result;
	__asm {
		mov		eax,[a]
		imul	[b]
		shrd	eax,edx,16
		mov		[result],eax
	}
	return result;
}

#if 1
// Inlining FixedDiv with VC++ generates too many bad optimizations.
#define FixedDiv(a,b)			FixedDiv_ASM(a,b)
#else
__inline fixed_t FixedDiv (fixed_t a, fixed_t b)
{
	if (abs(a) >> 14 >= abs(b))
		return (a^b)<0 ? MININT : MAXINT;
	else
	{
		fixed_t result;

		__asm {
			mov		eax,[a]
			mov		edx,[a]
			sar		edx,16
			shl		eax,16
			idiv	[b]
			mov		[result],eax
		}
		return result;
	}
}
#endif

#else

#define FixedMul(a,b)			FixedMul_ASM(a,b)
#define FixedDiv(a,b)			FixedDiv_ASM(a,b)

#endif

#else // !USEASM
#define FixedMul(a,b)			FixedMul_C(a,b)
#define FixedDiv(a,b)			FixedDiv_C(a,b)
#endif

#endif // !ALPHA

#endif


