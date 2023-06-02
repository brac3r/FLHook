/**
 * @date August, 2022
 * @author Raikkonen
 * @defgroup AwayFromKeyboard Away from Keyboard
 * @brief
 * The AFK plugin allows you to set yourself as Away from Keyboard.
 * This will notify other players if they try and speak to you, that you are not at your desk.
 *
 * @paragraph cmds Player Commands
 * All commands are prefixed with '/' unless explicitly specified.
 * - afk - Sets your status to Away from Keyboard. Other players will notified if they try to speak to you.
 * - back - Removes the AFK status.
 *
 * @paragraph adminCmds Admin Commands
 * There are no admin commands in this plugin.
 *
 * @paragraph configuration Configuration
 * No configuration file is needed.
 *
 * @paragraph ipc IPC Interfaces Exposed
 * This plugin does not expose any functionality.
 */
#include <unordered_set>
#include <FLHook.h>
#include <plugin.h>

unordered_set<uint> awayClients;
PLUGIN_RETURNCODE returnCode;

/** @ingroup AwayFromKeyboard
 * @brief This command is called when a player types /afk. It prints a message in red text to nearby players saying they are afk. It will also let anyone
 * who messages them know too.
 */
bool UserCmdAfk(uint client, const wstring &cmd, const wstring &param, const wchar_t* usage)
{
	awayClients.insert(client);
	const std::wstring playerName = reinterpret_cast<const wchar_t*>(Players.GetActiveCharacterName(client));

	uint systemId;
	pub::Player::GetSystem(client, systemId);

	const Universe::ISystem* iSys = Universe::get_system(systemId);
	wstring sysname = stows(iSys->nickname);

	HkMsgS(sysname.c_str(), playerName + L" is now away from keyboard.");
	PrintUserCmdText(client, L"Use the /back command to stop sending automatic replies to PMs.");
	return true;
}

/** @ingroup AwayFromKeyboard
 * @brief This command is called when a player types /back. It removes the afk status and welcomes the player back.
 * who messages them know too.
 */
bool UserCmdBack(uint client, const wstring &cmd, const wstring &param, const wchar_t* usage)
{
	if (awayClients.find(client) == awayClients.end())
	{
		return false;
	}

	uint systemId;
	pub::Player::GetSystem(client, systemId);
	const Universe::ISystem* iSys = Universe::get_system(systemId);
	wstring sysname = stows(iSys->nickname);

	const std::wstring playerName = reinterpret_cast<const wchar_t*>(Players.GetActiveCharacterName(client));
	HkMsgS(sysname.c_str(), playerName + L" is now away from keyboard.");

	awayClients.erase(client);
	return true;
}

// Clean up when a client disconnects
void ClearClientInfo(uint client)
{
	awayClients.erase(client);
}

// Hook on chat being sent (This gets called twice with the client and to
// swapped
void __stdcall Cb_SendChat(uint client, uint to, uint size, void* rdl)
{
	if (awayClients.find(to) != awayClients.end())
		PrintUserCmdText(client, L"This user is away from keyboard.");
}

// Hooks on chat being submitted
void __stdcall SubmitChat(struct CHAT_ID cId, unsigned long lP1, void const* rdlReader, struct CHAT_ID cIdToto,
    int iP2)
{
	if (awayClients.find(cId.iID) !=awayClients.end())
		UserCmdBack(cId.iID,L"",L"",L"");
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
	{ L"/afk", UserCmdAfk, L"Sets your status to \"Away from Keyboard\". Other players will notified if they try to speak to you." },
	{ L"/back*", UserCmdBack, L"Removes the AFK status." },
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FLHOOK STUFF
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Functions to hook

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO* p_PI = new PLUGIN_INFO();

	p_PI->sName = "AFK";
	p_PI->sShortName = "afk";
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returnCode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ClearClientInfo, PLUGIN_ClearClientInfo, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&Cb_SendChat, PLUGIN_HkCb_SendChat, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&SubmitChat, PLUGIN_HkIServerImpl_SubmitChat, 0));

	return p_PI;
}