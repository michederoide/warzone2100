/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
/*	VISIBILTY.C Pumpkin Studios, Eidos Interactive 1996.	*/

#include <stdio.h>
#include <string.h>

#include "objects.h"
#include "map.h"
#include "loop.h"
#include "raycast.h"
#include "geometry.h"
#include "hci.h"
#include "lib/gamelib/gtime.h"
#include "mapgrid.h"
#include "cluster.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "scriptextern.h"
#include "structure.h"

#include "visibility.h"

#include "multiplay.h"
#include "advvis.h"

// accuracy for the height gradient
#define GRAD_MUL	10000

// which object is being considered by the callback
static SDWORD			currObj;

// rate to change visibility level
const static float VIS_LEVEL_INC = 255 * 2;
const static float VIS_LEVEL_DEC = 50;

// fractional accumulator of how much to change visibility this frame
static float			visLevelIncAcc, visLevelDecAcc;

// integer amount to change visiblility this turn
static SDWORD			visLevelInc, visLevelDec;

// percentage of power over which objects start to be visible
// UNUSED. What for? - Per
#define VIS_LEVEL_START		100
#define VIS_LEVEL_RANGE		60

// alexl's sensor range.
BOOL bDisplaySensorRange;

/* Variables for the visibility callback */
static SDWORD		rayPlayer;				// The player the ray is being cast for
static SDWORD		startH;					// The height at the view point
static SDWORD		currG;					// The current obscuring gradient
static SDWORD		lastH, lastD;			// The last height and distance
static BOOL			rayStart;				// Whether this is the first point on the ray
static SDWORD		tarDist;				// The distance to the ray target
static BOOL			blockingWall;			// Whether walls block line of sight
static SDWORD		finalX,finalY;			// The final tile of the ray cast
static SDWORD		numWalls;				// Whether the LOS has hit a wall
static SDWORD		wallX,wallY;			// the position of a wall if it is on the LOS

// initialise the visibility stuff
BOOL visInitialise(void)
{
	visLevelIncAcc = 0;
	visLevelDecAcc = 0;
	visLevelInc = 0;
	visLevelDec = 0;

	return TRUE;
}

// update the visibility change levels
void visUpdateLevel(void)
{
	visLevelIncAcc += (float)frameTime * (VIS_LEVEL_INC / GAME_TICKS_PER_SEC);
	visLevelInc = visLevelIncAcc;
	visLevelIncAcc -= visLevelInc;
	visLevelDecAcc += (float)frameTime * (VIS_LEVEL_DEC / GAME_TICKS_PER_SEC);
	visLevelDec = visLevelDecAcc;
	visLevelDecAcc -= visLevelDec;
}

static SDWORD visObjHeight(BASE_OBJECT *psObject)
{
	SDWORD	height;

	switch (psObject->type)
	{
	case OBJ_DROID:
		height = 80;
//		height = psObject->sDisplay.imd->pos.max.y;
		break;
	case OBJ_STRUCTURE:
		height = psObject->sDisplay.imd->max.y;
		break;
	case OBJ_FEATURE:
		height = psObject->sDisplay.imd->max.y;
		break;
	default:
		ASSERT( FALSE,"visObjHeight: unknown object type" );
		height = 0;
		break;
	}

	return height;
}

