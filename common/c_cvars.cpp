// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2008 by The Odamex Team.
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
//	Command-line variables
//
//-----------------------------------------------------------------------------


#include <string.h>
#include <stdio.h>

#include "cmdlib.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "m_alloc.h"

#include "doomstat.h"
#include "c_cvars.h"
#include "d_player.h"

#include "d_netinf.h"
#include "dstrings.h"

#include "i_system.h"

bool cvar_t::m_DoNoSet = false;
bool cvar_t::m_UseCallback = false;

// denis - all this class does is delete the cvars during its static destruction
class ad_t {
public:
	cvar_t *&GetCVars() { static cvar_t *CVars; return CVars; }
	ad_t() {}
	~ad_t()
	{
		cvar_t *cvar = GetCVars();
		while (cvar)
		{
			cvar_t *next = cvar->GetNext();
			if(cvar->m_Flags & CVAR_AUTO)
				delete cvar;
			cvar = next;
		}
	}
}ad;

cvar_t* GetFirstCvar(void)
{
	return ad.GetCVars();
}

int cvar_defflags;

cvar_t::cvar_t (const char *var_name, const char *def, DWORD flags)
{
	InitSelf (var_name, def, flags, NULL);
}

cvar_t::cvar_t (const char *var_name, const char *def, DWORD flags, void (*callback)(cvar_t &))
{
	InitSelf (var_name, def, flags, callback);
}

void cvar_t::InitSelf (const char *var_name, const char *def, DWORD var_flags, void (*callback)(cvar_t &))
{
	cvar_t *var, *dummy;

	var = FindCVar (var_name, &dummy);

	m_Callback = callback;
	m_String = "";
	m_Value = 0.0f;
	m_Flags = 0;
	m_LatchedString = "";

	if (def)
		m_Default = def;
	else
		m_Default = "";

	if (var_name)
	{
		C_AddTabCommand (var_name);
		m_Name = var_name;
		m_Next = ad.GetCVars();
		ad.GetCVars() = this;
	}
	else
		m_Name = "";

	if (var)
	{
		ForceSet (var->m_String.c_str());
		if (var->m_Flags & CVAR_AUTO)
			delete var;
		else
			var->~cvar_t();
	}
	else if (def)
		ForceSet (def);

	m_Flags = var_flags | CVAR_ISDEFAULT;
}

cvar_t::~cvar_t ()
{
	if (m_Name.length())
	{
		cvar_t *var, *dummy = NULL;

		var = FindCVar (m_Name.c_str(), &dummy);

		if (var == this)
		{
			if (dummy)
				dummy->m_Next = m_Next;
			else
				ad.GetCVars() = m_Next;
		}
	}
}

void cvar_t::ForceSet (const char *val)
{
	if (m_Flags & CVAR_LATCH)
	{
		m_Flags |= CVAR_MODIFIED;
		if(val)
			m_LatchedString = val;
		else
			m_LatchedString = "";
	}
	else
	{
		m_Flags |= CVAR_MODIFIED;
		if(val)
			m_String = val;
		else
			m_String = "";
		m_Value = atof (val);

		if (m_Flags & CVAR_USERINFO)
			D_UserInfoChanged (this);
		if (m_Flags & CVAR_SERVERINFO)
			D_SendServerInfoChange (this, val);

		if (m_UseCallback)
			Callback ();
	}
	m_Flags &= ~CVAR_ISDEFAULT;
}

void cvar_t::ForceSet (float val)
{
	char string[32];

	sprintf (string, "%g", val);
	Set (string);
}

void cvar_t::Set (const char *val)
{
	if (!(m_Flags & CVAR_NOSET) || !m_DoNoSet)
		ForceSet (val);
}

void cvar_t::Set (float val)
{
	if (!(m_Flags & CVAR_NOSET) || !m_DoNoSet)
		ForceSet (val);
}

void cvar_t::SetDefault (const char *val)
{
	if(val)
		m_Default = val;
	else
		m_Default = "";

	if (m_Flags & CVAR_ISDEFAULT)
	{
		Set (val);
		m_Flags |= CVAR_ISDEFAULT;
	}
}

cvar_t *cvar_t::cvar_set (const char *var_name, const char *val)
{
	cvar_t *var, *dummy;

	if ( (var = FindCVar (var_name, &dummy)) )
		var->Set (val);

	return var;
}

cvar_t *cvar_t::cvar_forceset (const char *var_name, const char *val)
{
	cvar_t *var, *dummy;

	if ( (var = FindCVar (var_name, &dummy)) )
		var->ForceSet (val);

	return var;
}

void cvar_t::EnableNoSet ()
{
	m_DoNoSet = true;
}

void cvar_t::EnableCallbacks ()
{
	m_UseCallback = true;
	cvar_t *cvar = ad.GetCVars();

	while (cvar)
	{
		cvar->Callback ();
		cvar = cvar->m_Next;
	}
}

static int STACK_ARGS sortcvars (const void *a, const void *b)
{
	return strcmp (((*(cvar_t **)a))->name(), ((*(cvar_t **)b))->name());
}

