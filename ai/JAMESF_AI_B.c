/*
*  Nuno Das Neves z3465140, Andrew Li z3466313
*  Tuesday 12 Kora, William Archinal
*  1/6/2016
*  This is superTwerk.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "Game.h"
#include "mechanicalTurk.h"

//define all starting campus paths
#define ATOP ""
#define ABOT "RLRLRLRLRLL"
#define BTOP "RRLRL"
#define BBOT "LRRLRLRLLRLL"
#define CTOP "LRLRRLLLL"
#define CBOT "RRLRLLRLRL"

// board stuff
#define NUM_DISCIPLINES 6
#define NUM_VERTICES 54
#define NUM_NEIGHBORS 3

// path stuff
#define BASEPATH_R "RRLR"
#define BASEPATH_L "LRLRL"

// thing
#define INVALID -1

// translatePath stuff
#define DIREC_B 0
#define DIREC_R 1
#define DIREC_L 2

#define ALPHA_1 0
#define ALPHA_2 1
#define ALPHA_3 2
#define BETA_1 3
#define BETA_2 4
#define BETA_3 5

//for GO8 upgrades
// how many campuses we need to have before starting to consider GO8s
#define GO8_THRESHOLD 4
// this is a constant; max no of GO8s allowed in the game
#define MAX_GO8S 8

// choices
#define NONE 0
#define EXPAND 1
#define UPGRADE 2
#define SPINOFF 3

// --------------------- structs --------------------

struct pathArray { //for returning arrays of strings
	path paths[NUM_VERTICES];
};

typedef struct board * Board;

// the vertex struct
typedef struct vertex {
	int xPos, yPos;               // absolute coordinates
	int region[3];                // the 3 regions (or lack thereof) it's connected to
	int diceValue[3];             // dice values of corresponding regions
	int discipline[3];            // disciplines of corresponding regions
	int retrainCenter;            // 'NUL', 1-5 (can't retrain THD)
	int campusType;               // 'NUL', 1-3 (campusA, etc.) GO8 campuses (4-6)
	int arc[3];                   // 0-3 (see contents of ARC constants)
								  // maybe -1 to check for validity (can't build an arc there, off the map)
								  // 0 is unoccupied
	vertex * neighbor[3];          // pointers to the neighbors
	path myPath;                  // store a path from the origin
	int score;                    // point score
} vertex;

typedef struct playerStats {
	int kpiPointsCount;
	int arcCount;
	int go8Count;
	int campusCount;
	int ippCount;
	int publicationCount;
	int studentCount[NUM_DISCIPLINES];
	int retrainingCenterCount[NUM_DISCIPLINES];
} playerStats;

typedef struct board {
	int currentTurn;
	int diceRoll;
	int whoseTurn;
	int discipline[NUM_REGIONS];
	int dice[NUM_REGIONS];
	int mostARCs;           // we need to store who currently has the most ARCs
	int mostPublications;   // same idea

	playerStats unis[NUM_UNIS + 1];
	// 0 is a 'null' player ?
	vertex nodes[NUM_VERTICES];

} board;


//---------------- functions ----------------------

// evaluate whether it is possible to retrain and then build something this turn
static int getBuildPotential(Game g, int choiceCode);

// spinoff stuff
static action makeSpinoffs(Game g);
static int pickSpinRetrainTo(int countMJ, int countMTV,
	int countMMONEY);

// retraining and building campus/ARC
static int enoughForBuild(Game g, path chosenPoint, path pathMinusOne);
static action retrainForBuild(Game g);
static int pickBuildRetrainTo(int countBPS, int countBQN,
	int countMJ, int countMTV);
static action build(Game g, Board b, path chosenPoint, path pathMinusOne);

//upgrading to GO8
static int enoughForUpgrade(Game g);
static action retrainForUpgrade(Game g);
static int pickUpgradeRetrainTo(int countMJ, int countMMONEY);
static action upgrade(Game g, Board b, path chosenPoint);

// comparing vertices and getting their point values
static char* comparePoints(Game g, Board b, pathArray frontierPaths);
static char* compareCampuses(Game g, Board b, pathArray myCampusPaths);
static int diversityScore(Game g, vertex* point);
//static int upgradeTransform(Game g, Board b, vertex* point); //for transformation of upgrade candidate's scores

// frontier stuff
static pathArray searchMyCampus(Game g, Board b);
static pathArray myFrontier(Game g, Board b, pathArray myCampusPaths);

// Board functions
static Board getBoard(Game g);
static void freeBoard(Board b);
static int getVertIndex(Board b, int x, int y);
static int translatePath(Board b, path input);
static vertex* setNeighbor(Board b, int x, int y);
static vertex setPointers(Board b, vertex buffer, int orientation);
static int isNoAdjacentCampus(Board b, int vertIndex);
static void printVerticesInfo(Board b);

// prints out which campuses we have
static void printMap(Game g, Board b);

action decideAction(Game g) {

	//check game exists
	assert(g != NULL);

	// default action
	action theAction;
	theAction.actionCode = PASS;

	Board b = getBoard(g);
	//also check myCampusPaths and myFrontier stuffs is compatible with the board that we include here in the AI
	printVerticesInfo(b);
	//function here for loop through board to search for campuses info with getCampus(g, myPath)
	//Outputs array for 54 vertices with owned campus vertex path, else '\0'. Element number indicates vertex number
	pathArray myCampusPaths = searchMyCampus(g, b);
	//concatenate LL, LR, RL, RR, BL, BR onto myPath of non-empty element in the myCampusPaths output array
	//check legality (sufficient distance from existing assets, vacant arcs to reach vertex in two moves and vacant vertex)
	pathArray frontierPaths = myFrontier(g, b, myCampusPaths);

	// ----------------- make a choice based on our frontier and other factors -------------------
	// -ideally- this is the only code we change in decideAction (but maybe not not the case)

	int choice = NONE;
	int allowedGO8 = (getGO8s(g, UNI_A) + getGO8s(g, UNI_B) + getGO8s(g, UNI_C)) < MAX_GO8S;
	// if we're allowed to, we're above some number of campuses, and we have the resources, upgrade!
	if ((allowedGO8 == TRUE)
		&& (getCampuses(g, getWhoseTurn(g)) > GO8_THRESHOLD)     // a threshold we set so we don't build GO8s too early
		&& (getBuildPotential(g, UPGRADE) == TRUE)) {
		choice = UPGRADE;
		// otherwise, as long as our frontier isn't empty...
	} else if (strcmp(frontierPaths.paths[0], "0") != 0) { //when frontier is populated: array has structure [contiguous valid elements, contiguous NULLS]
		choice = EXPAND;
	} else if ((allowedGO8 == TRUE) && (getCampuses(g, getWhoseTurn(g)) > 0)) { //  when there's nowhere left to expand, try to build GO8s
																				//printf ("mechanicalTwerk: Frontier is empty!\n");
		choice = UPGRADE;
	} else {
		//printf ("mechanicalTwerk: Frontier is empty!\n");
		choice = SPINOFF; // otherwise, build spinoffs
	}

	// ---------------- based on the choice, logic to determine the specific action --------------
	if (choice == EXPAND) {
		//printf ("mechanicalTwerk: Choice = EXPAND\n");

		//finds optimal valid campus development two steps away by finding highest score point from frontier
		//and then builds the 2 arcs towards chosen point and build 1 campus on that chosen point
		char* chosenPoint = comparePoints(g, b, frontierPaths); //define chosenPoint as the optimal score path from frontier set
																//chosenPoint is two steps from an owned campus, pathMinusOne defines the one step vertex in between

		path pathMinusOne;
		strncpy(pathMinusOne, chosenPoint, strlen(chosenPoint) - 1);
		pathMinusOne[strlen(chosenPoint) - 1] = 0;

		if (enoughForBuild(g, chosenPoint, pathMinusOne) == FALSE) { //if not enough resources for building 2 arcs and 1 campus
																	 //printf ("  Not enough for build, attempting retrain\n");
			theAction = retrainForBuild(g); //retrain towards {0,3,3,1,1,0} requirement, otherwise pass
		} else { //if enough resources for building 2 arcs and 1 campus
			theAction = build(g, b, chosenPoint, pathMinusOne);
			//printf ("  Expanding!\n");
		}
		free(chosenPoint);

	} else if (choice == UPGRADE) {
		//printf ("mechanicalTwerk: Choice = UPGRADE\n");

		if (enoughForUpgrade(g) == FALSE) {
			//printf ("  Not enough for upgrade, attempting retrain\n");
			theAction = retrainForUpgrade(g);
		} else {
			char * chosenCampus = compareCampuses(g, b, myCampusPaths);

			theAction = upgrade(g, b, chosenCampus);
			//printf ("  Upgrading!\n");
			free(chosenCampus);
		}

	} else if (choice == SPINOFF) {
		//printf ("mechanicalTwerk: Choice = SPINOFF\n");
		theAction = makeSpinoffs(g); //spins and retrains for spins, otherwise pass

	} else if (choice == NONE) {
		//printf ("mechanicalTwerk: Choice = NONE\n");
	}

	// just in case
	if (isLegalAction(g, theAction) == FALSE) {
		//printf ("mechanicalTwerk: Oops, illegal move! Building SPINOFFs\n");
		theAction = makeSpinoffs(g);
	}

	printMap(g, b);

	freeBoard(b);

	return theAction;
}

static void printMap(Game g, Board b) {
	/*int vertIndex = 0;
	while (vertIndex < NUM_VERTICES) {
	if (b->nodes[vertIndex].campusType == getWhoseTurn(g)) {
	printf ("Player %d has a campus at (%d, %d)\n", getWhoseTurn(g), b->nodes[vertIndex].xPos, b->nodes[vertIndex].yPos);
	} else if (b->nodes[vertIndex].campusType == (getWhoseTurn(g) + 3)) {
	printf ("Player %d has a GO8 at (%d, %d)\n", getWhoseTurn(g), b->nodes[vertIndex].xPos, b->nodes[vertIndex].yPos);
	}
	vertIndex++;
	}*/
}

