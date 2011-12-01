// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2010 by The Odamex Team.
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
//	Particle effect thinkers
//
//-----------------------------------------------------------------------------


#include "doomtype.h"
#include "doomstat.h"
#include "c_cvars.h"
#include "actor.h"
#include "p_effect.h"
#include "p_local.h"
#include "g_level.h"
#include "v_video.h"
#include "m_random.h"
#include "r_defs.h"
#include "r_things.h"
#include "s_sound.h"

EXTERN_CVAR (cl_rockettrails)

#define FADEFROMTTL(a)	(255/(a))

static int grey1, grey2, grey3, grey4, red, green, blue, yellow, black,
		   red1, green1, blue1, yellow1, purple, purple1, white,
		   rblue1, rblue2, rblue3, rblue4, orange, yorange, dred, grey5,
		   maroon1, maroon2;

static const struct ColorList {
	int *color, r, g, b;
} Colors[] = {
	{&grey1,	85,  85,  85 },
	{&grey2,	171, 171, 171},
	{&grey3,	50,  50,  50 },
	{&grey4,	210, 210, 210},
	{&grey5,	128, 128, 128},
	{&red,		255, 0,   0  },  
	{&green,	0,   200, 0  },  
	{&blue,		0,   0,   255},
	{&yellow,	255, 255, 0  },  
	{&black,	0,   0,   0  },  
	{&red1,		255, 127, 127},
	{&green1,	127, 255, 127},
	{&blue1,	127, 127, 255},
	{&yellow1,	255, 255, 180},
	{&purple,	120, 0,   160},
	{&purple1,	200, 30,  255},
	{&white, 	255, 255, 255},
	{&rblue1,	81,  81,  255},
	{&rblue2,	0,   0,   227},
	{&rblue3,	0,   0,   130},
	{&rblue4,	0,   0,   80 },
	{&orange,	255, 120, 0  },
	{&yorange,	255, 170, 0  },
	{&dred,		80,  0,   0  },
	{&maroon1,	154, 49,  49 },
	{&maroon2,	125, 24,  24 },
	{NULL}
};

void P_InitEffects (void)
{
	const struct ColorList *color = Colors;
	DWORD *palette = DefaultPalette->basecolors;
	int numcolors = DefaultPalette->numcolors;

	while (color->color) {
		*(color->color) = BestColor (palette, color->r, color->g, color->b, numcolors);
		color++;
	}
}