void cvar_t::FilterCompactCVars (TArray<cvar_t *> &cvars, DWORD filter)
{
	cvar_t *cvar = ad.GetCVars();
	while (cvar)
	{
		if (cvar->m_Flags & filter)
			cvars.Push (cvar);
		cvar = cvar->m_Next;
	}
	if (cvars.Size () > 0)
	{
		qsort (&cvars[0], cvars.Size (), sizeof(cvar_t *), sortcvars);
	}
}

void cvar_t::C_WriteCVars (byte **demo_p, DWORD filter, bool compact)
{
	cvar_t *cvar = ad.GetCVars();
	byte *ptr = *demo_p;

	if (compact)
	{
		TArray<cvar_t *> cvars;
		ptr += sprintf ((char *)ptr, "\\\\%ux", (unsigned int)filter);
		FilterCompactCVars (cvars, filter);
		while (cvars.Pop (cvar))
		{
			ptr += sprintf ((char *)ptr, "\\%s", cvar->cstring());
		}
	}
	else
	{
		cvar = ad.GetCVars();
		while (cvar)
		{
			if (cvar->m_Flags & filter)
			{
				ptr += sprintf ((char *)ptr, "\\%s\\%s",
								cvar->name(), cvar->cstring());
			}
			cvar = cvar->m_Next;
		}
	}

	*demo_p = ptr + 1;
}

void cvar_t::C_ReadCVars (byte **demo_p)
{
	char *ptr = *((char **)demo_p);
	char *breakpt;

	if (*ptr++ != '\\')
		return;

	if (*ptr == '\\')
	{	// compact mode
		TArray<cvar_t *> cvars;
		cvar_t *cvar;
		DWORD filter;

		ptr++;
		breakpt = strchr (ptr, '\\');
		*breakpt = 0;
		filter = strtoul (ptr, NULL, 16);
		*breakpt = '\\';
		ptr = breakpt + 1;

		FilterCompactCVars (cvars, filter);

		while (cvars.Pop (cvar))
		{
			breakpt = strchr (ptr, '\\');
			if (breakpt)
				*breakpt = 0;
			cvar->Set (ptr);
			if (breakpt)
			{
				*breakpt = '\\';
				ptr = breakpt + 1;
			}
			else
				break;
		}
	}
	else
	{
		char *value;

		while ( (breakpt = strchr (ptr, '\\')) )
		{
			*breakpt = 0;
			value = breakpt + 1;
			if ( (breakpt = strchr (value, '\\')) )
				*breakpt = 0;

			cvar_set (ptr, value);

			*(value - 1) = '\\';
			if (breakpt)
			{
				*breakpt = '\\';
				ptr = breakpt + 1;
			}
			else
			{
				break;
			}
		}
	}
	*demo_p += strlen (*((char **)demo_p)) + 1;
}

static struct backup_s
{
	std::string name, string;
} CVarBackups[MAX_DEMOCVARS];

static int numbackedup = 0;

void cvar_t::C_BackupCVars (void)
{
	struct backup_s *backup = CVarBackups;
	cvar_t *cvar = ad.GetCVars();

	while (cvar)
	{
		if (((cvar->m_Flags & CVAR_DEMOSAVE)) && !(cvar->m_Flags & CVAR_LATCH))
		{
			if (backup == &CVarBackups[MAX_DEMOCVARS])
				I_Error ("C_BackupDemoCVars: Too many cvars to save (%d)", MAX_DEMOCVARS);
			backup->name = cvar->m_Name;
			backup->string = cvar->m_String;
			backup++;
		}
		cvar = cvar->m_Next;
	}
	numbackedup = backup - CVarBackups;
}

void cvar_t::C_RestoreCVars (void)
{
	struct backup_s *backup = CVarBackups;
	int i;

	for (i = numbackedup; i; i--, backup++)
	{
		cvar_set (backup->name.c_str(), backup->string.c_str());
		backup->name = backup->string = "";
	}
	numbackedup = 0;
}

cvar_t *cvar_t::FindCVar (const char *var_name, cvar_t **prev)
{
	cvar_t *var;

	if (var_name == NULL)
		return NULL;

	var = ad.GetCVars();
	*prev = NULL;
	while (var)
	{
		if (stricmp (var->m_Name.c_str(), var_name) == 0)
			break;
		*prev = var;
		var = var->m_Next;
	}
	return var;
}

void cvar_t::UnlatchCVars (void)
{
	cvar_t *var;

	var = ad.GetCVars();
	while (var)
	{
		if (var->m_Flags & (CVAR_MODIFIED | CVAR_LATCH))
		{
			unsigned oldflags = var->m_Flags & ~CVAR_MODIFIED;
			var->m_Flags &= ~(CVAR_LATCH);
			if (var->m_LatchedString.length())
			{
				var->Set (var->m_LatchedString.c_str());
				var->m_LatchedString = "";
			}
			var->m_Flags = oldflags;
		}
		var = var->m_Next;
	}
}

