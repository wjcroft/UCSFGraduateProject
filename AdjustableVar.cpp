#include <stdio.h>
#include <string.h>
#include "AdjustableVar.h"

#ifndef WIN32
#include <unistd.h>
#endif
#include <stdlib.h> // for MAX_PATH

int gMAXPLAYS = 0;
int gtransitioncodecalled = 0;
int gfeedbackscreencodecalled = 0;
int gCurrentGamePlayCount = 0;
float gStoryBoardMode = 0.0;
float gPrevStoryBoardMode = 20.0;
float gStoryBoardMasterLevelChange = 0.0;
float gStoryBoardRoadLevelChange = 0.0;
float gStoryBoardSignLevelChange = 0.0;

float gStoryBoardPlayLengthSeconds = 0.0;
char gStoryBoardTransitionScreenName[512] = {0};
char gfeedbackScreenName[512] = {0};

char goldfeedbackScreenName[512] = {0};
char goldStoryBoardTransitionScreenName[512] = {0};

int gCurrentSpeedLevel		= 1;
int gLastSpeedAdjustment	= 0;
int gCurrentSignLevel		= 1;
int gLastSignAdjustment		= 0;
float gSpeed_MIN			= 0.0;
float gSpeed_MAX			= 0.0;
float gSign_MIN				= 0.0;
float gSign_MAX				= 0.0;

int gOverallAccuracy = 0; 
int gCurrentDifficultyLevel = 1;
int gLastDifficultyAdjustment = 0;
int gLastGrayPercent = 0;
int gLastRedPercent = 0;
int gLastYellowPercent = 0;

// Just for testing purposes. 
int gTotalGraySamples = 0;
int gTotalYellowSamples = 0;
int gTotalRedSamples = 0;
int gTotalSamples = 0;

int gRedSpeedSamples = 0 ;
int gYellowSpeedSamples = 0 ;
int gGraySpeedSamples = 0;
int gLastGraySpeedPercent = 0;
int gLastRedSpeedPercent = 0;
int gLastYellowSpeedPercent = 0;

int gRedAccuracySamples=0;
int gYellowAccuracySamples=0;
int gGrayAccuracySamples=0;
int gLastGrayAccuracyPercent = 0;
int gLastRedAccuracyPercent = 0;
int gLastYellowAccuracyPercent = 0;

// Signs 
float gSignStartTime = 0.0;
float gSignLatency = 0.0;
float gTotalSignLatency = 0.0;
float gAverageSignLatency = 0.0;
int gAllSignsTotal = 0;
int gSignFalsePositives = 0;
int lastSignGo = 1;
bool alreadyHitNontargetSign;
bool alreadyMisshitNontargetSign;
bool alreadyHitTargetSign;
bool alreadyMisshitTargetSign;
bool alreadyIgnoredNontargetSign;
bool alreadyIgnoredTargetSign;
bool gSignsStarted = false; // did signs start presenting this run yet?
int gGoSignsTriggered = 0;
int gGoSignsTotal = 0;
int gLastSignVisible = 0;
int gLastGoSignPercent = 0;
int gLastFPSignPercent = 0;
int gSignsCorrect = 0;
int gSignsIncorrect = 0;

float gReactionTime = 0;


SignPressDecision gCurrentSignPressDecision = PRESS_NOT_READY;


struct AdjusterNameEntry
{
	bool mbInitialized;
	char const *mpName;
	float *mpMinValue;
	float *mpMaxValue;
	char mNameReportBuffer[256];
};

#define ADJUSTABLE_FLOAT_RANGE(name) \
	float name##_MIN = 0.0; \
	float name##_MAX = 0.0;
#include "AdjustableVariableList.h"
#undef ADJUSTABLE_FLOAT_RANGE


#define ADJUSTABLE_FLOAT_RANGE(name) { false, #name, &name##_MIN, &name##_MAX },
AdjusterNameEntry gAllAdjusters[] = 
{
	#include "AdjustableVariableList.h"
};
#undef ADJUSTABLE_FLOAT_RANGE

//

struct SpeedAdjusterNameEntry
{
	bool mbInitialized;
	char const *mpName;
	float *mpMinValue;
	float *mpMaxValue;
	char mNameReportBuffer[256];
};

