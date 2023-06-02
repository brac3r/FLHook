/**
 * @date Unknown
 * @author ||KOS||Acid (Ported by Raikkonen 2022)
 * @defgroup BountyHunt Bounty Hunt
 * @brief
 * The "Bounty Hunt" plugin allows players to put bounties on each other that can be collected by destroying that player.
 *
 * @paragraph cmds Player Commands
 * All commands are prefixed with '/' unless explicitly specified.
 * - bountyhunt <player> <amount> [timelimit] - Places a bounty on the specified player. When another player kills them, they gain <credits>.
 * - bountyhuntid <id> <amount> [timelimit] - Same as above but with an id instead of a player name. Use /ids
 *
 * @paragraph adminCmds Admin Commands
 * There are no admin commands in this plugin.
 *
 * @paragraph configuration Configuration
 * @code
 * {
 *     "enableBountyHunt": true,
 *     "levelProtect": 0,
 *     "minimalHuntTime": 1,
 *     "maximumHuntTime": 240,
 *     "defaultHuntTime": 30
 * }
 * @endcode
 *
 * @paragraph ipc IPC Interfaces Exposed
 * This plugin does not expose any functionality.
 *
 * @paragraph optional Optional Plugin Dependencies
 * None
 */

#include <unordered_set>
#include <FLHook.h>
#include <plugin.h>
#include <PluginUtilities.h>

struct BountyHunt
{
	uint targetId;
	uint initiatorId;
	std::wstring target;
	std::wstring initiator;
	int cash;
	mstime end;
};

std::vector<BountyHunt> bountyHunt;

bool enableBountyHunt = true;
int levelProtect = 0;
//! Minimal time a hunt can be set to, in minutes.
uint minimalHuntTime = 1;
//! Maximum time a hunt can be set to, in minutes.
uint maximumHuntTime = 240;
//! Hunt time in minutes, if not explicitly specified.
uint defaultHuntTime = 30;

PLUGIN_RETURNCODE returnCode;

/** @ingroup BountyHunt
 * @brief Removed an active bounty hunt
 */
void RemoveBountyHunt(const BountyHunt& bounty)
{
	auto it = bountyHunt.begin();
	while (it != bountyHunt.end())
	{
		if (it->targetId == bounty.targetId && it->initiatorId == bounty.initiatorId)
		{
			it = bountyHunt.erase(it);
		}
		else
		{
			++it;
		}
	}
}

/** @ingroup BountyHunt
 * @brief Print all the active bounty hunts to the player
 */
void PrintBountyHunts(uint& client)
{
	if (bountyHunt.begin() != bountyHunt.end())
	{
		PrintUserCmdText(client, L"Offered Bounty Hunts:");
		for (auto const& bounty : bountyHunt)
		{
			PrintUserCmdText(
			    client, L"Kill %ls and earn %u credits (%u minutes left)", bounty.target.c_str(), bounty.cash, (uint)((bounty.end - GetTimeInMS()) * 1000 / 60));
		}
	}
}

/** @ingroup BountyHunt
 * @brief User Command for /bountyhunt. Creates a bounty against a specified player.
 */
bool UserCmdBountyHunt(uint client, const std::wstring& cmd, const std::wstring& param, const wchar_t* usage)
{
	if (!enableBountyHunt)
		return false;

	const std::wstring target = GetParam(param, ' ', 0);
	const int prize = ToInt(GetParam(param, ' ', 1));
	uint huntTime = ToUInt(GetParam(param, ' ', 2));
	if (!target.length() || prize == 0)
	{
		PrintUserCmdText(client, L"Usage: /bountyhunt <playername> <credits> <time>");
		PrintBountyHunts(client);
		return false;
	}

	const auto targetId = HkGetClientIdFromCharname(target);

	int rankTarget;
	HkGetRank(target, rankTarget);

	if (targetId == UINT_MAX || HkIsInCharSelectMenu(targetId))
	{
		PrintUserCmdText(client, L"%ls is not online.", target.c_str());
		return false;
	}

	if (rankTarget < levelProtect)
	{
		PrintUserCmdText(client, L"Low level players may not be hunted.");
		return false;
	}

	// clamp the hunting time to configured range, or set default if not specified
	if (huntTime)
	{
		huntTime = min(maximumHuntTime, max(minimalHuntTime, huntTime));
	}
	else
	{
		huntTime = defaultHuntTime;
	}

	int clientCash;
	pub::Player::InspectCash(client, clientCash);
	if (clientCash < prize)
	{
		PrintUserCmdText(client, L"You do not possess enough credits.");
		return false;
	}

	for (const auto& bh : bountyHunt)
	{
		if (bh.initiatorId == client && bh.targetId == targetId)
		{
			PrintUserCmdText(client, L"You already have a bounty on this player.");
			return false;
		}
	}

	pub::Player::AdjustCash(client, -prize);
	const std::wstring initiator = reinterpret_cast<const wchar_t*>(Players.GetActiveCharacterName(client));

	BountyHunt bh;
	bh.initiatorId = client;
	bh.end = GetTimeInMS() + (static_cast<mstime>(huntTime) * 60000);
	bh.initiator = initiator;
	bh.cash = prize;
	bh.target = target;
	bh.targetId = targetId;

	bountyHunt.push_back(bh);

	HkMsgU(
	    bh.initiator + L" offers " + std::to_wstring(bh.cash) + L" credits for killing " + bh.target + L" in " + std::to_wstring(huntTime) + L" minutes.");
	return true;
}