static action makeSpinoffs(Game g) {

	action theAction;

	int player = getWhoseTurn(g);

	int countBPS = getStudents(g, player, STUDENT_BPS);
	int countBQN = getStudents(g, player, STUDENT_BQN);
	int countMJ = getStudents(g, player, STUDENT_MJ);
	int countMTV = getStudents(g, player, STUDENT_MTV);
	int countMMONEY = getStudents(g, player, STUDENT_MMONEY);

	//Spinoff action requires 1 MJ, 1 MTV and 1 MMONEY
	//Spinoff until no longer able to spinoff
	if ((countMJ > 0) && (countMTV > 0) && (countMMONEY > 0)) {
		theAction.actionCode = START_SPINOFF;
	} else {
		theAction.actionCode = RETRAIN_STUDENTS;

		if (countMTV >=
			(getExchangeRate(g, player, STUDENT_MTV, STUDENT_THD) + 1)) { //similarly, ensures min. 1
																		  //retrain from BQN
			theAction.disciplineFrom = STUDENT_MTV;
			theAction.disciplineTo = pickSpinRetrainTo(countMJ, countMTV, countMMONEY);

		} else if (countMMONEY >=
			(getExchangeRate(g, player, STUDENT_MMONEY, STUDENT_THD) + 1)) { //(min. 0) is exchange as soon as we have enough to do so
																			 //retrain from BQN
			theAction.disciplineFrom = STUDENT_MMONEY;
			theAction.disciplineTo = pickSpinRetrainTo(countMJ, countMTV, countMMONEY);

		} else if (getBuildPotential(g, SPINOFF) == TRUE) {
			if (countBPS >=
				(getExchangeRate(g, player, STUDENT_BPS, STUDENT_THD) + 0)) {
				//retrain from BPS
				theAction.disciplineFrom = STUDENT_BPS;
				theAction.disciplineTo = pickSpinRetrainTo(countMJ, countMTV, countMMONEY);

			} else if (countBQN >=
				(getExchangeRate(g, player, STUDENT_BQN, STUDENT_THD) + 0)) {
				//retrain from BQN
				theAction.disciplineFrom = STUDENT_BQN;
				theAction.disciplineTo = pickSpinRetrainTo(countMJ, countMTV, countMMONEY);

			} else if (countMJ >=
				(getExchangeRate(g, player, STUDENT_MJ, STUDENT_THD) + 1)) { //similarly, ensures min. 1
																			 //retrain from MJ
				theAction.disciplineFrom = STUDENT_MJ;
				theAction.disciplineTo = pickSpinRetrainTo(countMJ, countMTV, countMMONEY);
			} else { //otherwise pass
				theAction.actionCode = PASS;
			}
		} else { //otherwise pass
			theAction.actionCode = PASS;
		}
	}

	return theAction;
}

static int pickSpinRetrainTo(int countMJ, int countMTV,
	int countMMONEY) { //chooses next retrain to, approaches the {0,0,0,1,1,1} distribution

	int value = STUDENT_MJ;

	if ((countMJ <= countMTV) && (countMJ <= countMMONEY)) {
		value = STUDENT_MJ; //choose MJ => value = 3
	} else if ((countMTV < countMJ) && (countMTV <= countMMONEY)) {
		value = STUDENT_MTV; //choose MTV => value = 4
	} else if ((countMMONEY < countMJ) && (countMMONEY < countMTV)) {
		value = STUDENT_MMONEY; //choose MMONEY => value = 5
	}

	return value;
}

static int enoughForBuild(Game g, path chosenPoint, path pathMinusOne) { //enough to build 2 arcs and 1 campus?
	int enough = FALSE; //assumes insufficient for now
	int player = getWhoseTurn(g);
	//count of student types required to build 2 arcs and 1 campus
	int countBPS = getStudents(g, player, STUDENT_BPS);
	int countBQN = getStudents(g, player, STUDENT_BQN);
	int countMJ = getStudents(g, player, STUDENT_MJ);
	int countMTV = getStudents(g, player, STUDENT_MTV);
	int minCost = 3;

	if (getARC(g, pathMinusOne) == player) {
		minCost--;
		if (getARC(g, chosenPoint) == player) {
			minCost--;
		}
	}

	if ((countBPS >= minCost) && (countBQN >= minCost)
		&& (countMJ >= 1) && (countMTV >= 1)) { //if sufficient resources, then switch enough to TRUE
		enough = TRUE;
	}

	return enough;
}

