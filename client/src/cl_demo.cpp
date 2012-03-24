// Emacs style mode select   -*- C++ -*-
// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: cl_demo.cpp 2290 2011-06-27 05:05:38Z dr_sean $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//	Functions for recording and playing back recordings of network games
//
//-----------------------------------------------------------------------------

#include "doomtype.h"
#include "cl_main.h"
#include "p_ctf.h"
#include "d_player.h"
#include "m_argv.h"
#include "c_console.h"
#include "m_fileio.h"
#include "c_dispatch.h"
#include "d_net.h"
#include "cl_demo.h"
#include "m_swap.h"
#include "version.h"

EXTERN_CVAR(sv_maxclients)
EXTERN_CVAR(sv_maxplayers)

extern std::string digest;
extern playerskin_t* skins;
extern std::vector<std::string> wadfiles, wadhashes;

static const std::string default_netdemo_filename("%n_%g_%w-%m_%d");

NetDemo::NetDemo() :
	state(st_stopped), oldstate(st_stopped), filename(""),
	demofp(NULL)
{
    memset(&header, 0, sizeof(header));
}

NetDemo::~NetDemo()
{
	cleanUp();
}


//
// copy
//
//   Copies the data from one NetDemo object to another
 
void NetDemo::copy(NetDemo &to, const NetDemo &from)
{
	// free any memory used by structures and close open files
	cleanUp();

	to.state 			= from.state;
	to.oldstate			= from.oldstate;
	to.filename			= from.filename;
	to.demofp			= from.demofp;
	to.captured			= from.captured;
	to.snapshot_index	= from.snapshot_index;
	to.map_index		= from.map_index;
	memcpy(&to.header, &from.header, sizeof(header));
}


NetDemo::NetDemo(const NetDemo &rhs)
{
	copy(*this, rhs);
}

NetDemo& NetDemo::operator=(const NetDemo &rhs)
{
	copy(*this, rhs);
	return *this;
}


void NetDemo::reset()
{
	cleanUp();
	
	filename = "";	
	memset(&header, 0, sizeof(header));
	captured.clear();
}

//
// cleanUp
//
//   Attempts to close any open files and generally exit gracefully.
//

void NetDemo::cleanUp()
{
	if (isRecording())
	{
		stopRecording();	// Try to write any unwritten data
	}
	
	// close all files
	if (demofp)
	{
		fclose(demofp);
		demofp = NULL;
	}
	
	snapshot_index.clear();
	map_index.clear();
	state = oldstate = NetDemo::st_stopped;
}



void NetDemo::error(const std::string &message)
{
	cleanUp();
	gameaction = ga_nothing;
	gamestate = GS_FULLCONSOLE;

	Printf(PRINT_HIGH, "%s\n", message.c_str());
}


//
// writeHeader()
//
//   Writes the header struct to the netdemo file in little-endian format
//   Assumes that demofp has been opened correctly elsewhere.  Does not close
//   the file.

bool NetDemo::writeHeader()
{
	strncpy(header.identifier, "ODAD", 4);
	header.version = NETDEMOVER;
	header.compression = 0;
	header.snapshot_spacing = NetDemo::SNAPSHOT_SPACING;

	netdemo_header_t tmpheader;
	memcpy(&tmpheader, &header, sizeof(header));

	// convert from native byte ordering to little-endian
	tmpheader.snapshot_index_size	= SHORT(tmpheader.snapshot_index_size);
	tmpheader.snapshot_index_offset	= LONG(tmpheader.snapshot_index_offset);
	tmpheader.map_index_size		= SHORT(tmpheader.map_index_size);
	tmpheader.map_index_offset		= LONG(tmpheader.map_index_offset);
	tmpheader.snapshot_spacing		= SHORT(tmpheader.snapshot_spacing);
	tmpheader.starting_gametic		= LONG(tmpheader.starting_gametic);
	tmpheader.ending_gametic		= LONG(tmpheader.ending_gametic);
	
	fseek(demofp, 0, SEEK_SET);
	size_t cnt = 0;
	cnt += sizeof(tmpheader.identifier) *
		fwrite(&tmpheader.identifier, sizeof(tmpheader.identifier), 1, demofp);
	cnt += sizeof(tmpheader.version) *
		fwrite(&tmpheader.version, sizeof(tmpheader.version), 1, demofp);
	cnt += sizeof(tmpheader.compression) *
		fwrite(&tmpheader.compression, sizeof(tmpheader.compression), 1, demofp);
	cnt += sizeof(tmpheader.snapshot_index_size) *
		fwrite(&tmpheader.snapshot_index_size, sizeof(tmpheader.snapshot_index_size), 1, demofp);
	cnt += sizeof(tmpheader.snapshot_index_offset)*
		fwrite(&tmpheader.snapshot_index_offset, sizeof(tmpheader.snapshot_index_offset), 1, demofp);
	cnt += sizeof(tmpheader.map_index_size) *
		fwrite(&tmpheader.map_index_size, sizeof(tmpheader.map_index_size), 1, demofp);
	cnt += sizeof(tmpheader.map_index_offset)*
		fwrite(&tmpheader.map_index_offset, sizeof(tmpheader.map_index_offset), 1, demofp);
	cnt += sizeof(tmpheader.snapshot_spacing) *
		fwrite(&tmpheader.snapshot_spacing, sizeof(tmpheader.snapshot_spacing), 1, demofp);
	cnt += sizeof(tmpheader.starting_gametic) *
		fwrite(&tmpheader.starting_gametic, sizeof(tmpheader.starting_gametic), 1, demofp);
	cnt += sizeof(tmpheader.ending_gametic) *
		fwrite(&tmpheader.ending_gametic, sizeof(tmpheader.ending_gametic), 1, demofp);
	cnt += sizeof(tmpheader.reserved) *
		fwrite(&tmpheader.reserved, sizeof(tmpheader.reserved), 1, demofp);
	
	if (cnt < NetDemo::HEADER_SIZE)
	{
		return false;
	}

	return true;
}


//
// readHeader()
//
//   Reads the header struct from the netdemo file, converting it from
//   little-endian format to whatever the client's architecture uses.  Assumes
//   that demofp has been opened correctly elsewhere.  Does not close the file.