#define ADJUSTABLE_SPEED_FLOAT_RANGE(name) \
	float name##_MIN = 0.0; \
	float name##_MAX = 0.0;
#include "speedlevels.h"
#undef ADJUSTABLE_SPEED_FLOAT_RANGE


#define ADJUSTABLE_SPEED_FLOAT_RANGE(name) { false, #name, &name##_MIN, &name##_MAX },
SpeedAdjusterNameEntry gSpeedAdjusters[] = 
{
	#include "speedlevels.h"
};
#undef ADJUSTABLE_SPEED_FLOAT_RANGE
//
// Signs

struct SignAdjusterNameEntry
{
	bool mbInitialized;
	char const *mpName;
	float *mpMinValue;
	float *mpMaxValue;
	char mNameReportBuffer[256];
};

#define ADJUSTABLE_SIGN_FLOAT_RANGE(name) \
	float name##_MIN = 0.0; \
	float name##_MAX = 0.0;
#include "signlevels.h"
#undef ADJUSTABLE_SIGN_FLOAT_RANGE


#define ADJUSTABLE_SIGN_FLOAT_RANGE(name) { false, #name, &name##_MIN, &name##_MAX },
SignAdjusterNameEntry gSignAdjusters[] = 
{
	#include "signlevels.h"
};
#undef ADJUSTABLE_SIGN_FLOAT_RANGE

// 
// 
struct StoryBoardAdjusterNameEntry
{
	bool mbInitialized;
	char const *mpName;
	float *mpMode;
	float *mpLevel;
	float *mpPlayLengthSeconds;	
	char mNameReportBuffer[256];
};

 
#define ADJUSTABLE_STORYBOARD_FLOAT_RANGE(name) \
	float name##_MODE = 0.0; \
	float name##_LEVEL = 0.0; \
	float name##_TIME = 0.0;
#include "storyboard.h"
#undef ADJUSTABLE_STORYBOARD_FLOAT_RANGE

#define ADJUSTABLE_STORYBOARD_FLOAT_RANGE(name) { false, #name, &name##_MODE, &name##_LEVEL, &name##_TIME },
StoryBoardAdjusterNameEntry gStoryBoardAdjusters[] = 
{
	#include "storyboard.h"
};
#undef ADJUSTABLE_STORYBOARD_FLOAT_RANGE


//
int gNumAdjusters				= sizeof(gAllAdjusters) / sizeof(gAllAdjusters[0]);
int gSpeedNumAdjusters			= sizeof(gSpeedAdjusters) / sizeof(gSpeedAdjusters[0]);
int gSignNumAdjusters			= sizeof(gSignAdjusters) / sizeof(gSignAdjusters[0]);
int gStoryBoardNumAdjusters		= sizeof(gStoryBoardAdjusters) / sizeof(gStoryBoardAdjusters[0]);


int GetAdjustableVarCount()
{
	return gNumAdjusters;
}

int GetSpeedAdjustersCount()
{
	return gSpeedNumAdjusters;
}

int GetSignAdjustersCount()
{
	return gSignNumAdjusters;
}

int GetStoryBoardAdjustersCount()
{
	return gStoryBoardNumAdjusters;
}


// This section is just a spiffy trick to enumerate these variables.
char const *GetAdjustableVarString(int index)
{
	if (index < 0 || index >= gNumAdjusters)
	{
		return NULL;
	}
	else
	{   
		AdjusterNameEntry *pItem = &(gAllAdjusters[index]);
		sprintf(pItem->mNameReportBuffer, "%s %.3f - %.3f", pItem->mpName, *(pItem->mpMinValue), *(pItem->mpMaxValue));
		return pItem->mNameReportBuffer;
	}
}

//This section is just a spiffy trick to enumerate these variables.
char const *GetSpeedVarString(int index)
{
	if (index < 0 || index >= gSpeedNumAdjusters)
	{
		return NULL;
	}
	else
	{   
		SpeedAdjusterNameEntry *pItem = &(gSpeedAdjusters[index]);
		sprintf(pItem->mNameReportBuffer, "%s %.3f - %.3f", pItem->mpName, *(pItem->mpMinValue), *(pItem->mpMaxValue));
		return pItem->mNameReportBuffer;
	}
}