static action retrainForBuild(Game g) { //defines retrain action for build, approaches the {0,3,3,1,1,0} distribution
	action theAction;
	theAction.actionCode = RETRAIN_STUDENTS;
	int player = getWhoseTurn(g);

	int countBPS = getStudents(g, player, STUDENT_BPS);
	int countBQN = getStudents(g, player, STUDENT_BQN);
	int countMJ = getStudents(g, player, STUDENT_MJ);
	int countMTV = getStudents(g, player, STUDENT_MTV);
	int countMMONEY = getStudents(g, player, STUDENT_MMONEY);

	if (countMTV >=
		(getExchangeRate(g, player, STUDENT_MTV, STUDENT_THD) + 1)) { //similarly, ensures min. 1
																	  //retrain from BQN
		theAction.disciplineFrom = STUDENT_MTV;
		theAction.disciplineTo = pickBuildRetrainTo(countBPS, countBQN,
			countMJ, countMTV);
	} else if (countMMONEY >=
		(getExchangeRate(g, player, STUDENT_MMONEY, STUDENT_THD) + 0)) { //(min. 0) is exchange as soon as we have enough to do so
																		 //retrain from BQN
		theAction.disciplineFrom = STUDENT_MMONEY;
		theAction.disciplineTo = pickBuildRetrainTo(countBPS, countBQN,
			countMJ, countMTV);
	} else if (getBuildPotential(g, EXPAND) == TRUE) {
		if (countBPS >=
			(getExchangeRate(g, player, STUDENT_BPS, STUDENT_THD) + 3)) { //if count is more than (3 + exchangeRate) then retrain from it => 3 + exchangeRate - exchangeRate = 3
																		  //retrain from BPS
			theAction.disciplineFrom = STUDENT_BPS;
			theAction.disciplineTo = pickBuildRetrainTo(countBPS, countBQN,
				countMJ, countMTV);
		} else if (countBQN >=
			(getExchangeRate(g, player, STUDENT_BQN, STUDENT_THD) + 3)) { //similarly, ensures min. 3
																		  //retrain from BQN
			theAction.disciplineFrom = STUDENT_BQN;
			theAction.disciplineTo = pickBuildRetrainTo(countBPS, countBQN,
				countMJ, countMTV);
		} else if (countMJ >=
			(getExchangeRate(g, player, STUDENT_MJ, STUDENT_THD) + 1)) { //similarly, ensures min. 1
																		 //retrain from MJ
			theAction.disciplineFrom = STUDENT_MJ;
			theAction.disciplineTo = pickBuildRetrainTo(countBPS, countBQN,
				countMJ, countMTV);
		} else { //otherwise pass
			theAction.actionCode = PASS;
		}
	} else { //otherwise pass
		theAction.actionCode = PASS;
	}

	return theAction;
}

static int pickBuildRetrainTo(int countBPS, int countBQN,
	int countMJ, int countMTV) { //order of precedence: to BPS, to BQN, to MJ, to MTV (MTV last due to effect of rolling 7's)

	int value = STUDENT_BPS;

	if (countBPS < 3) {
		value = STUDENT_BPS;
	} else if (countBQN < 3) {
		value = STUDENT_BQN;
	} else if (countMJ < 1) {
		value = STUDENT_MJ;
	} else if (countMTV < 1) {
		value = STUDENT_MTV;
	}

	return value;
}

static int enoughForUpgrade(Game g) { //enough to upgrade a campus to GO8
	int enough = FALSE; //assumes insufficient for now
	int player = getWhoseTurn(g);
	//count of student types required to upgrade a campus to GO8
	int countMJ = getStudents(g, player, STUDENT_MJ);
	int countMMONEY = getStudents(g, player, STUDENT_MMONEY);

	if ((countMJ >= 2) && (countMMONEY >= 3)) { //if sufficient resources, then switch enough to TRUE
		enough = TRUE;
	}

	return enough;
}

static int getBuildPotential(Game g, int choiceCode) {
	int totalStudents = 0;
	// need to be 6 because of annoying convention THD = 0 etc
	int spare[6] = { 0 };
	int threshold[6] = { 0 };
	int player = getWhoseTurn(g);
	int potential = 0;
	int result = FALSE;

	if (choiceCode == EXPAND) {
		totalStudents = 8;
		threshold[STUDENT_BPS] = 3;
		threshold[STUDENT_BQN] = 3;
		threshold[STUDENT_MTV] = 1;
		threshold[STUDENT_MJ] = 1;
	} else if (choiceCode == UPGRADE) {
		totalStudents = 5;
		threshold[STUDENT_MJ] = 2;
		threshold[STUDENT_MMONEY] = 3;
	} else if (choiceCode == SPINOFF) {
		totalStudents = 3;
		threshold[STUDENT_MJ] = 1;
		threshold[STUDENT_MTV] = 1;
		threshold[STUDENT_MMONEY] = 1;
	}

	// annoying off by one buffer overflow fix
	int student = STUDENT_BPS;
	while (student <= STUDENT_MMONEY) {
		if (getStudents(g, player, student) > threshold[student]) {
			spare[student] = getStudents(g, player, student) - threshold[student];
			potential += threshold[student];
			potential += (spare[student] / getExchangeRate(g, player, student, STUDENT_THD));
		} else {
			potential += getStudents(g, player, student);
		}
		student++;
	}
	/*if (choiceCode == UPGRADE) {
	printf (" UPGRADE potential = %d\n", potential);
	}*/
	if (potential >= totalStudents) {
		result = TRUE;
	}

	return result;
}

static action retrainForUpgrade(Game g) { //defines retrain action for upgrade, approaches the {0,0,0,2,0,3} distribution
	action theAction;
	theAction.actionCode = RETRAIN_STUDENTS;

	int player = getWhoseTurn(g);
	int countBPS = getStudents(g, player, STUDENT_BPS);
	int countBQN = getStudents(g, player, STUDENT_BQN);
	int countMJ = getStudents(g, player, STUDENT_MJ);
	int countMTV = getStudents(g, player, STUDENT_MTV);
	int countMMONEY = getStudents(g, player, STUDENT_MMONEY);

	theAction.disciplineTo = pickUpgradeRetrainTo(countMJ, countMMONEY);

	//if count is more than (min + exchangeRate) then retrain from it e.g. 3 + exchangeRate - exchangeRate = 3
	if (countBPS >=
		(getExchangeRate(g, player, STUDENT_BPS, STUDENT_THD) + 0)) { //ensure min. 0
																	  //retrain from BPS
		theAction.disciplineFrom = STUDENT_BPS;
	} else if (countBQN >=
		(getExchangeRate(g, player, STUDENT_BQN, STUDENT_THD) + 0)) { //similarly, ensures min. 0
																	  //retrain from BQN
		theAction.disciplineFrom = STUDENT_BQN;
	} else if (countMJ >=
		(getExchangeRate(g, player, STUDENT_MJ, STUDENT_THD) + 2)) { //similarly, ensures min. 2
																	 //retrain from MJ
		theAction.disciplineFrom = STUDENT_MJ;
	} else if (countMTV >=
		(getExchangeRate(g, player, STUDENT_MTV, STUDENT_THD) + 0)) { //similarly, ensures min. 0
																	  //retrain from BQN
		theAction.disciplineFrom = STUDENT_MTV;
	} else if (countMMONEY >=
		(getExchangeRate(g, player, STUDENT_MMONEY, STUDENT_THD) + 3)) { //similarly, ensures min. 3
																		 //retrain from BQN
		theAction.disciplineFrom = STUDENT_MMONEY;
	} else { //otherwise pass
		theAction.actionCode = PASS;
	}

	return theAction;
}

static int pickUpgradeRetrainTo(int countMJ, int countMMONEY) {
	//order of precedence: to MJ, to MMONEY (MMONEY last due to effect of rolling 7's)
	int value = STUDENT_MJ;

	if (countMJ < 2) {
		value = STUDENT_MJ;
	} else if (countMMONEY < 3) {
		value = STUDENT_MMONEY;
	}

	return value;
}