/*
//
// P_RunEffects
//
// Run effects on all mobjs in world
//
void P_RunEffects (void)
{
	int pnum = (consoleplayer().camera->subsector->sector - sectors) * numsectors;
	AActor *actor;
	TThinkerIterator<AActor> iterator;

	while ( (actor = iterator.Next ()) )
	{
		if (actor->effects)
		{
			// Only run the effect if the mobj is potentially visible
			int rnum = pnum + (actor->subsector->sector - sectors);
			if (rejectempty || !(rejectmatrix[rnum>>3] & (1 << (rnum & 7))))
				P_RunEffect (actor, actor->effects);
		}
	}
}
*//*
void P_RunEffect (AActor *actor, int effects)
{
	angle_t moveangle = R_PointToAngle2(0,0,actor->momx,actor->momy);
	particle_t *particle;

	if ((effects & FX_ROCKET) && cl_rockettrails) {
		// Rocket trail

		fixed_t backx = actor->x - FixedMul (finecosine[(moveangle)>>ANGLETOFINESHIFT], actor->radius*2);
		fixed_t backy = actor->y - FixedMul (finesine[(moveangle)>>ANGLETOFINESHIFT], actor->radius*2);
		fixed_t backz = actor->z - (actor->height>>3) * (actor->momz>>16) + (2*actor->height)/3;

		angle_t an = (moveangle + ANG90) >> ANGLETOFINESHIFT;
		int i, speed;

		particle = JitterParticle (3 + (M_Random() & 31));
		if (particle) {
			fixed_t pathdist = M_Random()<<8;
			particle->x = backx - FixedMul(actor->momx, pathdist);
			particle->y = backy - FixedMul(actor->momy, pathdist);
			particle->z = backz - FixedMul(actor->momz, pathdist);
			speed = (M_Random () - 128) * (FRACUNIT/200);
			particle->velx += FixedMul (speed, finecosine[an]);
			particle->vely += FixedMul (speed, finesine[an]);
			particle->velz -= FRACUNIT/36;
			particle->accz -= FRACUNIT/20;
			particle->color = yellow;
			particle->size = 2;
		}
		for (i = 6; i; i--) {
			particle_t *particle = JitterParticle (3 + (M_Random() & 31));
			if (particle) {
				fixed_t pathdist = M_Random()<<8;
				particle->x = backx - FixedMul(actor->momx, pathdist);
				particle->y = backy - FixedMul(actor->momy, pathdist);
				particle->z = backz - FixedMul(actor->momz, pathdist) + (M_Random() << 10);
				speed = (M_Random () - 128) * (FRACUNIT/200);
				particle->velx += FixedMul (speed, finecosine[an]);
				particle->vely += FixedMul (speed, finesine[an]);
				particle->velz += FRACUNIT/80;
				particle->accz += FRACUNIT/40;
				if (M_Random () & 7)
					particle->color = grey2;
				else
					particle->color = grey1;
				particle->size = 3;
			} else
				break;
		}
	}
	if ((effects & FX_GRENADE) && (cl_rockettrails)) {
		// Grenade trail

		P_DrawSplash2 (6,
			actor->x - FixedMul (finecosine[(moveangle)>>ANGLETOFINESHIFT], actor->radius*2),
			actor->y - FixedMul (finesine[(moveangle)>>ANGLETOFINESHIFT], actor->radius*2),
			actor->z - (actor->height>>3) * (actor->momz>>16) + (2*actor->height)/3,
			moveangle + ANG180, 2, 2);
	}
	if (effects & FX_FOUNTAINMASK) {
		// Particle fountain

		static const int *fountainColors[16] = 
			{ &black,	&black,
			  &red,		&red1,
			  &green,	&green1,
			  &blue,	&blue1,
			  &yellow,	&yellow1,
			  &purple,	&purple1,
			  &black,	&grey3,
			  &grey4,	&white
			};
		int color = (effects & FX_FOUNTAINMASK) >> 15;
		MakeFountain (actor, *fountainColors[color], *fountainColors[color+1]);
	}
}
*/
void P_DrawSplash (int count, fixed_t x, fixed_t y, fixed_t z, angle_t angle, int kind)
{
	int color1, color2;

	switch (kind) {
		case 1:		// Spark
			color1 = orange;
			color2 = yorange;
			break;
		default:
			return;
	}

	for (; count; count--) {
		particle_t *p = JitterParticle (10);
		angle_t an;

		if (!p)
			break;

		p->size = 2;
		p->color = M_Random() & 0x80 ? color1 : color2;
		p->velz -= M_Random () * 512;
		p->accz -= FRACUNIT/8;
		p->accx += (M_Random () - 128) * 8;
		p->accy += (M_Random () - 128) * 8;
		p->z = z - M_Random () * 1024;
		an = (angle + (M_Random() << 21)) >> ANGLETOFINESHIFT;
		p->x = x + (M_Random () & 15)*finecosine[an];
		p->y = y + (M_Random () & 15)*finesine[an];
	}
}

void P_DrawSplash2 (int count, fixed_t x, fixed_t y, fixed_t z, angle_t angle, int updown, int kind)
{
	int color1, color2, zvel, zspread, zadd;

	switch (kind) {
		case 0:		// Blood
			color1 = red;
			color2 = dred;
			break;
		case 1:		// Gunshot
			color1 = grey3;
			color2 = grey5;
			break;
		case 2:		// Smoke
			color1 = grey3;
			color2 = grey1;
			break;
		default:
			return;
	}

	zvel = -128;
	zspread = updown ? -6000 : 6000;
	zadd = (updown == 2) ? -128 : 0;

	for (; count; count--) {
		particle_t *p = NewParticle ();
		angle_t an;

		if (!p)
			break;

		p->ttl = 12;
		p->fade = FADEFROMTTL(12);
		p->trans = 255;
		p->size = 4;
		p->color = M_Random() & 0x80 ? color1 : color2;
		p->velz = M_Random () * zvel;
		p->accz = -FRACUNIT/22;
		if (kind) {
			an = (angle + ((M_Random() - 128) << 23)) >> ANGLETOFINESHIFT;
			p->velx = (M_Random () * finecosine[an]) >> 11;
			p->vely = (M_Random () * finesine[an]) >> 11;
			p->accx = p->velx >> 4;
			p->accy = p->vely >> 4;
		}
		p->z = z + (M_Random () + zadd) * zspread;
		an = (angle + ((M_Random() - 128) << 22)) >> ANGLETOFINESHIFT;
		p->x = x + (M_Random () & 31)*finecosine[an];
		p->y = y + (M_Random () & 31)*finesine[an];
	}
}