// This section is just a spiffy trick to enumerate these variables.
char const *GetSignVarString(int index)
{
	if (index < 0 || index >= gSignNumAdjusters)
	{
		return NULL;
	}
	else
	{   
		SignAdjusterNameEntry *pItem = &(gSignAdjusters[index]);
		sprintf(pItem->mNameReportBuffer, "%s %.3f - %.3f", pItem->mpName, *(pItem->mpMinValue), *(pItem->mpMaxValue));
		return pItem->mNameReportBuffer;
	}
}

// For the StoryBoard. 
char const *GetStoryBoardVarString(int index)
{
	if (index < 0 || index >= gStoryBoardNumAdjusters)
	{
		return NULL;
	}
	else
	{   
		StoryBoardAdjusterNameEntry *pItem = &(gStoryBoardAdjusters[index]);
		sprintf(pItem->mNameReportBuffer, "%s %.3f - %.3f - %.3f", pItem->mpName, *(pItem->mpMode), *(pItem->mpLevel), *(pItem->mpPlayLengthSeconds));
		return pItem->mNameReportBuffer;
	}
}

//
bool CheckAdjustableVars()
{
	int numErrors = 0;
	for (int index = 0; index < GetAdjustableVarCount(); ++index)
	{
		if (! gAllAdjusters[index].mbInitialized)
		{
			// There's a problem
			printf("Error: The variable %s was not in AdjustableVars.txt\n", gAllAdjusters[index].mpName);
			numErrors++;
		}
	}
	if (numErrors > 0)
	{
//		MessageBox(
		while (1) {}	// hold here forever
		return false;
	}
	return true;
}

bool CheckRoadVars()
{
	int numErrors = 0;
	for (int index = 0; index < GetSpeedAdjustersCount(); ++index)
	{
		if (! gSpeedAdjusters[index].mbInitialized)
		{
			// There's a problem
			printf("Error: The variable %s was not in speedlevels.txt\n", gSpeedAdjusters[index].mpName);
			numErrors++;
		}
	}
	if (numErrors > 0)
	{
//		MessageBox(
		while (1) {}	// hold here forever
		return false;
	}
	return true;
}


bool CheckSignVars()
{
	int numErrors = 0;
	for (int index = 0; index < GetSignAdjustersCount(); ++index)
	{
		if (! gSignAdjusters[index].mbInitialized)
		{
			// There's a problem
			printf("Error: The variable %s was not in signlevels.txt\n", gSignAdjusters[index].mpName);
			numErrors++;
		}
	}
	if (numErrors > 0)
	{
		while (1) {}	// hold here forever
		return false;
	}
	return true;
}

// StoryBoard Check Variables. 
bool CheckStoryBoardVars()
{
	int numErrors = 0;
	for (int index = 0; index < GetStoryBoardAdjustersCount(); ++index)
	{
		if (! gStoryBoardAdjusters[index].mbInitialized)
		{
			// There's a problem
			printf("Error: The variable %s was not in storyboard.txt\n", gStoryBoardAdjusters[index].mpName);
			numErrors++;
		}
	}
	if (numErrors > 0)
	{
		while (1) {}	// hold here forever
		return false;
	}
	return true;
}

int gAdjustmentHistory[HISTORY_MAX];
int gSpeedAdjustmentHistory[HISTORY_MAX];
int gSignAdjustmentHistory[HISTORY_MAX];
int gStoryBoardAdjustmentHistory[HISTORY_MAX];
int gAdjustmentHistoryNum = 0;

