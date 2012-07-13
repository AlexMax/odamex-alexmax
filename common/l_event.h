// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2012 by The Odamex Team.
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
//   Event creation and hooking 'doom.event' library for Lua API.
//
//-----------------------------------------------------------------------------

#ifndef __L_EVENT_H__
#define __L_EVENT_H__

#include <map>
#include <vector>

#include "l_core.h"

struct Hook;
struct Event;

typedef std::pair<std::string, Hook> HookP;
typedef std::map<std::string, Hook> HooksT;
typedef std::vector<HooksT::iterator>::iterator HooksI;

typedef std::pair<std::string, Event> EventP;
typedef std::map<std::string, Event> EventsT;
typedef std::vector<EventsT::iterator>::iterator EventsI;

struct Hook
{
	Hook(int iref) : ref(iref) { }
	int ref;
	std::vector<EventsT::iterator> events;
};

struct Event
{
	Event(const int iresults, const bool ilua) : results(iresults), lua(ilua) { }
	int results;
	bool lua;
	std::vector<HooksT::iterator> hooks;
};

class EventHandler
{
	HooksT hooks;
	EventsT events;
	lua_State* L;
	int error;
public:
	enum
	{
		OK, EVENT_EXIST, EVENT_NOEXIST, EVENT_NOTLUA, HOOK_NOEXIST, HOOK_EXIST
	};
	EventHandler(lua_State* L);
	~EventHandler();
	bool add(const std::string& ename, const int results, const bool lua = false);
	bool fire(const std::string& ename, bool lua = false);
	bool remove(const std::string& ename, bool lua = false);
	bool hook(const std::string& hname, const std::string& ename, int ref);
	bool unhook(const std::string& hname);
	int getError()
		{
			int e = this->error;
			this->error = OK;
			return e;
		};
	static EventHandler* get(lua_State* L);
};

extern EventHandler* LuaEvent;

void L_AddEvent(lua_State* L, const char* ename, const int params);
void L_FireEvent(lua_State* L, const char* ename);
void luaopen_doom_event(lua_State* L);

#endif
