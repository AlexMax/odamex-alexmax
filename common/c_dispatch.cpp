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
//	Argument processing (?)
//
//-----------------------------------------------------------------------------

#if _MSC_VER == 1200
// MSVC6, disable broken warnings about truncated stl lines
#pragma warning(disable:4786)
#endif

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include "doomtype.h"
#include "cmdlib.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "m_argv.h"
#include "doomstat.h"
#include "m_alloc.h"
#include "d_player.h"
#include "r_defs.h"
#include "i_system.h"

IMPLEMENT_CLASS (DConsoleCommand, DObject)
IMPLEMENT_CLASS (DConsoleAlias, DConsoleCommand)

EXTERN_CVAR (lookspring)

typedef std::map<std::string, DConsoleCommand *> command_map_t;
command_map_t &Commands()
{
	static command_map_t _Commands;
	return _Commands;
}

struct ActionBits actionbits[NUM_ACTIONS] =
{
	{ 0x00409, ACTION_USE,		  "use" },
	{ 0x0074d, ACTION_BACK,		  "back" },
	{ 0x007e4, ACTION_LEFT,		  "left" },
	{ 0x00816, ACTION_JUMP,		  "jump" },
	{ 0x0106d, ACTION_KLOOK,	  "klook" },
	{ 0x0109d, ACTION_MLOOK,	  "mlook" },
	{ 0x010d8, ACTION_RIGHT,	  "right" },
	{ 0x0110a, ACTION_SPEED,	  "speed" },
	{ 0x01fc5, ACTION_ATTACK,	  "attack" },
	{ 0x021ae, ACTION_LOOKUP,	  "lookup" },
	{ 0x021fe, ACTION_MOVEUP,	  "moveup" },
	{ 0x02315, ACTION_STRAFE,	  "strafe" },
	{ 0x041c4, ACTION_FORWARD,	  "forward" },
	{ 0x08788, ACTION_LOOKDOWN,	  "lookdown" },
	{ 0x088c4, ACTION_MOVELEFT,	  "moveleft" },
	{ 0x088c8, ACTION_MOVEDOWN,	  "movedown" },
	{ 0x11268, ACTION_MOVERIGHT,  "moveright" },
	{ 0x2314d, ACTION_SHOWSCORES, "showscores" }
};
byte Actions[NUM_ACTIONS];

static int ListActionCommands (void)
{
	int i;

	for (i = 0; i < NUM_ACTIONS; i++)
	{
		Printf (PRINT_HIGH, "+%s\n", actionbits[i].name);
		Printf (PRINT_HIGH, "-%s\n", actionbits[i].name);
	}
	return NUM_ACTIONS * 2;
}

unsigned int MakeKey (const char *s)
{
	register unsigned int v = 0;

	if (*s)
		v = tolower(*s++);
	if (*s)
		v = (v*3) + tolower(*s++);
	while (*s)
		v = (v << 1) + tolower(*s++);

	return v;
}

// GetActionBit scans through the actionbits[] array
// for a matching key and returns an index or -1 if
// the key could not be found. This uses binary search,
// actionbits[] must be sorted in ascending order.

int GetActionBit (unsigned int key)
{
	int min = 0;
	int max = NUM_ACTIONS - 1;

	while (min <= max)
	{
		int mid = (min + max) / 2;
		unsigned int seekey = actionbits[mid].key;

		if (seekey == key)
			return actionbits[mid].index;
		else if (seekey < key)
			min = mid + 1;
		else
			max = mid - 1;
	}

	return -1;
}

bool safemode = false;

