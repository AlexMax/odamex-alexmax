// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
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
//		Map Objects, MObj, definition and handling.
//
//-----------------------------------------------------------------------------


#ifndef __ACTOR_H__
#define __ACTOR_H__

// Basics.
#include "tables.h"
#include "m_fixed.h"

// We need the thinker_t stuff.
#include "dthinker.h"

// We need the WAD data structure for Map things,
// from the THINGS lump.
#include "doomdata.h"

// States are tied to finite states are
//	tied to animation frames.
// Needs precompiled tables/data structures.
#include "info.h"

#include "szp.h"

// STL
#include <vector>
#include <map>

//
// NOTES: AActor
//
// Actors are used to tell the refresh where to draw an image,
// tell the world simulation when objects are contacted,
// and tell the sound driver how to position a sound.
//
// The refresh uses the next and prev links to follow
// lists of things in sectors as they are being drawn.
// The sprite, frame, and angle elements determine which patch_t
// is used to draw the sprite if it is visible.
// The sprite and frame values are almost always set
// from state_t structures.
// The statescr.exe utility generates the states.h and states.c
// files that contain the sprite/frame numbers from the
// statescr.txt source file.
// The xyz origin point represents a point at the bottom middle
// of the sprite (between the feet of a biped).
// This is the default origin position for patch_ts grabbed
// with lumpy.exe.
// A walking creature will have its z equal to the floor
// it is standing on.
//
// The sound code uses the x,y, and subsector fields
// to do stereo positioning of any sound effited by the AActor.
//
// The play simulation uses the blocklinks, x,y,z, radius, height
// to determine when AActors are touching each other,
// touching lines in the map, or hit by trace lines (gunshots,
// lines of sight, etc).
// The AActor->flags element has various bit flags
// used by the simulation.
//
// Every actor is linked into a single sector
// based on its origin coordinates.
// The subsector_t is found with R_PointInSubsector(x,y),
// and the sector_t can be found with subsector->sector.
// The sector links are only used by the rendering code,
// the play simulation does not care about them at all.
//
// Any actor that needs to be acted upon by something else
// in the play world (block movement, be shot, etc) will also
// need to be linked into the blockmap.
// If the thing has the MF_NOBLOCK flag set, it will not use
// the block links. It can still interact with other things,
// but only as the instigator (missiles will run into other
// things, but nothing can run into a missile).
// Each block in the grid is 128*128 units, and knows about
// every line_t that it contains a piece of, and every
// interactable actor that has its origin contained.  
//
// A valid actor is an actor that has the proper subsector_t
// filled in for its xy coordinates and is linked into the
// sector from which the subsector was made, or has the
// MF_NOSECTOR flag set (the subsector_t needs to be valid
// even if MF_NOSECTOR is set), and is linked into a blockmap
// block or has the MF_NOBLOCKMAP flag set.
// Links should only be modified by the P_[Un]SetThingPosition()
// functions.
// Do not change the MF_NO* flags while a thing is valid.
//
// Any questions?
//

//
// Misc. mobj flags
//
typedef enum
{
    // Call P_SpecialThing when touched.
    MF_SPECIAL		= 1,
    // Blocks.
    MF_SOLID		= 2,
    // Can be hit.
    MF_SHOOTABLE	= 4,
    // Don't use the sector links (invisible but touchable).
    MF_NOSECTOR		= 8,
    // Don't use the blocklinks (inert but displayable)
    MF_NOBLOCKMAP	= 16,                    
	
    // Not to be activated by sound, deaf monster.
    MF_AMBUSH		= 32,
    // Will try to attack right back.
    MF_JUSTHIT		= 64,
    // Will take at least one step before attacking.
    MF_JUSTATTACKED	= 128,
    // On level spawning (initial position),
    //  hang from ceiling instead of stand on floor.
    MF_SPAWNCEILING	= 256,
    // Don't apply gravity (every tic),
    //  that is, object will float, keeping current height
    //  or changing it actively.
    MF_NOGRAVITY	= 512,
	
    // Movement flags.
    // This allows jumps from high places.
    MF_DROPOFF		= 0x400,
    // For players, will pick up items.
    MF_PICKUP		= 0x800,
    // Player cheat. ???
    MF_NOCLIP		= 0x1000,
    // Player: keep info about sliding along walls.
    MF_SLIDE		= 0x2000,
    // Allow moves to any height, no gravity.
    // For active floaters, e.g. cacodemons, pain elementals.
    MF_FLOAT		= 0x4000,
    // Don't cross lines
    //   ??? or look at heights on teleport.
    MF_TELEPORT		= 0x8000,
    // Don't hit same species, explode on block.
    // Player missiles as well as fireballs of various kinds.
    MF_MISSILE		= 0x10000,	
    // Dropped by a demon, not level spawned.
    // E.g. ammo clips dropped by dying former humans.
    MF_DROPPED		= 0x20000,
    // Use fuzzy draw (shadow demons or spectres),
    //  temporary player invisibility powerup.
    MF_SHADOW		= 0x40000,
    // Flag: don't bleed when shot (use puff),
    //  barrels and shootable furniture shall not bleed.
    MF_NOBLOOD		= 0x80000,
    // Don't stop moving halfway off a step,
    //  that is, have dead bodies slide down all the way.
    MF_CORPSE		= 0x100000,
    // Floating to a height for a move, ???
    //  don't auto float to target's height.
    MF_INFLOAT		= 0x200000,
	
    // On kill, count this enemy object
    //  towards intermission kill total.
    // Happy gathering.
    MF_COUNTKILL	= 0x400000,
    
    // On picking up, count this item object
    //  towards intermission item total.
    MF_COUNTITEM	= 0x800000,
	
    // Special handling: skull in flight.
    // Neither a cacodemon nor a missile.
    MF_SKULLFLY		= 0x1000000,
	
    // Don't spawn this object
    //  in death match mode (e.g. key cards).
    MF_NOTDMATCH    	= 0x2000000,
	
    // Player sprites in multiplayer modes are modified
    //  using an internal color lookup table for re-indexing.
    // If 0x4 0x8 or 0xc,
    //  use a translation table for player colormaps
    MF_TRANSLATION  	= 0xc000000,
	
	// a frozen corpse (for blasting) [RH] was 0x800000
	MF_ICECORPSE		= 0x80000000
} mobjflag_t;

