FLHook Discovery 3.1.0 (PlaYsiA server mod)
=============

Thanks to
-------

All of the Discovery Developers over the years for this great mod.
All of the Discovery Developers who have helped me get stuff working.


What are we doing here?
-------

This is the code/configs for (AUS Non-RP) PlaYsiA's Discovery
Stripping out the RP stuff we don't like is 99% done in the configs


How I got it working
-------

- Use Visual Studio 2017 Community Edition with github plugins.
- Clone  the repo https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2017/version-control/git-clone-repository?view=vs-2017
- make a folder called "boost" inside the "Dependencies" directory
- Put the latest Boost library in there https://www.boost.org/users/download/
- -- Yes there will be /Dependencies/boost/boost/....
- Grab original dsacesrv.dll plugin from the mod release
- Disable plugins we don't want for our purposes
- -- MobileDocking, Laz/Help, MiscellaneousCommands
- Switch to release
- Now you should be able to Build->Clean Solution and then Build->Build Solution
- To solve this error: LINK : fatal error LNK1181: cannot open input file 'D:\git\FLHook\\Plugins\Public\hookext_plugin\release\ahookext.lib'
- -- Manually add the reference to HookExtension Plugin in References in Solution Explorer
- -- Don't ask me why that double // is there because I don't know

Changelog
-------

v1.2.0
Implemented Aingar's FLHook4 plugin ports.
afk, betting, bountyhunt, deathpenalty, kill_tracker, tax

v1.16
New racing restart, has 2 starbeams and neutral rep with zoners
/restart ZoneRacer

v1.15
New flhook command setallrep
Usage: setallrep <charname> <value>
value should be between -1 and 1
ie: to set all green command would be setallrep bob 0.9
to set all neutral would be setallrep bob 0

v1.14
Base core limit can now be changed via the config file

v1.13
Can use nomad gear (except cloaks), on all IDs.
Added restarts to the configs

v1.12
Base core limit now 5

v1.11
Fixed defense modes 1/2 to do what they say (removed siege only bad rep)

v1.1
Non-RP server configs added. (including base.cfg food fix)

v1.0
Re-enabled /invite$ and /i$
Disabled /snacclassic
Changed base defensemode to do as it says. (took out changes made on RP server because they had a griefing problem)
Applied shield damage fix (thanks Aingar)


Original Below:


FLHook Discovery 3.1.0
=============

This is the public repository for Discovery Freelancer's FLHook.
The client hook and the server anticheat are not available due to their sensitivity.
It's also currently missing ducktales and disabledock.

The original SVN can be found here: http://forge.the-starport.net/projects/flhookplugin

This project is configured to run under Visual Studio 2017. Everybody can contribute with the Community Edition.
The Windows SDK version is 8.1. 
The compiler is VC141.

Our current starport revision is #267.

If you find old contributions you wish to refactor, you are welcome to do so

How to use
-------

Open the solution FLHook.sln
Do not use the solution in DEBUG configuration. Always use RELEASE.

Your compiled plugins are in Binaries/bin-vc14/flhook_plugins.
Sometimes it also like to not copy over the new dll from the plugin's folder into bin-vc14/flhook_plugins for mysterious reasons.

License
-------

Don't claim other's work as your own. Past that, do what you want.
This has been a collaborative project for years and we expect you to respect that.

Contributing
------------

Send pull requests or create issues to discuss
