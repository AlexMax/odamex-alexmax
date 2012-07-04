// Emacs style mode select   -*- C++ -*-

#ifndef __L_CORE_H__
#define __L_CORE_H__

#include <string>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

class DLuaState
{
private:
	lua_State* L;
	static void* alloc(void *ud, void *ptr, size_t osize, size_t nsize);
public:
	DLuaState();
	~DLuaState();
	int loadbuffer(const std::string& buff, const std::string& name);
	void openlibs();
	int pcall(int nargs, int nresults, int errfunc);
	void pop(int n);
	std::string tostring(int index);
};

void L_Init();

#endif
