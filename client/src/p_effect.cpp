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

void P_ThinkParticles (void)
{
	int i;
	particle_t *particle, *prev;

	i = ActiveParticles;
	prev = NULL;
	while (i != -1) {
		byte oldtrans;

		particle = Particles + i;
		i = particle->next;
		oldtrans = particle->trans;
		particle->trans -= particle->fade;
		if (oldtrans < particle->trans || --particle->ttl == 0) {
			memset (particle, 0, sizeof(particle_t));
			if (prev)
				prev->next = i;
			else
				ActiveParticles = i;
			particle->next = InactiveParticles;
			InactiveParticles = particle - Particles;
			continue;
		}
		particle->x += particle->velx;
		particle->y += particle->vely;
		particle->z += particle->velz;
		particle->velx += particle->accx;
		particle->vely += particle->accy;
		particle->velz += particle->accz;
		prev = particle;
	}
}

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

//
// AddParticle
//
// Creates a particle with "jitter"
//
particle_t *JitterParticle (int ttl)
{
	particle_t *particle = NewParticle ();

	if (particle) {
		fixed_t *val = &particle->velx;
		int i;

		// Set initial velocities
		for (i = 3; i; i--, val++)
			*val = (FRACUNIT/4096) * (M_Random () - 128);
		// Set initial accelerations
		for (i = 3; i; i--, val++)
			*val = (FRACUNIT/16384) * (M_Random () - 128);

		particle->trans = 255;	// fully opaque
		particle->ttl = ttl;
		particle->fade = FADEFROMTTL(ttl);
	}
	return particle;
}

static void MakeFountain (AActor *actor, int color1, int color2)
{
	particle_t *particle;

	if (!(level.time & 1))
		return;

	particle = JitterParticle (51);

	if (particle) {
		angle_t an = M_Random()<<(24-ANGLETOFINESHIFT);
		fixed_t out = FixedMul (actor->radius, M_Random()<<8);

		particle->x = actor->x + FixedMul (out, finecosine[an]);
		particle->y = actor->y + FixedMul (out, finesine[an]);
		particle->z = actor->z + actor->height + FRACUNIT;
		if (out < actor->radius/8)
			particle->velz += FRACUNIT*10/3;
		else
			particle->velz += FRACUNIT*3;
		particle->accz -= FRACUNIT/11;
		if (M_Random() < 30) {
			particle->size = 4;
			particle->color = color2;
		} else {
			particle->size = 6;
			particle->color = color1;
		}
	}
}

void P_RunEffect (AActor *actor, int effects)
{
	//angle_t moveangle = R_PointToAngle2(0,0,actor->momx,actor->momy);
	//particle_t *particle;

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

void P_DisconnectEffect (AActor *actor)
{
	int i;

	for (i = 64; i; i--) {
		particle_t *p = JitterParticle (TICRATE*2);

		if (!p)
			break;

		p->x = actor->x + ((M_Random()-128)<<9) * (actor->radius>>FRACBITS);
		p->y = actor->y + ((M_Random()-128)<<9) * (actor->radius>>FRACBITS);
		p->z = actor->z + (M_Random()<<8) * (actor->height>>FRACBITS);
		p->accz -= FRACUNIT/4096;
		p->color = M_Random() < 128 ? maroon1 : maroon2;
		p->size = 4;
	}
}