#define MF_TRANSSHIFT	0x1A

#define TRANSLUC25			(FRACUNIT/4)
#define TRANSLUC33			(FRACUNIT/3)
#define TRANSLUC50			(FRACUNIT/2)
#define TRANSLUC66			((FRACUNIT*2)/3)
#define TRANSLUC75			((FRACUNIT*3)/4)

// Map Object definition.
class AActor : public DThinker
{
	DECLARE_SERIAL (AActor, DThinker)
	typedef szp<AActor> AActorPtr;
	AActorPtr self;

	class AActorPtrCounted
	{
		AActorPtr ptr;

		public:
		
		AActorPtrCounted() {}
		
		AActorPtr &operator= (AActorPtr other)
		{
			if(ptr)
				ptr->refCount--;
			if(other)
				other->refCount++;
			ptr = other;
			return ptr;
		}
		
		~AActorPtrCounted()
		{
			if(ptr)
				ptr->refCount--;
		}
		
		operator AActorPtr()
		{
			return ptr;
		}
		operator AActor*()
		{
			return ptr;
		}

		AActor &operator *()
		{
			return *ptr;
		}
		AActor *operator ->()
		{
			return ptr;
		}
	};
	
public:
	AActor ();
	AActor (const AActor &other);
	AActor &operator= (const AActor &other);
	AActor (fixed_t x, fixed_t y, fixed_t z, mobjtype_t type);
	void Destroy ();
	~AActor ();

	virtual void RunThink ();

    // Info for drawing: position.
    fixed_t		x;
    fixed_t		y;
    fixed_t		z;
	
	AActor			*snext, **sprev;	// links in sector (if needed)

    //More drawing info: to determine current sprite.
    angle_t		angle;	// orientation
    spritenum_t		sprite;	// used to find patch_t and flip value
    int			frame;	// might be ORed with FF_FULLBRIGHT
	fixed_t		pitch, roll;
	
	DWORD			effects;			// [RH] see p_effect.h

    // Interaction info, by BLOCKMAP.
    // Links in blocks (if needed).
	AActor			*bnext, **bprev;
	struct subsector_s		*subsector;
	
    // The closest interval over all contacted Sectors.
    fixed_t		floorz;
    fixed_t		ceilingz;

    // For movement checking.
    fixed_t		radius;
    fixed_t		height;	

    // Momentums, used to update position.
    fixed_t		momx;
    fixed_t		momy;
    fixed_t		momz;
	
    // If == validcount, already checked.
    int			validcount;
	
	mobjtype_t		type;
    mobjinfo_t*		info;	// &mobjinfo[mobj->type]
    int				tics;	// state tic counter
	state_t			*state;
	int				flags;
	int 			health;

    // Movement direction, movement generation (zig-zagging).
    byte			movedir;	// 0-7
    int				movecount;	// when 0, select a new dir
	char			visdir;

    // Thing being chased/attacked (or NULL),
    // also the originator for missiles.
	AActorPtr		target;
	AActorPtr		lastenemy;		// Last known enemy -- killogh 2/15/98
	
    // Reaction time: if non 0, don't attack yet.
    // Used by player to freeze a bit after teleporting.
    int				reactiontime;   
	
    // If >0, the target will be chased
    // no matter what (even if shot)
    int			threshold;
	
    // Additional info record for player avatars only.
    // Only valid if type == MT_PLAYER
	player_s*	player;
	
    // Player number last looked for.
    unsigned int	lastlook;	
	
    // For nightmare respawn.
    mapthing2_t		spawnpoint;	
	
	// Thing being chased/attacked for tracers.
	AActorPtr		tracer; 
	
	AActor			*inext, *iprev;	// Links to other mobjs in same bucket

	// denis - playerids of players to whom this object has been sent
	std::vector<size_t>	players_aware;

	AActorPtr		goal;			// Monster's goal if not chasing anything
	byte			*translation;	// Translation table (or NULL)
	fixed_t			translucency;	// 65536=fully opaque, 0=fully invisible
	byte			waterlevel;		// 0=none, 1=feet, 2=waist, 3=eyes
	
	fixed_t         onground;		// NES - Fixes infinite jumping bug like a charm.

	// a linked list of sectors where this object appears
	struct msecnode_s	*touching_sectorlist;				// phares 3/14/98

	short           deadtic;        // tics after player's death
	int             oldframe;

	unsigned char	rndindex;		// denis - because everything should have a random number generator, for prediction

	// ThingIDs
	static void ClearTIDHashes ();
	void AddToHash ();
	void RemoveFromHash ();
	AActor *FindByTID (int tid) const;
	static AActor *FindByTID (const AActor *first, int tid);
	AActor *FindGoal (int tid, int kind) const;
	static AActor *FindGoal (const AActor *first, int tid, int kind);

	int             netid;          // every object has its own netid
	short			tid;			// thing identifier

private:
	static AActor *TIDHash[128];
	static inline int TIDHASH (int key) { return key & 127; }

public:
	void LinkToWorld ();
	void UnlinkFromWorld ();
	void SetOrigin (fixed_t x, fixed_t y, fixed_t z);
	
	AActorPtr ptr(){ return AActorPtr(self); }
};

#endif


