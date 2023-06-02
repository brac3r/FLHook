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
		for (auto const& [targetId, initiatorId, target, initiator, cash, end] : bountyHunt)
		{
			PrintUserCmdText(
			    client, std::format(L"Kill {} and earn {} credits ({} minutes left)", target, cash, ((end - Hk::Time::GetUnixSeconds()) / 60)));
		}
	}
}

/** @ingroup BountyHunt
 * @brief User Command for /bountyhunt. Creates a bounty against a specified player.
 */
void UserCmdBountyHunt(uint& client, const std::wstring& param)
{
	if (!enableBountyHunt)
		return;

	const std::wstring target = GetParam(param, ' ', 0);
	const uint prize = ToUInt(GetParam(param, ' ', 1));
	const std::wstring timeString = GetParam(param, ' ', 2);
	if (!target.length() || prize == 0)
	{
		PrintUserCmdText(client, L"Usage: /bountyhunt <playername> <credits> <time>");
		PrintBountyHunts(client);
		return;
	}

	uint time = wcstol(timeString.c_str(), nullptr, 10);
	const auto targetId = Hk::Client::GetClientIdFromCharName(target);

	int rankTarget = Hk::Player::GetRank(targetId.value()).value();

	if (targetId == UINT_MAX || Hk::Client::IsInCharSelectMenu(targetId.value()))
	{
		PrintUserCmdText(client, std::format(L"{} is not online.", target));
		return;
	}

	if (rankTarget < levelProtect)
	{
		PrintUserCmdText(client, L"Low level players may not be hunted.");
		return;
	}

	// clamp the hunting time to configured range, or set default if not specified
	if (time)
	{
		time = std::min(maximumHuntTime, std::max(minimalHuntTime, time));
	}
	else
	{
		time = defaultHuntTime;
	}

	if (uint clientCash = Hk::Player::GetCash(client).value(); clientCash < prize)
	{
		PrintUserCmdText(client, L"You do not possess enough credits.");
		return;
	}

	for (const auto& bh : bountyHunt)
	{
		if (bh.initiatorId == client && bh.targetId == targetId)
		{
			PrintUserCmdText(client, L"You already have a bounty on this player.");
			return;
		}
	}

	Hk::Player::RemoveCash(client, prize);
	const std::wstring initiator = reinterpret_cast<const wchar_t*>(Players.GetActiveCharacterName(client));

	BountyHunt bh;
	bh.initiatorId = client;
	bh.end = Hk::Time::GetUnixMiliseconds() + (static_cast<mstime>(time) * 60000);
	bh.initiator = initiator;
	bh.cash = prize;
	bh.target = target;
	bh.targetId = targetId.value();

	bountyHunt.push_back(bh);

	Hk::Message::MsgU(
	    bh.initiator + L" offers " + std::to_wstring(bh.cash) + L" credits for killing " + bh.target + L" in " + std::to_wstring(time) + L" minutes.");
}

/** @ingroup BountyHunt
 * @brief User Command for /bountyhuntid. Creates a bounty against a specified player.
 */
void UserCmdBountyHuntID(uint& client, const std::wstring& param)
{
	if (!enableBountyHunt)
		return;

	const std::wstring target = GetParam(param, ' ', 0);
	const std::wstring credits = GetParam(param, ' ', 1);
	const std::wstring time = GetParam(param, ' ', 2);
	if (!target.length() || !credits.length())
	{
		PrintUserCmdText(client, L"Usage: /bountyhuntid <id> <credits> <time>");
		PrintBountyHunts(client);
		return;
	}

	ClientId clientTarget = ToInt(target);
	if (!Hk::Client::IsValidClientID(clientTarget) || Hk::Client::IsInCharSelectMenu(clientTarget))
	{
		PrintUserCmdText(client, L"Error: Invalid client id.");
		return;
	}

	const std::wstring charName = reinterpret_cast<const wchar_t*>(Players.GetActiveCharacterName(clientTarget));
	const auto paramNew = std::wstring(charName + L" " + credits + L" " + time);
	UserCmdBountyHunt(client, paramNew);
}

/** @ingroup BountyHunt
 * @brief Checks for expired bounties.
 */
void BhTimeOutCheck()
{
	auto bounty = bountyHunt.begin();

	while (bounty != bountyHunt.end())
	{
		if (bounty->end < Hk::Time::GetUnixMiliseconds())
		{
			if (const auto cashError = Hk::Player::AddCash(bounty->target, bounty->cash); cashError.has_error())
			{
				Console::ConWarn(wstos(Hk::Err::ErrGetText(cashError.error())));
				return;
			}

			Hk::Message::MsgU(bounty->target + L" was not hunted down and earned " + std::to_wstring(bounty->cash) + L" credits.");
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
				Hk::Message::MsgU(L"The hunt for " + bounty.target + L" still goes on.");
				continue;
			}

			if (std::wstring winnerCharacterName = reinterpret_cast<const wchar_t*>(Players.GetActiveCharacterName(killer)); !winnerCharacterName.empty())
			{
				if (const auto cashError = Hk::Player::AddCash(winnerCharacterName, bounty.cash); cashError.has_error())
				{
					Console::ConWarn(wstos(Hk::Err::ErrGetText(cashError.error())));
					return;
				}
				Hk::Message::MsgU(winnerCharacterName + L" has killed " + bounty.target + L" and earned " + std::to_wstring(bounty.cash) + L" credits.");
			}
			else
			{
				if (const auto cashError = Hk::Player::AddCash(bounty.initiator, bounty.cash); cashError.has_error())
				{
					Console::ConWarn(wstos(Hk::Err::ErrGetText(cashError.error())));
					return;
				}
			}
			RemoveBountyHunt(bounty);
			BillCheck(killer, killer);
			break;
		}
	}
}

// Timer Hook
const std::vector<Timer> timers = {{BhTimeOutCheck, 60}};

/** @ingroup BountyHunt
 * @brief Hook for SendDeathMsg to call BillCheck
 */
void SendDeathMsg(const std::wstring& msg, uint system, uint clientVictim, uint clientKiller)
{
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
			if (const auto cashError = Hk::Player::AddCash(it.initiator, it.cash); cashError.has_error())
			{
				Console::ConWarn(wstos(Hk::Err::ErrGetText(cashError.error())));
				return;
			}
			Hk::Message::MsgU(L"The coward " + it.target + L" has fled. " + it.initiator + L" has been refunded.");
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
	checkIfPlayerFled(client);
}

/** @ingroup BountyHunt
 * @brief Hook for CharacterSelect to see if the player had a bounty on them
 */
void CharacterSelect(struct CHARACTER_ID const& cId, uint client)
{
	checkIfPlayerFled(client);
}

// Client command processing
const std::vector commands = {{
    CreateUserCommand(L"/bountyhunt", L"<charname> <credits> [minutes]", UserCmdBountyHunt,
        L"Places a bounty on the specified player. When another player kills them, they gain <credits>."),
    CreateUserCommand(
        L"/bountyhuntid", L"<id> <credits> [minutes]", UserCmdBountyHuntID, L"Same as above but with an id instead of a player name. Use /ids"),
}};

// Load Settings
void LoadSettings()
{
	auto config = Serializer::JsonToObject<Config>();
	config = std::make_unique<Config>(config);
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
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&DisConnect, PLUGIN_HkIServerImpl_DisConnect, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&CharacterSelect, PLUGIN_HkIServerImpl_CharacterSelect, 0));

	return p_PI;
}