bool NetDemo::readHeader()
{
	fseek(demofp, 0, SEEK_SET);
	
	size_t cnt = 0;
	cnt += sizeof(header.identifier) *
		fread(&header.identifier, sizeof(header.identifier), 1, demofp);
	cnt += sizeof(header.version) *
		fread(&header.version, sizeof(header.version), 1, demofp);
	cnt += sizeof(header.compression) *
		fread(&header.compression, sizeof(header.compression), 1, demofp);
	cnt += sizeof(header.snapshot_index_size) *
		fread(&header.snapshot_index_size, sizeof(header.snapshot_index_size), 1, demofp);
	cnt += sizeof(header.snapshot_index_offset)*
		fread(&header.snapshot_index_offset, sizeof(header.snapshot_index_offset), 1, demofp);
	cnt += sizeof(header.map_index_size) *
		fread(&header.map_index_size, sizeof(header.map_index_size), 1, demofp);
	cnt += sizeof(header.map_index_offset)*
		fread(&header.map_index_offset, sizeof(header.map_index_offset), 1, demofp);
	cnt += sizeof(header.snapshot_spacing) *
		fread(&header.snapshot_spacing, sizeof(header.snapshot_spacing), 1, demofp);
	cnt += sizeof(header.starting_gametic) *
		fread(&header.starting_gametic, sizeof(header.starting_gametic), 1, demofp);
	cnt += sizeof(header.ending_gametic) *
		fread(&header.ending_gametic, sizeof(header.ending_gametic), 1, demofp);
	cnt += sizeof(header.reserved) *
		fread(&header.reserved, sizeof(header.reserved), 1, demofp);
	
	if (cnt < NetDemo::HEADER_SIZE)
	{
		return false;
	}

	// convert from little-endian to native byte ordering
	header.snapshot_index_size 		= SHORT(header.snapshot_index_size);
	header.snapshot_index_offset 	= LONG(header.snapshot_index_offset);
	header.map_index_size 			= SHORT(header.map_index_size);
	header.map_index_offset 		= LONG(header.map_index_offset);
	header.snapshot_spacing 		= SHORT(header.snapshot_spacing);
	header.starting_gametic 		= LONG(header.starting_gametic);
	header.ending_gametic			= LONG(header.ending_gametic);
	
	return true;
}


//
// writeIndex()
//
//   Writes the snapshot index to the netdemo file, converting it to
//   little-endian format from whatever the client's architecture uses.  Assumes
//   that demofp has been opened correctly elsewhere.  Does not close the file.

bool NetDemo::writeIndex()
{
	fseek(demofp, header.snapshot_index_offset, SEEK_SET);

	for (size_t i = 0; i < snapshot_index.size(); i++)
	{
		netdemo_index_entry_t entry;
		// convert to little-endian
		entry.ticnum = LONG(snapshot_index[i].ticnum);
		entry.offset = LONG(snapshot_index[i].offset);
		
		size_t cnt = 0;
		cnt += sizeof(entry.ticnum) *
			fwrite(&entry.ticnum, sizeof(entry.ticnum), 1, demofp);
		cnt += sizeof(entry.offset) *
			fwrite(&entry.offset, sizeof(entry.offset), 1, demofp);
		
		if (cnt < NetDemo::INDEX_ENTRY_SIZE)
		{
			return false;
		}
	}

	return true;
}


//
// readIndex()
//
//   Reads the snapshot index from the netdemo file, converting it from
//   little-endian format to whatever the client's architecture uses.  Assumes
//   that demofp has been opened correctly elsewhere.  Does not close the file.

bool NetDemo::readIndex()
{
	fseek(demofp, header.snapshot_index_offset, SEEK_SET);

	for (int i = 0; i < header.snapshot_index_size; i++)
	{
		netdemo_index_entry_t entry;
		
		size_t cnt = 0;
		cnt += sizeof(entry.ticnum) *
			fread(&entry.ticnum, sizeof(entry.ticnum), 1, demofp);
		cnt += sizeof(entry.offset) *
			fread(&entry.offset, sizeof(entry.offset), 1, demofp);
		
		if (cnt < INDEX_ENTRY_SIZE)
		{
			return false;
		}

		// convert from little-endian to native
		entry.ticnum = LONG(entry.ticnum);	
		entry.offset = LONG(entry.offset);

		snapshot_index.push_back(entry);
	}

	return true;
}


bool NetDemo::writeMapIndex()
{
	fseek(demofp, header.map_index_offset, SEEK_SET);

	for (size_t i = 0; i < map_index.size(); i++)
	{
		netdemo_index_entry_t entry;
		// convert to little-endian
		entry.ticnum = LONG(map_index[i].ticnum);
		entry.offset = LONG(map_index[i].offset);
		
		size_t cnt = 0;
		cnt += sizeof(entry.ticnum) *
			fwrite(&entry.ticnum, sizeof(entry.ticnum), 1, demofp);
		cnt += sizeof(entry.offset) *
			fwrite(&entry.offset, sizeof(entry.offset), 1, demofp);
		
		if (cnt < NetDemo::INDEX_ENTRY_SIZE)
		{
			return false;
		}
	}

	return true;
}

bool NetDemo::readMapIndex()
{
	fseek(demofp, header.map_index_offset, SEEK_SET);

	for (int i = 0; i < header.map_index_size; i++)
	{
		netdemo_index_entry_t entry;
		
		size_t cnt = 0;
		cnt += sizeof(entry.ticnum) *
			fread(&entry.ticnum, sizeof(entry.ticnum), 1, demofp);
		cnt += sizeof(entry.offset) *
			fread(&entry.offset, sizeof(entry.offset), 1, demofp);
		
		if (cnt < INDEX_ENTRY_SIZE)
		{
			return false;
		}

		// convert from little-endian to native
		entry.ticnum = LONG(entry.ticnum);	
		entry.offset = LONG(entry.offset);

		map_index.push_back(entry);
	}

	return true;
}



//
// startRecording()
//
//   Creates the netdemo file with the specified filename.  A temporary
//   header is written which will be overwritten with the proper information
//   in stopRecording().