void C_DoCommand (const char *cmd)
{
	size_t argc, argsize;
	char **argv;
	char *args, *arg, *realargs;
	const char *data;
	DConsoleCommand *com;
	int check = -1;

	data = ParseString (cmd);
	if (!data)
		return;

	// Check if this is an action
	if (*com_token == '+')
	{
		check = GetActionBit (MakeKey (com_token + 1));
		//if (Actions[check] < 255)
		//	Actions[check]++;
		if (check != -1)
			Actions[check] = 1;
	}
	else if (*com_token == '-')
	{
		check = GetActionBit (MakeKey (com_token + 1));
		//if (Actions[check])
		//	Actions[check]--;
		if (check != -1)
			Actions[check] = 0;

		if ((check == ACTION_LOOKDOWN || check == ACTION_LOOKUP || check == ACTION_MLOOK) && lookspring)
			AddCommandString ("centerview");
	}

	// Check if this is a normal command
	if (check == -1)
	{
		argc = 1;
		argsize = strlen (com_token) + 1;

		realargs = new char[strlen (data) + 1];
		strcpy (realargs, data);

		while ( (data = ParseString (data)) )
		{
			argc++;
			argsize += strlen (com_token) + 1;
		}

		args = new char[argsize];
		argv = new char *[argc];

		arg = args;
		data = cmd;
		argsize = 0;
		while ( (data = ParseString (data)) )
		{
			strcpy (arg, com_token);
			argv[argsize] = arg;
			arg += strlen (arg);
			*arg++ = 0;
			argsize++;
		}

		// Checking for matching commands follows this search order:
		//	1. Check the Commands map
		//	2. Check the CVars list
		command_map_t::iterator c = Commands().find(StdStringToLower(argv[0]));

		if (c != Commands().end())
		{
			com = c->second;

			if(!safemode
			|| stricmp(argv[0], "if")==0
			|| stricmp(argv[0], "exec")==0)
			{
				com->argc = argc;
				com->argv = argv;
				com->args = realargs;
				com->m_Instigator = consoleplayer().mo;
				com->Run ();
			}
			else
			{
				Printf (PRINT_HIGH, "Not a cvar command \"%s\"\n", argv[0]);
			}
		}
		else
		{
			// Check for any CVars that match the command
			cvar_t *var, *dummy;

			if ( (var = cvar_t::FindCVar (argv[0], &dummy)) )
			{
				if (argc >= 2)
				{
					c = Commands().find("set");

					if(c != Commands().end())
					{
						com = c->second;

						com->argc = argc + 1;
						com->argv = argv - 1;	// Hack
						com->m_Instigator = consoleplayer().mo;
						com->Run ();
					}
					else Printf (PRINT_HIGH, "set command not found\n");
				}
                // [Russell] - Don't make the user feel inadequate, tell
                // them its either enabled, disabled or its other value
                else if (var->cstring()[0] == '1' && !(var->m_Flags & CVAR_NOENABLEDISABLE))
                    Printf (PRINT_HIGH, "\"%s\" is enabled.\n", var->name());
                else if (var->cstring()[0] == '0' && !(var->m_Flags & CVAR_NOENABLEDISABLE))
                    Printf (PRINT_HIGH, "\"%s\" is disabled.\n", var->name());
                else
                    Printf (PRINT_HIGH, "\"%s\" is \"%s\"\n", var->name(), var->cstring());
			}
			else
			{
				// We don't know how to handle this command
				Printf (PRINT_HIGH, "Unknown command \"%s\"\n", argv[0]);
			}
		}
		delete[] argv;
		delete[] args;
		delete[] realargs;
	}
}

void AddCommandString (std::string cmd, bool onlycvars)
{
	char *brkpt;
	int more;

	if (!cmd.length())
		return;

	char *allocated = new char[cmd.length() + 1]; // denis - fixme - this ugly fix is required because in some places, the function is called with a string that must not be modified, eg AddCommandString("hello"); - please rewrite - should have a const parameter
	char *copy = allocated;
	memcpy(copy, cmd.c_str(), cmd.length() + 1);

	while (*copy)
	{
		brkpt = copy;
		while (*brkpt != ';' && *brkpt != '\0')
		{
			if (*brkpt == '\"')
			{
				brkpt++;
				while (*brkpt != '\"' && *brkpt != '\0')
					brkpt++;
			}
			brkpt++;
		}
		if (*brkpt == ';')
		{
			*brkpt = '\0';
			more = 1;
		}
		else
		{
			more = 0;
		}

		safemode |= onlycvars;

		C_DoCommand (copy);

		if(onlycvars)
			safemode = false;

		if (more)
		{
			*brkpt = ';';
		}
		copy = brkpt + more;
	}

	delete[] allocated;
}

#define MAX_EXEC_DEPTH 32

static bool if_command_result;