//note: since we included myCampusPaths to frontier, weight the campus upgrades by risk exposure to 7 rolls
//identify with getCampus() and apply score scaling.
//frontier contains the first n valid build moves followed by m valid upgrade moves and followed by empties (contiguous)
static char* comparePoints(Game g, Board b, pathArray frontierPaths) {
	int diceValues[] = { 2,3,4,5,6,7,8,9,10,11,12 };
	int dicePoints[] = { 2,3,4,5,6,7,6,5,4,3,2 };
	// need an extra entry here because invalid region is
	int disciplinePoints[] = { 0,0,5,5,4,3,2 };	// expansion stuff worth more; more volatile worth less

	char* thePath = new char[150];  // the path we're going to return

	vertex * frontier[NUM_VERTICES];
	int i = 0;
	while (strcmp(frontierPaths.paths[i], "0") != 0) {
		frontier[i] = &(b->nodes[translatePath(b, frontierPaths.paths[i])]);
		i++;
	}
	frontier[i] = NULL;  // set the one after to null

	vertex* vert = frontier[0]; //start with holding the first element of frontier, if no better alternatives then return this
	strcpy(thePath, frontierPaths.paths[0]);
	int frontierID = 1; //counter starts with second element of frontier as comparison candidates
						/*
						if ((strlen(thePath) < 2)) {
						printf ("thePath is %s\n", thePath);
						printf ("frontierPaths.paths[0] is %s\n", frontierPaths.paths[0]);
						printf ("frontierID is %d\n", frontierID);
						printf ("frontierPaths.paths[frontierID] is %s\n", frontierPaths.paths[frontierID]);
						}
						*/
	while (frontier[frontierID] != NULL) { //search through valid frontier elements until first NULL is reached (contiguous property of frontier enables this)

		vert->score = 0; //initialise the score for the current best vertex
		frontier[frontierID]->score = 0; //initialise the score for the comparison candidate vertex

		int regCounter = 0; //counter for neighbour regions {0,1,2}
		int arrayCounter = 0; //counter which matches diceValue with dicePoint and allows search all

		while (regCounter <= 2) { //check all three neighbour regions
			arrayCounter = 0; //reset after updating vertex score by adding current region score

							  // add points for different regions. We need +1 because of empty regions
			vert->score += disciplinePoints[vert->discipline[regCounter] + 1];
			frontier[frontierID]->score += disciplinePoints[frontier[frontierID]->discipline[regCounter] + 1];

			while (arrayCounter < 12) {
				if ((vert->diceValue[regCounter] == diceValues[arrayCounter])  //find current neighbour region's score only
					&& (vert->discipline[regCounter] != STUDENT_THD)) {  // cancel dice points if THD
					vert->score += dicePoints[arrayCounter]; //update vertex score
				}

				//similar to vert
				if ((frontier[frontierID]->diceValue[regCounter] == diceValues[arrayCounter])
					&& (frontier[frontierID]->discipline[regCounter] != STUDENT_THD)) {         // cancel dice points if THD
					frontier[frontierID]->score += dicePoints[arrayCounter];
				}

				arrayCounter++;
			}

			regCounter++;
		}
		//  ----- reward retraining centres
		if (vert->retrainCenter > 0) {
			vert->score += 5;
		}
		if (frontier[frontierID]->retrainCenter > 0) {
			frontier[frontierID]->score += 5;
		}

		// ------------
		if ((frontier[frontierID]->score) > (vert->score)) { //replace current vert with better alternative if alternative score is an improvement.
			vert = frontier[frontierID];
			strcpy(thePath, frontierPaths.paths[frontierID]);
		} /*else if ((frontier[frontierID]->score) == (vert->score)) { //if scores based on first factor are equal, check a second priority factor. (for build alternative)
		  //diversity scores for comparison
		  int currentDiversity = diversityScore(g, vert);
		  int alternativeDiversity = diversityScore(g, frontier[frontierID]);

		  if (alternativeDiversity > currentDiversity) { //replace current vert with better alternative if alternative score is an improvement.
		  vert = frontier[frontierID];
		  strcpy(thePath, frontierPaths.paths[frontierID]);
		  }
		  }*/

		frontierID++;
	}

	return thePath;
}

// NOTE: this function assumes there will be at least one campusPath that is valid to upgrade to a GO8
static char* compareCampuses(Game g, Board b, pathArray myCampusPaths) {

	int diceValues[] = { 2,3,4,5,6,7,8,9,10,11,12 };
	int dicePoints[] = { 2,3,4,5,6,7,6,5,4,3,2 };
	int disciplinePoints[] = { 0,0,4,4,3,3,2 };

	char* theUpgradePath = new char[150];  // the campus path we're going to return

	vertex * campuses[NUM_VERTICES];

	int i = 0;
	int campusesIndex = 0;
	while (strcmp(myCampusPaths.paths[i], "0") != 0) {
		if (b->nodes[translatePath(b, myCampusPaths.paths[i])].campusType <= 3) {  // make sure it's not already a GO8
			campuses[campusesIndex] = &(b->nodes[translatePath(b, myCampusPaths.paths[i])]);
			campusesIndex++;
		}
		i++;
	}
	campuses[i] = NULL;  // set the one after to null

	vertex * vert = campuses[0]; //start with holding the first element of frontier, if no better alternatives then return this
	strcpy(theUpgradePath, campuses[0]->myPath);

	if (campuses[1] != NULL) {
		int frontierID = 1; //counter starts with second element of frontier as comparison candidates
		int regCounter = 0; //counter for neighbour regions {0,1,2}
		int arrayCounter = 0; //counter which matches diceValue with dicePoint and allows search all

		while (campuses[frontierID] != NULL && frontierID != 5) { //search through valid frontier elements until first NULL is reached (contiguous property of frontier enables this)
			vert->score = 0; //initialise the score for the current best vertex
			campuses[frontierID]->score = 0; //initialise the score for the comparison candidate vertex
			regCounter = 0;

			while (regCounter <= 2) { //check all three neighbour regions
				arrayCounter = 0; //reset after updating vertex score by adding current region score

				vert->score += disciplinePoints[vert->discipline[regCounter] + 1];
				campuses[frontierID]->score += disciplinePoints[campuses[frontierID]->discipline[regCounter] + 1];

				while (arrayCounter < 12) {
					if ((vert->diceValue[regCounter] == diceValues[arrayCounter])  //find current neighbour region's score only
						&& (vert->discipline[regCounter] != STUDENT_THD)) {  // cancel dice points if THD
						vert->score += dicePoints[arrayCounter]; //update vertex score
					}

					//similar to vert
					if ((campuses[frontierID]->diceValue[regCounter] == diceValues[arrayCounter])
						&& (campuses[frontierID]->discipline[regCounter] != STUDENT_THD)) {         // cancel dice points if THD
						campuses[frontierID]->score += dicePoints[arrayCounter];
					}

					arrayCounter++;
				}

				regCounter++;
			}
			// Diversity makes sense for GO8s i guess.
			if ((campuses[frontierID]->score) > (vert->score)) { //replace current vert with better alternative if alternative score is an improvement.
				vert = campuses[frontierID];
				strcpy(theUpgradePath, campuses[frontierID]->myPath);
			} else if ((campuses[frontierID]->score) == (vert->score)) { //if scores based on first factor are equal, check a second priority factor. (for build alternative)
																		 //diversity scores for comparison
				int currentDiversity = diversityScore(g, vert);
				int alternativeDiversity = diversityScore(g, campuses[frontierID]);

				if (alternativeDiversity > currentDiversity) { //replace current vert with better alternative if alternative score is an improvement.
					vert = campuses[frontierID];
					strcpy(theUpgradePath, campuses[frontierID]->myPath);
				}
			}

			frontierID++;
		}
	}

	return theUpgradePath;
}

