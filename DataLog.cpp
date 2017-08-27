
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "brTypes.h"
#include "brRoad.h"
#include "AdjustableVar.h"

#include "DataLog.h"

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

uint32 GetMillisecondTimer()
{
#ifdef _WIN32
	LARGE_INTEGER	count, freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	uint32 ms = (uint32)((count.QuadPart * 1000) / freq.QuadPart);
//	printf("ms = %u, freq = %u\n", ms, ((uint32)freq.QuadPart));
	return(ms);
#endif
}


static DataLogEntry gDataLog[MAX_TOTAL_DATA_SAMPLES];
static int32 gCurrentDataLogItem = 0;
static uint32 gStartTimeMS = 0;

void SetDataLogEntry(DataLogEntry const *pDataLog)
{
#ifdef _WIN32
	// This is a more accurate timer
	uint32 milliseconds = GetMillisecondTimer();
#else
	// ...but the Mac version isn't done yet.
	uint32 milliseconds = (uint32)(pDataLog->mTimeStampSECONDS * 1000.0f);
#endif

	if (gCurrentDataLogItem == 0)
	{
		gStartTimeMS = milliseconds;
	}
	gDataLog[gCurrentDataLogItem] = *pDataLog;

	if (gCurrentDataLogItem < MAX_TOTAL_DATA_SAMPLES)
	{
		// Override with the more accurate timer
		gDataLog[gCurrentDataLogItem].mTimeStampSECONDS = (float)(milliseconds - gStartTimeMS) / 1000.0f;
		gDataLog[gCurrentDataLogItem].mTimeStampSECONDS -= LeadTime321Go_MAX;

// To record negative times, just comment the next line out.
//		if (gDataLog[gCurrentDataLogItem].mTimeStampSECONDS >= 0.0f)
		{
			gCurrentDataLogItem++;
		}
	}
}

void WriteDataLog()
{
	float lookAheadTime = (NUM_ROAD_SECTIONS / ((gSpeed_MIN + gSpeed_MAX) * 0.5f)) * 0.75f;

//	int lookAheadLog = lookAheadTime * 60;
	int ii = gCurrentDataLogItem;
	while (ii >= 0)
	{
		int lookAheadLog = 0;
		if (gDataLog[ii].mTimeStampSECONDS + LeadTime321Go_MAX > lookAheadTime)
		{
			while (gDataLog[ii].mTimeStampSECONDS <= gDataLog[ii - lookAheadLog].mTimeStampSECONDS + lookAheadTime)
			{
				lookAheadLog++;
			}
		}
		else
		{
			lookAheadLog = ii; //first ~2 seconds of driving is pre-drawn before game starts; this just replicates the first section in the log
		}
		gDataLog[ii].mCurrentHill = gDataLog[ii - lookAheadLog].mCurrentHill;
		gDataLog[ii].mCurrentTurn = gDataLog[ii - lookAheadLog].mCurrentTurn;
	
		ii--;
	}

	time_t clocktime;

	clocktime = time (NULL);
	//printf ("%ld seconds since 0:00, 01/01/1970", clocktime);

	static int serialNumber = 1;
	char fileName[512];
	sprintf(fileName, "DataLog_Out_%d_mode%1.0f.txt", clocktime, gPrevStoryBoardMode);  // OLD WAY: This wants to get a serial number, or a time/date stamp
	//sprintf(fileName, "DataLog_Out_%d_mode%1.0f.txt", serialNumber++,gPrevStoryBoardMode);  // JOAQ MODIFIED to NOT have those crazy numbers for ease of analyzing data sets later
	FILE *fp = fopen(fileName, "w");
	if (fp != NULL)
	{	
		// What else do we want to print out. 
		fprintf(fp, "Overall Difficulty Level: %d, Road Difficulty Level: %d Sign Difficulty Level: %d Go Sign False Positive Button Triggers: %d Average Go Sign Latency: %4.6f Driving Accuracy: %d Vertical Accuracy: %d Horizontal Accuracy: %d Sign Accuracy: %d OverallAccuracy:%d Go Signs Total: %d GamePlayMode: %4.1f\n", 	gLastDifficultyAdjustment, gLastSpeedAdjustment, gLastSignAdjustment, gLastFPSignPercent, gAverageSignLatency, gLastGrayPercent, gLastGraySpeedPercent, gLastGrayAccuracyPercent,gLastGoSignPercent,gOverallAccuracy,gGoSignsTotal,gPrevStoryBoardMode);

		//fprintf(fp, "time,posX,posY,joyX,joyY,signType,flags,leftred,leftyellow,rightred,rightyellow,joystickbutton\n"); 
		fprintf(fp, "time,posX,posY,joyX,joyY,signType,flags,leftred,leftyellow,rightred,rightyellow,joystickbutton,TurnType,HillType\n"); 
		
		for (int i = 0; i < gCurrentDataLogItem; ++i)
		{
			DataLogEntry *pDataLog = &(gDataLog[i]);
			//fprintf(fp, "%f,%f,%f,%f,%f,%d,%d,%d,%d,%d,%d,%d\n",
			fprintf(fp, "%f,%f,%f,%f,%f,%d,%d,%d,%d,%d,%d,%d,%0.0f,%0.0f\n",
			pDataLog->mTimeStampSECONDS, pDataLog->mRoadCenterOffset, pDataLog->mCurrentSpeed, pDataLog->mJoystickX, pDataLog->mJoystickY,
			pDataLog->mVisibleSign, pDataLog->mFlags,pDataLog->mLeftRed,pDataLog->mLeftYellow,pDataLog->mRightRed,pDataLog->mRightYellow,pDataLog->mJoystickButton,pDataLog->mCurrentTurn,pDataLog->mCurrentHill); 

			 
		}
		fclose(fp);
	}
	gCurrentDataLogItem = 0;	// reset us for the next run.
}












