# Mario Kart 64 Practice ROM

This is a set of patches for Mario Kart 64 that add useful functions for speedrun practice, as well as a framework for developing patches in C.

(TODO: screenshots)

This is still a work in progress; many features are buggy/missing.


## Current features
- title screen menu for quickly jumping into the game
- improved debug functions and input display
- better crash handler

## Planned features
- save settings to memory card/flash cart
- fix timer precision bugs
- skip logo screen
- output time/game state info over USB for tracking (auto splitter, ghost sharing, etc)
- load resources over USB for quick testing of custom tracks
- optional tweaks for tournament play (stat rebalancing, etc)
- optional tweaks for performance on emulators (improve FPS/resolution)
- linked multiplayer (USB/local wifi)
- save states and other nice features stolen from OoT GZ ROM


## Project structure
The project is divided into modules:

### bootstrap
This is the patch that loads other patches from ROM and executes them at boot. Since it sets up the framework for the others, it can't use it itself, so it's just one assembly file.


### crash-handler
This patch replaces the game's crash handler with a more useful one, similar to the one found in Ocarina of Time. It displays without requiring any "cheat code" and shows several pages:

1. GPRs, game state, exception info, hardware info
    - OP: the opcode, if PC is within RAM
    - HE, HS: heap end/start
    - MT, PT, ST: main thread task, prev task, subtask
    - SM: screen mode
    - RT: race type
    - TN: track number
    - NP: number of players
2. FPRs (currently disabled)
3. Stack dump (can scroll with controller 1)
4. RAM dump
5. Frame buffer 1 (last displayed frames before crash)
6. Frame buffer 2


### debug
This patch improves some of the built-in debug functions and adds some more.
- display list moved to corner of screen
- debug mode can be toggled on/off with L+R+Z or with the button on the 64drive cartridge
- input display
- player coords, speed, race progress display
- live RAM viewer (currently disabled)


### new-menu
This patch adds a new title screen menu for quickly jumping to any game state.
- Race Mode: Mario GP, Time Trial, VS, Battle
- GP Cup: Which cup to start on
- GP Round: Which round of the cup to start on (not yet implemented)
- Course: Course to play for non-GP modes (buggy)
- Players: Number of players and screen setup
- Player 1/2/3/4: Player character selection
- Class: 50cc/100cc/150cc/Extra (separate from Mirror Mode)
- Mirror Mode: on/off
- Items: on/off (not yet implemented)
- Music: on/off (not yet implemented)
- Debug Mode: on/off

This menu allows choosing some settings that aren't possible normally, such as vertical splitscreen, 50cc mirror mode, multiple players with the same character, etc.