// diversity
static int diversityScore(Game g, vertex* point) { //add points for every first incidence of a discipline.
	int diversity = 0;
	int firstBPS = FALSE; //THD ignored, always zero value
	int firstBQN = FALSE;
	int firstMJ = FALSE;
	int firstMTV = FALSE; //consider lower value, 7's convert this.
	int firstMMONEY = FALSE; //consider a lowest relative score compared to resources used for build. Also, 7's convert this.
	int regCounter = 0;

	while (regCounter <= 2) { // 6,4,3,2 levels allows 3+2>4 and 6>3+2 so defines priority of diversity and distribution of diversity by case.
		if ((point->discipline[regCounter] == STUDENT_BPS) && (firstBPS != TRUE)) {
			diversity += 6; //highest priority
			firstBPS = TRUE;
		} else if ((point->discipline[regCounter] == STUDENT_BQN) && (firstBQN != TRUE)) {
			diversity += 6; //highest priority
			firstBQN = TRUE;
		} else if ((point->discipline[regCounter] == STUDENT_MJ) && (firstMJ != TRUE)) {
			diversity += 4; //high priority
			firstMJ = TRUE;
		} else if ((point->discipline[regCounter] == STUDENT_MTV) && (firstMTV != TRUE)) {
			diversity += 3; //medium priority
			firstMTV = TRUE;
		} else if ((point->discipline[regCounter] == STUDENT_BQN) && (firstMMONEY != TRUE)) {
			diversity += 2; //low priority
			firstMMONEY = TRUE;
		}
		//HOW MUCH DO WE REWARD RETRAINING CENTRES?? <----------------------------------------------------------------------------------decide and write (low priority)
		regCounter++;
	}

	return diversity;
}

/*static int upgradeTransform(Game g, Board b, vertex* point) {
int transform = 0;

//creates rules for creating the transform value

return transform;
}*/
static action build(Game g, Board b, path chosenPoint, path pathMinusOne) { //change chosenPoint to chosenPath!!!!!!!!!!!
	action theAction;

	//printf ("chosenPoint = %s\n", chosenPoint);
	//printf ("  index = %d\n", translatePath(b, chosenPoint));
	//printf ("  pathMinusOne = %s\n", pathMinusOne);
	//printf ("  index = %d\n", translatePath(b, pathMinusOne));
	if (getARC(g, pathMinusOne) == VACANT_ARC) { //first priority, if owned point to one step point is vacant, then acquire the edge
		theAction.actionCode = OBTAIN_ARC;
		strcpy(theAction.destination, pathMinusOne);
	} else if (getARC(g, chosenPoint) == VACANT_ARC) { //second priority, if one step point to two step chosen point, then acquire that edge
		theAction.actionCode = OBTAIN_ARC;
		strcpy(theAction.destination, chosenPoint);
	} else if (getCampus(g, chosenPoint) == VACANT_VERTEX) { //third priority, if chosen point has no campus, then build it there
		theAction.actionCode = BUILD_CAMPUS;
		strcpy(theAction.destination, chosenPoint);
	}
	//printf("I'm here!\n");

	return theAction;
}

static action upgrade(Game g, Board b, path chosenPoint) {
	action theAction;

	if (getCampus(g, chosenPoint) == getWhoseTurn(g)) { //check that it is owned by current player and that it is campus (not GO8)
		theAction.actionCode = BUILD_GO8;
		strcpy(theAction.destination, chosenPoint);
	}

	return theAction;
}

static pathArray searchMyCampus(Game g, Board b) { //array of strings, for paths of current player current campuses.
	pathArray myCampus;

	//initialise myCampus with '\0's
	int pathCounter = 0;
	while (pathCounter < NUM_VERTICES) {
		strcpy(myCampus.paths[pathCounter], "0"); // not null characters! 0s! null is a legit path to the top campus
		pathCounter++;
	}

	int vertCounter = 0;
	int i = 0;

	while (vertCounter < NUM_VERTICES) {
		if ((getCampus(g, b->nodes[vertCounter].myPath) == getWhoseTurn(g))
			|| (getCampus(g, b->nodes[vertCounter].myPath) == getWhoseTurn(g) + 3)) { //if the current vertex has a campus or GO8 owned by the current player
			strcpy(myCampus.paths[i], b->nodes[vertCounter].myPath); //then, store the path of the vertex (with defined size limit)
																	 //printf ("  paths[%d] = %s\n", i, myCampus.paths[vertCounter]);
			i++;
		}
		vertCounter++;
	}

	return myCampus;
}

//concatenate LL, LR, RL, RR, BL, BR onto myPath of relevant element in the myCampusPaths array
//check legality (vertex exists, sufficient distance from existing assets, vacant arcs to reach vertex in two moves and vacant vertex)
static pathArray myFrontier(Game g, Board b, pathArray myCampusPaths) {
	pathArray frontier;

	//initialise myFrontier[] to NULLs
	int f = 0;
	while (f < NUM_VERTICES) {
		strcpy(frontier.paths[f], "0");   // initialize to the actual string 0
		f++;
	}

	path options[6];
	path branches[] = { "LL", "LR", "RL", "RR", "BL", "BR" };
	path pathMinusOne;

	int frontierIndex = 0;
	int inputIndex = 0;
	int opCounter = 0;

	while ((strcmp(myCampusPaths.paths[inputIndex], "0") != 0)) { //check all of my campuses
		opCounter = 0;
		while (opCounter < 6) { //generate all 6 'potential' moves around my campus
								//printf ("inputIndex = %d", inputIndex);
			strcpy(options[opCounter], myCampusPaths.paths[inputIndex]);
			strcat(options[opCounter], branches[opCounter]);
			//printf ("options[%d] = %s\n", opCounter, options[opCounter]);

			strncpy(pathMinusOne, options[opCounter], strlen(options[opCounter]) - 1);
			pathMinusOne[strlen(options[opCounter]) - 1] = 0;

			int nodeIndex = translatePath(b, options[opCounter]);
			//printf ("pathMinusOne = %s\n", pathMinusOne);
			//printf ("options[opCounter] = %s\n", options[opCounter]);

			if (nodeIndex != INVALID) {
				if ((isNoAdjacentCampus(b, nodeIndex) == TRUE)                          //check: existence of nearby assets
					&& (getCampus(g, options[opCounter]) == VACANT_VERTEX)     //check: vertex is vacant
					&& ((getARC(g, pathMinusOne) == VACANT_ARC)
						|| (getARC(g, pathMinusOne) == getWhoseTurn(g)))  //check: that first step to point is obtainable or owned
					&& ((getARC(g, options[opCounter]) == VACANT_ARC)
						|| (getARC(g, options[opCounter]) == getWhoseTurn(g))) //check: that second step to point is obtainable or owned
					) {
					strcpy(frontier.paths[frontierIndex], options[opCounter]); //output to myFrontier
					frontierIndex++;
				}
			}

			opCounter++;
		}
		inputIndex++;
	}

	return frontier;
}

// returns TRUE if there are no adjacent campuses
static int isNoAdjacentCampus(Board b, int vertIndex) {
	int noAdjacentCampus = TRUE;
	int neighborID = 0;
	// loop through the neighboring vertices
	while (neighborID < NUM_NEIGHBORS) {
		// if any are occupied, then this will return FALSE
		if (b->nodes[vertIndex].neighbor[neighborID] != NULL) {
			if (b->nodes[vertIndex].neighbor[neighborID]->campusType != VACANT_VERTEX) {
				noAdjacentCampus = FALSE;
			}
		}
		neighborID++;
	}
	return noAdjacentCampus;
}

// ----------------------------------------------------------------------------------------------------------------
// -------------------------------------- Board stuff and translatePath etc ---------------------------------------