/* The terrain revealing ray callback */
static BOOL rayTerrainCallback(SDWORD x, SDWORD y, SDWORD dist)
{
	SDWORD		newH, newG;		// The new gradient
	MAPTILE		*psTile;

	ASSERT(x >= 0
	    && x < world_coord(mapWidth)
	    && y >= 0
	    && y < world_coord(mapHeight),
			"rayTerrainCallback: coords off map" );

	psTile = mapTile(map_coord(x), map_coord(y));

	/* Not true visibility - done on sensor range */

	if (dist == 0)
	{
		debug(LOG_ERROR, "rayTerrainCallback: dist is 0, which is not a valid distance");
		dist = 1;
	}

	newH = psTile->height * ELEVATION_SCALE;
	newG = (newH - startH) * GRAD_MUL / (SDWORD)dist;
	if (newG >= currG)
	{
		currG = newG;

		SET_TILE_VISIBLE(rayPlayer, psTile);

		if (selectedPlayer != rayPlayer && bMultiPlayer && game.alliance == ALLIANCES_TEAMS
		    && aiCheckAlliances(selectedPlayer, rayPlayer))
		{
			SET_TILE_VISIBLE(selectedPlayer,psTile);		//reveal radar
		}

		/* Not true visibility - done on sensor range */

		if(getRevealStatus())
		{
			if ((UDWORD)rayPlayer == selectedPlayer
			    || (bMultiPlayer && game.alliance == ALLIANCES_TEAMS
				&& aiCheckAlliances(selectedPlayer, rayPlayer)))
			{
				// can see opponent moving
				avInformOfChange(map_coord(x), map_coord(y));		//reveal map
				psTile->activeSensor = TRUE;
			}
		}
	}

	return TRUE;
}

/* The los ray callback */
static BOOL rayLOSCallback(SDWORD x, SDWORD y, SDWORD dist)
{
	SDWORD		newG;		// The new gradient
	SDWORD		distSq;
	SDWORD		tileX,tileY;
	MAPTILE		*psTile;

	ASSERT(x >= 0
	    && x < world_coord(mapWidth)
	    && y >= 0
	    && y < world_coord(mapHeight),
			"rayLOSCallback: coords off map" );

	distSq = dist*dist;

	if (rayStart)
	{
		rayStart = FALSE;
	}
	else
	{
		// Calculate the current LOS gradient
		newG = (lastH - startH) * GRAD_MUL / lastD;
		if (newG >= currG)
		{
			currG = newG;
		}
	}

	// See if the ray has reached the target
	if (distSq >= tarDist)
	{
		lastD = dist;
		return FALSE;
	}
	else
	{
		// Store the height at this tile for next time round
		tileX = map_coord(x);
		tileY = map_coord(y);

		if (blockingWall && !((tileX == finalX) && (tileY == finalY)))
		{
			psTile = mapTile(tileX, tileY);
			if (TILE_HAS_WALL(psTile) && !TILE_HAS_SMALLSTRUCTURE(psTile))
			{
				lastH = 2*UBYTE_MAX * ELEVATION_SCALE;
	//			currG = UBYTE_MAX * ELEVATION_SCALE * GRAD_MUL / lastD;
				numWalls += 1;
				wallX = x;
				wallY = y;
	//			return FALSE;
			}
			else
			{
				lastH = map_Height((UDWORD)x, (UDWORD)y);
			}
		}
		else
		{
			lastH = map_Height((UDWORD)x, (UDWORD)y);
		}
		lastD = dist;
	}

	return TRUE;
}

#define VTRAYSTEP	(NUM_RAYS/120)
#define	DUPF_SCANTERRAIN 0x01

BOOL visTilesPending(BASE_OBJECT *psObj)
{
	ASSERT( psObj->type == OBJ_DROID,"visTilesPending : Only implemented for droids" );

	return (((DROID*)psObj)->updateFlags & DUPF_SCANTERRAIN);
}

/* Check which tiles can be seen by an object */
void visTilesUpdate(BASE_OBJECT *psObj)
{
	SDWORD	range;
	SDWORD	ray;

	// Get the sensor Range and power
	switch (psObj->type)
	{
	case OBJ_DROID:	// Done whenever a droid is built or moves to a new tile.
		range = ((DROID *)psObj)->sensorRange;
		break;
	case OBJ_STRUCTURE:	// Only done when structure initialy built.
		range = ((STRUCTURE *)psObj)->sensorRange;
		break;
	default:
		ASSERT( FALSE,
			"visTilesUpdate: visibility checking is only implemented for"
			"units and structures" );
		return;
	}

	rayPlayer = psObj->player;

		// Do the whole circle.
		for(ray=0; ray < NUM_RAYS; ray += NUM_RAYS/80)
		{
			// initialise the callback variables
			startH = psObj->pos.z + visObjHeight(psObj);
			currG = -UBYTE_MAX * GRAD_MUL;

			// Cast the rays from the viewer
			rayCast(psObj->pos.x,psObj->pos.y,ray, range, rayTerrainCallback);
		}
}

