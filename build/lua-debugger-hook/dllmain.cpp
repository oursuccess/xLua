#include "debugger/facade.h"
#include "cplhook.h"

#if WIN32
#include <Windows.h>
#include "shme.h"

using namespace CPL;

static SharedFile file;
HINSTANCE g_hInstance = NULL;

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD fdwReason, LPVOID lpvReserved)
{
	g_hInstance = hModule;
	DebuggerFacade::Get().SetWorkMode(WorkMode::Attach);
	DebuggerFacade::Get().StartHook = FindAndHook;

	return TRUE;
}

#endif