Board getBoard(Game g) {
	// malloc the memory
	Board b = new board;
	// initial conditions
	b->currentTurn = getTurnNumber(g);
	b->whoseTurn = getWhoseTurn(g);

	int d = 0;
	// I'd like to just have array = array but i dont think that works in C
	while (d < NUM_REGIONS) {
		b->discipline[d] = getDiscipline(g, d);
		b->dice[d] = getDiceValue(g, d);
		d++;
	}

	int p = 1;
	while (p <= NUM_UNIS) {
		b->unis[p].kpiPointsCount = getKPIpoints(g, p);
		b->unis[p].arcCount = getARCs(g, p);
		b->unis[p].go8Count = getGO8s(g, p);
		b->unis[p].campusCount = getCampuses(g, p);
		b->unis[p].ippCount = getIPs(g, p);
		b->unis[p].publicationCount = getPublications(g, p);
		d = 0;
		while (d < NUM_DISCIPLINES) {
			b->unis[p].studentCount[d] = getStudents(g, p, d);
			d++;
		}
		p++;
	}
	// this is the hard part
	vertex v;          // a vertex object to put our stuff in

	v.retrainCenter = 0; // yeah
	v.campusType = VACANT_VERTEX;   // ..

	int x = 0; int y = 3;
	int R = 0; int Rb = 0;        // Big R; the region we're currently on
	int nReg = 2; int nRegb = 2;  // neighbor in region, going from 2->0 from highest to lowest
	int i = 0; int k = 0;         // just iteration variables; i loops through the array
								  // loop through all coordinates, placing verts in the array
								  // also add the region/dice/discipline data to each vert
								  // path stuff
	path rungDirection;
	int yCount = 0;
	int strlength = 0;

	while (!(x == 5 && y < -3)) {

		// -------------- this bit deals with paths .......zzzzzzzzzzz
		// get the paths for BASEPATH_R
		strcpy(v.myPath, "");   // just make a blank string to start with
		if (x <= 2) {
			// this copies in the current basePath minus a number based on x
			strlength = strlen(BASEPATH_R) - (x * 2);
			strncpy(v.myPath, BASEPATH_R, strlength);
			v.myPath[strlength] = 0;   // we need to do this apparently
									   // figure out if its the origin or not basically,
									   // and where our starting y value is
			if (x < 2) {         // if we're in x = 0 or x = 1
				strcpy(rungDirection, "L");
				if (x == 0) {
					yCount = 2;
					if (y <= 2) {
						strcat(v.myPath, "L");
					}
				} else {
					yCount = 3;
					if (y <= 3) {
						strcat(v.myPath, "L");
					}
				}
			} else {
				strcpy(rungDirection, "L");
				yCount = 4;
				if (y <= 4) {
					strcat(v.myPath, "R");
				}
			}

			// now we need to use our y position to figure out the rung
			while (yCount > y) {
				strcat(v.myPath, rungDirection);
				if (strcmp(rungDirection, "L") == 0) {
					strcpy(rungDirection, "R");
				} else {
					strcpy(rungDirection, "L");
				}
				yCount--;
			}
		} else {
			// this copies in the current basePath minus a corresponding x value
			strlength = strlen(BASEPATH_L) - ((-2 * x) + 10);
			strncpy(v.myPath, BASEPATH_L, strlength);
			v.myPath[strlength] = 0;   // we need to do this apparently

									   // figure out if its the origin or not basically,
									   // and where our starting y value is
			strcpy(rungDirection, "R");
			if (x == 3) {
				yCount = 4;
				// add an extra R if we're below the top rung
				if (y <= 4) {
					strcat(v.myPath, rungDirection);
				}
			} else if (x == 4) {
				yCount = 3;
				if (y <= 3) {
					strcat(v.myPath, rungDirection);
				}
			} else {
				yCount = 2;
				if (y <= 2) {
					strcat(v.myPath, rungDirection);
				}
			}

			// now we need to use our y position to figure out the rung
			while (yCount > y) {
				strcat(v.myPath, rungDirection);
				if (strcmp(rungDirection, "L") == 0) {
					strcpy(rungDirection, "R");
				} else {
					strcpy(rungDirection, "L");
				}
				yCount--;
			}
		}
		// --------------------------------------------------------------
		//printf ("nodes[%d] (%d, %d) path = %s \n", i, x, y, v.myPath);
		v.campusType = getCampus(g, v.myPath);

		k = 0;
		// for each new loop iteration, initialize all this jazz
		while (k < 3) {
			v.region[k] = INVALID;
			v.diceValue[k] = INVALID;
			v.discipline[k] = INVALID;
			k++;
		}

		//place the regions to the RIGHT of each vertex
		if (x < 5) {
			// we don't add regions to the right on these points, so just skip them
			if (!(x == 3 && (y == 5 || y == -5))
				&& !(x == 4 && (y == 4 || y == -4))) {
				// set the vertex's region, dice, discipline for the R (Region) we're up to
				v.region[nReg] = R;
				v.diceValue[nReg] = b->dice[R];
				v.discipline[nReg] = b->discipline[R];
				if (nReg == 0) {
					R++;
					nReg = 2;
					// if we're at the intersection of two regions, we need the next region on this vertex as well
					// (BUT NOT IF WE'RE AT THE BOTTOM OF A COLUMN - there's only 1 Region to the right of us!)
					if (!((x == 0 || x == 4) && y == -3)
						&& !((x == 1 || x == 3) && y == -4)
						&& !(x == 2 && y == -5)) {
						v.region[nReg] = R;
						v.diceValue[nReg] = b->dice[R];
						v.discipline[nReg] = b->discipline[R];
						nReg--;
					}
				} else {
					nReg--;
				}
			}
		}
		// now we place the regions to the LEFT of each vertex
		if (x >= 1) {
			// we don't add regions to the left on these points, so just skip them
			if (!(x == 1 && (y == 4 || y == -4))
				&& !(x == 2 && (y == 5 || y == -5))) {
				v.region[nRegb] = Rb;
				v.diceValue[nRegb] = b->dice[Rb];
				v.discipline[nRegb] = b->discipline[Rb];
				if (nRegb == 0) {
					Rb++;
					nRegb = 2;
					// if we're at the intersection of two regions, we need the next region on this vertex as well
					// (BUT NOT IF WE'RE AT THE BOTTOM OF A COLUMN - there's only 1 Region to the left of us!)
					if (!((x == 1 || x == 5) && y == -3)
						&& !((x == 2 || x == 4) && y == -4)
						&& !(x == 3 && y == -5)) {
						v.region[nRegb] = Rb;
						v.diceValue[nRegb] = b->dice[Rb];
						v.discipline[nRegb] = b->discipline[Rb];
						nRegb--;
					}
				} else {
					nRegb--;
				}
			}
		}

		// stick our vertex in the array and increment the iterator
		v.xPos = x; v.yPos = y;
		b->nodes[i] = v;
		i++;
		// coords stuff; this should ensure we loop through all our legal coordinates
		y--;
		if (x == 0 && y < -3) {
			x++;
			y = 4;
		} else if (x == 1 && y < -4) {
			x++;
			y = 5;
		} else if (x == 2 && y < -5) {
			x++;
			y = 5;
		} else if (x == 3 && y < -5) {
			x++;
			y = 4;
		} else if (x == 4 && y < -4) {
			x++;
			y = 3;
		}
	}

	// Now we're going to add the vertex neighbors
	i = 0;
	int j = 0;  // need another iterator..
	int yOffset = 0;
	int xOffset = 0;
	// we use alpha/beta to denote the two types of intersection >- and -<
	int ab = 1;       // 1 is alpha, -1 is beta; this flips every time except when it doesnt
	int nIndex = -1;  //index of the neighbor we're looking at
	while (i < NUM_VERTICES) {
		x = b->nodes[i].xPos;
		y = b->nodes[i].yPos;
		j = 0;   // need to reset j! LULZ
		while (j <= 2) {
			// alpha and beta don't matter for the y offset
			if (j == 0) {
				yOffset = 1; xOffset = 0;
			} else if (j == 2) {
				yOffset = -1; xOffset = 0;
			} else {
				// alpha and beta define the x offset
				yOffset = 0; xOffset = ab;
			}
			nIndex = getVertIndex(b, x + xOffset, y + yOffset);
			// make sure its a valid index!
			if (nIndex >= 0 && nIndex < NUM_VERTICES) {
				b->nodes[i].neighbor[j] = &(b->nodes[nIndex]);
				b->nodes[i].arc[j] = VACANT_ARC; // set all  legal arcs to vacant
			} else {
				b->nodes[i].neighbor[j] = NULL;  // otherwise its a null ptr
				b->nodes[i].arc[j] = INVALID;    // invalid arcs are invalid
			}
			j++;
		}
		// flip alpha/beta
		if (ab == 1) {
			ab = -1;
		} else {
			ab = 1;
		}
		// alpha/beta shouldn't always be flipped! we need to correct it
		if ((x == 0 && y == -3)
			|| (x == 1 && y == -4)) {
			ab = 1;
		} else if ((x == 3 && y == -5)
			|| (x == 4 && y == -4)) {
			ab = -1;
		}
		i++;
	}

	// retraining centers
	b->nodes[getVertIndex(b, 1, -3)].retrainCenter = STUDENT_BPS;
	b->nodes[getVertIndex(b, 1, -4)].retrainCenter = STUDENT_BPS;
	b->nodes[getVertIndex(b, 5, 0)].retrainCenter = STUDENT_BQN;
	b->nodes[getVertIndex(b, 5, -1)].retrainCenter = STUDENT_BQN;
	b->nodes[getVertIndex(b, 4, -3)].retrainCenter = STUDENT_MJ;
	b->nodes[getVertIndex(b, 4, -4)].retrainCenter = STUDENT_MJ;
	b->nodes[getVertIndex(b, 1, 4)].retrainCenter = STUDENT_MTV;
	b->nodes[getVertIndex(b, 2, 4)].retrainCenter = STUDENT_MTV;
	b->nodes[getVertIndex(b, 3, 4)].retrainCenter = STUDENT_MMONEY;
	b->nodes[getVertIndex(b, 4, 4)].retrainCenter = STUDENT_MMONEY;
	// with any luck we now have a board!

	return b;
}

