#include "cplhook.h"
#include <cassert>
#include <mutex>
#include <set>
#include <unordered_map>
#include "debugger/facade.h"
#include "util/util.h"
#include <ShlObj.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include "io.h"
#include "transporter/socket_server_transporter.h"

#include "shme.h"


int StartupHookMode(void* lpParam)
{
	return 0;
}

void FindAndHook()
{
}