/** @ingroup BountyHunt
 * @brief User Command for /bountyhuntid. Creates a bounty against a specified player.
 */
bool UserCmdBountyHuntID(uint client, const std::wstring& cmd, const std::wstring& param, const wchar_t* usage)
{
	if (!enableBountyHunt)
		return false;

	const std::wstring target = GetParam(param, ' ', 0);
	const std::wstring credits = GetParam(param, ' ', 1);
	const std::wstring time = GetParam(param, ' ', 2);
	if (!target.length() || !credits.length())
	{
		PrintUserCmdText(client, L"Usage: /bountyhuntid <id> <credits> <time>");
		PrintBountyHunts(client);
		return false;
	}

	uint clientTarget = ToInt(target);
	if (!HkIsValidClientID(clientTarget) || HkIsInCharSelectMenu(clientTarget))
	{
		PrintUserCmdText(client, L"Error: Invalid client id.");
		return false;
	}

	const std::wstring charName = reinterpret_cast<const wchar_t*>(Players.GetActiveCharacterName(clientTarget));
	const auto paramNew = std::wstring(charName + L" " + credits + L" " + time);
	UserCmdBountyHunt(client, cmd, paramNew, usage);
	return true;
}

/** @ingroup BountyHunt
 * @brief Checks for expired bounties.
 */
void BhTimeOutCheck()
{
	returnCode = DEFAULT_RETURNCODE;
	if (time(0) % 60 != 0)
		return;

	auto bounty = bountyHunt.begin();

	while (bounty != bountyHunt.end())
	{
		if (bounty->end < GetTimeInMS())
		{
			pub::Player::AdjustCash(bounty->targetId, bounty->cash);
			HkMsgU(bounty->target + L" was not hunted down and earned " + std::to_wstring(bounty->cash) + L" credits.");
			bounty = bountyHunt.erase(bounty);
		}
		else
		{
			++bounty;
		}
	}
}

/** @ingroup BountyHunt
 * @brief Processes a ship death to see if it was part of a bounty.
 */
void BillCheck(uint& client, uint& killer)
{
	for (auto& bounty : bountyHunt)
	{
		if (bounty.targetId == client)
		{
			if (killer == 0 || client == killer)
			{
				HkMsgU(L"The hunt for " + bounty.target + L" still goes on.");
				continue;
			}
			std::wstring winnerCharacterName = reinterpret_cast<const wchar_t*>(Players.GetActiveCharacterName(killer));
			if (!winnerCharacterName.empty())
			{
				pub::Player::AdjustCash(killer, bounty.cash);
				HkMsgU(winnerCharacterName + L" has killed " + bounty.target + L" and earned " + std::to_wstring(bounty.cash) + L" credits.");
			}
			else
			{
				pub::Player::AdjustCash(bounty.initiatorId, bounty.cash);
			}
			RemoveBountyHunt(bounty);
			BillCheck(killer, killer);
			break;
		}
	}
}

/** @ingroup BountyHunt
 * @brief Hook for SendDeathMsg to call BillCheck
 */