bool NetDemo::startRecording(const std::string &filename)
{
	this->filename = filename;

	if (isPlaying() || isPaused())
	{
		error("Cannot record a netdemo while not connected to a server.");
		return false;
	}

	// Already recording so just ignore the command
	if (isRecording())
		return true;

	if (demofp != NULL)		// file is already open for some reason
	{
		fclose(demofp);
		demofp = NULL;
	}

	demofp = fopen(filename.c_str(), "wb");
	if (!demofp)
	{
		error("Unable to create netdemo file.");
		return false;
	}

	memset(&header, 0, sizeof(header));
	// Note: The header is not finalized at this point.  Write it anyway to
	// reserve space in the output file for it and overwrite it later.
	if (!writeHeader())
	{
		error("Unable to write netdemo header.");
		return false;
	}

	state = NetDemo::st_recording;
	header.starting_gametic = gametic;
	Printf(PRINT_HIGH, "Recording netdemo %s.\n", filename.c_str());

	if (connected)
	{
		// write a simulation of the connection sequence since the server
		// has already sent it to the client and it wasn't captured
		static buf_t tempbuf(4096);
		
		SZ_Clear(&tempbuf);
		writeLauncherSequence(&tempbuf);
		capture(&tempbuf);
		writeMessages();
		
		SZ_Clear(&tempbuf);
		writeConnectionSequence(&tempbuf);
		capture(&tempbuf);
		writeMessages();
	}

	return true;
}


//
// startPlaying()
//
//

bool NetDemo::startPlaying(const std::string &filename)
{
	this->filename = filename;
	
	if (filename.empty())
	{
		error("No netdemo filename specified.");
		return false;
	}	

	if (isPlaying())
	{
		// restart playing
		cleanUp();
		return startPlaying(filename);
	}

	if (isRecording())
	{
		error("Cannot play a netdemo while recording.");
		return false;
	}

	if (!(demofp = fopen(filename.c_str(), "rb")))
	{
		error("Unable to open netdemo file.");
		return false;
	}

	if (!readHeader())
	{
		error("Unable to read netdemo header.");
		return false;
	}

    if (header.version != NETDEMOVER)
    {
        // Do nothing since there is only one version of netdemo files currently
    }

	// read the demo's index
	if (fseek(demofp, header.snapshot_index_offset, SEEK_SET) != 0)
	{
		error("Unable to find netdemo index.\n");
		return false;
	}

	if (!readIndex())
	{
		error("Unable to read netdemo index.\n");
		return false;
	}

	// read the demo's map index
	if (fseek(demofp, header.map_index_offset, SEEK_SET) != 0)
	{
		error("Unable to find netdemo map index.\n");
		return false;
	}

	if (!readMapIndex())
	{
		error("Unable to read netdemo map index.\n");
		return false;
	}

	// get set up to read server cmds
	fseek(demofp, NetDemo::HEADER_SIZE, SEEK_SET);
	state = NetDemo::st_playing;

	Printf(PRINT_HIGH, "Playing netdemo %s.\n", filename.c_str());
	return true;
}


// 
// pause()
//
//   Changes the netdemo's state to paused.  No messages will be read or written
//   while in this state.

bool NetDemo::pause()
{
	if (isPlaying())
	{
		oldstate = state;
		state = NetDemo::st_paused;
		return true;
	}
	
	return false;
}


//
// resume()
//
//   Changes the netdemo's state to its state prior to the call to pause()
//

bool NetDemo::resume()
{
	if (isPaused())
	{
		state = oldstate;
		return true;
	}

	return false;
}

//
// stopRecording()
//
//   Writes the netdemo index to file and rewrites the netdemo header before
//   closing the netdemo file.

bool NetDemo::stopRecording()
{
	if (!isRecording())
	{
		return false;
	}
	state = NetDemo::st_stopped;

	// write any remaining messages that have been captured
	writeMessages();

	// write the end-of-demo marker
	byte marker = svc_netdemostop;
	writeChunk(&marker, sizeof(marker), NetDemo::msg_packet);

	// write the number of the last gametic in the recording
	header.ending_gametic = gametic;

	// tack the snapshot index onto the end of the recording
	fflush(demofp);
	header.snapshot_index_offset = ftell(demofp);
	header.snapshot_index_size = snapshot_index.size();

	if (!writeIndex())
	{
		error("Unable to write netdemo index.");
		return false;
	}

	// tack the map index on to the end of the snapshot index
	fflush(demofp);
	header.map_index_offset = ftell(demofp);
	header.map_index_size = map_index.size();

	if (!writeMapIndex())
	{
		error("Unable to write netdemo map index.");
		return false;
	}

	// rewrite the header since snapshot_index_offset and 
	// snapshot_index_size are now known
	if (!writeHeader())
	{
		error("Unable to write updated netdemo header.");
		return false;
	}

	fclose(demofp);
	demofp = NULL;

	Printf(PRINT_HIGH, "Demo recording has stopped.\n");
	reset();
	return true;
}


//
// stopPlaying()
//
//   Closes the netdemo file and sets the state to stopped
//

bool NetDemo::stopPlaying()
{
	state = NetDemo::st_stopped;
	SZ_Clear(&net_message);
	CL_QuitNetGame();

	if (demofp)
	{
		fclose(demofp);
		demofp = NULL;
	}
	
	Printf(PRINT_HIGH, "Demo has ended.\n");
	reset();
    gameaction = ga_fullconsole;
    gamestate = GS_FULLCONSOLE;
	
	return true;
}


//
// writeSnapshotData()
//
//   Write the entire state of the game to netbuffer.  Called by
//   writeSnapshot() and used to simulate SV_ClientFullUpdate() when
//   writing the connection sequence at the start of a netdemo.
//

