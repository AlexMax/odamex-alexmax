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
//	Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------


#ifdef UNIX
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#ifndef O_BINARY
#define O_BINARY		0
#endif
#endif

#ifdef WIN32
#include <io.h>
#else
#define strcmpi	strcasecmp
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include "doomtype.h"
#include "m_swap.h"
#include "m_fileio.h"
#include "i_system.h"
#include "z_zone.h"
#include "cmdlib.h"
#include "m_argv.h"
#include "md5.h"

#include "w_wad.h"


#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <iostream>
#include <iomanip>


//
// GLOBALS
//

// Location of each lump on disk.
lumpinfo_t*		lumpinfo;
size_t			numlumps;

void**			lumpcache;

static unsigned	stdisk_lumpnum;

#define MAX_HASHES 10

typedef struct
{
    std::string name;
    std::string hash[MAX_HASHES];
} gamewadinfo_t;

#if _MSC_VER <= 1200	// GhostlyDeath -- Work on VC6
static gamewadinfo_t doomwadnames[13];
bool WasVC6Inited = false;

void W_VC6Init(void)
{
	if (!WasVC6Inited)
	{
		// DOOM2F
		doomwadnames[0].name = "DOOM2F.WAD";

		// DOOM2
		doomwadnames[1].name = "DOOM2.WAD";
		doomwadnames[1].hash[0] = "25E1459CA71D321525F84628F45CA8CD",
		doomwadnames[1].hash[1] = "C3BEA40570C23E511A7ED3EBCD9865F7"; // BFG Edition

        // DOOM2 BFG EDITION
        doomwadnames[2].name = "DOOM2BFG.WAD";
        doomwadnames[2].hash[0] = "C3BEA40570C23E511A7ED3EBCD9865F7"; // BFG Edition

		// PLUTONIA
		doomwadnames[3].name = "PLUTONIA.WAD";
		doomwadnames[3].hash[0] = "75C8CF89566741FA9D22447604053BD7";

		// TNT
		doomwadnames[4].name = "TNT.WAD";
		doomwadnames[4].hash[0] = "4E158D9953C79CCF97BD0663244CC6B6";

		// DOOMU
		doomwadnames[5].name = "DOOMU.WAD";
		doomwadnames[5].hash[0] = "C4FE9FD920207691A9F493668E0A2083";

		// DOOM
		doomwadnames[6].name = "DOOM.WAD";
		doomwadnames[6].hash[0] = "C4FE9FD920207691A9F493668E0A2083";
		doomwadnames[6].hash[1] = "1CD63C5DDFF1BF8CE844237F580E9CF3";
		doomwadnames[6].hash[2] = "FB35C4A5A9FD49EC29AB6E900572C524"; // BFG Edition

		// DOOM BFG EDITION
		doomwadnames[7].name = "DOOMBFG.WAD";
        doomwadnames[7].hash[0] = "FB35C4A5A9FD49EC29AB6E900572C524"; // BFG Edition

		// DOOM SHAREWARE
		doomwadnames[8].name = "DOOM1.WAD";
		doomwadnames[8].hash[0] = "F0CEFCA49926D00903CF57551D901ABE";

		// FREEDOOM
		doomwadnames[9].name = "FREEDOOM.WAD";

		// FREEDM
		doomwadnames[10].name = "FREEDM.WAD";

		// CHEX
		doomwadnames[11].name = "CHEX.WAD";
		doomwadnames[11].hash[0] = "25485721882b050afa96a56e5758dd52";


		WasVC6Inited = true;
	}
}

#define MSVC6_SETUPWADS if (!WasVC6Inited) W_VC6Init();

