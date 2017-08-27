
#define ADJUSTABLE_FLOAT_RANGE(name) extern float name##_MIN; extern float name##_MAX;
#include "AdjustableVariableList.h"
#undef ADJUSTABLE_FLOAT_RANGE

#define ADJUSTABLE_SPEED_FLOAT_RANGE(name) extern float name##_MIN; extern float name##_MAX;
#include "speedlevels.h"
#undef ADJUSTABLE_SPEED_FLOAT_RANGE

#define ADJUSTABLE_SIGN_FLOAT_RANGE(name) extern float name##_MIN; extern float name##_MAX;
#include "signlevels.h" 
#undef ADJUSTABLE_SIGN_FLOAT_RANGE

#define ADJUSTABLE_STORYBOARD_FLOAT_RANGE(name) extern float name##_MODE; extern float name##_LEVEL; extern float name##_TIME;
#include "storyboard.h" 
#undef ADJUSTABLE_STORYBOARD_FLOAT_RANGE

extern int gMAXPLAYS;

extern int gCurrentGamePlayCount;
extern char gStoryBoardTransitionScreenName[512];
extern char gfeedbackScreenName[512];

extern char goldfeedbackScreenName[512];
extern char goldStoryBoardTransitionScreenName[512];

extern float gStoryBoardMode;
extern float gPrevStoryBoardMode;
extern float gStoryBoardMasterLevelChange;
extern float gStoryBoardRoadLevelChange;
extern float gStoryBoardSignLevelChange;
extern float gStoryBoardPlayLengthSeconds;

extern float gReactionTime;

extern float gSpeed_MIN;
extern float gSpeed_MAX;
extern float gSign_MIN;
extern float gSign_MAX;

extern int gOverallAccuracy;

extern int gCurrentSpeedLevel;
extern int gCurrentSignLevel;
extern int gLastSpeedAdjustment;
extern int gLastSignAdjustment;

extern int gCurrentDifficultyLevel;
extern int gLastDifficultyAdjustment;
extern int gtransitioncodecalled;
extern int gfeedbackscreencodecalled;

extern int gLastGrayPercent;
extern int gLastRedPercent;
extern int gLastYellowPercent;

extern int gTotalGraySamples;
extern int gTotalRedSamples;
extern int gTotalYellowSamples;
extern int gTotalSamples;

extern int gRedSpeedSamples;
extern int gYellowSpeedSamples;
extern int gGraySpeedSamples;
extern int gLastYellowSpeedPercent;
extern int gLastRedSpeedPercent;
extern int gLastGraySpeedPercent;

extern int gRedAccuracySamples;
extern int gYellowAccuracySamples;
extern int gGrayAccuracySamples;
extern int gLastYellowAccuracyPercent;
extern int gLastRedAccuracyPercent;
extern int gLastGrayAccuracyPercent;

extern int   gAllSignsTotal;
extern float gSignStartTime;
extern float gSignLatency;
extern float gTotalSignLatency;
extern float gAverageSignLatency;

extern int lastSignGo;
extern bool alreadyHitNontargetSign;
extern bool alreadyMisshitNontargetSign;
extern bool alreadyHitTargetSign;
extern bool alreadyMisshitTargetSign;
extern bool alreadyIgnoredNontargetSign;
extern bool alreadyIgnoredTargetSign;
extern bool gSignsStarted;
extern int gSignFalsePositives;
extern int gGoSignsTriggered;
extern int gGoSignsTotal;
extern int gLastSignVisible;
extern int gLastGoSignPercent;
extern int gLastFPSignPercent;
extern int gSignsCorrect;
extern int gSignsIncorrect;

enum SignPressDecision {
	PRESS_NOT_READY,
	PRESS_READY,
	DID_PRESS,
	DID_NOT_PRESS
};

extern SignPressDecision gCurrentSignPressDecision;

char const *GetAdjustableVarString(int index);
char const *GetSpeedVarString(int index);
char const *GetSignVarString(int index);
char const *GetStoryBoardVarString(int index);

int GetAdjustableVarCount();
int GetSpeedAdjustersCount();
int GetSignAdjustersCount();
int GetStoryBoardAdjustersCount();

bool LoadAdjustableVars(int difficultyLevel);
bool LoadSpeedLevels(int SpeedLevelAdjustment);
bool LoadSignLevels(int SignLevelAdjustment);
// StoryBoard Addition. 
bool LoadStoryBoard(int StoryBoardCount);

bool SetDifficultyLevel(int difficultyLevel);
bool SetSpeedLevel(int speedLevel);
bool SetSignLevel(int signsLevel);
bool SetStoryBoardLevel(int StoryBoardLevel);

void AdjustAllDifficultyLevels(int difficultyAdjustment, int speedAdjustment, int signAdjustment, int storyLevel);


#define HISTORY_MAX	50
extern int gAdjustmentHistory[HISTORY_MAX];
extern int gAdjustmentHistoryNum;

// New functions for new level changing. 
extern int gSpeedAdjustmentHistory[HISTORY_MAX];
extern int gSignAdjustmentHistory[HISTORY_MAX];
extern int gStoryBoardAdjustmentHistory[HISTORY_MAX];