/* Check whether psViewer can see psTarget.
 * psViewer should be an object that has some form of sensor,
 * currently droids and structures.
 * psTarget can be any type of BASE_OBJECT (e.g. a tree).
 * struckBlock controls whether structures block LOS
 */
BOOL visibleObject(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget)
{
	SDWORD		x,y, ray;
	SDWORD		xdiff,ydiff, rangeSquared;
	SDWORD		range;
	UDWORD		senPower, ecmPower;
	SDWORD		tarG, top;
	STRUCTURE	*psStruct;

	/* Get the sensor Range and power */
	switch (psViewer->type)
	{
	case OBJ_DROID:
		range = ((DROID *)psViewer)->sensorRange;
		senPower = ((DROID *)psViewer)->sensorPower;
		if (((DROID*)psViewer)->droidType == DROID_COMMAND)
		{
			range = 3 * range / 2;
		}
		break;
	case OBJ_STRUCTURE:
		psStruct = (STRUCTURE *)psViewer;

		// a structure that is being built cannot see anything
		if (psStruct->status != SS_BUILT)
		{
			return FALSE;
		}

		if ((psStruct->pStructureType->type == REF_WALL) ||
			(psStruct->pStructureType->type == REF_WALLCORNER))
		{
			return FALSE;
		}

		if ((structCBSensor((STRUCTURE *)psViewer) ||
			 structVTOLCBSensor((STRUCTURE *)psViewer)) &&
			 ((STRUCTURE *)psViewer)->psTarget[0] == psTarget)
		{
			// if a unit is targetted by a counter battery sensor
			// it is automatically seen
			return TRUE;
		}

		range = ((STRUCTURE *)psViewer)->sensorRange;
		senPower = ((STRUCTURE *)psViewer)->sensorPower;

		// increase the sensor range for AA sites
		// AA sites are defensive structures that can only shoot in the air
		if ( (psStruct->pStructureType->type == REF_DEFENSE) &&
			 (asWeaponStats[psStruct->asWeaps[0].nStat].surfaceToAir == SHOOT_IN_AIR) )
		{
			range = 3 * range / 2;
		}

		break;
	default:
		ASSERT( FALSE,
			"visibleObject: visibility checking is only implemented for"
			"units and structures" );
		return FALSE;
		break;
	}

	/* Get the target's ecm power (if it has one)
	 * or that of a nearby ECM droid.
	 */
	switch (psTarget->type)
	{
	case OBJ_DROID:
		ecmPower = ((DROID *)psTarget)->ECMMod;
		break;
	case OBJ_STRUCTURE:
		ecmPower = ((STRUCTURE *)psTarget)->ecmPower;
		range = 4 * range / 3;
		break;
	default:
		/* No ecm so zero power */
		ecmPower = 0;
		break;
	}

	/* Implement ECM by making sensor range two thirds of normal when
	 * enemy's ECM rating is higher than our sensor power rating. */
	if (ecmPower > senPower)
	{
		range = range * 2 / 3;
	}

	/* First see if the target is in sensor range */
	x = (SDWORD)psViewer->pos.x;
	xdiff = x - (SDWORD)psTarget->pos.x;
	if (xdiff < 0)
	{
		xdiff = -xdiff;
	}
	if (xdiff > range)
	{
		// too far away, reject
		return FALSE;
	}

	y = (SDWORD)psViewer->pos.y;
	ydiff = y - (SDWORD)psTarget->pos.y;
	if (ydiff < 0)
	{
		ydiff = -ydiff;
	}
	if (ydiff > range)
	{
		// too far away, reject
		return FALSE;
	}

	rangeSquared = xdiff*xdiff + ydiff*ydiff;
	if (rangeSquared > (range*range))
	{
		/* Out of sensor range */
		return FALSE;
	}

	if (rangeSquared == 0)
	{
		// Should never be on top of each other, but ...
		return TRUE;
	}

	// initialise the callback variables
	startH = psViewer->pos.z;
	startH += visObjHeight(psViewer);
	currG = -UBYTE_MAX * GRAD_MUL * ELEVATION_SCALE;
	tarDist = rangeSquared;
	rayStart = TRUE;
	currObj = 0;
	ray = NUM_RAYS-1 - calcDirection(psViewer->pos.x,psViewer->pos.y, psTarget->pos.x,psTarget->pos.y);
	finalX = map_coord(psTarget->pos.x);
	finalY = map_coord(psTarget->pos.y);

	// Cast a ray from the viewer to the target
	rayCast(x,y, ray, range, rayLOSCallback);

	// See if the target can be seen
	top = ((SDWORD)psTarget->pos.z + visObjHeight(psTarget) - startH);
	tarG = top * GRAD_MUL / lastD;

	return tarG >= currG;
}

