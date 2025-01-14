#ifndef __MAIN_H__
#define __MAIN_H__ 1

#include <windows.h>
#include <stdio.h>
#include <string>
#include <time.h>
#include <math.h>
#include <list>
#include <map>
#include <algorithm>
#include <FLHook.h>
#include <plugin.h>
#include <PluginUtilities.h>

using namespace std;


#pragma pack(push, 1)
struct Tax
{
	uint targetId;
	uint initiatorId;
	std::wstring target;
	std::wstring initiator;
	int cash;
	bool f1;
};
#pragma pack(pop)

#endif