void NetDemo::writeSnapshotData(buf_t *netbuffer)
{
	// Make sure the gamestate will be correct when loading a snapshot
	if (gamestate == GS_LEVEL)
	{
		MSG_WriteMarker		(netbuffer, svc_loadmap);
		MSG_WriteString		(netbuffer, level.mapname);
	}
	else if (gamestate == GS_INTERMISSION)
	{
		MSG_WriteMarker		(netbuffer, svc_exitlevel);
	}

	for (size_t i = 0; i < players.size(); i++)
	{
		if (players[i].mo)
		{
			// Server spawns player
			MSG_WriteMarker	(netbuffer, svc_spawnplayer);
			MSG_WriteByte	(netbuffer, players[i].id);
			MSG_WriteShort	(netbuffer, players[i].mo->netid);
			MSG_WriteLong	(netbuffer, players[i].mo->angle);
			MSG_WriteLong	(netbuffer, players[i].mo->x);
			MSG_WriteLong	(netbuffer, players[i].mo->y);
			MSG_WriteLong	(netbuffer, players[i].mo->z);
		}
	    
		// userinfo updates
		MSG_WriteMarker (netbuffer, svc_userinfo);
		MSG_WriteByte   (netbuffer, players[i].id);
		MSG_WriteString (netbuffer, players[i].userinfo.netname);
		MSG_WriteByte   (netbuffer, players[i].userinfo.team);
		MSG_WriteLong   (netbuffer, players[i].userinfo.gender);
		MSG_WriteLong   (netbuffer, players[i].userinfo.color);
		MSG_WriteString (netbuffer, skins[players[i].userinfo.skin].name);
		MSG_WriteShort  (netbuffer, players[i].GameTime);

		// updates for frag/kill count
		MSG_WriteMarker	(netbuffer, svc_updatefrags);
		MSG_WriteByte	(netbuffer, players[i].id);
		if (sv_gametype != GM_COOP)
			MSG_WriteShort	(netbuffer, players[i].fragcount);
		else
			MSG_WriteShort	(netbuffer, players[i].killcount);
		MSG_WriteShort	(netbuffer, players[i].deathcount);
		MSG_WriteShort	(netbuffer, players[i].points);

		// whether or not player is a spectator
		MSG_WriteMarker	(netbuffer, svc_spectate);
		MSG_WriteByte	(netbuffer, players[i].id);
		MSG_WriteByte	(netbuffer, players[i].spectator);
	}

	// team point updates
	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		MSG_WriteMarker	(netbuffer, svc_teampoints);
		for (int n = 0; n < NUMTEAMS; n++)
			MSG_WriteShort	(netbuffer, TEAMpoints[n]);
	}

	// updates for all mobjs
	AActor *mo;
	TThinkerIterator<AActor> iterator;
	while ( (mo = iterator.Next() ) )
	{
		if (!mo || mo->player)
			continue;
	
		MSG_WriteMarker	(netbuffer, svc_spawnmobj);
		MSG_WriteLong	(netbuffer, mo->x);
		MSG_WriteLong	(netbuffer, mo->y);
		MSG_WriteLong	(netbuffer, mo->z);
		MSG_WriteLong	(netbuffer, mo->angle);
		MSG_WriteShort	(netbuffer, mo->type);
		MSG_WriteShort	(netbuffer, mo->netid);
		MSG_WriteByte	(netbuffer, mo->rndindex);
		MSG_WriteShort	(netbuffer, mo->state - states);

		if (mo->translation)
		{
			MSG_WriteMarker	(netbuffer, svc_mobjtranslation);
			MSG_WriteShort	(netbuffer, mo->netid);
			MSG_WriteByte	(netbuffer, (mo->translation - translationtables) >> 8);
		}
				
		if (mo->flags & MF_MISSILE || mobjinfo[mo->type].flags & MF_MISSILE)
		{
			MSG_WriteShort	(netbuffer, mo->target ? mo->target->netid : 0);
			MSG_WriteShort	(netbuffer, mo->netid);
			MSG_WriteLong	(netbuffer, mo->angle);
			MSG_WriteLong	(netbuffer, mo->momx);
			MSG_WriteLong	(netbuffer, mo->momy);
			MSG_WriteLong	(netbuffer, mo->momz);
		}
		else if (mo->flags & MF_AMBUSH || mo->flags & MF_DROPPED)
		{
			MSG_WriteMarker	(netbuffer, svc_mobjinfo);
			MSG_WriteShort	(netbuffer, mo->netid);
			MSG_WriteLong	(netbuffer, mo->flags);
		}
		
		if ((mo->flags & MF_CORPSE) && (mo->state - states != S_GIBS))
		{
			MSG_WriteMarker	(netbuffer, svc_corpse);
			MSG_WriteShort	(netbuffer, mo->netid);
			MSG_WriteByte	(netbuffer, mo->frame);
			MSG_WriteByte	(netbuffer, mo->tics);
		}
	}

	// updates about flags in CTF
	if (sv_gametype == GM_CTF)
	{
		MSG_WriteMarker	(netbuffer, svc_ctfevent);
		MSG_WriteByte	(netbuffer, SCORE_NONE);

		for (size_t n = 0; n < NUMFLAGS; n++)
		{
			MSG_WriteByte	(netbuffer, CTFdata[n].state);
			MSG_WriteByte	(netbuffer, CTFdata[n].flagger); 
    	}

		MSG_WriteMarker	(netbuffer, svc_ctfevent);
		MSG_WriteByte	(netbuffer, SCORE_REFRESH);
		MSG_WriteByte	(netbuffer, 0);
		MSG_WriteByte	(netbuffer, consoleplayer().id);
		MSG_WriteByte	(netbuffer, consoleplayer().points);

		for (size_t n = 0; n < NUMFLAGS; n++)
			MSG_WriteLong	(netbuffer, TEAMpoints[n]);
	}

	// updates about moving sectors
	for (int n = 0; n < numsectors; n++)
	{
		sector_t *sec = &sectors[n];
		
		if (sec->moveable)
		{
			MSG_WriteMarker	(netbuffer, svc_sector);
			MSG_WriteShort	(netbuffer, n);
			MSG_WriteShort	(netbuffer, sec->floorheight >> FRACBITS);
			MSG_WriteShort	(netbuffer, sec->ceilingheight >> FRACBITS);
			MSG_WriteShort	(netbuffer, sec->floorpic);
			MSG_WriteShort	(netbuffer, sec->ceilingpic);
		}
	}

	// updates about switches on walls
	for (int n = 0; n < numlines; n++)
	{
		unsigned int state = 0, time = 0;
		if (P_GetButtonInfo(&lines[n], state, time) || lines[n].wastoggled)
		{
			MSG_WriteMarker	(netbuffer, svc_switch);
			MSG_WriteLong	(netbuffer, n);
			MSG_WriteByte	(netbuffer, lines[n].wastoggled);
			MSG_WriteLong	(netbuffer, state);
			MSG_WriteLong	(netbuffer, time);
		}
	}

	// updates about guns & ammo & health
	MSG_WriteMarker	(netbuffer, svc_playerinfo);

	for (int n = 0; n < NUMWEAPONS; n++)
	{
		MSG_WriteByte	(netbuffer, consoleplayer().weaponowned[n]);
	}

	for (int n = 0; n < NUMAMMO; n++)
	{
		MSG_WriteShort	(netbuffer, consoleplayer().maxammo[n]);
		MSG_WriteShort	(netbuffer, consoleplayer().ammo[n]);
	}

	MSG_WriteByte	(netbuffer, consoleplayer().health);
	MSG_WriteByte	(netbuffer, consoleplayer().armorpoints);
	MSG_WriteByte	(netbuffer, consoleplayer().armortype);
	MSG_WriteByte	(netbuffer, consoleplayer().readyweapon);
	MSG_WriteByte	(netbuffer, consoleplayer().backpack);

	// update the server settings
	// TODO!	
}


