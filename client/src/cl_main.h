// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
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
//	CL_MAIN
//
//-----------------------------------------------------------------------------

#ifndef __I_CLMAIN_H__
#define __I_CLMAIN_H__

#include "i_net.h"
#include "d_ticcmd.h"

extern netadr_t  serveraddr;
extern BOOL      connected;
extern int       connecttimeout;

extern bool      noservermsgs;
extern int       last_received;

extern buf_t     net_buffer;

//demos - NullPoint
extern bool netdemoRecord;
extern bool netdemoPlayback;

#define MAXSAVETICS 70
extern ticcmd_t localcmds[MAXSAVETICS];

extern bool predicting;

struct plat_pred_t
{
	size_t secnum;

	byte state;
	int count;
	int tic;

	unsigned long floorheight;
};

extern std::vector <plat_pred_t> real_plats;

void CL_QuitNetGame(void);
void CL_InitNetwork (void);
void CL_RequestConnectInfo(void);
bool CL_PrepareConnect(void);
void CL_ParseCommands(void);
void CL_ReadPacketHeader(void);
void CL_SendCmd(void);
void CL_SaveCmd(void);
void CL_MoveThing(AActor *mobj, fixed_t x, fixed_t y, fixed_t z);
void CL_PredictMove (void);
void CL_SendUserInfo(void);
bool CL_Connect(void);

//demos - NullPoint
void CL_BeginNetRecord(char* demoname);
void CL_WirteNetDemoMessages(buf_t netbuffer);
void CL_StopRecordingNetDemo(void);
void CL_ReadNetDemoMeassages(int* realrate);
void CL_StopDemoPlayBack(void);
void CL_StartDemoPlayBack(std::string demoname);
void CL_CaptureDeliciousPackets(buf_t netbuffer);

#endif