#else
// Valid IWAD file names
static const gamewadinfo_t doomwadnames[] =
{
    { "DOOM2F.WAD", { "" } },
    { "DOOM2.WAD", { "25E1459CA71D321525F84628F45CA8CD", "C3BEA40570C23E511A7ED3EBCD9865F7" } },
    { "DOOM2BFG.WAD", { "C3BEA40570C23E511A7ED3EBCD9865F7" } },
    { "PLUTONIA.WAD", { "75C8CF89566741FA9D22447604053BD7" } },
    { "TNT.WAD", { "4E158D9953C79CCF97BD0663244CC6B6" } },
    { "DOOMU.WAD", { "C4FE9FD920207691A9F493668E0A2083" } },
    { "DOOM.WAD", { "C4FE9FD920207691A9F493668E0A2083", "1CD63C5DDFF1BF8CE844237F580E9CF3","FB35C4A5A9FD49EC29AB6E900572C524" } },
    { "DOOMBFG.WAD", { "FB35C4A5A9FD49EC29AB6E900572C524" } },
    { "DOOM1.WAD", { "F0CEFCA49926D00903CF57551D901ABE" } },
    { "FREEDOOM.WAD", { "" } },
    { "FREEDM.WAD", { "" } },
    { "CHEX.WAD", { "25485721882b050afa96a56e5758dd52" } },
    { "", { "" } }
};
#define MSVC6_SETUPWADS
#endif

//
// W_IsIWAD
//
// Checks to see whether a given filename is an IWAD (Internal WAD), it can take
// shortened (base name) versions of standard file names (eg doom2, plutonia),
// it can also do an optional hash comparison against a correct hash list
// for more "accurate" detection.
BOOL W_IsIWAD(std::string filename, std::string hash)
{
	MSVC6_SETUPWADS

    std::string name;

    if (!filename.length())
        return false;

    // uppercase our comparison strings
    std::transform(filename.begin(), filename.end(), filename.begin(), toupper);

    if (!hash.empty())
        std::transform(hash.begin(), hash.end(), hash.begin(), toupper);

    // Just get the base name
    M_ExtractFileBase(filename, name);

    // find our match if there is one
    for (DWORD i = 0; !doomwadnames[i].name.empty(); i++)
    {
        std::string basename;

        // We want a base name comparison aswell
        M_ExtractFileBase(doomwadnames[i].name, basename);

        // hash comparison
        if (!hash.empty())
        for (DWORD j = 0; !doomwadnames[i].hash[j].empty(); j++)
        {
            // the hash is always right! even if the name is wrong..
            if (doomwadnames[i].hash[j] == hash)
                return true;
        }

        if ((filename == doomwadnames[i].name) || (name == basename))
            return true;
    }

    return false;
}


//
// uppercoppy
//
// [RH] Copy up to 8 chars, upper-casing them in the process
//
void uppercopy (char *to, const char *from)
{
	int i;

	for (i = 0; i < 8 && from[i]; i++)
		to[i] = toupper (from[i]);
	for (; i < 8; i++)
		to[i] = 0;
}


// denis - Standard MD5SUM
std::string W_MD5(std::string filename)
{
	const int file_chunk_size = 8192;
	FILE *fp = fopen(filename.c_str(), "rb");

	if(!fp)
		return "";

	md5_state_t state;
	md5_init(&state);

	unsigned n = 0;
	unsigned char buf[file_chunk_size];

	while((n = fread(buf, 1, sizeof(buf), fp)))
		md5_append(&state, (unsigned char *)buf, n);

	md5_byte_t digest[16];
	md5_finish(&state, digest);

	fclose(fp);

	std::stringstream hash;

	for(int i = 0; i < 16; i++)
		hash << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (short)digest[i];

	return hash.str().c_str();
}


//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//
// Map reloads are supported through WAD reload
// so no need for vanilla tilde reload hack here
//

