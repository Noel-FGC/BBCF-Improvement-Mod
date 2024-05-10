# BBCF-Improvement-Mod
Additional features and tweaks for BlazBlue Centralfiction

The goal is to extend the game with community-created content and provide additional graphical options.

Join the [BB Improvement Mod Discord Server](https://discord.gg/j2mCX9s) to discuss about the development and the game itself!

## PR's and Bug reports
This repository only exists as a fork of [libreofficecalc/BBCF-Improvement-Mod](https://github.com/libreofficecalc/BBCF-Improvement-Mod) while their replay server does not respect user privacy, 

i do not have the skillset to, nor do i plan on maintaining this outside of reviwing and merging new commits from [libreofficecalc/BBCF-Improvement-Mod](https://github.com/libreofficecalc/BBCF-Improvement-Mod)

unless you are 100% sure a bug is caused by my removal of the function to upload replays, submit bug reports to [libreofficecalc/BBCF-Improvement-Mod](https://github.com/libreofficecalc/BBCF-Improvement-Mod/issues)

## Why?
libreofficecalc recently added a "feature" that unless explicitly disabled, automatically uploads all replays to a server, while i may not agree with it, something being opt-out instead of opt-in in the end, isnt a big deal

the big deal is that this "feature" is not listed anywhere on the repository (at the time of writing), and even if it was, im willing to bet atleast 60% of the people who download the mod just went straight to releases after their friend told them to. and how many people simply updating to the latest release would actually bother to check what new features are added, and then look into them far enough to realize what its actually doing?

packaging something like this into a long running mod many people use without even a message asking if you want your replays uploaded, or hell, a way of properly opting out or deleting your data from their server, is misleading and in my opinion a violation of trust.

unless you go out of your way to look for this, you would have no idea all of your replays were being uploaded

this combined with the fact it uploads replays regardless of if the opponent has the mod or not, or even if they explicitly turn off the feature makes the improvement mod, in my book, spyware

i have discussed in the [BB Improvement Mod Discord Server](https://discord.gg/j2mCX9s) and it seems noone in control of the situation gives a damn about what i or the many other people i have asked think

yes, i should open an issue before forking the repo, but i dont have the confidence to do that its honestly a miracle i managed to talk about it in the discord, if anyone wants to open an issue for me be my guest.

i will private this repository once libreofficecalc fixes the issues mentioned above until then, here we are.

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