void P_DrawRailTrail (vec3_t start, vec3_t end)
{
	float length;
	int steps, i;
	float deg;
	vec3_t step, dir, pos, extend;

	VectorSubtract (end, start, dir);
	length = VectorLength (dir);
	steps = (int)(length*0.3333f);

	if (length) {
		// The railgun's sound is a special case. It gets played from the
		// point on the slug's trail that is closest to the hearing player.
		AActor *mo = consoleplayer().camera;
		vec3_t point;
		float r;
		float dirz;

		length = 1 / length;

		if (abs(mo->x - FLOAT2FIXED(start[0])) < 20 * FRACUNIT
			&& (mo->y - FLOAT2FIXED(start[1])) < 20 * FRACUNIT)
		{ // This player (probably) fired the railgun
			S_Sound (mo, CHAN_WEAPON, "weapons/railgf", 1, ATTN_NORM);
		}
		else
		{
			// Only consider sound in 2D (for now, anyway)
			r = ((start[1] - FIXED2FLOAT(mo->y)) * (-dir[1]) -
					(start[0] - FIXED2FLOAT(mo->x)) * (dir[0])) * length * length;

			dirz = dir[2];
			dir[2] = 0;
			VectorMA (start, r, dir, point);
			dir[2] = dirz;

			S_Sound (FLOAT2FIXED(point[0]), FLOAT2FIXED(point[1]),
				CHAN_WEAPON, "weapons/railgf", 1, ATTN_NORM);
		}
	} else {
		// line is 0 length, so nothing to do
		return;
	}

	VectorScale2 (dir, length);
	PerpendicularVector (extend, dir);
	VectorScale2 (extend, 3);
	VectorScale (dir, 3, step);

	VectorCopy (start, pos);
	deg = 270;
	for (i = steps; i; i--) {
		particle_t *p = NewParticle ();
		vec3_t tempvec;

		if (!p)
			return;

		p->trans = 255;
		p->ttl = 35;
		p->fade = FADEFROMTTL(35);
		p->size = 3;

		RotatePointAroundVector (tempvec, dir, extend, deg);
		p->velx = FLOAT2FIXED(tempvec[0])>>4;
		p->vely = FLOAT2FIXED(tempvec[1])>>4;
		p->velz = FLOAT2FIXED(tempvec[2])>>4;
		VectorAdd (tempvec, pos, tempvec);
		deg += 14;
		if (deg >= 360)
			deg -= 360;
		p->x = FLOAT2FIXED(tempvec[0]);
		p->y = FLOAT2FIXED(tempvec[1]);
		p->z = FLOAT2FIXED(tempvec[2]);
		VectorAdd (pos, step, pos);

		{
			int rand = M_Random();

			if (rand < 155)
				p->color = rblue2;
			else if (rand < 188)
				p->color = rblue1;
			else if (rand < 222)
				p->color = rblue3;
			else
				p->color = rblue4;
		}
	}

	VectorCopy (start, pos);
	for (i = steps; i; i--) {
		particle_t *p = JitterParticle (33);

		if (!p)
			return;

		p->size = 2;
		p->x = FLOAT2FIXED(pos[0]);
		p->y = FLOAT2FIXED(pos[1]);
		p->z = FLOAT2FIXED(pos[2]);
		p->accz -= FRACUNIT/4096;
		VectorAdd (pos, step, pos);
		{
			int rand = M_Random();

			if (rand < 85)
				p->color = grey4;
			else if (rand < 170)
				p->color = grey2;
			else
				p->color = grey1;
		}
		p->color = white;
	}
}

VERSION_CONTROL (p_effect_cpp, "$Id$")