std::string W_AddFile (std::string filename)
{
	wadinfo_t		header;
	lumpinfo_t*		lump_p;
	size_t			i;
	FILE			*handle;
	size_t			length;
	size_t			startlump;
	size_t          res;
	filelump_t*		fileinfo;
	filelump_t		singleinfo;

	FixPathSeparator (filename);
	std::string name = filename;
	M_AppendExtension (name, ".wad");

    // open the file
	if ( (handle = fopen (filename.c_str(), "rb")) == NULL)
	{
		Printf (PRINT_HIGH, " couldn't open %s\n", filename.c_str());
		return "";
	}

	Printf (PRINT_HIGH, "adding %s\n", filename.c_str());

	startlump = numlumps;

	res = fread (&header, sizeof(header), 1, handle);
	header.identification = LELONG(header.identification);

	if (header.identification != IWAD_ID && header.identification != PWAD_ID)
	{
		// raw lump file
		fileinfo = &singleinfo;
		singleinfo.filepos = 0;
		singleinfo.size = M_FileLength(handle);
		M_ExtractFileBase (filename, name);
		numlumps++;
		Printf (PRINT_HIGH, " (single lump)\n", header.numlumps);
	}
	else
	{
		// WAD file
		header.numlumps = LELONG(header.numlumps);
		header.infotableofs = LELONG(header.infotableofs);
		length = header.numlumps*sizeof(filelump_t);

		if(length > (unsigned)M_FileLength(handle))
		{
			Printf (PRINT_HIGH, " bad number of lumps for %s\n", filename.c_str());
			fclose(handle);
			return "";
		}

		fileinfo = (filelump_t *)Z_Malloc (length, PU_STATIC, 0);
		fseek (handle, header.infotableofs, SEEK_SET);
		res = fread (fileinfo, length, 1, handle);
		numlumps += header.numlumps;
		Printf (PRINT_HIGH, " (%d lumps)\n", header.numlumps);
	}

    // Fill in lumpinfo
	lumpinfo = (lumpinfo_t *)Realloc (lumpinfo, numlumps*sizeof(lumpinfo_t));

	if (!lumpinfo)
		I_Error ("Couldn't realloc lumpinfo");

	lump_p = &lumpinfo[startlump];

	for (i=startlump ; i<numlumps ; i++,lump_p++, fileinfo++)
	{
		lump_p->handle = handle;
		lump_p->position = LELONG(fileinfo->filepos);
		lump_p->size = LELONG(fileinfo->size);
		strncpy (lump_p->name, fileinfo->name, 8);

		// W_CheckNumForName needs all lump names in upper case
		std::transform(lump_p->name, lump_p->name+8, lump_p->name, toupper);
	}

	return W_MD5(filename);
}

//
//
// IsMarker
//
// (from BOOM)
//
//

static BOOL IsMarker (const lumpinfo_t *lump, const char *marker)
{
	return (lump->namespc == ns_global) && (!strncmp (lump->name, marker, 8) ||
			(*(lump->name) == *marker && !strncmp (lump->name + 1, marker, 7)));
}

//
// W_MergeLumps
//
// Merge multiple tagged groups into one
// Basically from BOOM, too, although I tried to write it independently.
//