//
// writeSnapshot()
//
//   

void NetDemo::writeSnapshot(buf_t *netbuffer)
{
	buf_t tempbuf(NetDemo::MAX_SNAPSHOT_SIZE);

	writeSnapshotData(&tempbuf);
	MSG_WriteChunk(netbuffer, tempbuf.data, tempbuf.size());
}



//
// writeSnapshotIndexEntry()
//
//   
void NetDemo::writeSnapshotIndexEntry()
{
	// Update the snapshot index
	netdemo_index_entry_t entry;
	
	fflush(demofp);
	entry.offset = ftell(demofp);
	entry.ticnum = gametic;
	snapshot_index.push_back(entry);
}

//
// writeMapIndexEntry()
//
//   
void NetDemo::writeMapIndexEntry()
{
	// Update the map index
	netdemo_index_entry_t entry;
	
	fflush(demofp);
	entry.offset = ftell(demofp);
	entry.ticnum = gametic;
	map_index.push_back(entry);
}


//
// writeLocalCmd()
//
//   Generates a message indicating the current position and angle of the
//   consoleplayer, taking the place of ticcmds.  
void NetDemo::writeLocalCmd(buf_t *netbuffer) const
{
	// Record the local player's data
	player_t *player = &consoleplayer();
	if (!player->mo)
		return;

	AActor *mo = player->mo;

	MSG_WriteByte(netbuffer, svc_netdemocap);
	MSG_WriteByte(netbuffer, player->cmd.ucmd.buttons);
	MSG_WriteByte(netbuffer, player->cmd.ucmd.impulse);
	MSG_WriteShort(netbuffer, player->cmd.ucmd.yaw);
	MSG_WriteShort(netbuffer, player->cmd.ucmd.forwardmove);
	MSG_WriteShort(netbuffer, player->cmd.ucmd.sidemove);
	MSG_WriteShort(netbuffer, player->cmd.ucmd.upmove);
	MSG_WriteShort(netbuffer, player->cmd.ucmd.roll);

	MSG_WriteByte(netbuffer, mo->waterlevel);
	MSG_WriteLong(netbuffer, mo->x);
	MSG_WriteLong(netbuffer, mo->y);
	MSG_WriteLong(netbuffer, mo->z);
	MSG_WriteLong(netbuffer, mo->momx);
	MSG_WriteLong(netbuffer, mo->momy);
	MSG_WriteLong(netbuffer, mo->momz);
	MSG_WriteLong(netbuffer, mo->angle);
	MSG_WriteLong(netbuffer, mo->pitch);
	MSG_WriteLong(netbuffer, mo->roll);
	MSG_WriteLong(netbuffer, player->viewheight);
	MSG_WriteLong(netbuffer, player->deltaviewheight);
	MSG_WriteLong(netbuffer, player->jumpTics);
	MSG_WriteLong(netbuffer, mo->reactiontime);
	MSG_WriteByte(netbuffer, player->readyweapon);
	MSG_WriteByte(netbuffer, player->pendingweapon);
}


void NetDemo::writeChunk(const byte *data, size_t size, netdemo_message_t type)
{
	message_header_t msgheader;
	memset(&msgheader, 0, sizeof(msgheader));
	
	msgheader.type = static_cast<byte>(type);
	msgheader.length = LONG((uint32_t)size);
	msgheader.gametic = LONG(gametic);
	
	size_t cnt = 0;
	cnt += sizeof(msgheader.type) *
		fwrite(&msgheader.type, sizeof(msgheader.type), 1, demofp);
	cnt += sizeof(msgheader.length) *
		fwrite(&msgheader.length, sizeof(msgheader.length), 1, demofp);
	cnt += sizeof(msgheader.gametic) *
		fwrite(&msgheader.gametic, sizeof(msgheader.gametic), 1, demofp);

	cnt += fwrite(data, 1, size, demofp);
	if (cnt < size + NetDemo::MESSAGE_HEADER_SIZE)
	{
		error("Unable to write netdemo message chunk\n");
		return;
	}
}

//
// writeMessages()
//
//   Writes the packets received from the server and captures local player
//   input and writes to the netdemo file.
// 