void SendDeathMsg(const std::wstring& msg, uint system, uint clientVictim, uint clientKiller)
{
	returnCode = DEFAULT_RETURNCODE;
	if (enableBountyHunt)
	{
		BillCheck(clientVictim, clientKiller);
	}
}

void checkIfPlayerFled(uint& client)
{
	for (auto& it : bountyHunt)
	{
		if (it.targetId == client)
		{
			pub::Player::AdjustCash(it.initiatorId, it.cash);
			HkMsgU(L"The coward " + it.target + L" has fled. " + it.initiator + L" has been refunded.");
			RemoveBountyHunt(it);
			return;
		}
	}
}

/** @ingroup BountyHunt
 * @brief Hook for Disconnect to see if the player had a bounty on them
 */
void __stdcall DisConnect(uint client, const enum EFLConnection state)
{
	returnCode = DEFAULT_RETURNCODE;
	checkIfPlayerFled(client);
}

/** @ingroup BountyHunt
 * @brief Hook for CharacterSelect to see if the player had a bounty on them
 */
void __stdcall CharacterSelect(struct CHARACTER_ID const& cId, uint client)
{
	returnCode = DEFAULT_RETURNCODE;
	checkIfPlayerFled(client);
}
typedef bool(*_UserCmdProc)(uint, const wstring &, const wstring &, const wchar_t*);

struct USERCMD
{
	wchar_t *wszCmd;
	_UserCmdProc proc;
	wchar_t *usage;
};

USERCMD UserCmds[] =
{
	{ L"/bountyhunt", UserCmdBountyHunt, L"Usage: /bountyhunt <name>" },
	{ L"/bountyhuntid", UserCmdBountyHuntID, L"Usage: /bountyhuntid <id>" },
};

bool UserCmd_Process(uint iClientID, const wstring &wscCmd)
{
	returnCode = DEFAULT_RETURNCODE;

	wstring wscCmdLineLower = ToLower(wscCmd);

	// If the chat string does not match the USER_CMD then we do not handle the
	// command, so let other plugins or FLHook kick in. We require an exact match
	for (uint i = 0; (i < sizeof(UserCmds) / sizeof(USERCMD)); i++)
	{

		if (wscCmdLineLower.find(UserCmds[i].wszCmd) == 0)
		{
			// Extract the parameters string from the chat string. It should
			// be immediately after the command and a space.
			wstring wscParam = L"";
			if (wscCmd.length() > wcslen(UserCmds[i].wszCmd))
			{
				if (wscCmd[wcslen(UserCmds[i].wszCmd)] != ' ')
					continue;
				wscParam = wscCmd.substr(wcslen(UserCmds[i].wszCmd) + 1);
			}

			// Dispatch the command to the appropriate processing function.
			if (UserCmds[i].proc(iClientID, wscCmd, wscParam, UserCmds[i].usage))
			{
				// We handled the command tell FL hook to stop processing this
				// chat string.
				returnCode = SKIPPLUGINS_NOFUNCTIONCALL; // we handled the command, return immediatly
				return true;
			}
		}
	}
	return false;
}

// Load Settings
void LoadSettings()
{
	returnCode = DEFAULT_RETURNCODE;
	char szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
	string cfg_file = string(szCurDir) + "\\flhook_plugins\\bounty_hunt.cfg";
	INI_Reader ini;
	if (ini.open(cfg_file.c_str(), false))
	{
		while (ini.read_header())
		{
			if (ini.is_header("general"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("levelProtect"))
					{
						levelProtect = ini.get_value_int(0);
					}
					else if (ini.is_value("min_hunt_time"))
					{
						minimalHuntTime = ini.get_value_int(0);
					}
					else if (ini.is_value("max_hunt_time"))
					{
						minimalHuntTime = ini.get_value_int(0);
					}
					else if (ini.is_value("default_hunt_time"))
					{
						defaultHuntTime = ini.get_value_int(0);
					}
				}
			}
		}
		ini.close();
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FLHOOK STUFF
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO* p_PI = new PLUGIN_INFO();

	p_PI->sName = "Bounty Hunt";
	p_PI->sShortName = "bountyhunt";
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returnCode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&SendDeathMsg, PLUGIN_SendDeathMsg, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&BhTimeOutCheck, PLUGIN_HkTimerCheckKick, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&CharacterSelect, PLUGIN_HkIServerImpl_CharacterSelect, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process, 0));

	return p_PI;
}