void W_MergeLumps (const char *start, const char *end, int space)
{
	char ustart[8], uend[8];
	lumpinfo_t *newlumpinfos;
	unsigned newlumps, oldlumps;
	BOOL insideBlock;
	unsigned flatHack, i;

	strncpy(ustart, start, 8);
	strncpy(uend, end, 8);

	std::transform(ustart, ustart + sizeof(ustart), ustart, toupper);
	std::transform(uend, uend + sizeof(uend), uend, toupper);

	// Some pwads use an icky hack to get flats with regular Doom.
	// This tries to detect them.
	flatHack = 0;
	if (!strcmp ("F_START", ustart) && !Args.CheckParm ("-noflathack"))
	{
		int fudge = 0;
		unsigned start = 0;

		for (i = 0; i < numlumps; i++) {
			if (IsMarker (lumpinfo + i, ustart))
				fudge++, start = i;
			else if (IsMarker (lumpinfo + i, uend))
				fudge--, flatHack = i;
		}
		if (start > flatHack)
			fudge--;
		if (fudge >= 0)
			flatHack = 0;
	}

	newlumpinfos = new lumpinfo_t[numlumps];

	newlumps = 0;
	oldlumps = 0;
	insideBlock = false;

	for (i = 0; i < numlumps; i++)
	{
		if (!insideBlock)
		{
			// Check if this is the start of a block
			if (IsMarker (lumpinfo + i, ustart))
			{
				insideBlock = true;

				// Create start marker if we haven't already
				if (!newlumps)
				{
					newlumps++;
					strncpy (newlumpinfos[0].name, ustart, 8);
					newlumpinfos[0].handle = NULL;
					newlumpinfos[0].position =
						newlumpinfos[0].size = 0;
					newlumpinfos[0].namespc = ns_global;
				}
			}
			else
			{
				// Copy lumpinfo down this list
				lumpinfo[oldlumps++] = lumpinfo[i];
			}
		}
		else
		{
			// Check if this is the end of a block
			if (flatHack)
			{
				if (flatHack == i)
				{
					insideBlock = false;
					flatHack = 0;
				}
				else
				{
					if (lumpinfo[i].size != 4096)
					{
						lumpinfo[oldlumps++] = lumpinfo[i];
					}
					else
					{
						newlumpinfos[newlumps] = lumpinfo[i];
						newlumpinfos[newlumps++].namespc = space;
					}
				}
			}
			else if (i && lumpinfo[i].handle != lumpinfo[i-1].handle)
			{
				// Blocks cannot span multiple files
				insideBlock = false;
				lumpinfo[oldlumps++] = lumpinfo[i];
			}
			else if (IsMarker (lumpinfo + i, uend))
			{
				// It is. We'll add the end marker once
				// we've processed everything.
				insideBlock = false;
			}
			else
			{
				newlumpinfos[newlumps] = lumpinfo[i];
				newlumpinfos[newlumps++].namespc = space;
			}
		}
	}

	// Now copy the merged lumps to the end of the old list
	// and create the end marker entry.

	if (newlumps)
	{
		if (oldlumps + newlumps > numlumps)
			lumpinfo = (lumpinfo_t *)Realloc (lumpinfo, oldlumps + newlumps);

		memcpy (lumpinfo + oldlumps, newlumpinfos, sizeof(lumpinfo_t) * newlumps);

		numlumps = oldlumps + newlumps;

		strncpy (lumpinfo[numlumps].name, uend, 8);
		lumpinfo[numlumps].handle = NULL;
		lumpinfo[numlumps].position =
			lumpinfo[numlumps].size = 0;
		lumpinfo[numlumps].namespc = ns_global;
		numlumps++;
	}

	delete[] newlumpinfos;
}

//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
std::vector<std::string> W_InitMultipleFiles (std::vector<std::string> &filenames)
{
	size_t		size, i;

    // open all the files, load headers, and count lumps
    // will be realloced as lumps are added
	numlumps = 0;

	M_Free(lumpinfo);

	std::vector<std::string> hashes(filenames);

	// open each file once, load headers, and count lumps
	int j = 0;
	std::vector<std::string> loaded;
	for(i = 0; i < filenames.size(); i++)
	{
		if(std::find(loaded.begin(), loaded.end(), filenames[i].c_str()) == loaded.end())
		{
			hashes[j++] = W_AddFile(filenames[i].c_str());
			loaded.push_back(filenames[i].c_str());
		}
	}
	filenames = loaded;
	hashes.resize(j);

	if (!numlumps)
		I_Error ("W_InitFiles: no files found");

	// [RH] Set namespace markers to global for everything
	for (i = 0; i < numlumps; i++)
		lumpinfo[i].namespc = ns_global;

	// [RH] Merge sprite and flat groups.
	//		(We don't need to bother with patches, since
	//		Doom doesn't use markers to identify them.)
	W_MergeLumps ("S_START", "S_END", ns_sprites); // denis - fixme - security
	W_MergeLumps ("F_START", "F_END", ns_flats);
	W_MergeLumps ("C_START", "C_END", ns_colormaps);

    // set up caching
	M_Free(lumpcache);

	size = numlumps * sizeof(*lumpcache);
	lumpcache = (void **)Malloc (size);

	if (!lumpcache)
		I_Error ("Couldn't allocate lumpcache");

	memset (lumpcache,0, size);

	stdisk_lumpnum = W_GetNumForName("STDISK");

	return hashes;
}

