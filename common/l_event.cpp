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

#include "l_event.h"

EventHandler* LuaEvent = NULL;

// We use the address of this variable to ensure a unique Lua
// registry key for this library.
static const char registry_key = 'k';

// Constructor
EventHandler::EventHandler(lua_State* L) : L(L), error(OK)
{
	// Store a pointer to object in the Lua registry, so we can access it
	// from the context of lua_CFunctions later.
	lua_pushlightuserdata(this->L, static_cast<void*>(this));
	lua_rawseti(this->L, LUA_REGISTRYINDEX, registry_key);
}

// Destructor
EventHandler::~EventHandler()
{
	// Remove the object reference from the Lua registry.
	lua_pushnil(this->L);
	lua_rawseti(this->L, LUA_REGISTRYINDEX, registry_key);
}

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
		this->error = EVENT_EXIST;
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
		this->error = EVENT_NOEXIST;
		return false;
	}
	if (it->second.lua == false && lua == true)
	{
		// Attempting to fire a non-Lua event from Lua.
		this->error = EVENT_NOTLUA;
		return false;
	}
	// Call every hooked function in the list of hooks with the
	// parameters that are currently on the Lua stack.
	int n = lua_gettop(L);
	for (HooksI itr = it->second.hooks.begin();itr != it->second.hooks.end();++itr)
	{
		lua_rawgeti(this->L, LUA_REGISTRYINDEX, (*itr)->second.ref);
		for (int i = 1;i <= n;i++)
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
		this->error = EVENT_NOEXIST;
		return false;
	}
	// Create the hook.
	std::pair<HooksT::iterator, bool> hook = this->hooks.insert(HookP(hname, Hook(ref)));
	if (hook.second == false)
	{
		// Hook already exists with that name.
		this->error = HOOK_EXIST;
		return false;
	}
	// Push the newly-created hook onto the vector of hooks for the event.
	it->second.hooks.push_back(hook.first);
	return true;
}

// Returns a pointer to an EventHandler object that is attached to the
// passed Lua state, or NULL if there is no EventHandler in the state.
EventHandler* EventHandler::get(lua_State* L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, registry_key);
	if (!lua_islightuserdata(L, -1))
	{
		// The event handler doesn't exist.
		lua_pop(L, 1);
		return NULL;
	}
	EventHandler* eh = static_cast<EventHandler*>(lua_touserdata(L, -1));
	lua_pop(L, 1);
	return eh;
}

// Add a custom Lua event.
int LCmd_event_add(lua_State* L)
{
	EventHandler* LocalEvent = EventHandler::get(L);
	if (LocalEvent == NULL)
		return luaL_error(L, "EventHandler not found in lua_State.\n");
	// Parameter checking
	const char* ename = luaL_checkstring(L, 1);
	// Run the event handler method
	if (!LocalEvent->add(ename, true))
	{
		const int error = LocalEvent->getError();
		switch (error)
		{
		case EventHandler::EVENT_EXIST:
			return luaL_error(L, "Event '%s' already exists.\n", ename);
		default:
			return luaL_error(L, "Unknown error\n");
		}
	}
	return 0;
}

// Fire a custom Lua event.  Note that this Lua function is incapable of
// firing a built-in event, use EventHandler::fire() directly for that.
int LCmd_event_fire(lua_State* L)
{
	EventHandler* LocalEvent = EventHandler::get(L);
	if (LocalEvent == NULL)
		return luaL_error(L, "EventHandler not found in lua_State.\n");
	// Parameter checking
	const char* ename = luaL_checkstring(L, 1);
	// Run the event handler method
	if (!LocalEvent->fire(ename, true))
	{
		const int error = LocalEvent->getError();
		switch (error)
		{
		case EventHandler::EVENT_NOEXIST:
			return luaL_error(L, "Event '%s' does not exist.\n", ename);
		case EventHandler::EVENT_NOTLUA:
			return luaL_error(L, "Event '%s' cannot be fired from Luat.\n", ename);
		default:
			return luaL_error(L, "Unknown error\n");
		}
	}
	return 0;
}

// Remove a custom Lua event.
int LCmd_event_remove(lua_State* L)
{
	return 0;
}

// Add a named function callback to either a built-in or custom Lua event.
int LCmd_event_hook(lua_State* L)
{
	EventHandler* LocalEvent = EventHandler::get(L);
	if (LocalEvent == NULL)
		return luaL_error(L, "EventHandler not found in lua_State.\n");
	// Parameter checking
	const char* hname = luaL_checkstring(L, 1);
	const char* ename = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TFUNCTION);
	const int ref = luaL_ref(L, LUA_REGISTRYINDEX);
	// Run the event handler method
	if (!LocalEvent->hook(hname, ename, ref))
	{
		luaL_unref(L, LUA_REGISTRYINDEX, ref);
		luaL_error(L, "Something happened...\n");
	}
	return 0;
}

// Remove a named function callback to either a built-in or custom Lua event.
int LCmd_event_unhook(lua_State* L)
{
	return 0;
}

void luaopen_doom_event(lua_State* L)
{
	LuaEvent = new EventHandler(L);
	lua_pushstring(L, "event");
	{
		lua_newtable(L);
		lua_pushstring(L, "add");
		lua_pushcfunction(L, LuaCFunction<LCmd_event_add>);
		lua_rawset(L, -3);
		lua_pushstring(L, "fire");
		lua_pushcfunction(L, LuaCFunction<LCmd_event_fire>);
		lua_rawset(L, -3);
		lua_pushstring(L, "remove");
		lua_pushcfunction(L, LuaCFunction<LCmd_event_remove>);
		lua_rawset(L, -3);
		lua_pushstring(L, "hook");
		lua_pushcfunction(L, LuaCFunction<LCmd_event_hook>);
		lua_rawset(L, -3);
		lua_pushstring(L, "unhook");
		lua_pushcfunction(L, LuaCFunction<LCmd_event_hook>);
		lua_rawset(L, -3);
	}
	lua_rawset(L, -3);
}