void cvar_t::C_SetCVarsToDefaults (void)
{
	cvar_t *cvar = ad.GetCVars();

	while (cvar)
	{
		// Only default save-able cvars
		if (cvar->m_Flags & CVAR_ARCHIVE)
			if(cvar->m_Default.length())
			cvar->Set (cvar->m_Default.c_str());
		cvar = cvar->m_Next;
	}
}

void cvar_t::C_ArchiveCVars (void *f)
{
	cvar_t *cvar = ad.GetCVars();

	while (cvar)
	{
		if (cvar->m_Flags & CVAR_ARCHIVE)
		{
			fprintf ((FILE *)f, "set %s \"%s\"\n", cvar->name(), cvar->cstring());
		}
		cvar = cvar->m_Next;
	}
}

void cvar_t::cvarlist()
{
	cvar_t *var = ad.GetCVars();
	int count = 0;

	while (var)
	{
		unsigned flags = var->m_Flags;

		count++;
		Printf (PRINT_HIGH, "%c%c%c%c %s \"%s\"\n",
				flags & CVAR_ARCHIVE ? 'A' : ' ',
				flags & CVAR_USERINFO ? 'U' : ' ',
				flags & CVAR_SERVERINFO ? 'S' : ' ',
				flags & CVAR_NOSET ? '-' :
					flags & CVAR_LATCH ? 'L' :
					flags & CVAR_UNSETTABLE ? '*' : ' ',
				var->name(),
				var->cstring());
		var = var->m_Next;
	}
	Printf (PRINT_HIGH, "%d cvars\n", count);
}

BEGIN_COMMAND (set)
{
	if (argc != 3)
	{
		Printf (PRINT_HIGH, "usage: set <variable> <value>\n");
	}
	else
	{
		cvar_t *var, *prev;

		var = cvar_t::FindCVar (argv[1], &prev);
		if (!var)
			var = new cvar_t (argv[1], NULL, CVAR_AUTO | CVAR_UNSETTABLE | cvar_defflags);

		if (var->flags() & CVAR_NOSET)
			Printf (PRINT_HIGH, "%s is write protected.\n", argv[1]);
		else if (!serverside && (var->flags() & CVAR_SERVERINFO))
		{
			Printf (PRINT_HIGH, "%s is under server control and hasn't been changed.\n", argv[1]);
			return;
		}
		else if (var->flags() & CVAR_LATCH)
		{
			if(strcmp(var->cstring(), argv[2])) // if different from current value
				if(strcmp(var->latched(), argv[2])) // and if different from latched value
					Printf (PRINT_HIGH, "%s will be changed for next game.\n", argv[1]);
		}

        // [Russell] - Allow the user to specify either 'enable' and 'disable',
        // this will get converted to either 1 or 0
        if (!strcmp("enabled", argv[2]) && !(var->flags() & CVAR_NOENABLEDISABLE))
        {
            var->Set(1.0);

            return;
        }
        else if (!strcmp("disabled", argv[2]) && !(var->flags() & CVAR_NOENABLEDISABLE))
        {
            var->Set(0.0);

            return;
        }

		var->Set (argv[2]);
	}
}
END_COMMAND (set)

BEGIN_COMMAND (get)
{
	cvar_t *var, *prev;

	if (argc >= 2)
	{
		if ( (var = cvar_t::FindCVar (argv[1], &prev)) )
		{
			// [Russell] - Don't make the user feel inadequate, tell
			// them its either enabled, disabled or its other value
			if (var->cstring()[0] == '1')
                Printf (PRINT_HIGH, "\"%s\" is enabled.\n", var->name());
            else if (var->cstring()[0] == '0')
                Printf (PRINT_HIGH, "\"%s\" is disabled.\n", var->name());
            else
                Printf (PRINT_HIGH, "\"%s\" is \"%s\"\n", var->name(), var->cstring());
		}
		else
		{
			Printf (PRINT_HIGH, "\"%s\" is unset\n", argv[1]);
		}
	}
	else
	{
		Printf (PRINT_HIGH, "get: need variable name\n");
	}
}
END_COMMAND (get)

BEGIN_COMMAND (toggle)
{
	cvar_t *var, *prev;

	if (argc > 1)
	{
		if ( (var = cvar_t::FindCVar (argv[1], &prev)) )
		{
			var->Set ((float)(!var->value()));

			// [Russell] - Don't make the user feel inadequate, tell
			// them its either enabled, disabled or its other value
			if (var->cstring()[0] == '1')
                Printf (PRINT_HIGH, "\"%s\" is enabled.\n", var->name());
            else if (var->cstring()[0] == '0')
                Printf (PRINT_HIGH, "\"%s\" is disabled.\n", var->name());
            else
                Printf (PRINT_HIGH, "\"%s\" is \"%s\"\n", var->name(), var->cstring());
		}
	}
}
END_COMMAND (toggle)

BEGIN_COMMAND (cvarlist)
{
	cvar_t::cvarlist();
}
END_COMMAND (cvarlist)

VERSION_CONTROL (c_cvars_cpp, "$Id$")