//
// W_CheckNumForName
// Returns -1 if name not found.
//

int W_CheckNumForName (const char* name, int namespc)
{
	union {
		char	s[9];
		int	x[2];

	} name8;

	int		v1;
	int		v2;
	lumpinfo_t*	lump_p;

    // make the name into two integers for easy compares
	strncpy (name8.s,name,9);

    // in case the name was a fill 8 chars
	name8.s[8] = 0;

    // case insensitive
	std::transform(name8.s, name8.s + strlen(name8.s), name8.s, toupper);

	v1 = name8.x[0];
	v2 = name8.x[1];


    // scan backwards so patch lump files take precedence
	lump_p = lumpinfo + numlumps;

	while (lump_p-- != lumpinfo)
	{
		if ( *(int *)lump_p->name == v1
			&& *(int *)&lump_p->name[4] == v2 && lump_p->namespc == namespc)
		{
			return lump_p - lumpinfo;
		}
	}

    // TFB. Not found.
	return -1;
}




//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName (const char* name)
{
	int	i;

	i = W_CheckNumForName (name);

	if (i == -1)
		I_Error ("W_GetNumForName: %s not found!", name);

	return i;
}


//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
unsigned W_LumpLength (unsigned lump)
{
	if (lump >= numlumps)
		I_Error ("W_LumpLength: %i >= numlumps",lump);

	return lumpinfo[lump].size;
}



//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void
W_ReadLump
( unsigned	lump,
 void*		dest )
{
	int		c;
	lumpinfo_t*	l;

	if (lump >= numlumps)
		I_Error ("W_ReadLump: %i >= numlumps",lump);

	l = lumpinfo + lump;

	if (lump != stdisk_lumpnum)
    	I_BeginRead();

	fseek (l->handle, l->position, SEEK_SET);
	c = fread (dest, l->size, 1, l->handle);

	if (feof(l->handle))
		I_Error ("W_ReadLump: only read %i of %i on lump %i", c, l->size, lump);

	if (lump != stdisk_lumpnum)
    	I_EndRead();
}

//
// W_ReadChunk
//
// denis - for wad downloading
//
unsigned W_ReadChunk (const char *file, unsigned offs, unsigned len, void *dest, unsigned &filelen)
{
	FILE *fp = fopen(file, "rb");
	unsigned read = 0;

	if(fp)
	{
		filelen = M_FileLength(fp);

		fseek(fp, offs, SEEK_SET);
		read = fread(dest, 1, len, fp);
		fclose(fp);
	}
	else filelen = 0;

	return read;
}


//
// W_CheckLumpName
//
bool W_CheckLumpName (unsigned lump, const char *name)
{
	if (lump >= numlumps)
		return false;

	return !strnicmp (lumpinfo[lump].name, name, 8);
}

//
// W_GetLumpName
//
void W_GetLumpName (char *to, unsigned  lump)
{
	if (lump >= numlumps)
		*to = 0;
	else
	{
		memcpy (to, lumpinfo[lump].name, 8); // denis - todo -string limit?
		to[8] = '\0';
		std::transform(to, to + strlen(to), to, toupper);
	}
}

//
// W_CacheLumpNum
//
void*
W_CacheLumpNum
( unsigned	lump,
 int		tag )
{
	byte*	ptr;

	if ((unsigned)lump >= numlumps)
		I_Error ("W_CacheLumpNum: %i >= numlumps",lump);

	if (!lumpcache[lump])
	{
		// read the lump in

		//printf ("cache miss on lump %i\n",lump);
		ptr = (byte *)Z_Malloc (W_LumpLength (lump) + 1, tag, &lumpcache[lump]);
		W_ReadLump (lump, lumpcache[lump]);
		ptr [W_LumpLength (lump)] = 0;
	}
	else
	{
		//printf ("cache hit on lump %i\n",lump);
		Z_ChangeTag (lumpcache[lump],tag);
	}

	return lumpcache[lump];
}

