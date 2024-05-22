# BBCF-Improvement-Mod
Additional features and tweaks for BlazBlue Centralfiction

The goal is to extend the game with community-created content and provide additional graphical options.

Join the [BB Improvement Mod Discord Server](https://discord.gg/j2mCX9s) to discuss about the development and the game itself!

## Why?
while there are still issues with the feature libreofficecalc has recently updated his mod such that if one party has the feature disabled, none of the others will upload the replay, this was, in my opinion, the biggest issue with the feature, this repo will no longer be updated and you should instead use the [libreofficecalc improvement mod](https://github.com/libreofficecalc/BBCF-Improvement-Mod) and simply disable the feature.

## What this mod provides
- Adds extra game modes
- Adds hitbox overlay
- Adds replay takeover
- Adds P2 State Library
- Adds wakeup action thru state library
- Adds gap action thru state library
- Adds training dummy slot introspection
- Adds training dummy slot saving/loading to/from local files
- Adds wakeup action thru training dummy slots
- Adds gap action thru training dummy slots
- Adds local replay file loading
- more experimental features


- Create and load custom palettes and effects without file modifications
- See each other's custom palettes in online matches
- More flexibility to change the graphics options
- Change avatars and accessories in online rooms/lobbies without going back into menu
- Freely adjustable ingame currency value

## Installing
Download dinput8.dll, settings.ini and optionally palettes.ini from the latest release and put them in your BlazBlue Centralfiction folder.

if on linux add the following ```WINEDLLOVERRIDES="dinput8=n,b"``` to your launch options

## Installing with ASI Loader
Download dinput8.dll, settings.ini and optionally palettes.ini from the latest release and put them in your BlazBlue Centralfiction folder. 
Rename dinput.dll to BBCFIM.asi
Install ASI Loader (i use d3d9.dll)


## Compiling and usage
BBCF Improvement Mod is coded using Visual Studio 2019 (toolset v142). <br>
To compile, you should only need to load the sln file and compile as-is. No changes should be needed to the solution or source.<br>
Copy the compiled binary, settings.ini, and palettes.ini files from the bin/ folder into the game's root folder.

## Requirements
- Visual Studio 2019 (toolset v142) (Windows SDK 10)

## Thanks to the people who have helped the mod along the way
* GrimFlash
* KoviDomi
* Neptune
* Rouzel
* Dormin
* NeoStrayCat
* KDing
* PC_volt
* libreofficecalc
* TheDukeofErl
* Everybody in the BlazBlue PC community that has given support or feedback of any kind!

## Extra thanks to
Atom0s for their DirectX9.0 Hooking article<br>
Durante for their dsfix source

## Legal
```
BBCF Improvement Mod is NOT associated with Arc System Works or any of its partners / affiliates.
BBCF Improvement Mod is NOT intended for malicious use.
BBCF Improvement Mod is NOT intended to give players unfair advantages in online matches.
BBCF Improvement Mod is NOT intended to unlock unreleased / unpurchased contents of the game.
BBCF Improvement Mod should only be used on the official version that you legally purchased and own.

Use BBCF Improvement Mod at your own risk.
I, Noel-FGC, am not responsible for what happens while using the BBCF Improvement Mod. You take full reponsibility for anything that happens to you as a result of using this software.
```
