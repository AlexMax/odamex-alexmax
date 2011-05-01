// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
// 		Functions and commands for client-side network demo recording and playback.
//		This uses code provided to us from Sean White, aka NightFang, to be relicensed
//		under GPL v2 or higher.  It also may use portions of DarkPlaces, an open source
//		Quake engine (http://icculus.org/twilight/darkplaces/).
//
//-----------------------------------------------------------------------------

#include "cl_main.h"
#include "d_player.h"
#include "m_argv.h"
#include "c_console.h"
#include "m_fileio.h"
#include "c_dispatch.h"
#include "d_net.h"

FILE *recordnetdemo_fp = NULL;

bool netdemoFinish = false;
//bool netdemoPaused = false;
bool netdemoRecord = false;
bool netdemoPlayback = false;


void CL_BeginNetRecord(char* demoname){
	netdemoRecord = true;
	netdemoPlayback = false;

	strcat(demoname, ".odd");
	Printf(PRINT_HIGH, "Writing %s\n", demoname);

	if(recordnetdemo_fp){ //this is to see if it already open
		fclose(recordnetdemo_fp);
        recordnetdemo_fp = NULL;
	}

	recordnetdemo_fp = fopen(demoname, "w+b");

	if(!recordnetdemo_fp){
		Printf(PRINT_HIGH, "Couldn't open the file\n");
		return;
	}

}

void CL_WirteNetDemoMessages(buf_t netbuffer){
	//captures the net packets and local movements
	player_t clientPlayer = consoleplayer();
	size_t len = netbuffer.size();
	fixed_t x, y, z, momx, momy, momz;
	angle_t angle;
	
	/*if(netdemoPaused)
		return;*/
	/*check if we can store the data if not make it bigger
	  46 is about the size of data needed to be stored */
	if(netbuffer.cursize + 46 > netbuffer.maxsize()){
		buf_t netbuffercopy;
		netbuffercopy.resize(netbuffer.cursize);
		netbuffercopy.data = netbuffer.data;
		netbuffer.resize(netbuffer.cursize + 46);
		netbuffer.data = netbuffercopy.data;
	}

	if(clientPlayer.mo){
		x = clientPlayer.mo->x;
		y = clientPlayer.mo->y;
		z = clientPlayer.mo->z;
		momx = clientPlayer.mo->momx;
		momy = clientPlayer.mo->momy;
		momz = clientPlayer.mo->momz;
		angle = clientPlayer.mo->angle;
	}

	MSG_WriteByte(&netbuffer, svc_netdemocap);
	MSG_WriteLong(&netbuffer, gametic);
	MSG_WriteByte(&netbuffer, clientPlayer.cmd.ucmd.buttons);
	MSG_WriteShort(&netbuffer, clientPlayer.cmd.ucmd.pitch);
	MSG_WriteShort(&netbuffer, clientPlayer.cmd.ucmd.yaw);
	MSG_WriteShort(&netbuffer, clientPlayer.cmd.ucmd.forwardmove);
	MSG_WriteShort(&netbuffer, clientPlayer.cmd.ucmd.sidemove);
	MSG_WriteShort(&netbuffer, clientPlayer.cmd.ucmd.upmove);
	MSG_WriteShort(&netbuffer, clientPlayer.cmd.ucmd.roll);

	MSG_WriteLong(&netbuffer, x);
	MSG_WriteLong(&netbuffer, y);
	MSG_WriteLong(&netbuffer, z);
	MSG_WriteLong(&netbuffer, momx);
	MSG_WriteLong(&netbuffer, momy);
	MSG_WriteLong(&netbuffer, momz);
	MSG_WriteLong(&netbuffer, angle);

	if(clientPlayer.pendingweapon != wp_nochange){
		MSG_WriteByte(&netbuffer, svc_changeweapon);
		MSG_WriteByte(&netbuffer, clientPlayer.pendingweapon);
	}


	len = netbuffer.size();
	fwrite(&len, sizeof(size_t), 1, recordnetdemo_fp);	
	fwrite(netbuffer.data, 1, len, recordnetdemo_fp);
	SZ_Clear(&netbuffer);
}
	