// If player does well, call this with 1
// Otherwise, call it with -1
void AdjustAllDifficultyLevels(int difficultyAdjustment, int speedAdjustment, int signAdjustment, int storyLevel)
{
	
	gAdjustmentHistory[gAdjustmentHistoryNum] = difficultyAdjustment;
	gSpeedAdjustmentHistory[gAdjustmentHistoryNum] = speedAdjustment;

	gSignAdjustmentHistory[gAdjustmentHistoryNum]  = signAdjustment;
	gStoryBoardAdjustmentHistory[gAdjustmentHistoryNum] = storyLevel;

	// These need to be adjusted first as the overall difficulty adjustment is based on these two values. 
	SetSpeedLevel(gCurrentSpeedLevel + speedAdjustment);
	SetSignLevel(gCurrentSignLevel + signAdjustment);
	
	// Difficulty adjustment is based on the speed and sign Adjusment. 
	SetDifficultyLevel(gCurrentDifficultyLevel + difficultyAdjustment);
	
	// storyboard override. 
	SetStoryBoardLevel(storyLevel);
	
	gAdjustmentHistoryNum++;
}



bool SetDifficultyLevel(int difficultyLevel)
{
	gCurrentDifficultyLevel = difficultyLevel;
	return true; 

}


bool SetSpeedLevel(int speedLevel)
{
	int tryLevel = speedLevel;
	if (LoadSpeedLevels(tryLevel))
	{
		printf("Loaded speed level %d okay\n", tryLevel);
		gCurrentSpeedLevel = tryLevel;
		return true;
	}
	printf("Failed to load vars for level %d\n", tryLevel);
	return false;
}


bool SetSignLevel(int signLevel)
{
	int tryLevel = signLevel;
	if (LoadSignLevels(tryLevel))
	{
		printf("Loaded sign level %d okay\n", tryLevel);
		gCurrentSignLevel = tryLevel;
		return true;
	}
	printf("Failed to load vars for level %d\n", tryLevel);
	return false;
}


bool SetStoryBoardLevel(int StoryBoardLevel)
{
	int tryLevel = StoryBoardLevel;

	//while (tryLevel != 0)
	//{
		if (LoadStoryBoard(tryLevel))
		{
			printf("Loaded vars for storylevel %d okay\n", tryLevel);
			StoryBoardLevel = tryLevel;
			return true;
		}
		printf("Failed to load vars for level %d\n", tryLevel);
		if (tryLevel > 0)
			tryLevel--;
		else
			tryLevel++;
	//}
	return false;

}


bool LoadAdjustableVars(int difficultyLevel)
{

#ifndef WIN32
	char buffer[1000];
	char *currentDirectory = getcwd( buffer, 1000);
	// printf("Directorily: %s\n",currentDirectory);
	char *loadpath = strcat(currentDirectory,"/runtime");
	chdir(currentDirectory);
	//chdir("/Users/SPL/Desktop/DWDT/dwd/BrainMobile/runtime");	
	
	chdir("/Applications/DWD/dwd/NeuroRacer/runtime");	
#endif

	bool returnVal = false;
	char name[512];
	sprintf(name, "AdjustableVars.txt", difficultyLevel);
	FILE *fp = fopen(name, "r");
	if (fp != NULL)
	{
		returnVal = true;	// We found the file!
		while (!feof(fp))
		{
			char line[512];
			char varName[512];
			float min, max;
			line[0] = 0;
			varName[0] = 0;
			fgets(line, sizeof(line), fp);
			if (3 == sscanf(line, " %s %f %f ", varName, &min, &max))
			{
				for (int i = 0; i < GetAdjustableVarCount(); ++i)
				{
					AdjusterNameEntry *pItem = &(gAllAdjusters[i]);
					if (!strcmp(pItem->mpName, varName))
					{
						*pItem->mpMinValue = min;
						*pItem->mpMaxValue = max;
						pItem->mbInitialized = true;
					}
				}
			}
		}
		fclose(fp);
	}
	CheckAdjustableVars();
	return returnVal;
}


// Changes the speed and sign level based on the history, and resets the appropriate variables. 
// The overall level change is then calculated differently. 
bool LoadSpeedLevels(int SpeedLevelAdjustment)
{
	
	bool returnVal = false;
	
	// start load the speed file// 	
	char name[512];
	sprintf(name, "speedlevels.txt");
	FILE *fp = fopen(name, "r");
	int i = 0;
	
	if (fp != NULL)
	{
		returnVal = true;	// We found the file!
		while (!feof(fp))
		{
			char line[512];
			char varName[512];
			float min, max;
			line[0] = 0;
			varName[0] = 0;
			fgets(line, sizeof(line), fp);
			if (3 == sscanf(line, " %s %f %f ", varName, &min, &max))
			{
				if (i == (SpeedLevelAdjustment+1)) {
					// printf("i:%d %f %f \n",i,min,max);
					gSpeed_MIN = min;
					gSpeed_MAX = max;					
				}  // end if statement. 
				i++; 
			}
		}
		fclose(fp);
	}
	// end load file
	return returnVal;
}




