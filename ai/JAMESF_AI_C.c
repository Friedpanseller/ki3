/*  EXPERIMENTAL EXPERIMENTAL EXPERIMENTAL EXPERIMENTAL EXPERIMENTAL
*  Mr Pass.  Brain the size of a planet!
*
*  Proundly Created by Richard Buckland
*  Share Freely Creative Commons SA-BY-NC 3.0.
*  used a different method...
*  Using LEO Retraining; Without Printfs; FASTER
*	AIv9.6.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "Game.h"
#include "mechanicalTurk.h"

#define MAX_ROAD_LENGTH 12
#define BINARY_COUNT pow(2, MAX_ROAD_LENGTH - 1)
#define TRUE 1
#define FALSE 0

static void testBoard(Game g, int actionCode, int path);
static int isNotLegal(Game g);
static int needMJ(int building);
static int needMMONEY(int building);
static int needMTV(int building);
static void convertD2B(int number, int bin[]);
static void convertB2S(char * str, int bin[], int swap);
static int cannotBuild(int building);

/* Function to print permutations of string
This function takes three parameters:
1. String
2. Starting index of the string
3. Ending index of the string. */
static action finalAction;

static int PLAYER;
static int BQN;
static int BPS;
static int MTV;
static int MMONEY;
static int MJ;
static int CAMPUS;
static int ARC;
static int GO8;

int cannotBuild(int building) {
	if (building == BUILD_CAMPUS) {
		return (BPS && BQN && MJ && MTV);
	} else if (building == BUILD_GO8) {
		return (MJ >= 2 && MMONEY >= 3);
	} else if (building == OBTAIN_ARC) {
		return (BPS && BQN);
	}
	return FALSE;
}

