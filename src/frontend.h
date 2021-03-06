/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef __INCLUDED_SRC_FRONTEND_H__
#define __INCLUDED_SRC_FRONTEND_H__

#include "lib/widget/widgbase.h"

// determines which option screen to use. when in GS_TITLE_SCREEN mode.
enum tMode
{
	TITLE,					///< intro mode
	SINGLE,					///< single player menu
	MULTI,					///< multiplayer menu
	OPTIONS,				///< options menu
	GAME,					///<
	TUTORIAL,				///< tutorial/fastplay
	CREDITS,				///< credits
	MULTIOPTION,			///< Host Screen
	MULTILIMIT,				///< MULTIPLAYER, Limit the multistuff.
	STARTGAME,				///< Fire up the game
	SHOWINTRO,				///< reshow the intro
	QUIT,					///< leaving game
	LOADSAVEGAME,			///< loading a save game
	KEYMAP,					///< keymap editor
	GRAPHICS_OPTIONS,       ///< graphics options menu
	AUDIO_OPTIONS,          ///< audio options menu
	VIDEO_OPTIONS,          ///< video options menu
	MOUSE_OPTIONS,          ///< mouse options menu
};

extern tMode titleMode;					// the global case
extern tMode lastTitleMode;

#define MAX_LEVEL_NAME_SIZE	(256)

extern char	aLevelName[MAX_LEVEL_NAME_SIZE+1];	//256];			// vital! the wrf file to use.

extern bool	bLimiterLoaded;


void changeTitleMode(tMode mode);
bool runTitleMenu(void);
bool runSinglePlayerMenu(void);
bool runGameOptionsMenu(void);
bool runOptionsMenu(void);
bool runGraphicsOptionsMenu(void);
bool runAudioOptionsMenu(void);
bool runVideoOptionsMenu(void);
bool runMouseOptionsMenu(void);
bool runTutorialMenu(void);

void addTopForm(void);
void addBottomForm(void);
void addBackdrop(void);
void addTextButton(UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt, unsigned int style);
void addSmallTextButton(UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt, unsigned int style);
void addTextHint(UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt);
void addText(UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt, UDWORD formID);
void addSideText(UDWORD id, UDWORD PosX, UDWORD PosY, const char *txt);
void addFESlider(UDWORD id, UDWORD parent, UDWORD x, UDWORD y, UDWORD stops, UDWORD pos);
void addFEAISlider(UDWORD id, UDWORD parent, UDWORD x, UDWORD y, UDWORD stops, UDWORD pos);