static void printVerticesInfo(Board b) {
	//printf ("nodes[0] -> xPos = %d\n", g->nodes[0].xPos);
	int i = 0;

	while (i < NUM_VERTICES) {
		if (b->nodes[i].campusType != 0) {
			//printf ("nodes[%d] -> (%d,%d)\n", i, b->nodes[i].xPos, b->nodes[i].yPos);
			//printf ("   campusType = %d\n", b->nodes[i].campusType);
			//printf ("   myPath = %s\n", b->nodes[i].myPath);
			/*int j = 0;
			while (j <= 2) {
			//printf ("   n%d = %p\n", j, g->nodes[i].neighbor[j]);
			//if (g->nodes[i].neighbor[j] != NULL) { // lolol seg fault
			printf ("   region[%d] = %d\n", j, g->nodes[i].region[j]);
			printf ("     discipline[%d] = %d\n", j, g->nodes[i].discipline[j]);
			printf ("     diceValue[%d] = %d\n", j, g->nodes[i].diceValue[j]);
			//} else {
			//   printf ("   n%d = NULL\n", j);
			//}
			j++;
			}*/
		}
		i++;
	}
}

static void freeBoard(Board b) {
	free(b);
}

static int getVertIndex(Board b, int x, int y) {

	int count = 0;
	int index = INVALID;

	while (count < NUM_VERTICES) {
		if ((b->nodes[count].xPos == x) && (b->nodes[count].yPos == y)) {
			index = count;
			break;
		}
		count++;
	}

	return index;
}