void NetDemo::writeMessages()
{
	if (!isRecording())
		return;

	static buf_t netbuf_snapshot(NetDemo::MAX_SNAPSHOT_SIZE);

	if (connected && gamestate == GS_LEVEL)
	{
		// is it time to write a snapshot?
		if ((gametic - header.starting_gametic) % header.snapshot_spacing == 0
			&& (unsigned)gametic > header.starting_gametic)
		{
			SZ_Clear(&netbuf_snapshot);	

			writeSnapshotData(&netbuf_snapshot);
			writeSnapshotIndexEntry();
			
			writeChunk(netbuf_snapshot.ptr(), netbuf_snapshot.size(), NetDemo::msg_snapshot);
		}
		
		// Write the console player's game data
		buf_t netbuf_localcmd(128);
		writeLocalCmd(&netbuf_localcmd);
		captured.push_back(netbuf_localcmd);
	}

	byte *output_buf = new byte[captured.size() * MAX_UDP_PACKET];

	uint32_t output_len = 0;
	while (!captured.empty())
	{
		buf_t netbuf(captured.front());
		uint32_t len = netbuf.BytesLeftToRead();

		byte *chunk = netbuf.ReadChunk(len);
		memcpy(output_buf + output_len, chunk, len);
		output_len += len;
		
		captured.pop_front();
	}

	writeChunk(output_buf, output_len, NetDemo::msg_packet);

	delete [] output_buf;
}


//
// readMessageHeader()
//
//   Reads the message length and gametic from the netdemo file into the
//   len and tic parameters.
//   Returns false upon file read error.

bool NetDemo::readMessageHeader(netdemo_message_t &type, uint32_t &len, uint32_t &tic) const
{
	len = tic = 0;

	message_header_t msgheader;
	
	size_t cnt = 0;
	cnt += sizeof(msgheader.type) *
		fread(&msgheader.type, sizeof(msgheader.type), 1, demofp);
	cnt += sizeof(msgheader.length) *
		fread(&msgheader.length, sizeof(msgheader.length), 1, demofp);
	cnt += sizeof(msgheader.gametic) *
		fread(&msgheader.gametic, sizeof(msgheader.gametic), 1, demofp);
	
	if (cnt < NetDemo::MESSAGE_HEADER_SIZE)
	{
		return false;
	}

	// convert the values to native byte order
	len = LONG(msgheader.length);
	tic = LONG(msgheader.gametic);
	type = static_cast<netdemo_message_t>(msgheader.type);

	return true;
}


//
// readMessageBody()
//
//   Reads a message of length len from the netdemo file and stores the
//   message in netbuffer.
//
 
void NetDemo::readMessageBody(buf_t *netbuffer, uint32_t len)
{
	char *msgdata = new char[len];
	
	size_t cnt = fread(msgdata, 1, len, demofp);
	if (cnt < len)
	{
		delete[] msgdata;
		error("Can not read netdemo message.");

		return;
	}

	// ensure netbuffer has enough free space to hold this packet
	if (netbuffer->maxsize() - netbuffer->size() < len)
	{
		netbuffer->resize(len + netbuffer->size() + 1, false);
	}

	netbuffer->WriteChunk(msgdata, len);
	delete [] msgdata;

	if (!connected)
	{
		int type = MSG_ReadLong();
		if (type == CHALLENGE)
		{
			CL_PrepareConnect();
		}
		else if (type == 0)
		{
			CL_Connect();
		}
	}
	else
	{
		last_received = gametic;
		noservermsgs = false;
		// Since packets are captured after the header is read, we do not
		// have to read the packet header
		CL_ParseCommands();
		CL_SaveCmd();
		if (gametic - last_received > 65)
		{
			noservermsgs = true;
		}
	}
}


//
// readMessages()
//
//

void NetDemo::readMessages(buf_t* netbuffer)
{
	if (!isPlaying())
	{
		return;
	}

	netdemo_message_t type;
	uint32_t len = 0, tic = 0;
	
	// get the values for type, len and tic
	readMessageHeader(type, len, tic);
	
	while (type == NetDemo::msg_snapshot)
	{
		// skip over snapshots and read the next message instead
		fseek(demofp, len, SEEK_CUR);
		readMessageHeader(type, len, tic);
	}

	// read from the input file and put the data into netbuffer
	gametic = tic;
	readMessageBody(netbuffer, len);
}


//
// capture()
//
//   Copies data from inputbuffer just before the game parses it 
//

void NetDemo::capture(const buf_t* inputbuffer)
{
	if (!isRecording())
	{
		return;
	}

	if (gamestate == GS_DOWNLOAD)
	{
		// NullPoint: I think this will skip the downloading process
		return;
	}

	if (inputbuffer->size() > 0)
	{
		captured.push_back(*inputbuffer);
	}
}


//
// writeLauncherSequence()
//
//   Emulates the sequence of messages the server sends a launcher program or
//   the client when a client first contacts a server to initiate a connection.
//   As much of this data is parsed and ignored by a connecting client, a good
//   deal of the data written to netbuffer is simply place holding data and not
//   accurate.
//