//
// W_CacheLumpName
//
void*
W_CacheLumpName
( const char*		name,
 int		tag )
{
	return W_CacheLumpNum (W_GetNumForName(name), tag);
}

size_t R_CalculateNewPatchSize(patch_t *patch, size_t length);
void R_ConvertPatch(patch_t *rawpatch, patch_t *newpatch);

//
// W_CachePatch
//
// [SL] Reads and caches a patch from disk. This takes care of converting the
// patch from the standard Doom format of posts with 1-byte lengths and offsets
// to a new format for posts that uses 2-byte lengths and offsets.
//
patch_t* W_CachePatch(unsigned lumpnum, int tag)
{
	if (lumpnum >= numlumps)
		I_Error ("W_CachePatch: %u >= numlumps", lumpnum);

	if (!lumpcache[lumpnum])
	{
		// temporary storage of the raw patch in the old format
		byte *rawlumpdata = new byte[W_LumpLength(lumpnum)];

		W_ReadLump(lumpnum, rawlumpdata);
		patch_t *rawpatch = (patch_t*)(rawlumpdata);

		size_t newlumplen = R_CalculateNewPatchSize(rawpatch, W_LumpLength(lumpnum));

		if (newlumplen > 0)
		{
			// valid patch
			byte *ptr = (byte *)Z_Malloc(newlumplen + 1, tag, &lumpcache[lumpnum]);
			patch_t *newpatch = (patch_t*)lumpcache[lumpnum];

			R_ConvertPatch(newpatch, rawpatch);
			ptr[newlumplen] = 0;
		}
		else
		{
			// invalid patch - just create a header with width = 0, height = 0
			Z_Malloc(sizeof(patch_t) + 1, tag, &lumpcache[lumpnum]);
			memset(lumpcache[lumpnum], 0, sizeof(patch_t) + 1);
		}

		delete [] rawlumpdata;
	}
	else
	{
		Z_ChangeTag(lumpcache[lumpnum], tag);
	}

	// denis - todo - would be good to check whether the patch violates W_LumpLength here
	// denis - todo - would be good to check for width/height == 0 here, and maybe replace those with a valid patch

	return (patch_t*)lumpcache[lumpnum];
}

patch_t* W_CachePatch(const char* name, int tag)
{
	return W_CachePatch(W_GetNumForName(name), tag);
	// denis - todo - would be good to replace non-existant patches with a default '404' patch
}

//
// W_FindLump
//
// Find a named lump. Specifically allows duplicates for merging of e.g.
// SNDINFO lumps.
//

int W_FindLump (const char *name, int *lastlump)
{
	char name8[9];
	int v1, v2;
	lumpinfo_t *lump_p;

	// make the name into two integers for easy compares
	strncpy (name8, name, 8); // denis - todo -string limit?
	std::transform(name8, name8 + 8, name8, toupper);

	v1 = *(int *)name8;
	v2 = *(int *)&name8[4];

	lump_p = lumpinfo + *lastlump;
	while (lump_p < lumpinfo + numlumps)
	{
		if (*(int *)lump_p->name == v1 && *(int *)&lump_p->name[4] == v2)
		{
			int lump = lump_p - lumpinfo;
			*lastlump = lump + 1;
			return lump;
		}
		lump_p++;
	}

	*lastlump = numlumps;
	return -1;
}

//
// W_Close
//
// Close all open files
//

void W_Close ()
{
	// store closed handles, so that fclose isn't called multiple times
	// for the same handle
	std::vector<FILE *> handles;

	lumpinfo_t * lump_p = lumpinfo;
	while (lump_p < lumpinfo + numlumps)
	{
		// if file not previously closed, close it now
		if(lump_p->handle &&
		   std::find(handles.begin(), handles.end(), lump_p->handle) == handles.end())
		{
			fclose(lump_p->handle);
			handles.push_back(lump_p->handle);
		}
		lump_p++;
	}
}

VERSION_CONTROL (w_wad_cpp, "$Id$")