// Do visibility check, but with walls completely blocking LOS.
BOOL visibleObjWallBlock(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget)
{
	BOOL	result;

	blockingWall = TRUE;
	result = visibleObject(psViewer,psTarget);
	blockingWall = FALSE;

	return result;
}

// Find the wall that is blocking LOS to a target (if any)
BOOL visGetBlockingWall(BASE_OBJECT *psViewer, BASE_OBJECT *psTarget, STRUCTURE **ppsWall)
{
	SDWORD		tileX, tileY, player;
	STRUCTURE	*psCurr, *psWall;

	blockingWall = TRUE;
	numWalls = 0;
	visibleObject(psViewer, psTarget);
	blockingWall = FALSE;

	// see if there was a wall in the way
	psWall = NULL;
	if (numWalls == 1)
	{
		tileX = map_coord(wallX);
		tileY = map_coord(wallY);
		for(player=0; player<MAX_PLAYERS; player += 1)
		{
			for(psCurr = apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
			{
				if (map_coord(psCurr->pos.x) == tileX
				 && map_coord(psCurr->pos.y) == tileY)
				{
					psWall = psCurr;
					goto found;
				}
			}
		}
	}

found:
	*ppsWall = psWall;

	return psWall != NULL;;
}

/* Find out what can see this object */
void processVisibility(BASE_OBJECT *psObj)
{
	DROID		*psDroid;
	STRUCTURE	*psBuilding;
	UDWORD		i, maxPower, ecmPoints;
	ECM_STATS	*psECMStats;
	BOOL		prevVis[MAX_PLAYERS], currVis[MAX_PLAYERS];
	SDWORD		visLevel;
	BASE_OBJECT	*psViewer;
	MESSAGE		*psMessage;
	UDWORD		player, ally;

	// calculate the ecm power for the object based on other ECM's in the area

	maxPower = 0;

	// set the current ecm power
	switch (psObj->type)
	{
	case OBJ_DROID:
		psDroid = (DROID *)psObj;
		psECMStats = asECMStats + psDroid->asBits[COMP_ECM].nStat;
		ecmPoints = ecmPower(psECMStats, psDroid->player);
		if (ecmPoints < maxPower)
		{
			psDroid->ECMMod = maxPower;
		}
		else
		{
			psDroid->ECMMod = ecmPoints;
			maxPower = psDroid->ECMMod;
		}
		// innate cyborg bonus
		if (cyborgDroid((DROID*)psObj))
		{
			psDroid->ECMMod += 500;
		}
		break;
	case OBJ_STRUCTURE:
		psBuilding = (STRUCTURE *)psObj;
		psECMStats = psBuilding->pStructureType->pECM;
		if (psECMStats && psECMStats->power > maxPower)
		{
			psBuilding->ecmPower = (UWORD)psECMStats->power;
		}
		else
		{
			psBuilding->ecmPower = (UWORD)maxPower;
			maxPower = psBuilding->ecmPower;
		}
		break;
	case OBJ_FEATURE:
	default:
		// no ecm's on features
		break;
	}

	// initialise the visibility array
	for (i=0; i<MAX_PLAYERS; i++)
	{
		prevVis[i] = psObj->visible[i] != 0;
	}
	if (psObj->type == OBJ_DROID)
	{
		memset (currVis, 0, sizeof(BOOL) * MAX_PLAYERS);

		// one can trivially see oneself
		currVis[psObj->player]=TRUE;
	}
	else
	{
		memcpy(currVis, prevVis, sizeof(BOOL) * MAX_PLAYERS);
	}

	// get all the objects from the grid the droid is in
	gridStartIterate((SDWORD)psObj->pos.x, (SDWORD)psObj->pos.y);

	// Make sure allies can see us
	if (bMultiPlayer && game.alliance == ALLIANCES_TEAMS)
	{
		for(player=0; player<MAX_PLAYERS; player++)
		{
			if(player!=psObj->player)
			{
				if(aiCheckAlliances(player,psObj->player))
				{
					currVis[player] = TRUE;
				}
			}
		}
	}

	// if a player has a SAT_UPLINK structure, they can see everything!
	for (player=0; player<MAX_PLAYERS; player++)
	{
		if (getSatUplinkExists(player))
		{
			currVis[player] = TRUE;
			if (psObj->visible[player] == 0)
			{
				psObj->visible[player] = 1;
			}
		}
	}

	psViewer = gridIterate();
	while (psViewer != NULL)
	{
		// If we've got ranged line of sight...
 		if ( (psViewer->type != OBJ_FEATURE) &&
			 !currVis[psViewer->player] &&
			 visibleObject(psViewer, psObj) )
 		{
			// Tell system that this side can see this object
 			currVis[psViewer->player]=TRUE;
			if (!prevVis[psViewer->player])
			{

				if (psObj->visible[psViewer->player] == 0)
				{
					psObj->visible[psViewer->player] = 1;
				}
				if(psObj->type != OBJ_FEATURE)
				{
					// features are not in the cluster system
					clustObjectSeen(psObj, psViewer);
				}
			}

 		}

		psViewer = gridIterate();
	}

	//forward out vision to our allies
	if (bMultiPlayer && game.alliance == ALLIANCES_TEAMS)
	{
		for(player = 0; player < MAX_PLAYERS; player++)
		{
			for(ally = 0; ally < MAX_PLAYERS; ally++)
			{
				if (currVis[player] && aiCheckAlliances(player, ally))
				{
					currVis[ally] = TRUE;
				}
			}
		}
	}

	// update the visibility levels
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if (i == psObj->player)
		{
			// owner can always see it fully
			psObj->visible[i] = UBYTE_MAX;
			continue;
		}

		visLevel = 0;
		if (currVis[i])
		{
			visLevel = UBYTE_MAX;
		}

		if ( (visLevel < psObj->visible[i]) &&
			 (psObj->type == OBJ_DROID) )
		{
			if (psObj->visible[i] <= visLevelDec)
			{
				psObj->visible[i] = 0;
			}
			else
			{
				psObj->visible[i] = (UBYTE)(psObj->visible[i] - visLevelDec);
			}
		}
		else if (visLevel > psObj->visible[i])
		{
			if (psObj->visible[i] + visLevelInc >= UBYTE_MAX)
			{
				psObj->visible[i] = UBYTE_MAX;
			}
			else
			{
				psObj->visible[i] = (UBYTE)(psObj->visible[i] + visLevelInc);
			}
		}
	}

	/* Make sure all tiles under a feature/structure become visible when you see it */
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if( (psObj->type == OBJ_STRUCTURE || psObj->type == OBJ_FEATURE) &&
			(!prevVis[i] && psObj->visible[i]) )
		{
			setUnderTilesVis(psObj,i);
		}
	}

	// if a feature has just become visible set the tile flags
	if (psObj->type == OBJ_FEATURE && !prevVis[selectedPlayer] && psObj->visible[selectedPlayer])
	{
		/*if this is an oil resource we want to add a proximity message for
		the selected Player - if there isn't an Resource Extractor on it*/
		if (((FEATURE *)psObj)->psStats->subType == FEAT_OIL_RESOURCE)
		{
			if(!TILE_HAS_STRUCTURE(mapTile(map_coord(psObj->pos.x),
			                               map_coord(psObj->pos.y))))
			{
				psMessage = addMessage(MSG_PROXIMITY, TRUE, selectedPlayer);
				if (psMessage)
				{
					psMessage->pViewData = (MSG_VIEWDATA *)psObj;
				}
				if(!bInTutorial)
				{
					//play message to indicate been seen
					audio_QueueTrackPos( ID_SOUND_RESOURCE_HERE,
										psObj->pos.x, psObj->pos.y, psObj->pos.z );
				}
			}
		}

			if (((FEATURE *)psObj)->psStats->subType == FEAT_GEN_ARTE)
			{
				psMessage = addMessage(MSG_PROXIMITY, TRUE, selectedPlayer);
				if (psMessage)
				{
					psMessage->pViewData = (MSG_VIEWDATA *)psObj;
				}
				if(!bInTutorial)
				{
					//play message to indicate been seen
					audio_QueueTrackPos( ID_SOUND_ARTEFACT_DISC,
									psObj->pos.x, psObj->pos.y, psObj->pos.z );
				}
			}
	}
}