BEGIN_COMMAND (exec)
{
	if (argc < 2)
		return;

	static std::vector<std::string> exec_stack;
	static std::vector<bool>	tag_stack;

	if(std::find(exec_stack.begin(), exec_stack.end(), argv[1]) != exec_stack.end())
	{
		Printf (PRINT_HIGH, "Ignoring recursive exec \"%s\"\n", argv[1]);
		return;
	}

	if(exec_stack.size() >= MAX_EXEC_DEPTH)
	{
		Printf (PRINT_HIGH, "Ignoring recursive exec \"%s\"\n", argv[1]);
		return;
	}

	std::ifstream ifs(argv[1]);

	if(ifs.fail())
	{
		Printf (PRINT_HIGH, "Could not open \"%s\"\n", argv[1]);
		return;
	}

	exec_stack.push_back(argv[1]);

	while(ifs)
	{
		std::string line;
		std::getline(ifs, line);

		if(!line.length())
			continue;

        size_t QuoteIndex = line.find_first_of('"');
        size_t CommentIndex = line.find_first_of("//");

		// commented line
		if(line.length() > 1 &&
           ((line[0] == '/' && line[1] == '/') ||
           (QuoteIndex > CommentIndex)))
        {
            continue;
        }

		// start tag
		if(line.substr(0, 3) == "#if")
		{
			AddCommandString(line.c_str() + 1);
			tag_stack.push_back(if_command_result);

			continue;
		}

		// else tag
		if(line.substr(0, 5) == "#else")
		{
			if(tag_stack.empty())
				Printf(PRINT_HIGH, "Ignoring stray #else\n");
			else
				tag_stack.back() = !tag_stack.back();

			continue;
		}

		// end tag
		if(line.substr(0, 6) == "#endif")
		{
			if(tag_stack.empty())
				Printf(PRINT_HIGH, "Ignoring stray #endif\n");
			else
				tag_stack.pop_back();

			continue;
		}

		// inside tag that evaluated false?
		if(!tag_stack.empty() && !tag_stack.back())
			continue;

		AddCommandString(line);
	}

	exec_stack.pop_back();
}
END_COMMAND (exec)

// denis
// if cvar eq blah "command";
BEGIN_COMMAND (if)
{
	if_command_result = false;

	if (argc < 4)
		return;

	cvar_t *var, *dummy;
	var = cvar_t::FindCVar (argv[1], &dummy);

	if (!var)
	{
		Printf(PRINT_HIGH, "if: no cvar named %s\n", argv[1]);
		return;
	}

	std::string op = argv[2];

	if(op == "eq")
	{
		if_command_result = !strcmp(var->cstring(), argv[3]);
	}
	else if(op == "ne")
	{
		if_command_result = strcmp(var->cstring(), argv[3]);
	}
	else
	{
		Printf(PRINT_HIGH, "if: no operator %s\n", argv[2]);
		Printf(PRINT_HIGH, "if: operators are eq, ne\n");
		return;
	}

	if(if_command_result && argc > 4)
	{
		std::string param = BuildString (argc - 4, (const char **)&argv[4]);
		AddCommandString(param);
	}
}
END_COMMAND (if)