void displayTextOption(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

bool CancelPressed(void);


// ////////////////////////////////////////////////////////////////////////////
// defines.

#define FRONTEND_TOPFORMX		80
#define FRONTEND_TOPFORMY		10
#define FRONTEND_TOPFORMW		480
#define FRONTEND_TOPFORMH		150


#define FRONTEND_TOPFORM_WIDEX	28
#define FRONTEND_TOPFORM_WIDEY	10
#define FRONTEND_TOPFORM_WIDEW	588
#define FRONTEND_TOPFORM_WIDEH	150


#define FRONTEND_BOTFORMX		80
#define FRONTEND_BOTFORMY		170
#define FRONTEND_BOTFORMW		480
#define FRONTEND_BOTFORMH		300


#define FRONTEND_BUTWIDTH		FRONTEND_BOTFORMW-40 // text button sizes.
#define FRONTEND_BUTHEIGHT		30

#define FRONTEND_POS1X			20				// button positions
#define FRONTEND_POS1Y			10
#define FRONTEND_POS1M			290

#define FRONTEND_POS2X			20
#define FRONTEND_POS2Y			50
#define FRONTEND_POS2M			290

#define FRONTEND_POS3X			20
#define FRONTEND_POS3Y			90
#define FRONTEND_POS3M			290

#define FRONTEND_POS4X			20
#define FRONTEND_POS4Y			130
#define FRONTEND_POS4M			290

#define FRONTEND_POS5X			20
#define FRONTEND_POS5Y			170
#define FRONTEND_POS5M			290

#define FRONTEND_POS6X			20
#define FRONTEND_POS6Y			210
#define FRONTEND_POS6M			290

#define FRONTEND_POS7X			20
#define FRONTEND_POS7Y			250
#define FRONTEND_POS7M			290

#define FRONTEND_POS8X			-30				// special case for our hyperlink
#define FRONTEND_POS8Y			278


#define FRONTEND_SIDEX			44
#define FRONTEND_SIDEY			FRONTEND_BOTFORMY
#define FRONTEND_LOGOW			248
#define FRONTEND_LOGOH			118

enum
{
	FRONTEND_BACKDROP		= 20000,
	FRONTEND_TOPFORM,
	FRONTEND_BOTFORM,
	FRONTEND_LOGO,
	FRONTEND_SIDETEXT,					// side-ee-ways text
	FRONTEND_SIDETEXT1,					// side-ee-ways text
	FRONTEND_SIDETEXT2,					// side-ee-ways text
	FRONTEND_SIDETEXT3,					// side-ee-ways text
	FRONTEND_SIDETEXT4,					// side-ee-ways text
	FRONTEND_LOADCAM2,					// loading via --GAME CAM_2A
	FRONTEND_LOADCAM3,					// loading via --GAME CAM_3A
	FRONTEND_PASSWORDFORM,
	FRONTEND_LOGINFORM,
	FRONTEND_HYPERLINK,
	// begin menu
	FRONTEND_SINGLEPLAYER	= 20100,	// title screen
	FRONTEND_MULTIPLAYER,
	FRONTEND_TUTORIAL,
	FRONTEND_PLAYINTRO,
	FRONTEND_OPTIONS,
	FRONTEND_QUIT,
	FRONTEND_FASTPLAY,					//tutorial menu option
	FRONTEND_NEWGAME		= 20200,	// single player (menu)
	FRONTEND_LOADGAME,
	FRONTEND_SKIRMISH,
	FRONTEND_CHALLENGES,
	FE_P0				= 20300,		// player 0 buton
	FE_P1,								// player 1 buton
	FE_P2,								// player 2 buton
	FE_P3,								// player 3 buton
	FE_P4,								// player 4 buton
	FE_P5,								// player 5 buton
	FE_P6,								// player 6 buton
	FE_P7,								// player 7 buton

	FRONTEND_GAMEOPTIONS = 21000,           // Game Options menu
	FRONTEND_LANGUAGE,
	FRONTEND_LANGUAGE_R,
	FRONTEND_RADAR,
	FRONTEND_RADAR_R,
	FRONTEND_COLOUR,
	FRONTEND_DIFFICULTY,
	FRONTEND_DIFFICULTY_R,
	FRONTEND_SCROLLSPEED_SL,
	FRONTEND_SCROLLSPEED,				// screen scroll speed

	FRONTEND_GRAPHICSOPTIONS = 22000,       // Graphics Options Menu
	FRONTEND_SSHAKE,
	FRONTEND_SSHAKE_R,
	FRONTEND_FMVMODE,
	FRONTEND_FMVMODE_R,
	FRONTEND_SCANLINES,
	FRONTEND_SCANLINES_R,
	FRONTEND_SUBTITLES,
	FRONTEND_SUBTITLES_R,
	FRONTEND_SHADOWS,
	FRONTEND_SHADOWS_R,

	FRONTEND_AUDIOOPTIONS = 23000,          // Audio Options Menu
	FRONTEND_3D_FX,						// 3d sound volume
	FRONTEND_FX,						// 2d (voice) sound volume
	FRONTEND_MUSIC,						// music volume
	FRONTEND_3D_FX_SL,
	FRONTEND_FX_SL,
	FRONTEND_MUSIC_SL,

	FRONTEND_VIDEOOPTIONS = 24000,          // video Options Menu
	FRONTEND_WINDOWMODE,
	FRONTEND_WINDOWMODE_R,
	FRONTEND_RESOLUTION,
	FRONTEND_RESOLUTION_R,
	FRONTEND_TEXTURESZ,
	FRONTEND_TEXTURESZ_R,
	FRONTEND_TAKESEFFECT,
	FRONTEND_VSYNC,
	FRONTEND_VSYNC_R,
	FRONTEND_FSAA,
	FRONTEND_FSAA_R,

	FRONTEND_MOUSEOPTIONS = 25000,          // Mouse Options Menu
	FRONTEND_TRAP,
	FRONTEND_TRAP_R,
	FRONTEND_MFLIP,
	FRONTEND_MFLIP_R,
	FRONTEND_MBUTTONS,
	FRONTEND_MBUTTONS_R,
	FRONTEND_MMROTATE,
	FRONTEND_MMROTATE_R,

	FRONTEND_KEYMAP			= 26000,	// Keymap menu
	FRONTEND_LOBBYERROR 	= 31666	// Shows an lobby error

};

#endif // __INCLUDED_SRC_FRONTEND_H__