action decideAction(Game g) {
	int count = 0;
	int curAction = BUILD_GO8;

	PLAYER = getWhoseTurn(g);
	BQN = getStudents(g, PLAYER, STUDENT_BQN);
	BPS = getStudents(g, PLAYER, STUDENT_BPS);
	MTV = getStudents(g, PLAYER, STUDENT_MTV);
	MMONEY = getStudents(g, PLAYER, STUDENT_MMONEY);
	MJ = getStudents(g, PLAYER, STUDENT_MJ);
	CAMPUS = getCampuses(g, PLAYER);
	ARC = getARCs(g, PLAYER);
	GO8 = getGO8s(g, PLAYER);
	char playerName[20] = "NO ONE";
	char actionName[50] = "PASS";

	while ((isLegalAction(g, finalAction) == FALSE && count <= 2) || count == 0) {
		if (count == 0) {
			curAction = BUILD_GO8;
		} else if (count == 1) {
			curAction = BUILD_CAMPUS;
		} else if (count == 2) {
			if (ARC < (((CAMPUS + GO8) - 1) * 3) + (((CAMPUS + GO8) > 5) * 99)) {
				curAction = OBTAIN_ARC;
			} else {
				break;
			}
		}
		//printf("testing %d\n", curAction);
		testBoard(g, curAction, 0);
		if (isLegalAction(g, finalAction) == FALSE) {
			testBoard(g, curAction, 1);
		}
		if (isLegalAction(g, finalAction) == TRUE) {
			if (PLAYER == UNI_A) {
				strcpy(playerName, "UNI_A");
			} else if (PLAYER == UNI_B) {
				strcpy(playerName, "UNI_B");
			} else if (PLAYER == UNI_C) {
				strcpy(playerName, "UNI_C");
			}
			if (curAction == BUILD_GO8) {
				strcpy(actionName, "Build GO8");
			} else if (curAction == BUILD_CAMPUS) {
				strcpy(actionName, "Build Campus");
			} else if (curAction == OBTAIN_ARC) {
				strcpy(actionName, "Build ARC");
			}
			printf("||***AIV94C***|| player %s does action: %s\n", playerName, actionName);
			//getchar();
			return finalAction;
		}
		count++;
	}

	/*RETRAINING HAVE ENOUGH FOR ARC*/
	if (isLegalAction(g, finalAction) == FALSE) {
		if (cannotBuild(OBTAIN_ARC)) {
			//getchar();
			finalAction.actionCode = RETRAIN_STUDENTS;

			/*RETRAINING FROMS*/
			if (BQN > getExchangeRate(g, PLAYER, STUDENT_BQN, STUDENT_MJ)) {
				finalAction.disciplineFrom = STUDENT_BQN;
				//printf("//***AIV94C***// Retraining from STUDENT_BQN %d\n", BQN);
			} else if (BPS > getExchangeRate(g, PLAYER, STUDENT_BPS, STUDENT_MJ)) {
				finalAction.disciplineFrom = STUDENT_BPS;
				//printf("//***AIV94C***// Retraining from STUDENT_BPS %d\n", BPS);
			}

			if (needMMONEY(BUILD_GO8)) {
				finalAction.disciplineTo = STUDENT_MMONEY;
				//printf("//***AIV94C***// Retraining to STUDENT_MMONEY %d\n", MMONEY);
			} else if (needMTV(BUILD_CAMPUS)) {
				finalAction.disciplineTo = STUDENT_MTV;
				//printf("//***AIV94C***// Retraining to STUDENT_MTV %d\n", MTV);
			}

			if (needMJ(BUILD_CAMPUS)) {
				finalAction.disciplineTo = STUDENT_MJ;
				//printf("//***AIV94C***// Retraining to STUDENT_MJ %d\n", MJ);
			}
		}
	}

	/*TOO MANY BQN AND BPS WITHOUT BEING ABLE TO BUILD ARCS*/

	if (isLegalAction(g, finalAction) == FALSE) {
		finalAction.actionCode = RETRAIN_STUDENTS;
		if ((BQN - 1) > getExchangeRate(g, PLAYER, STUDENT_BQN, STUDENT_BPS) && BPS == 0) {
			//getchar();
			finalAction.disciplineFrom = STUDENT_BQN;
			//printf("//***AIV94C***// Retraining from STUDENT_BQN %d\n", BQN);
			finalAction.disciplineTo = STUDENT_BPS;
			//printf("//***AIV94C***// Retraining To STUDENT_BPS %d\n", BPS);
			isNotLegal(g);
		} else if ((BPS - 1) > getExchangeRate(g, PLAYER, STUDENT_BPS, STUDENT_BQN) && BQN == 0) {
			finalAction.disciplineFrom = STUDENT_BPS;
			//printf("//***AIV94C***// Retraining from STUDENT_BPS %d\n", BPS);
			finalAction.disciplineTo = STUDENT_BQN;
			//printf("//***AIV94C***// Retraining To STUDENT_BQN %d\n", BQN);
			isNotLegal(g);
		}
	}

	//GET RID OF MJs//

	if (isLegalAction(g, finalAction) == FALSE && MJ > getExchangeRate(g, PLAYER, STUDENT_MJ, STUDENT_MMONEY)) {
		finalAction.actionCode = RETRAIN_STUDENTS;
		finalAction.disciplineFrom = STUDENT_MJ;
		//printf("//***AIV94C***// Retraining from STUDENT_MJ %d\n", MJ);
		/*if((MJ - 2) > getExchangeRate(g, PLAYER, STUDENT_MJ, STUDENT_MMONEY) && (needMMONEY(BUILD_GO8) || needMMONEY(START_SPINOFF))) {
		finalAction.disciplineFrom = STUDENT_MJ;
		printf("||***AIV94C***|| Retraining from STUDENT_MJ %d\n", MJ);
		finalAction.disciplineTo = STUDENT_MMONEY;
		printf("||***AIV94C***|| Retraining To STUDENT_MMONEY %d\n", MMONEY);
		isNotLegal(g);
		}*/
		if (BPS == 0) {
			finalAction.disciplineTo = STUDENT_BPS;
			//printf("//***AIV94C***// Retraining To STUDENT_BPS %d\n", BPS );
			isNotLegal(g);
		} else if (BQN == 0) {
			finalAction.disciplineTo = STUDENT_BQN;
			//printf("//***AIV94C***// Retraining To STUDENT_BQN %d\n", BQN );
			isNotLegal(g);
		}
		if (needMTV(BUILD_CAMPUS)) {
			finalAction.disciplineTo = STUDENT_MTV;
			//printf("//***AIV94C***// Retraining To STUDENT_MTV %d\n", MTV);
			isNotLegal(g);
		}
	}

	if (isLegalAction(g, finalAction) == FALSE) {
		finalAction.actionCode = START_SPINOFF;
	}

	if (isLegalAction(g, finalAction) == FALSE) {
		finalAction.actionCode = PASS;
		//printf("//***AIV94C***// PASSING\n");
		//getchar();
	}

	return finalAction;
}