void	setUnderTilesVis(BASE_OBJECT *psObj,UDWORD player)
{
UDWORD		i,j;
UDWORD		mapX, mapY, width,breadth;
FEATURE		*psFeature;
STRUCTURE	*psStructure;
FEATURE_STATS	*psStats;
MAPTILE		*psTile;

	if(psObj->type == OBJ_FEATURE)
	{
		psFeature = (FEATURE*)psObj;
		psStats = psFeature->psStats;
		width = psStats->baseWidth;
		breadth = psStats->baseBreadth;
	 	mapX = map_coord(psFeature->pos.x - width * TILE_UNITS / 2);
		mapY = map_coord(psFeature->pos.y - breadth * TILE_UNITS / 2);
	}
	else
	{
		/* Must be a structure */
		psStructure = (STRUCTURE*)psObj;
		width = psStructure->pStructureType->baseWidth;
		breadth = psStructure->pStructureType->baseBreadth;
		mapX = map_coord(psStructure->pos.x - width * TILE_UNITS / 2);
		mapY = map_coord(psStructure->pos.y - breadth * TILE_UNITS / 2);
	}

	for (i = 0; i < width; i++)
	{
		for (j = 0; j < breadth; j++)
		{

			/* Slow fade up */
			if(getRevealStatus())
			{
				if(player == selectedPlayer)
				{
					avInformOfChange(mapX+i,mapY+j);
				}
			}

			psTile = mapTile(mapX+i,mapY+j);
			SET_TILE_VISIBLE(player, psTile);
		}
	}
}

void updateSensorDisplay()
{
	MAPTILE		*psTile = psMapTiles;
	int		x;
	DROID		*psDroid;
	STRUCTURE	*psStruct;

	// clear sensor info
	for (x = 0; x < mapWidth * mapHeight; x++)
	{
		psTile->activeSensor = FALSE;
		psTile++;
	}

	// process the sensor range of all droids/structs.

	// units.
	for(psDroid = apsDroidLists[selectedPlayer];psDroid;psDroid=psDroid->psNext)
	{
		visTilesUpdate((BASE_OBJECT*)psDroid);
	}

	// structs.
	for(psStruct = apsStructLists[selectedPlayer];psStruct;psStruct=psStruct->psNext)
	{
		if (psStruct->pStructureType->type != REF_WALL
 		    && psStruct->pStructureType->type != REF_WALLCORNER)
		{
			visTilesUpdate((BASE_OBJECT*)psStruct);
		}
	}
}