// Takes a path as input
// Returns the coordinate (accordint to grid system)
// Help with the trailing elses
static int translatePath(Board b, path input) {
	vertex buffer;
	int index = 0;
	int pCount = 0; //element of path
	int isOdd;
	int weDone = FALSE;

	// origin is (2,5), keep track of its back
	// hardcode a special case for the origin
	// that knows where B, R, and L are

	// start the path at the origin (2,5)
	buffer.xPos = 2;
	buffer.yPos = 5;

	// hardcode the B, L, and R for the origin
	// back doesn't exist for the first case, returns null
	buffer.neighbor[DIREC_R] = &b->nodes[getVertIndex(b, 2, 4)];
	buffer.neighbor[DIREC_L] = &b->nodes[getVertIndex(b, 3, 5)];
	// this is an alpha orientation (first one)

	// first case, orientation alpha 1
	// set the back once you move
	if (input[pCount] == 'R') {
		// update buffer (x, y--)
		buffer.yPos--;

		// update pointers, orientation beta 1
		buffer = setPointers(b, buffer, BETA_1);

	} else if (input[pCount] == 'L') {
		// update buffer (x++, y)
		buffer.xPos++;

		// update pointers, orientation beta 2
		buffer = setPointers(b, buffer, BETA_2);

		//printf("back pointer is (%d, %d)\n", buffer.neighbor[DIREC_B]->xPos, buffer.neighbor[DIREC_B]->yPos);
		//printf("right pointer is (%d, %d)\n", buffer.neighbor[DIREC_R]->xPos, buffer.neighbor[DIREC_R]->yPos);
		//printf("left pointer is (%d, %d)\n", buffer.neighbor[DIREC_L]->xPos, buffer.neighbor[DIREC_L]->yPos);

	} else if (input[pCount] == '\0') {
		weDone = TRUE;
	} else {
		index = INVALID;
	}

	pCount++;
	while ((input[pCount] != '\0') && (index != INVALID) && weDone != TRUE) {
		// figure out what orientation buffer is in
		// in a 6 section if/else tree, based on its back

		// Depending on next character in path (input[pCount]),
		// update buffer to go to that position.

		// Update the pointers to the neighbors based on new orientation.
		// Note that alphas become betas, betas become alphas
		// Check that neghbors aren't pointing at empty nodes

		// odd flippers are alpha orientation
		// even flippers are beta orientation
		isOdd = (pCount + 1) % 2;   // 0 or 1

		if ((buffer.neighbor[DIREC_B]->xPos == buffer.xPos) &&
			(buffer.neighbor[DIREC_B]->yPos == buffer.yPos + 1) &&
			(isOdd == TRUE)) {
			// orientation alpha 1
			if (input[pCount] == 'B') {
				// update buffer (x, y++)
				buffer.yPos++;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation beta 3
					buffer = setPointers(b, buffer, BETA_3);
				}

			} else if (input[pCount] == 'R') {
				// update buffer (x, y--)
				buffer.yPos--;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation beta 1
					buffer = setPointers(b, buffer, BETA_1);
				}

			} else if (input[pCount] == 'L') {
				// update buffer (x++, y)
				buffer.xPos++;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation beta 2
					buffer = setPointers(b, buffer, BETA_2);
				}

			} else {
				index = INVALID;
			}

		} else if ((buffer.neighbor[DIREC_B]->xPos == buffer.xPos + 1) &&
			(buffer.neighbor[DIREC_B]->yPos == buffer.yPos) &&
			(isOdd == TRUE)) {
			// orientation alpha 2
			if (input[pCount] == 'B') {
				// update buffer (x--, y)
				buffer.xPos--;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation beta 2
					buffer = setPointers(b, buffer, BETA_2);
				}

			} else if (input[pCount] == 'R') {
				// update buffer (x, y++)
				buffer.yPos++;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation beta 3
					buffer = setPointers(b, buffer, BETA_3);
				}

			} else if (input[pCount] == 'L') {
				// update buffer (x, y--)
				buffer.yPos--;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation beta 1
					buffer = setPointers(b, buffer, BETA_1);
				}

			} else {
				index = INVALID;
			}

		} else if ((buffer.neighbor[DIREC_B]->xPos == buffer.xPos) &&
			(buffer.neighbor[DIREC_B]->yPos == buffer.yPos - 1) &&
			(isOdd == TRUE)) {
			// orientation alpha 3
			if (input[pCount] == 'B') {
				// update buffer (x, y--)
				buffer.yPos--;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation beta 1
					buffer = setPointers(b, buffer, BETA_1);
				}

			} else if (input[pCount] == 'R') {
				// update buffer (x++, y)
				buffer.xPos++;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation beta 2
					buffer = setPointers(b, buffer, BETA_2);
				}

			} else if (input[pCount] == 'L') {
				// update buffer (x, y++)
				buffer.yPos++;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation beta 3
					buffer = setPointers(b, buffer, BETA_3);
				}

			} else {
				index = INVALID;
			}

		} else if ((buffer.neighbor[DIREC_B]->xPos == buffer.xPos) &&
			(buffer.neighbor[DIREC_B]->yPos == buffer.yPos + 1) &&
			(isOdd == FALSE)) {
			// orientation beta 1
			if (input[pCount] == 'B') {
				// update buffer (x, y++)
				buffer.yPos++;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation alpha 3
					buffer = setPointers(b, buffer, ALPHA_3);
				}

			} else if (input[pCount] == 'R') {
				// update buffer (x--, y)
				buffer.xPos--;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation alpha 2
					buffer = setPointers(b, buffer, ALPHA_2);
				}

			} else if (input[pCount] == 'L') {
				// update buffer (x, y--)
				buffer.yPos--;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation alpha 1
					buffer = setPointers(b, buffer, ALPHA_1);
				}

			} else {
				index = INVALID;
			}

		} else if ((buffer.neighbor[DIREC_B]->xPos == buffer.xPos - 1) &&
			(buffer.neighbor[DIREC_B]->yPos == buffer.yPos) &&
			(isOdd == FALSE)) {
			// orientation beta 2
			if (input[pCount] == 'B') {
				// update buffer (x--, y)
				buffer.xPos--;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation alpha 2
					buffer = setPointers(b, buffer, ALPHA_2);
				}

			} else if (input[pCount] == 'R') {
				// update buffer (x, y--)
				buffer.yPos--;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation alpha 1
					buffer = setPointers(b, buffer, ALPHA_1);
				}

			} else if (input[pCount] == 'L') {
				// update buffer (x, y++)
				buffer.yPos++;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation alpha 3
					buffer = setPointers(b, buffer, ALPHA_3);
				}

			} else {
				index = INVALID;
			}

		} else if ((buffer.neighbor[DIREC_B]->xPos == buffer.xPos) &&
			(buffer.neighbor[DIREC_B]->yPos == buffer.yPos - 1) &&
			(isOdd == FALSE)) {
			// orientation beta 3
			if (input[pCount] == 'B') {
				// update buffer (x, y--)
				buffer.yPos--;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation alpha 1
					buffer = setPointers(b, buffer, ALPHA_1);
				}

			} else if (input[pCount] == 'R') {
				// update buffer (x, y++)
				buffer.yPos++;
				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation alpha 3
					buffer = setPointers(b, buffer, ALPHA_3);
				}

			} else if (input[pCount] == 'L') {
				// update buffer (x--, y)
				buffer.xPos--;

				if (getVertIndex(b, buffer.xPos, buffer.yPos) == INVALID) {
					index = INVALID;
				} else {
					// update pointers, orientation alpha 2
					buffer = setPointers(b, buffer, ALPHA_2);
				}

			} else {
				index = INVALID;
			}


		} else {
			index = INVALID;
		}

		pCount++;
	}

	if (index != INVALID) {
		index = getVertIndex(b, buffer.xPos, buffer.yPos);
	}

	return index;
}

static vertex* setNeighbor(Board b, int x, int y) {
	vertex* address;

	if (getVertIndex(b, x, y) != INVALID) {
		address = &b->nodes[getVertIndex(b, x, y)];
	} else {
		// point not on the map, null pointer
		address = NULL;
	}

	return address;
}

static vertex setPointers(Board b, vertex buffer, int orientation) {

	int x, y;

	if (orientation == ALPHA_1) {

		x = buffer.xPos;
		y = buffer.yPos + 1;
		buffer.neighbor[DIREC_B] = setNeighbor(b, x, y);

		x = buffer.xPos;
		y = buffer.yPos - 1;
		buffer.neighbor[DIREC_R] = setNeighbor(b, x, y);

		x = buffer.xPos + 1;
		y = buffer.yPos;
		buffer.neighbor[DIREC_L] = setNeighbor(b, x, y);

	} else if (orientation == ALPHA_2) {

		x = buffer.xPos + 1;
		y = buffer.yPos;
		buffer.neighbor[DIREC_B] = setNeighbor(b, x, y);

		x = buffer.xPos;
		y = buffer.yPos + 1;
		buffer.neighbor[DIREC_R] = setNeighbor(b, x, y);

		x = buffer.xPos;
		y = buffer.yPos - 1;
		buffer.neighbor[DIREC_L] = setNeighbor(b, x, y);

	} else if (orientation == ALPHA_3) {

		x = buffer.xPos;
		y = buffer.yPos - 1;
		buffer.neighbor[DIREC_B] = setNeighbor(b, x, y);

		x = buffer.xPos + 1;
		y = buffer.yPos;
		buffer.neighbor[DIREC_R] = setNeighbor(b, x, y);

		x = buffer.xPos;
		y = buffer.yPos + 1;
		buffer.neighbor[DIREC_L] = setNeighbor(b, x, y);

	} else if (orientation == BETA_1) {

		x = buffer.xPos;
		y = buffer.yPos + 1;
		buffer.neighbor[DIREC_B] = setNeighbor(b, x, y);

		x = buffer.xPos - 1;
		y = buffer.yPos;
		buffer.neighbor[DIREC_R] = setNeighbor(b, x, y);

		x = buffer.xPos;
		y = buffer.yPos - 1;
		buffer.neighbor[DIREC_L] = setNeighbor(b, x, y);

	} else if (orientation == BETA_2) {

		x = buffer.xPos - 1;
		y = buffer.yPos;
		buffer.neighbor[DIREC_B] = setNeighbor(b, x, y);

		x = buffer.xPos;
		y = buffer.yPos - 1;
		buffer.neighbor[DIREC_R] = setNeighbor(b, x, y);

		x = buffer.xPos;
		y = buffer.yPos + 1;
		buffer.neighbor[DIREC_L] = setNeighbor(b, x, y);

	} else if (orientation == BETA_3) {

		x = buffer.xPos;
		y = buffer.yPos - 1;
		buffer.neighbor[DIREC_B] = setNeighbor(b, x, y);

		x = buffer.xPos;
		y = buffer.yPos + 1;
		buffer.neighbor[DIREC_R] = setNeighbor(b, x, y);

		x = buffer.xPos - 1;
		y = buffer.yPos;
		buffer.neighbor[DIREC_L] = setNeighbor(b, x, y);

	} else {
		// what's the trailing else?
		// shouldn't be executed, only ever called by preset
		// if/else statements

		// if executed, it's a typo in this program
		assert(FALSE);
	}

	return buffer;
}