// ParseString2 is adapted from COM_Parse
// found in the Quake2 source distribution
const char *ParseString2 (const char *data)
{
	int c;
	int len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
		{
			return NULL;			// end of string encountered
		}
		data++;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1) {
			c = *data++;
			if (c == '\"' || c == '\0')
			{
				if (c == '\0')
					data--;
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse a regular word
	do {
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	} while (c>32);

	com_token[len] = 0;
	return data;
}

// ParseString calls ParseString2 to remove the first
// token from an input string. If this token is of
// the form $<cvar>, it will be replaced by the
// contents of <cvar>.
const char *ParseString (const char *data)
{
	cvar_t *var, *dummy;

	if ( (data = ParseString2 (data)) )
	{
		if (com_token[0] == '$')
		{
			if ( (var = cvar_t::FindCVar (&com_token[1], &dummy)) )
			{
				strcpy (com_token, var->cstring());
			}
		}
	}
	return data;
}

DConsoleCommand::DConsoleCommand (const char *name)
{
	static bool firstTime = true;

	if (firstTime)
	{
		char tname[16];
		int i;

		firstTime = false;

		// Add all the action commands for tab completion
		for (i = 0; i < NUM_ACTIONS; i++)
		{
			strcpy (&tname[1], actionbits[i].name);
			tname[0] = '+';
			C_AddTabCommand (tname);
			tname[0] = '-';
			C_AddTabCommand (tname);
		}
	}

	m_Name = name;

	Commands()[name] = this;
	C_AddTabCommand(name);
}

DConsoleCommand::~DConsoleCommand ()
{
	C_RemoveTabCommand (m_Name.c_str());
}

DConsoleAlias::DConsoleAlias (const char *name, const char *command)
	:	DConsoleCommand(StdStringToLower(name).c_str()),  state_lock(false),
		m_Command(command)
{
}

DConsoleAlias::~DConsoleAlias ()
{
}

void DConsoleAlias::Run()
{
	if(!state_lock)
	{
		state_lock = true;

        m_CommandParam = m_Command;

		// [Russell] - Allows for aliases with parameters
		if (argc > 1)
        {
            for (size_t i = 1; i < argc; i++)
            {
                m_CommandParam += " ";
                m_CommandParam += argv[i];
            }
        }

        AddCommandString (m_CommandParam.c_str());

		state_lock = false;
	}
	else
	{
		Printf(PRINT_HIGH, "warning: ignored recursive alias");
	}
}

std::string BuildString (size_t argc, const char **argv)
{
	std::string out;

	for(size_t i = 0; i < argc; i++)
	{
		if(strchr(argv[i], ' '))
		{
			out += "\"";
			out += argv[i];
			out += "\"";
		}
		else
		{
			out += argv[i];
		}

		if(i + 1 < argc)
		{
			out += " ";
		}
	}

	return out;
}

std::string BuildString (size_t argc, std::vector<std::string> args)
{
	std::string out;

	for(size_t i = 0; i < argc; i++)
	{
		if(args[i].find_first_of(' ') != std::string::npos)
		{
			out += "\"";
			out += args[i];
			out += "\"";
		}
		else
		{
			out += args[i];
		}

		if(i + 1 < argc)
		{
			out += " ";
		}
	}

	return out;
}

static int DumpHash (BOOL aliases)
{
	int count = 0;

	for (command_map_t::iterator i = Commands().begin(), e = Commands().end(); i != e; ++i)
	{
		DConsoleCommand *cmd = i->second;

		count++;
		if (cmd->IsAlias())
		{
			if (aliases)
				static_cast<DConsoleAlias *>(cmd)->PrintAlias ();
		}
		else if (!aliases)
			cmd->PrintCommand ();
	}

	return count;
}

void DConsoleAlias::Archive (FILE *f)
{
	fprintf (f, "alias \"%s\" \"%s\"\n", m_Name.c_str(), m_Command.c_str());
}

void DConsoleAlias::C_ArchiveAliases (FILE *f)
{
	for (command_map_t::iterator i = Commands().begin(), e = Commands().end(); i != e; ++i)
	{
		DConsoleCommand *alias = i->second;

		if (alias->IsAlias())
			static_cast<DConsoleAlias *>(alias)->Archive (f);
	}
}

BEGIN_COMMAND (alias)
{
	if (argc == 1)
	{
		Printf (PRINT_HIGH, "Current alias commands:\n");
		DumpHash (true);
	}
	else
	{
		command_map_t::iterator i = Commands().find(StdStringToLower(argv[1]));

		if(i != Commands().end())
		{
			if(i->second->IsAlias())
			{
				// Remove the old alias
				delete i->second;
				Commands().erase(i);
			}
			else
			{
				Printf(PRINT_HIGH, "%s: is a command, can not become an alias\n", argv[1]);
				return;
			}
		}
		else if(argc == 2)
		{
			Printf(PRINT_HIGH, "%s: not an alias\n", argv[1]);
			return;
		}

		if(argc > 2)
		{
			// Build the new alias
			std::string param = BuildString (argc - 2, (const char **)&argv[2]);
			new DConsoleAlias (argv[1], param.c_str());
		}
	}
}
END_COMMAND (alias)

BEGIN_COMMAND (cmdlist)
{
	int count;

	count = ListActionCommands ();
	count += DumpHash (false);
	Printf (PRINT_HIGH, "%d commands\n", count);
}
END_COMMAND (cmdlist)

BEGIN_COMMAND (key)
{
	if (argc > 1)
	{
		while (argc > 1)
		{
			Printf (PRINT_HIGH, " %08x", MakeKey (argv[1]));
			argc--;
			argv++;
		}
		Printf (PRINT_HIGH, "\n");
	}
}
END_COMMAND (key)

// Execute any console commands specified on the command line.
// These all begin with '+' as opposed to '-'.
// If onlyset is true, only "set" commands will be executed,
// otherwise only non-"set" commands are executed.
// If onlylogfile is true... well, you get the point.
void C_ExecCmdLineParams (bool onlyset, bool onlylogfile)
{
	size_t cmdlen, argstart;
	int didlogfile = 0;

	for (size_t currArg = 1; currArg < Args.NumArgs(); )
	{
		if (*Args.GetArg (currArg++) == '+')
		{
			int setComp = stricmp (Args.GetArg (currArg - 1) + 1, "set");
			int logfileComp = stricmp (Args.GetArg (currArg - 1) + 1, "logfile");
			if ((onlyset && setComp) || (onlylogfile && logfileComp) ||
                (!onlyset && !setComp) || (!onlylogfile && !logfileComp))
			{
				continue;
			}

			cmdlen = 1;
			argstart = currArg - 1;

			while (currArg < Args.NumArgs())
			{
				if (*Args.GetArg (currArg) == '-' || *Args.GetArg (currArg) == '+')
					break;
				currArg++;
				cmdlen++;
			}

			std::string cmdString = BuildString (cmdlen, Args.GetArgList(argstart));
			if (cmdString.length()) {
				C_DoCommand (cmdString.c_str() + 1);
				if (onlylogfile) didlogfile = 1;
			}
		}
	}

    // [Nes] - Calls version at startup if no logfile.
	if (onlylogfile && !didlogfile) AddCommandString("version");
}

BEGIN_COMMAND (dumpactors)
{
	AActor *mo;
	TThinkerIterator<AActor> iterator;
	Printf (PRINT_HIGH, "Actors at level.time == %d:\n", level.time);
	while ( (mo = iterator.Next ()) )
	{
		Printf (PRINT_HIGH, "%s (%x, %x, %x | %x) state: %d tics: %d\n", mobjinfo[mo->type].name, mo->x, mo->y, mo->z, mo->angle, mo->state - states, mo->tics);
	}
}
END_COMMAND (dumpactors)

BEGIN_COMMAND (logfile)
{
	time_t rawtime;
	struct tm * timeinfo;
	const char* DEFAULT_LOG_FILE = (serverside ? "odasrv.log" : "odamex.log");

	if (LOG.is_open()) {
		if ((argc == 1 && LOG_FILE == DEFAULT_LOG_FILE) || (argc > 1 && LOG_FILE == argv[1])) {
			Printf (PRINT_HIGH, "Log file %s already in use\n", LOG_FILE);
			return;
		}

    	time (&rawtime);
    	timeinfo = localtime (&rawtime);
    	Printf (PRINT_HIGH, "Log file %s closed on %s\n", LOG_FILE, asctime (timeinfo));
		LOG.close();
	}

	LOG_FILE = (argc > 1 ? argv[1] : DEFAULT_LOG_FILE);
	LOG.open (LOG_FILE, std::ios::app);

	if (!LOG.is_open())
		Printf (PRINT_HIGH, "Unable to create logfile: %s\n", LOG_FILE);
	else {
		time (&rawtime);
    	timeinfo = localtime (&rawtime);
    	LOG.flush();
    	LOG << std::endl;
		Printf (PRINT_HIGH, "Logging in file %s started %s\n", LOG_FILE, asctime (timeinfo));
    }
}
END_COMMAND (logfile)

BEGIN_COMMAND (stoplog)
{
	time_t rawtime;
	struct tm * timeinfo;

	if (LOG.is_open()) {
		time (&rawtime);
    	timeinfo = localtime (&rawtime);
		Printf (PRINT_HIGH, "Logging to file %s stopped %s\n", LOG_FILE, asctime (timeinfo));
		LOG.close();
	}
}
END_COMMAND (stoplog)

bool P_StartScript (AActor *who, line_t *where, int script, char *map, int lineSide,
					int arg0, int arg1, int arg2, int always);

BEGIN_COMMAND (puke)
{
	if (argc < 2 || argc > 5) {
		Printf (PRINT_HIGH, " puke <script> [arg1] [arg2] [arg3]\n");
	} else {
		int script = atoi (argv[1]);
		int arg0=0, arg1=0, arg2=0;

		if (argc > 2) {
			arg0 = atoi (argv[2]);
			if (argc > 3) {
				arg1 = atoi (argv[3]);
				if (argc > 4) {
					arg2 = atoi (argv[4]);
				}
			}
		}
		P_StartScript (m_Instigator, NULL, script, level.mapname, 0, arg0, arg1, arg2, false);
	}
}
END_COMMAND (puke)

BEGIN_COMMAND (error)
{
	std::string text = BuildString (argc - 1, (const char **)(argv + 1));
	I_Error (text.c_str());
}
END_COMMAND (error)

VERSION_CONTROL (c_dispatch_cpp, "$Id$")

