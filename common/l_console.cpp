#include <string>

#include "l_console.h"

#include "c_dispatch.h" // BEGIN_COMMAND

extern DLuaState* Lua;

// XXX: HOLY MOLY this is insecure if exposed to rcon.  Implement sanxboxing
//      and refactor, pronto.  See <http://lua-users.org/wiki/SandBoxes>.
BEGIN_COMMAND (lua)
{
	int error;
	error = Lua->loadbuffer(C_ArgCombine(argc - 1, (const char**)(argv + 1)), "console") || Lua->pcall(0, 0, 0);

	if (error)
	{
		Printf(PRINT_HIGH, "%s", Lua->tostring(-1).c_str());
		Lua->pop(1);
	}
}
END_COMMAND (lua)