void testBoard(Game g, int actionCode, int path) {
	int counter = 0;
	finalAction.actionCode = actionCode;
	int bin[MAX_ROAD_LENGTH];
	for (int x = 0; x < MAX_ROAD_LENGTH; x++) {
		bin[x] = -1;
	}
	while (counter < BINARY_COUNT) {
		//printf("TESTING NUMBER: %d\n", counter);
		convertD2B(counter, bin);
		convertB2S(finalAction.destination, bin, 0);
		//printf("TESTING PATH $%s$ player = %d\n", finalAction.destination, getWhoseTurn(g));
		//	getchar();
		if (isLegalAction(g, finalAction) == TRUE) {
			//printf("TESTING PATH $%s$\nACTION CODE: %d\nBPS %d\nBQN %d\n MJ %d\n MMONEY %d\n MTV %d\nTurn: %d\n\n", finalAction.destination, actionCode, BPS, BQN, MJ, MMONEY, MTV, getTurnNumber(g));
			return;
		}
		convertB2S(finalAction.destination, bin, 1);
		if (isLegalAction(g, finalAction) == TRUE) {
			//printf("TESTING PATH $%s$\nACTION CODE: %d\nBPS %d\nBQN %d\n MJ %d\n MMONEY %d\n MTV %d\nTurn: %d\n\n", finalAction.destination, actionCode, BPS, BQN, MJ, MMONEY, MTV, getTurnNumber(g));
			return;
		}
		counter++;
	}
}

int isNotLegal(Game g) {
	//If you have something you're sure is legal, this tells you if it is not legal//
	int notLegal = (isLegalAction(g, finalAction) == FALSE);
	if (notLegal) {
		printf("//***AIV94C***// //***ERROR***// %d This should be legal but it is not! //***ERROR***//\n", getWhoseTurn(g));
	} else {
		//getchar();
	}
	return (isLegalAction(g, finalAction) == FALSE);
}

int needMJ(int building) {
	//Sees if you are close to building something, just need 1 more MJ
	int returnValue = FALSE;
	if (building == BUILD_GO8) {
		if (MJ < 2 && MMONEY >= 3) {
			returnValue = TRUE;
		}
	} else if (building == BUILD_CAMPUS) {
		if (MJ < 1 && BQN && BPS && MTV) {
			//printf("NEED MJ CAMPUS\n");
			returnValue = TRUE;
		}
	} else if (building == START_SPINOFF) {
		if (MJ < 1 && MTV && MMONEY) {
			returnValue = TRUE;
		}
	}
	return returnValue;
}

int needMTV(int building) {
	//Sees if you are close to building something, just need 1 more MTV
	int returnValue = FALSE;
	if (building == BUILD_CAMPUS) {
		if (MTV < 1 && BQN && BPS && MJ) {

			returnValue = TRUE;
		}
	}
	return returnValue;
}

int needMMONEY(int building) {
	//Sees if you are close to building something, just need 1 more MMONEY
	int returnValue = FALSE;
	if (building == BUILD_GO8) {
		if (MJ >= 2 && MMONEY < 3) {
			returnValue = TRUE;
		}
	} else if (building == START_SPINOFF) {
		if (MJ >= 1 && MTV >= 1 && MMONEY < 1) {
			returnValue = TRUE;
		}
	}
	return returnValue;
}

void convertD2B(int number, int bin[]) {
	int remainder;
	int count = 0;
	while (number) {
		remainder = number % 2;
		number /= 2;
		bin[count] = remainder;
		count++;
	}
}

void convertB2S(char * str, int bin[], int swap) {
	int x = 0;
	char zero;
	char one;
	if (swap) {
		zero = 'R';
		one = 'L';
	} else {
		zero = 'L';
		one = 'R';
	}
	while (x < MAX_ROAD_LENGTH) {
		if (bin[x] == 0) {
			str[x] = zero;
		} else if (bin[x] == 1) {
			str[x] = one;
		} else {
			str[x] = 0;
		}
		x++;
	}
}