void NetDemo::writeLauncherSequence(buf_t *netbuffer)
{
	cvar_t *var = NULL, *prev_cvar = NULL;
	
	// Server sends launcher info
	MSG_WriteLong	(netbuffer, CHALLENGE);
	MSG_WriteLong	(netbuffer, 0);		// server_token
	
	// get sv_hostname and write it
	var = cvar_t::FindCVar("sv_hostname", &prev_cvar);
	MSG_WriteString (netbuffer, "server_hostname");
	
	int playersingame = 0;
	for (size_t i = 0; i < players.size(); i++)
	{
		if (players[i].ingame())
			playersingame++;
	}
	MSG_WriteByte	(netbuffer, playersingame);
	MSG_WriteByte	(netbuffer, 0);				// sv_maxclients
	MSG_WriteString	(netbuffer, level.mapname);

	// names of all the wadfiles on the server	
	size_t numwads = wadfiles.size();
	if (numwads > 0xff)
		numwads = 0xff;
	MSG_WriteByte	(netbuffer, numwads - 1);

	for (size_t n = 1; n < numwads; n++)
	{
		std::string tmpname = wadfiles[n];
		
		// strip absolute paths, as they present a security risk
		FixPathSeparator(tmpname);
		size_t slash = tmpname.find_last_of(PATHSEPCHAR);
		if (slash != std::string::npos)
			tmpname = tmpname.substr(slash + 1, tmpname.length() - slash);

		MSG_WriteString	(netbuffer, tmpname.c_str());
	}
		
	MSG_WriteBool	(netbuffer, 0);		// deathmatch?
	MSG_WriteByte	(netbuffer, 0);		// sv_skill
	MSG_WriteBool	(netbuffer, (sv_gametype == GM_TEAMDM));
	MSG_WriteBool	(netbuffer, (sv_gametype == GM_CTF));

	for (size_t i = 0; i < players.size(); i++)
	{
		// Notes: client just ignores this data but still expects to parse it
		if (players[i].ingame())
		{
			MSG_WriteString	(netbuffer, "");	// player's netname
			MSG_WriteShort	(netbuffer, 0);		// player's fragcount
			MSG_WriteLong	(netbuffer, 0);		// player's ping
			MSG_WriteByte	(netbuffer, 0);		// player's team
		}
	}

	// MD5 hash sums for all the wadfiles on the server
	for (size_t n = 1; n < numwads; n++)
		MSG_WriteString	(netbuffer, wadhashes[n].c_str());

	MSG_WriteString	(netbuffer, "");	// sv_website.cstring()

	if (sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		MSG_WriteLong	(netbuffer, 0);		// sv_scorelimit
		for (size_t n = 0; n < NUMTEAMS; n++)
		{
			MSG_WriteBool	(netbuffer, false);
		}
	}	

    MSG_WriteShort	(netbuffer, VERSION);
  
  	// Note: these are ignored by clients when the client connects anyway
  	// so they don't need real data
	MSG_WriteString	(netbuffer, "");	// sv_email.cstring()  

	MSG_WriteShort	(netbuffer, 0);		// sv_timelimit
	MSG_WriteShort	(netbuffer, 0);		// timeleft before end of level
	MSG_WriteShort	(netbuffer, 0);		// sv_fraglimit

	MSG_WriteBool	(netbuffer, false);	// sv_itemrespawn
	MSG_WriteBool	(netbuffer, false);	// sv_weaponstay
	MSG_WriteBool	(netbuffer, false);	// sv_friendlyfire
	MSG_WriteBool	(netbuffer, false);	// sv_allowexit
	MSG_WriteBool	(netbuffer, false);	// sv_infiniteammo
	MSG_WriteBool	(netbuffer, false);	// sv_nomonsters
	MSG_WriteBool	(netbuffer, false);	// sv_monstersrespawn
	MSG_WriteBool	(netbuffer, false);	// sv_fastmonsters
	MSG_WriteBool	(netbuffer, false);	// sv_allowjump
	MSG_WriteBool	(netbuffer, false);	// sv_freelook
	MSG_WriteBool	(netbuffer, false);	// sv_waddownload
	MSG_WriteBool	(netbuffer, false);	// sv_emptyreset
	MSG_WriteBool	(netbuffer, false);	// sv_cleanmaps
	MSG_WriteBool	(netbuffer, false);	// sv_fragexitswitch
	
	for (size_t i = 0; i < players.size(); i++)
	{
		if (players[i].ingame())
		{
			MSG_WriteShort	(netbuffer, players[i].killcount);
			MSG_WriteShort	(netbuffer, players[i].deathcount);
			
			int timeingame = (time(NULL) - players[i].JoinTime)/60;
			if (timeingame < 0)
				timeingame = 0;
			MSG_WriteShort	(netbuffer, timeingame);
		}
	}
	
	MSG_WriteLong(netbuffer, (DWORD)0x01020304);
	MSG_WriteShort(netbuffer, sv_maxplayers);
    
	for (size_t i = 0; i < players.size(); i++)
	{
		if (players[i].ingame())
			MSG_WriteBool	(netbuffer, players[i].spectator);
	}
	
	MSG_WriteLong	(netbuffer, (DWORD)0x01020305);
	MSG_WriteShort	(netbuffer, 0);	// join_passowrd

	MSG_WriteLong	(netbuffer, GAMEVER);

    // TODO: handle patch files
	MSG_WriteByte	(netbuffer, 0);  // patchfiles.size()
//	MSG_WriteByte	(netbuffer, patchfiles.size());
    
//	for (size_t n = 0; n < patchfiles.size(); n++)
//		MSG_WriteString(netbuffer, patchfiles[n].c_str());
}


//
// writeConnectionSequence()
//
//   Emulates the sequence of messages that the server sends to a client in
//   the packet with sequence number 0 and writes them to netbuffer.
//

void NetDemo::writeConnectionSequence(buf_t *netbuffer)
{
	// The packet sequence id
	MSG_WriteLong	(netbuffer, 0);
	
	// Server sends our player id and digest
	MSG_WriteMarker	(netbuffer, svc_consoleplayer);
	MSG_WriteByte	(netbuffer, consoleplayer().id);
	MSG_WriteString	(netbuffer, digest.c_str());

	// our userinfo
	MSG_WriteMarker	(netbuffer, svc_userinfo);
	MSG_WriteByte	(netbuffer, consoleplayer().id);
	MSG_WriteString	(netbuffer, consoleplayer().userinfo.netname);
	MSG_WriteByte	(netbuffer, consoleplayer().userinfo.team);
	MSG_WriteLong	(netbuffer, consoleplayer().userinfo.gender);
	MSG_WriteLong	(netbuffer, consoleplayer().userinfo.color);
	MSG_WriteString	(netbuffer, skins[consoleplayer().userinfo.skin].name);
	MSG_WriteShort	(netbuffer, consoleplayer().GameTime);
	
	// Server sends its settings
	MSG_WriteMarker	(netbuffer, svc_serversettings);
	cvar_t *var = GetFirstCvar();
	while (var)
	{
		if (var->flags() & CVAR_SERVERINFO)
		{
			MSG_WriteByte	(netbuffer, 1);
			MSG_WriteString	(netbuffer,	var->name());
			MSG_WriteString	(netbuffer,	var->cstring());
		}
		var = var->GetNext();
	}
	MSG_WriteByte	(netbuffer, 2);		// end of server settings marker

	// Server tells everyone if we're a spectator
	MSG_WriteMarker	(netbuffer, svc_spectate);
	MSG_WriteByte	(netbuffer, consoleplayer().id);
	MSG_WriteByte	(netbuffer, consoleplayer().spectator);

	// Server sends map name
	MSG_WriteMarker	(netbuffer, svc_loadmap);
	MSG_WriteString	(netbuffer, level.mapname);

	// Server spawns the player
	MSG_WriteMarker	(netbuffer, svc_spawnplayer);
	MSG_WriteByte	(netbuffer, consoleplayer().id);
	if (consoleplayer().mo)
	{
		MSG_WriteShort	(netbuffer, consoleplayer().mo->netid);
		MSG_WriteLong	(netbuffer, consoleplayer().mo->angle);
		MSG_WriteLong	(netbuffer, consoleplayer().mo->x);
		MSG_WriteLong	(netbuffer, consoleplayer().mo->y);
		MSG_WriteLong	(netbuffer, consoleplayer().mo->z);
	}
	else
	{
		// The client hasn't yet received his own position from the server
		// This happens with cl_autorecord
		// Just fake a position for now
		MSG_WriteShort	(netbuffer, MAXSHORT);
		MSG_WriteLong	(netbuffer, 0);
		MSG_WriteLong	(netbuffer, 0);
		MSG_WriteLong	(netbuffer, 0);
		MSG_WriteLong	(netbuffer, 0);
	}
}