bool LoadSignLevels(int SignLevelAdjustment)
{	
	
	bool returnVal = false;
	// start load the speed file// 	
	char name[512];
	sprintf(name, "signlevels.txt");
	FILE *fp = fopen(name, "r");
	int i = 0;
	
	if (fp != NULL)
	{
		returnVal = true;	// We found the file!
		while (!feof(fp))
		{
			char line[512];
			char varName[512];
			float reaction;
			line[0] = 0;                            
			varName[0] = 0;		
			fgets(line, sizeof(line), fp);
			
			if (2 == sscanf(line, " %s %f", varName, &reaction))
			{			
				if (i == (SignLevelAdjustment+1)) {
					// printf("i:%d %f \n",i,reaction);
					gReactionTime = reaction;
				}  // end if statement. 
				i++; // increment i index for every line. i is the line in the file where sscanf result = 5, which is different from the level number.  
			} // end other if stateent. 
			
		} //end while not end of file statement. 
		fclose(fp);
	}
	
	// end of sign modification. 
	gSign_MAX = 10.0;    
	
	return returnVal;
	
}


// Storyboard addition. 
bool LoadStoryBoard(int StoryBoardCount)
{
	bool returnVal = false;
	
	// start load the speed file// 	
	char name[512];
	sprintf(name, "storyboard.txt");
	FILE *fp = fopen(name, "r");
	int i = 0; 
	
	strcpy(goldfeedbackScreenName,gfeedbackScreenName);
	strcpy(goldStoryBoardTransitionScreenName,gStoryBoardTransitionScreenName);
//	printf("Loading Transition Screen: %s\n", goldStoryBoardTransitionScreenName);

	if (fp != NULL)
	{
		returnVal = true;	// We found the file!
		while (!feof(fp))
		{
			char line[512];
			char varName[512];
			float mode,level,roadlevel,signlevel,playlengthseconds;
			line[0] = 0;                            
			varName[0] = 0;		
			char transitionScreenName[512];
			char feedbackScreenName[512];
			fgets(line, sizeof(line), fp);
			
			if (8 == sscanf(line, " %s %f %f %f %f %f %s %s", varName, &mode, &level, &roadlevel, &signlevel, &playlengthseconds, feedbackScreenName, transitionScreenName))
			{			
					if (i == (StoryBoardCount)) {
//						printf("storyboard count: %d\n", i);
//						printf("feedback & transition: '%s'   '%s'\n",feedbackScreenName, transitionScreenName);
						gPrevStoryBoardMode = gStoryBoardMode;
						gStoryBoardMode = mode;
						gStoryBoardMasterLevelChange = level; 
						gStoryBoardRoadLevelChange = roadlevel;
						gStoryBoardSignLevelChange = signlevel;
						gStoryBoardPlayLengthSeconds = playlengthseconds;
						strcpy(gfeedbackScreenName,feedbackScreenName);
						strcpy(gStoryBoardTransitionScreenName,transitionScreenName);
					}  // end if statement. 
						
					i++; // increment i index for every line. i is the line in the file where sscanf result = 5, which is different from the level number.  
				    // the maximal i is the HISTORY_MAX variable. 
				
				    gMAXPLAYS = i;

			} // end other if state
			
		} //end while not end of file statement. 
		fclose(fp);
	}
	// end load file

//	printf("mode: %d play length: %d\n",gStoryBoardMode,gStoryBoardPlayLengthSeconds);
	printf("feedbackScreenName: %s StoryBoardTransitionScreenName: %s\n",gfeedbackScreenName,gStoryBoardTransitionScreenName);

	
	return returnVal;
}
