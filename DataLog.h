 
#ifndef DATALOG_H
#define DATALOG_H

#include "brTypes.h"

#define DATALOG_FLAG_LEFTYELLOW			0x00000001        // 
#define DATALOG_FLAG_RIGHTYELLOW		0x00000002        // 
#define DATALOG_FLAG_FASTYELLOW			0x00000004        // 
#define DATALOG_FLAG_SLOWYELLOW			0x00000008        // 
#define DATALOG_FLAG_LEFTRED			0x00000010        // 
#define DATALOG_FLAG_RIGHTRED			0x00000020        // 
#define DATALOG_FLAG_FASTRED			0x00000040        // 
#define DATALOG_FLAG_SLOWRED			0x00000080        // 
#define DATALOG_FLAG_JOYSTICKBUTTON     0x00000100		  // 256


#define DATALOG_FLAG_ALLRED		(DATALOG_FLAG_LEFTRED | DATALOG_FLAG_RIGHTRED | DATALOG_FLAG_FASTRED | DATALOG_FLAG_SLOWRED)
#define DATALOG_FLAG_ALLYELLOW	(DATALOG_FLAG_LEFTYELLOW | DATALOG_FLAG_RIGHTYELLOW | DATALOG_FLAG_FASTYELLOW | DATALOG_FLAG_SLOWYELLOW)


struct DataLogEntry
{
public:
	float mTimeStampSECONDS;
	float mJoystickX;
	float mJoystickY;
	float mCurrentSpeed;
	float mRoadCenterOffset;
	uint32 mFlags;
	
	int32 mLeftRed;
	int32 mLeftYellow;	
	int32 mRightRed;
	int32 mRightYellow;	
	int32 mJoystickButton;		

	int32 mFastRed;
	int32 mFastYellow;	
	int32 mSlowRed;
	int32 mSlowYellow;	

	int32 mVisibleSign;	// The type of visible sign
	
	float mCurrentTurn;
	float mCurrentHill;
};

#define MAX_TEST_MINUTES	30
#define MAX_TEST_SECONDS	(MAX_TEST_MINUTES * 60)
#define MAX_SAMPLES_PER_SECOND 60
#define MAX_TOTAL_DATA_SAMPLES (MAX_SAMPLES_PER_SECOND * MAX_TEST_SECONDS)

void SetDataLogEntry(DataLogEntry const *pDataLog);
void WriteDataLog();
uint32 GetMillisecondTimer();


#endif //DATALOG_H