//
// snapshotLookup()
//
//		Returns the snapshot that preceeds the ticnum parameter or returns
//		NULL if the ticnum is out of bounds.
//
const NetDemo::netdemo_index_entry_t *NetDemo::snapshotLookup(int ticnum) const
{
	int index = (ticnum - header.starting_gametic) / header.snapshot_spacing;
	if (ticnum < gametic)
		index--;

	if (index < 0)
		index = 0;

	if (index >= header.snapshot_index_size)
		return NULL;

	return &snapshot_index[index];
}

//
// skipTo()
//
//		Reads the snapshot that preceeds the ticnum parameter and fills
//		netbuffer with its contents.
//
void NetDemo::skipTo(buf_t *netbuffer, int ticnum)
{
	const NetDemo::netdemo_index_entry_t *snap = snapshotLookup(ticnum);
	if (!snap)
		return;

	SZ_Clear(netbuffer);
	readSnapshot(netbuffer, snap);
}

//
// getCurrentMapIndex()
//
//		Returns the index into the map_index vector for the map that the
//		is currently being played.
//
int NetDemo::getCurrentMapIndex()
{
	for (int i = 0; i < header.map_index_size - 1; i++)
	{
		if ((int)map_index[i + 1].ticnum > gametic)
			return i;
	}

	return header.map_index_size - 1;
}

//
// nextMap()
//
//		Reads the snapshot at the begining of the next map and fills netbuffer
//		with its contents.
//
void NetDemo::nextMap(buf_t *netbuffer)
{
	int nextmapindex = getCurrentMapIndex() + 1;
	if (nextmapindex >= header.map_index_size)
		return;

	const NetDemo::netdemo_index_entry_t *snap = &map_index[nextmapindex];
	
	SZ_Clear(netbuffer);
	readSnapshot(netbuffer, snap);
}

//
// prevMap()
//
//		Reads the snapshot at the begining of the previous map and fills netbuffer
//		with its contents.
//
void NetDemo::prevMap(buf_t *netbuffer)
{
	int prevmapindex = getCurrentMapIndex() - 1; 
	if (prevmapindex < 0)
		prevmapindex = 0;

	const NetDemo::netdemo_index_entry_t *snap = &map_index[prevmapindex];
	
	SZ_Clear(netbuffer);
	readSnapshot(netbuffer, snap);
}


//
// readSnapshot()
//
//
void NetDemo::readSnapshot(buf_t *netbuffer, const netdemo_index_entry_t *snap)
{
	if (!isPlaying() || !snap)
		return;

	// Remove all players
	players.clear();

	// Remove all actors
	TThinkerIterator<AActor> iterator;
	AActor *mo;
	while ( (mo = iterator.Next() ) )
	{
		mo->Destroy();
	}

	gametic = snap->ticnum;
	int file_offset = snap->offset;
	fseek(demofp, file_offset, SEEK_SET);
	
	// read the values for length, gametic, and message type
	netdemo_message_t type;
	uint32_t len = 0, tic = 0;
	readMessageHeader(type, len, tic);

	if (netbuffer->maxsize() - netbuffer->size() < len)
		netbuffer->resize(netbuffer->size() + len + 1, false);
	
	byte *data = new byte[len];
	size_t cnt = fread(data, 1, len, demofp);
	if (cnt < len)
	{
		delete [] data;
		error("Unable to read snapshot from data file");
	}
	
	netbuffer->WriteChunk((char *)data, len, netbuffer->size());
	
	delete [] data;
}


//
// calculateTotalTime()
//
//   Returns the total length of the demo in seconds
//
int NetDemo::calculateTotalTime()
{
	if (!isPlaying())
		return 0;

	return ((header.ending_gametic - header.starting_gametic) / TICRATE);
}


//
// calculateTimeElapsed()
//
//   Returns the number of seconds since the demo started playing
//
int NetDemo::calculateTimeElapsed()
{
	if (!isPlaying())
		return 0;

	return ((gametic - header.starting_gametic) / TICRATE);	
}

const std::vector<int> NetDemo::getMapChangeTimes()
{
	std::vector<int> times;

	for (size_t i = 0; i < map_index.size(); i++)
	{
		int start_time = (map_index[i].ticnum - header.starting_gametic) / TICRATE;
		times.push_back(start_time);
	}
	
	return times;
}


void NetDemo::writeMapChange()
{
	static buf_t netbuf_snapshot(NetDemo::MAX_SNAPSHOT_SIZE);

	if (connected && gamestate == GS_LEVEL)
	{
		SZ_Clear(&netbuf_snapshot);	
	
		writeSnapshotData(&netbuf_snapshot);
		writeMapIndexEntry();
		
		writeChunk(netbuf_snapshot.ptr(), netbuf_snapshot.size(), NetDemo::msg_snapshot);
	}
}

VERSION_CONTROL (cl_demo_cpp, "$Id: cl_demo.cpp 2290 2011-06-27 05:05:38Z dr_sean $")