void CL_StopRecordingNetDemo(){
	buf_t buf;
	
	unsigned char bufdata [64];

	if(!netdemoRecord){
		Printf(PRINT_HIGH, "Not recording a demo no need to stop it.\n");
		return;
	}

	buf.data = bufdata;
	buf.cursize = sizeof(bufdata);
	SZ_Clear(&buf);
	
	MSG_WriteByte(&buf, svc_disconnect); //choose something else that does the same thing
	CL_CaptureDeliciousPackets(buf);
	fclose(recordnetdemo_fp);
	recordnetdemo_fp = NULL;
	netdemoRecord = false;
	Printf(PRINT_HIGH, "Demo has been stop recording\n");
}

void CL_ReadNetDemoMeassages(int* realrate){
	size_t len = 0;
	player_t clientPlayer = consoleplayer();
	
	if(!netdemoPlayback){
		return;
	}
	
	/*if(netdemoPaused){
		return;
	}*/
	
	fread(&len, sizeof(size_t), 1, recordnetdemo_fp);
	net_message.setcursize(len);
	net_message.readpos = 0;
	fread(net_message.data, 1, len, recordnetdemo_fp);
	
	if(!connected){
		int type = MSG_ReadLong();

		if(type == CHALLENGE)
		{
			CL_PrepareConnect();
		} else if(type == 0){
			CL_Connect();
		}
	} else {
		realrate += len;
		last_received = gametic;
		noservermsgs = false;
		CL_ReadPacketHeader();
        CL_ParseCommands();
		CL_SaveCmd();
	}
}


void CL_StopDemoPlayBack(){
	
	if(!netdemoPlayback){
		return;
	}

	fclose(recordnetdemo_fp);
	netdemoPlayback = false;
	recordnetdemo_fp = NULL;
	gameaction = ga_nothing;
}

void CL_StartDemoPlayBack(std::string demoname){
	FixPathSeparator (demoname);
	
	if(recordnetdemo_fp){ //this is to see if it already open
		fclose(recordnetdemo_fp);
        recordnetdemo_fp = NULL;
	}
	
	recordnetdemo_fp = fopen(demoname.c_str(), "r+b");

	if(!recordnetdemo_fp){
		Printf(PRINT_HIGH, "Couldn't open file");
		gameaction = ga_nothing;
		gamestate = GS_FULLCONSOLE;
		return;
	}


	netdemoPlayback = true;
	gamestate = GS_CONNECTING;
	netdemoRecord = false;
	netdemoFinish = false;
	//netdemoPaused = false;
}

void CL_CaptureDeliciousPackets(buf_t netbuffer){
	//captures just the net packets before the game
	size_t len = netbuffer.size();
	fwrite(&len, sizeof(size_t), 1, recordnetdemo_fp);
	fwrite(netbuffer.data, 1, len, recordnetdemo_fp);
	SZ_Clear(&netbuffer);
}

BEGIN_COMMAND(netrecord){
	char* demonamearg = argv[1];
	if(gamestate == GS_LEVEL){
		Printf(PRINT_HIGH, "Can't record when already in the level.\n");
		return;
	}

	if(argc < 1){
		Printf(PRINT_HIGH, "Please add a name for the demo\n");
		return;
	}

	if(recordnetdemo_fp){
		Printf(PRINT_HIGH, "Need to stop recording / playing of a demo before recording a new one.\n");
		return;
	}

	

	CL_BeginNetRecord(demonamearg);
}
END_COMMAND(netrecord)

/*BEGIN_COMMAND(netpuase){
	if(netdemoPaused){
		netdemoPaused = false;
	} else {
		netdemoPaused = true;
	}
}
END_COMMAND(netpuase)
*/

BEGIN_COMMAND(netplay){
	if(argc < 1){
		Printf(PRINT_HIGH, "Usage: netplay <demoname>\n");
		return;
	}

	if(recordnetdemo_fp){
		Printf(PRINT_HIGH, "Need to stop recording / playing of a demo before recording a new one.\n");
		return;
	}

	if(connected){
		CL_QuitNetGame();
	}

	char* demonamearg = argv[1];

	CL_StartDemoPlayBack(demonamearg);

}
END_COMMAND(netplay)

BEGIN_COMMAND(stopnetdemo){
	if(netdemoRecord){
		CL_StopRecordingNetDemo();
	} else if(netdemoPlayback){
		CL_StopDemoPlayBack();
	}
}
END_COMMAND(stopnetdemo)

VERSION_CONTROL (cl_main_cpp, "$Id$")
