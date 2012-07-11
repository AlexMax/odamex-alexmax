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
	Event(bool ilua) : lua(ilua) { }
	bool lua;
	std::vector<HooksT::iterator> hooks;
};

class EventHandler
{
	HooksT hooks;
	EventsT events;
	lua_State* L;
public:
	EventHandler(lua_State* L);
	bool add(const std::string& ename, bool lua = false);
	bool fire(const std::string& ename, bool lua = false);
	bool remove(const std::string& ename, bool lua = false);
	bool hook(const std::string& hname, const std::string& ename, int ref);
	bool unhook(const std::string& hname);
};

// We use the address of this variable to ensure a unique Lua
// registry key for this library.
static const char registry_key = 'k';

// Add a hookable event, returns true if successful.  Second parameter is
// true if you want to be able to fire the event from the Lua API, otherwise
// the event is only firable by calling EventHandler::fire() from C++ without
// a second parameter.
bool EventHandler::add(const std::string& ename, bool lua)
{
	std::pair<EventsT::iterator, bool> event = this->events.insert(EventP(ename, Event(lua)));
	if (event.second == false)
	{
		// Event already exists.
		return false;
	}
	return true;
}

// Fire off all callbacks associated with an event, returns true if the
// event exists and we're not trying to call a non-Lua event from the Lua
// API.  This also means that it will return true if there are no hooks to
// run or if one of the hook's callback functions errors out.  Second
// parameter should be set to true if you're calling this method from the
// Lua API.  Lua parameters that you want to pass to the hooks should already
// be on the Lua stack before calling this method.
bool EventHandler::fire(const std::string& ename, bool lua)
{
	EventsT::iterator it = this->events.find(ename);
	if (it == this->events.end())
	{
		// No such event exists.
		return false;
	}
	if (it->second.lua == false && lua == true)
	{
		// Attempting to fire a non-Lua event from Lua.
		return false;
	}
	// Call every hooked function in the list of hooks with the
	// parameters that are currently on the Lua stack.
	int n = lua_gettop(L);
	for (HooksI itr = it->second.hooks.begin();itr != it->second.hooks.end();++itr)
	{
		lua_rawgeti(this->L, LUA_REGISTRYINDEX, (*itr)->second.ref);
		for (int i = 0;i <= n;i++)
			lua_pushvalue(this->L, i);
		lua_pcall(this->L, n, 0, 0);
	}
	return true;
}

// Attach a callback function (called a hook) to a named event.  Returns true
// as long as the event exists and there isn't a name collision with another
// hook.
bool EventHandler::hook(const std::string& hname, const std::string& ename, int ref)
{
	EventsT::iterator it = this->events.find(ename);
	if (it == this->events.end())
	{
		// No such event exists.
		return false;
	}
	// Create the hook.
	std::pair<HooksT::iterator, bool> hook = this->hooks.insert(HookP(hname, Hook(ref)));
	if (hook.second == false)
	{
		// Hook already exists with that name.
		return false;
	}
	// Push the newly-created hook onto the vector of hooks for the event.
	it->second.hooks.push_back(hook.first);
	return true;
}

// Add a custom Lua event.
int LCmd_add(lua_State* L)
{
	return 0;
}

// Fire a custom Lua event.  Note that this will not fire a built-in event.
int LCmd_fire(lua_State* L)
{
	return 0;
}

// Remove a custom Lua event.
int LCmd_remove(lua_State* L)
{
	return 0;
}

// Add a named function callback to either a built-in or custom Lua event.
int LCmd_hook(lua_State* L)
{
	return 0;
}

// Remove a named function callback to either a built-in or custom Lua event.
int LCmd_unhook(lua_State* L)
{
	return 0;
}

void luaopen_doom_event(lua_State* L)
{
	lua_pushstring(L, "event");
	{
		lua_newtable(L);
		lua_pushstring(L, "add");
		lua_pushcfunction(L, LuaCFunction<LCmd_add>);
		lua_rawset(L, -3);
		lua_pushstring(L, "fire");
		lua_pushcfunction(L, LuaCFunction<LCmd_fire>);
		lua_rawset(L, -3);
		lua_pushstring(L, "remove");
		lua_pushcfunction(L, LuaCFunction<LCmd_remove>);
		lua_rawset(L, -3);
		lua_pushstring(L, "hook");
		lua_pushcfunction(L, LuaCFunction<LCmd_hook>);
		lua_rawset(L, -3);
		lua_pushstring(L, "unhook");
		lua_pushcfunction(L, LuaCFunction<LCmd_hook>);
		lua_rawset(L, -3);
	}
	lua_rawset(L, -3);
}
