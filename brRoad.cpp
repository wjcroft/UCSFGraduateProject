
#ifdef _WIN32  // #if WINDOWS
#include <stdio.h>
#include <windows.h>
#include "pt_ioctl.c"
#endif

#include <string.h>
//#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#ifdef DWD_QT
#include <QGLWidget>
#elif defined _WIN32
#include <glut.h>
#include <gl.h>
#include <glu.h>
#else
#include <GLUT/glut.h>
#include <OpenGL/glext.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <Accelerate/Accelerate.h>
#endif

#ifndef DWD_QT
#include "texture.h"
#endif
#include "brTypes.h"
#include "cbMath.h"
#include "brRoad.h"
#include "AdjustableVar.h"
#include "DataLog.h"

//uint32 gTotalSamples = 0;
//uint32 gTotalGraySamples = 0;
//uint32 gTotalRedSamples = 0;
//uint32 gTotalYellowSamples = 0;

extern bool blackout;
float gBumpLevel = 0.0f;	// This is used to shake things when the car has gone off the road.


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// SHUFFLE Mechanism is below.


#define SIGN_NONE (-1)

struct trackPiece
{
	float      duration;    // Track length in seconds
	int        turn;        // -9 to 9
	int        hill;        // -9 to 9
	int        signType;    // (sign # or SIGN_NONE for signs)
	float      signJitter;	// 0.0-1.0  0.0 the sign appears at beginning of this track piece, 1.0 it's at the end
};

#define MAX_PIECES	10000

trackPiece UnrandomizedPieces[MAX_PIECES] =		// These are all of the available track pieces
{
	//dur  turn hill  signType    signJitter
	{ 0.0,   0,    0,   -1,    0.0},
	
	{0} // end of list
};

int NumTrackPieces = 0;					 // This gets filled with the number of track pieces
float currentTrackTime = 0.0f;			 // This is how many seconds along the current track we are
trackPiece ScratchPieces[MAX_PIECES];	 // Used in randomization
trackPiece RandomizedPieces[MAX_PIECES]; // These are the randomized pieces
bool SignAlreadyTriggered[MAX_PIECES];   // This is a list of which signs have been triggered, so we don't trigger them twice.
int mbCurrentHillLog = 0;
int mbCurrentTurnLog = 0;

// Given the time, get where the car is, AND see if we need to trigger a sign.
void GetSignSection(float currentTime, trackPiece *list, int *outSignTrigger)
{
	*outSignTrigger = SIGN_NONE;
	float t = LeadTime321Go_MAX;
	int section = 0;
	while (section < NumTrackPieces + 1)
	{
		t += list[section].duration;
//		printf("CurrentTime: %.2f OutSignTrigger: %d  t: %.2f \n", currentTime, outSignTrigger, t);
		
		if ((list[section].signType != SIGN_NONE) && (! SignAlreadyTriggered[section]))
		{
			float triggerT = 2.0f;
			if (currentTime + list[section].duration > t)
			{
				triggerT = 1.0 - ((t - currentTime) / list[section].duration);
			}
			if (triggerT > list[section].signJitter && triggerT < 2.0)
			{
				*outSignTrigger = list[section].signType;
				SignAlreadyTriggered[section] = true;
				printf("trigger sign %d triggerT %.2f \n", outSignTrigger, triggerT);
				return;
			}
		}
		section++;
	}
}

// Given the time, get which piece we're drawing

trackPiece *GetTrackSection(float currentTime, trackPiece *list)
{
//	printf("CurrentTime: %.2f \n", currentTime);
	float t = LeadTime321Go_MAX;
	int section = 0;

	float lookAheadTime = (NUM_ROAD_SECTIONS / ((gSpeed_MIN + gSpeed_MAX) * 0.5f)) * 0.75f;

	while (section < NumTrackPieces + 1) // && t < currentTime) //1.5 seconds off at start
	{
		t += list[section].duration;

		if (t > currentTime + lookAheadTime) //adding LeadTime here only doubles the first run,
		{
//				printf("now on %.2f - %d: %.1f %d %d %d %.1f\n", currentTrackTime, section, list[section].duration, list[section].turn, list[section].hill, list[section].signType, list[section].signJitter);
			return list + section;
		}
		section++;
	}
	return list + NumTrackPieces-1;	// we're off the list, so return the last section
}


void RandomizeTrack()
{
    printf("There are %d track pieces.\n", NumTrackPieces);
	
    // Copy it into the unrandomized list
    for (int i = 0; i < NumTrackPieces; ++i)
    {
        ScratchPieces[i] = UnrandomizedPieces[i];
//        printf("Unrandomized %d: %.1f %d %d %d %.1f\n", i, UnrandomizedPieces[i].duration, UnrandomizedPieces[i].turn, UnrandomizedPieces[i].hill, UnrandomizedPieces[i].signType, UnrandomizedPieces[i].signJitter);
    }
	
	
    int recentSign = SIGN_NONE;
	
    // Now SHUFFLE the rest into the random list
    int piecesRemaining = NumTrackPieces;
    for (int i = 0; i < NumTrackPieces; ++i)
    {
        int j = 0;
		
        if (i > 0)    // the first tile is ALWAYS first, not randomized
        {
            j = (RANDOMINT & 0x7fffffff) % piecesRemaining;        // Pick a random piece
            // Check for two GO signs in a row, and re-roll to make it very unlikely
            if (recentSign == SIGN_GO && ScratchPieces[j].signType == SIGN_GO)
            {
                j = (RANDOMINT & 0x7fffffff) % piecesRemaining;        // Pick a random piece
            }
            if (ScratchPieces[j].signType != SIGN_NONE)
            {
                recentSign = ScratchPieces[j].signType;
            }
        }
		
		
        RandomizedPieces[i] = ScratchPieces[j];
        // Now remove the chosen selection by replacing it with the one on the end
        ScratchPieces[j] = ScratchPieces[piecesRemaining - 1];
        piecesRemaining--;
//        printf("Randomized %d: %.1f %d %d %d %.1f\n", i, RandomizedPieces[i].duration, RandomizedPieces[i].turn, RandomizedPieces[i].hill, RandomizedPieces[i].signType, RandomizedPieces[i].signJitter);
    }
}

// This gets called ONCE before each race
void ResetTrack(char const *fileName)
{
    // Try to load the track file, if there is one.
    // If it's NULL, just use the same track we have.
    if (fileName != NULL && fileName[0] != 0)
    {
        FILE *fp = fopen(fileName, "r");
        if (fp != NULL)
        {
            printf("Reading track file '%s':\n", fileName);
            NumTrackPieces = 0;
			
            while (!feof(fp) && (NumTrackPieces < MAX_PIECES - 1))
            {
                char line[2048];
                line[0] = 0;
                fgets(line, sizeof(line) - 1, fp);
                int result = sscanf(line, " %f , %d , %d , %d , %f ", &(UnrandomizedPieces[NumTrackPieces].duration), &(UnrandomizedPieces[NumTrackPieces].turn), &(UnrandomizedPieces[NumTrackPieces].hill), &(UnrandomizedPieces[NumTrackPieces].signType), &(UnrandomizedPieces[NumTrackPieces].signJitter));
                if (result == 5)
                {
                    NumTrackPieces++;
                }
            }
            fclose(fp);
            printf("...loaded %d track pieces.\n", NumTrackPieces);
        }
        else
        {
            printf("FAILED to read track file '%s':\n", fileName);
        }
    }
	
    RandomizeTrack();
    currentTrackTime = 0.0f;
    for (int i = 0; i < NumTrackPieces + 1; ++i)
    {
        SignAlreadyTriggered[i] = false;
    }   
}

void GetCurrentTrack(float deltaTime, bool random, float *outTurn, float *outHill, int *outNewSign)
{
	currentTrackTime += deltaTime;
//	printf("time is: %.2f\n", currentTrackTime);
	trackPiece *track;
	if (random)
	{
		track = GetTrackSection(currentTrackTime, RandomizedPieces);
		GetSignSection(currentTrackTime, RandomizedPieces, outNewSign);
	}
	else 
	{
		track = GetTrackSection(currentTrackTime, UnrandomizedPieces);
		GetSignSection(currentTrackTime, UnrandomizedPieces, outNewSign);
	}
	
	float newTurn = CurveSharpness_MAX * (1.0f /  9.0f) * (float)track->turn;
	float newHill = HillSteepness_MAX  * (1.0f /  9.0f) * (float)track->hill;
	
	// Now place the return values
	*outTurn = newTurn;
	*outHill = newHill;
	mbCurrentHillLog = track->hill;		//logs current turn
	mbCurrentTurnLog = track->turn;		//logs current hill

	//printf("mbCurrentTurnLog: %d \n", mbCurrentTurnLog);
	//use LeadTime321Go_MAX to offset logging; can try to use this to offset signs too
	float lookAheadTime = (NUM_ROAD_SECTIONS / ((gSpeed_MIN + gSpeed_MAX) * 0.5f)) * 0.75f;
	if (currentTrackTime + lookAheadTime < LeadTime321Go_MAX)
	{
		*outTurn = 0.0f;
		*outHill = 0.0f;
		mbCurrentHillLog = 0;
		mbCurrentTurnLog = 0;
	}
	
}


Car::Car()
{
	mWidth = CarWidth_MIN;
	
	//	mpModel = glmReadOBJ("models/Brain_Bug_1.obj");
	mpModel = glmReadOBJ("models/BB_Test_2.obj");
	glmUnitize(mpModel);
	glmScale(mpModel, mWidth);
	
	mTexture = LoadTexture("models/bug_1.bmp", false, true);
	//	mTexture = LoadTexture("images/tesla_128x256.bmp", false, true);
	Mat44MakeIdentity(mMatrix);
	Vec4Set(mMatrix.t, 0.0f, 0.2f, 10.0f, 1.0f);
	
	mCurrentSpeed = (gSpeed_MIN + gSpeed_MAX) * 0.5f;
	mOdometer = 0.0f;
	mCarRoadCenterOffset = 0.0f;
	mbKeyboardAccel = false;
	mbKeyboardBrake = false;
	mbKeyboardLeft = false;
	mbKeyboardRight = false;
	mbKeyboardButton = false;
	mbKeyboardButtonPressed = false;
	mbKeyboardButton_Prev = false;
	mJoystickX = 0.0f;
	mJoystickY = 0.0f;
	mbJoystickButton = false;
	mbJoystickRight = false;
	mbJoystickLeft = false;
	mbJoystickRight_Prev = false;
	mbJoystickLeft_Prev = false;
	mbJoystickRightPressed = false;
	mbJoystickLeftPressed = false;
	mbButtonDown = false;
	mbButtonDown_Prev = false;
	mbButtonPressed = false;
	mbButtonReleased = false;
}

Car::~Car()
{
}

RoadSign::RoadSign()
{
	for (int i = 0; i < NUM_DIFFERENT_ROADSIGNS; ++i)
	{
		char signName[512];
		sprintf(signName, "images/roadsign_%d.bmp", i);
		mTextures[i] = LoadTexture(signName, false, true);
		
	}
	
	Mat44MakeIdentity(mMatrix);
	Vec4Set(mMatrix.t, 0.0f, -1000.0f, 0.0f, 1.0f);
	mTimer = RANDOM_IN_RANGE(TimeBeforeFirstSign_MIN, TimeBeforeFirstSign_MAX);;
	mbVisible = false;
	mbNewlyVisible = false;
	mCurrentSignTexture = -1;
	mBlinkerCountdown3 = 0;  
	mBlinkerCountdown4 = 0; 
	mBlinkerCountdown2 = 0;  
	mBlinkerCountdown = 0; 
	
}

RoadSign::~RoadSign()
{
}

Road::Road()
{
	// enable the depth test and 2D texturing
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	
	//mCurrentGameScreen = GAMESCREEN_START;
	mCurrentGameScreen = GAMESCREEN_SPLASH;	
	
	mTransitionTime = 0.0f;
	mGameScreenTotalTime = 0.0f;
	mbSpeedometerOnRoad = true;
	mbFixation = 0;
	
	mBlinkerTexture = LoadTexture("images/roadsignVisibleBlinker.bmp", false, true);
	mBlinkerTexture2 = LoadTexture("images/roadsignVisibleBlinker2.bmp", false, true);	
	mBlinkerTexture3 = LoadTexture("images/roadsignVisibleBlinker3.bmp", false, true);
	mBlinkerTexture4 = LoadTexture("images/roadsignVisibleBlinker4.bmp", false, true);	
	
	// load texture as compressed
	mTexture_Road = LoadTexture("images/road_center.bmp", false, true);
	mTexture_Grass = LoadTexture("images/road_grass.bmp", false, true);
	mTexture_Curb = LoadTexture("images/road_curb.bmp", false, true);
	
	mTexture_StartScreen = LoadTexture("images/startscreen.bmp", false, true);
	mTexture_TransitionScreen = 0; //LoadTexture("images/transition2.bmp", false, true);
	mTexture_FinalTransitionScreen = 0; //LoadTexture("images/finalScreen.bmp", false, true);
	
	mTexture_TransCheck = LoadTexture("images/transCheck2.bmp", false, true);
	mTexture_TransX = LoadTexture("images/transX2.bmp", false, true);
	mTexture_TransDash = LoadTexture("images/transDash2.bmp", false, true);
	
	//mTexture_TransCheck = LoadTexture("images/check.tga", false, true);
	//mTexture_TransX = LoadTexture("images/X.tga", false, true);
	//mTexture_TransDash = LoadTexture("images/tga.bmp", false, true);
	mTexture_SplashScreen = LoadTexture("images/SplashScreen.bmp", false, true);	
	mTexture_FinishScreen = LoadTexture("images/finishscreen.bmp", false, true);
	mTexture_PauseScreen = LoadTexture("images/pausescreen.bmp", false, true);
	
	///////joaq attempt at static signs////////////////
	for (int i = 0; i < NUM_DIFFERENT_ROADSIGNS; ++i)
	{
		char signName[512];
		sprintf(signName, "images/roadsign_%d.bmp", i);
		mTextures[i] = LoadTexture(signName, false, true);
		//mTexture_StaticSign = LoadTexture("images/roadsign_0.bmp", false, true);
		if (i > 0)
			mTextures[i] || mTexture_StaticSign;
		mTexture_StaticSign = LoadTexture(signName, false, true);
		
	}	
	///end of joaq attempt at static signs//////////////
	
	mTexture_Count3 = LoadTexture("images/count_3.bmp", false, true);
	mTexture_Count2 = LoadTexture("images/count_2.bmp", false, true);
	mTexture_Count1 = LoadTexture("images/count_1.bmp", false, true);
	mTexture_CountGo = LoadTexture("images/count_go.bmp", false, true);
	mTexture_SpeedBar = LoadTexture("images/speed_bar.bmp", false, true);
	mTexture_SpeedBump = LoadTexture("images/speed_bump.bmp", false, true);
	mTexture_SpeedBumpMiddle = LoadTexture("images/speed_bump_middle.bmp", false, true);
	
	for (int i = 0; i < 10; ++i)
	{
		char name[256];
		int w, h;
		sprintf(name, "images/digit_%d.bmp", i);
		mTexture_Digit[i] = LoadTexture(name, false, true, &w, &h);
		mTexture_DigitSize[i][0] = w;
		mTexture_DigitSize[i][1] = h;
	}
	
	
	
	
	
	
	mSectionDistance = 1.0f;
	mCurrentCarSection = 0;
	mLookAheadSections = (NUM_ROAD_SECTIONS * 3) / 4;
	mNewRebuiltSection = 0;
	mFractionalSection = 0.0f;
	mRoadWidth = RoadGrayWidth_MIN;
	mYellowWidth = RoadYellowWidth_MIN;
	mRedWidth = RoadRedWidth_MIN;
	mTextureScaleAlongRoad = 0.03f;
	
	mbTurning = false;
	mCurrentTurnTimer = 3.0f;
	mCurrentTurnStrength = 0.0f;
	
	mbOnHill = false;
	mCurrentHillTimer = 3.0f;
	mCurrentHillStrength = 0.0f;
	
	// initialise performance history. 
	for (int i=0; i<(NUM_AVERAGED_VALUES); i++) {
		performanceHistory[i] = 0.0f;
		
		//DWD shuffle: This line may need to be tweaked to [ performanceHistory[i] = 70.0f; ]
		
	}
	
	
	extropIndexRoad = 0;
	extropIndexSign = 0;
	signThreshCount = 0;
	roadThreshCount = 0;
	storedValRoad = false;
	storedValSign = false;
	
	for (int i = 0; i < NUM_ROAD_SECTIONS; ++i)
	{
		mSections[i].mSerialNumber = i;
		mSections[i].mRise = 0.0f;
		mSections[i].mTurn = 0.0f;
		mSections[i].mRoadTextureOffset = i * mTextureScaleAlongRoad;
		Mat44MakeIdentity(mSections[i].mMatrix);
		Vec4Set(mSections[i].mMatrix.t, 0.0f, 0.0f, i * mSectionDistance, 1.0f);
	}
	
	mbAutopilot = false;
	mbOverrideSpeed = false;
}

Road::~Road()
{
}

void Road::UpdateControls(float deltaTime)
{
	mCar.mbButtonDown = (mCar.mbJoystickButton || mCar.mbKeyboardButton);
	mCar.mbButtonPressed  = false;
	mCar.mbButtonReleased = false;
	if (mCar.mbButtonDown && ! mCar.mbButtonDown_Prev)
	{
		mCar.mbButtonPressed = true;
	}
	else if (mCar.mbButtonDown_Prev && ! mCar.mbButtonDown)
	{
		mCar.mbButtonReleased = true;
	}
	mCar.mbButtonDown_Prev = mCar.mbButtonDown;

	mCar.mbJoystickLeftPressed = false;
	if (mCar.mbJoystickLeft && ! mCar.mbJoystickLeft_Prev)
	{
		mCar.mbJoystickLeftPressed = true;
	}
	mCar.mbJoystickLeft_Prev = mCar.mbJoystickLeft;

	mCar.mbJoystickRightPressed = false;
	if (mCar.mbJoystickRight && ! mCar.mbJoystickRight_Prev)
	{
		mCar.mbJoystickRightPressed = true;
	}
	mCar.mbJoystickRight_Prev = mCar.mbJoystickRight;

	mCar.mbKeyboardButtonPressed = false;
	if (mCar.mbKeyboardButton  && ! mCar.mbKeyboardButton_Prev)
	{
		mCar.mbKeyboardButtonPressed = true;
		printf("Keyboard button pressed.\n");
	}
	mCar.mbKeyboardButton_Prev = mCar.mbKeyboardButton;
}

void Road::CheckForScreenTransition(float deltaTime)
{
	GameScreen newScreen = mCurrentGameScreen;
	switch (mCurrentGameScreen)
	{
		case GAMESCREEN_START:
			if (mCar.mbButtonPressed && Behav0EEG1fMRI2_MIN !=2)
				newScreen = GAMESCREEN_PLAY;
/*			//During fMRI, user starts game during thresholding and cannot during any other mode ('5' starts game)
			else if (Behav0EEG1fMRI2_MIN == 2 && gCurrentGamePlayCount >= 8 && ((gStoryBoardMode == 10 || gStoryBoardMode == 12 || gStoryBoardMode == 13) && (gPrevStoryBoardMode == 10 || gPrevStoryBoardMode == 12 || gPrevStoryBoardMode == 13)))
				newScreen = GAMESCREEN_PLAY;
			else if (Behav0EEG1fMRI2_MIN == 2 && gCurrentGamePlayCount < 7 && gCurrentGamePlayCount != 0)
				newScreen = GAMESCREEN_PLAY;
*/			//Only at the beginning of superruns (1, 2, or 3a) does the scanner ('5) start; for all rest (3a, 3b), it starts immediately
//Sign Localizer storyboard below
			else if (Behav0EEG1fMRI2_MIN == 2 && (gCurrentGamePlayCount) % 6 != 0)
				newScreen = GAMESCREEN_PLAY;
			break;
		case GAMESCREEN_SPLASH:
			if (mCar.mbButtonPressed)
				newScreen = GAMESCREEN_START;
			break;			
		case GAMESCREEN_PLAY:
			if (mGameScreenTotalTime > gStoryBoardPlayLengthSeconds + LeadTime321Go_MAX)  // add in "3,2,1,GO!!"
				newScreen = GAMESCREEN_FINISH;
			break;
		case GAMESCREEN_FINISH:
			if (mCar.mbButtonPressed)
				newScreen = GAMESCREEN_TRANSITION;
			if (mGameScreenTotalTime > EndScreenTimeSECONDS_MAX)
				newScreen = GAMESCREEN_TRANSITION;
			break;
		case GAMESCREEN_TRANSITION:
			mTransitionTime += deltaTime;
			if (Behav0EEG1fMRI2_MIN == 1) // during EEG mode
			{
				if ((gStoryBoardMode != 0 && gPrevStoryBoardMode == 0) || (gPrevStoryBoardMode == 5 && gStoryBoardMode != 5))     
				{//If thresholding just finished, wait for experimenter to hit a keyboard
					if (mCar.mbKeyboardButtonPressed)
					{
						if (strncmp (goldStoryBoardTransitionScreenName,"default",2) != 0)
							newScreen = GAMESCREEN_FINAL_TRANSITION;
				
						else
							newScreen = GAMESCREEN_START;
					}
				}
				else if(mCar.mbButtonPressed)  
				{//Otherwise, just let user advance game
					if (strncmp (goldStoryBoardTransitionScreenName,"default",2) != 0)
						newScreen = GAMESCREEN_FINAL_TRANSITION;
				
					else
						newScreen = GAMESCREEN_START;
				}
			}
			else if (Behav0EEG1fMRI2_MIN == 2) // during fMRI mode
			{
				// deltaTime GetTimerFuc() GetMillisecondTimer();
				// wait 15 seconds...
				printf("%.2f\n", mTransitionTime);
				if (gCurrentGamePlayCount < 8)
				{
					if(mTransitionTime >= fMRILocalizerTime_MAX)
					{
						if (strncmp (goldStoryBoardTransitionScreenName,"default",2) != 0)
							newScreen = GAMESCREEN_FINAL_TRANSITION;
				
						else
							newScreen = GAMESCREEN_START;
					}
				}
				else
				{
					if(mTransitionTime >= fMRIExperimentTime_MIN)
					{
						if (strncmp (goldStoryBoardTransitionScreenName,"default",2) != 0)
							newScreen = GAMESCREEN_FINAL_TRANSITION;
				
						else
							newScreen = GAMESCREEN_START;
					}
				}
			}
			else
			{
				if(mCar.mbButtonPressed)  
				{//Otherwise, just let user advance game
					if (strncmp (goldStoryBoardTransitionScreenName,"default",2) != 0)
						newScreen = GAMESCREEN_FINAL_TRANSITION;
				
					else
						newScreen = GAMESCREEN_START;
				}
			}
//Basically, if thresholding is done, wait for keyboard press so experimenter can record result
//Otherwise, always allow joystick button press to advance transition 
// unless in fMRI mode where 15 seconds needs to be allocated before advancing -- 15.625
			break;
		case GAMESCREEN_FINAL_TRANSITION:
			mTransitionTime += deltaTime;
			if (Behav0EEG1fMRI2_MIN == 0) {
				if (mCar.mbButtonPressed) {
					newScreen = GAMESCREEN_START;
				}
			}
			else if (Behav0EEG1fMRI2_MIN == 1) {
				if (mCar.mbKeyboardButtonPressed || ((gStoryBoardMode == 0 || gStoryBoardMode == 5) && mCar.mbButtonPressed) ) {
					newScreen = GAMESCREEN_START;
				}
			}
			else if (Behav0EEG1fMRI2_MIN == 2) {				
				printf("%.2f\n", mTransitionTime);
				//Task instruction 
				if (gCurrentGamePlayCount < 8)
				{
					if(mTransitionTime >= fMRILocalizerTime_MIN)
					{
						newScreen = GAMESCREEN_START;
					}
				}
				else
				{
				//Localizer instruction
					if(mTransitionTime >= fMRIExperimentTime_MAX)
					{
						newScreen = GAMESCREEN_START;
					}
				}
			}
			
			break;
		default:
			break;
	}
	
	if (newScreen != mCurrentGameScreen)
	{
		TransitionToScreen(newScreen);
	}
}

void Road::TogglePause()
{
	// Close down the old screen
	if (mCurrentGameScreen == GAMESCREEN_PLAY)	// pause the game!
	{
		mCurrentGameScreen = GAMESCREEN_PAUSE;
	}
	else if (mCurrentGameScreen == GAMESCREEN_PAUSE)  // un-pause the game!
	{
		mCurrentGameScreen = GAMESCREEN_PLAY;
	}
}

void Road::TransitionToScreen(GameScreen toScreen)
{
	// Close down the old screen
	switch (mCurrentGameScreen)
	{
		case GAMESCREEN_START:
			if (gCurrentGamePlayCount == 0) {
				if (gStoryBoardMode == 0 || gStoryBoardMode == 12 || gStoryBoardMode == 13) {
					TimeBeforeFirstSign_MIN = gStoryBoardPlayLengthSeconds +10;
					TimeBeforeFirstSign_MAX = gStoryBoardPlayLengthSeconds +10;
				}		
				mbAutopilot = false;
				mbOverrideSpeed = false;
			}
			
			break;
		case GAMESCREEN_PLAY:
			// WriteDataLog();
			break;
		case GAMESCREEN_FINISH:
			break;
		case GAMESCREEN_PAUSE:
			break;
		case GAMESCREEN_TRANSITION:
			break;
		case GAMESCREEN_FINAL_TRANSITION:
			break;
		case GAMESCREEN_SPLASH:
			break;			
		default:
			break;
	}
	
	mCurrentGameScreen = toScreen;
	mGameScreenTotalTime = 0.0f;
	mTransitionTime = 0.0f;
	
	// Start up the new screen
	switch (mCurrentGameScreen)
	{
		case GAMESCREEN_START:
			break;
		case GAMESCREEN_SPLASH:
			break;			
		case GAMESCREEN_PLAY:
		
			if (Behav0EEG1fMRI2_MIN != 2)
			{
				if (gStoryBoardMode == 0){
					ResetTrack("track01.csv"); //NEW DWD SHuffle addition; This track is for Road Thresholding (no signs, 63sec run)
				}
			
				if (gStoryBoardMode == 5){
					ResetTrack("track02.csv"); //NEW DWD SHuffle addition; This track is for Sign Thresholding (signs, no turns/hills, 135 sec run)
				}
			
				if (gStoryBoardMode == 13 || gStoryBoardMode == 12){
					ResetTrack("track03.csv"); //NEW DWD SHuffle addition; This track is for DWD Behave (NO signs, 198 sec run)
				}
			
				if (gStoryBoardMode == 6  || gStoryBoardMode == 8 || gStoryBoardMode == 10 || gStoryBoardMode == 11 || gStoryBoardMode == 14 || gStoryBoardMode == 15){
					ResetTrack("track04.csv"); //NEW DWD SHuffle addition; This track is for (nearly) all other conditions in DWD Behave (signs, 198 sec run)
				}
			
				if (gStoryBoardMode == 16 || gStoryBoardMode == 17){
					ResetTrack("track05.csv"); //NEW DWD SHuffle addition; This track is for DWD Behave ("Black out", signs, no hills/turns, 198 sec run)
				}
			}
			else if (Behav0EEG1fMRI2_MIN == 2) //fMRI needs new csv controller structure oba
			{
				int levelnumber = gCurrentGamePlayCount + 10;
				char MRIfilename[15];
				sprintf(MRIfilename, "track%d.csv", levelnumber);
				printf("Calling filname: %s\n", MRIfilename);
				ResetTrack(MRIfilename);
//				}
			}
			
			gTotalSamples = 0;
			gTotalRedSamples = 0;
			gTotalYellowSamples = 0;
			
			gRedSpeedSamples =0 ;
			gYellowSpeedSamples =0 ;
			gGraySpeedSamples=0;
			
			gRedAccuracySamples=0;
			gYellowAccuracySamples=0;
			gGrayAccuracySamples=0;
			
			gTotalGraySamples = 0;
			gSignStartTime = 0.0;
			gSignLatency = 0.0;
			gTotalSignLatency = 0.0;
			gAverageSignLatency = 0.0;
			gAllSignsTotal = 0;
			gSignFalsePositives = 0;
			gGoSignsTriggered = 0;
			gGoSignsTotal = 0;
			gSignsCorrect = 0;
			gSignsIncorrect = 0; 
			gLastSignVisible = 0;
			gLastGoSignPercent = 0;
			gLastFPSignPercent = 0;
			
			if (gStoryBoardMode ==5  || gStoryBoardMode == 8 || gStoryBoardMode == 9 || gStoryBoardMode == 11 || gStoryBoardMode == 12 || gStoryBoardMode == 14  || gStoryBoardMode == 16 || gStoryBoardMode == 17){
				mbAutopilot = true;
			}
			else{
				mbAutopilot = false;
			}
			if (gStoryBoardMode == 5 || gStoryBoardMode == 16 || gStoryBoardMode == 17) {
				blackout = true;
			}
			else {
				blackout = false;
			}
			
			mbOverrideSpeed = false;
			
			break;
		case GAMESCREEN_FINISH:
			break;
		case GAMESCREEN_PAUSE:
			break;
		case GAMESCREEN_FINAL_TRANSITION:
			break;
		case GAMESCREEN_TRANSITION:
		{
			
			float percentGray = 0.0f;
			// float percentYellow = 0.0f;
			float percentRed			= 0.0f;
			float averagePercentGray	= 0.0f;
			float percentAccuracyGray	= 0.0f;
			float percentAccuracyRed	= 0.0f;
			float percentSpeedGray		= 0.0f;
			float percentSpeedRed		= 0.0f;
			// float levelchangePercent	= 0.0f;
			float percentSignAccuracy   = 0.0f;
			float fpPercentSignAccuracy = 0.0f;			
			float averageSignAccuracy	= 0.0f;
			
			
			// Now we calculate accuracy percentages separately. 
			percentAccuracyGray = 100.0 * ((float)gGrayAccuracySamples/(float)gTotalSamples);
			gLastGrayAccuracyPercent = (int)(percentAccuracyGray + 0.5f);
			percentAccuracyRed = 100.0 * ((float)gRedAccuracySamples/(float)gTotalSamples);
			gLastRedAccuracyPercent = (int)(percentAccuracyRed + 0.5f);
			gLastYellowAccuracyPercent = 100 - (gLastGrayAccuracyPercent + gLastRedAccuracyPercent);		
			
			// Now we calculate the speed percentages separately. 
			percentSpeedGray = 100.0 * ((float)gGraySpeedSamples/(float)gTotalSamples);
			gLastGraySpeedPercent = (int)(percentSpeedGray + 0.5f);
			percentSpeedRed = 100.0 * ((float)gRedSpeedSamples/(float)gTotalSamples);
			gLastRedSpeedPercent = (int)(percentSpeedRed + 0.5f);
			gLastYellowSpeedPercent = 100 - (gLastGraySpeedPercent + gLastRedSpeedPercent);				
			
			//			percentGray = (percentAccuracyGray + percentSpeedGray)/2;
			//			gLastGrayPercent = (int)(percentGray + 0.5f);
			
			// We will use these combined percentages 
			percentGray = 100.0 * ((float)gTotalGraySamples/(float)gTotalSamples);
			gLastGrayPercent = (int)(percentGray + 0.5f);
			percentRed = 100.0 * ((float)gTotalRedSamples/(float)gTotalSamples);
			gLastRedPercent = (int)(percentRed + 0.5f);
			gLastYellowPercent = 100 - (gLastGrayPercent + gLastRedPercent);
			
			// increment the results buffer 
			//
			// DWD SHUFFLE CHANGE HERE: MAY NEED TO TWEAK THIS 
			
			for (int i=0; i<(NUM_AVERAGED_VALUES-1); i++) {
			    performanceHistory[i] = performanceHistory[i+1];
			}
			performanceHistory[2] = percentGray;
			
			
			
			/*			if (gStoryBoardMode == 0)
			 {
			 for (int i=0; i<(NUM_AVERAGED_VALUES-1); i++) {
			 performanceHistory[i] = performanceHistory[i+1];
			 }
			 }
			 performanceHistory[NUM_AVERAGED_VALUES - 1] = percentGray;
			 if (gStoryBoardMode != 0  && prevStoryBoardMode == 0)
			 {
			 for (int i=0; i<(NUM_AVERAGED_VALUES - 1); i++) {
			 if (performanceHistory[i] < 80)
			 {
			 if (performanceHistory[i - 1] < 80)
			 {
			 
			 }
			 }
			 if (performanceHistory[i] < 80)
			 {
			 
			 }
			 if (performanceHistory[i] == 80)
			 {
			 
			 }
			 }
			 */
			
			// Implement Moving Average Filter here.
			// Take this out for now.  
			//float sum = 0;       // Start the total sum at 0.
			//for (int i=0; i<NUM_AVERAGED_VALUES; i++) {
			//	sum = sum + performanceHistory[i];  // Add the next element to the total
			//}
			//averagePercentGray = (float)sum/NUM_AVERAGED_VALUES;
			averagePercentGray = percentGray;
			
			// Go Sign Accuracy 
			// New percentage for last run. 
			if (gGoSignsTotal > 1) {
//				percentSignAccuracy = 100.f - 100.0f * ((float)(gGoSignsTotal - gGoSignsTriggered + gSignFalsePositives)/(float)gAllSignsTotal);
				percentSignAccuracy = 100.0f * (gSignsCorrect)/(gSignsCorrect + gSignsIncorrect);
				gLastGoSignPercent = (int)(percentSignAccuracy + 0.5f); 
//				printf("Percent Sign Accuracy: %.2f,  Signs Correct%: %.2f gSignsTotal: %d gAllSignsTotal: %.2f \n", percentSignAccuracy, (float)gSignsCorrect/(float)gAllSignsTotal, (gSignsCorrect + gSignsIncorrect), (float)gAllSignsTotal);
			}
			else {
				gLastGoSignPercent = 0;
			}
			
			// False Positive Sign Accuracy
			if ((gAllSignsTotal - gGoSignsTotal)!=0) {
				fpPercentSignAccuracy = 100.0f * ((float)gSignFalsePositives/(float)(gAllSignsTotal - gGoSignsTotal));
				gLastFPSignPercent = (int)(fpPercentSignAccuracy + 0.5f);
			}
			else {
				gLastFPSignPercent = 0;
			}
			
			// Average Go Sign Latency 
			if (gGoSignsTriggered!=0) {
				gAverageSignLatency = ((float)gTotalSignLatency/(float)gGoSignsTriggered);
			}
			else {
				gAverageSignLatency = 0;
			}			
			// Test to see if this makes a difference. 
			// gGoSignsTriggered = 0;
			
			// The two adjustments are, sign, and road. 
			//
			// int generalroadperformance = (percentSpeedGray + averagePercentGray)/2;
			// Take the lesser of the two. 
			// 
			int generalroadperformance = 0;
			generalroadperformance = percentGray;
			
			
			//Road thresholder that extrapolates based on historical performance, skipping the first three runs
			if (gStoryBoardMode == 0)
			{
				if (roadThreshCount < 3)  //first three runs
				{
					roadThreshCount++;
				}
				else if (storedValRoad)
				{
					if (generalroadperformance == 80)		// if its 80%, just take the value
					{
						extrapolatedRoad[extropIndexRoad] = gCurrentSpeedLevel;
						extropIndexRoad = extropIndexRoad + 1;
					}
					else if (lastNeg)	//if the prev value was below 80%
					{
						if (generalroadperformance < 80 && generalroadperformance > lastNegativePerf)
						{
							//is the current value below 80% but closer to 80% than previous stored value? then replace					
							lastNegativePerf = generalroadperformance;
							lastNegativeLvl = gCurrentSpeedLevel;
							lastNeg = true;
						}
						if (generalroadperformance > 80) //is current value over 80 while prev was below?
						{
							extrapolatedRoad[extropIndexRoad] = gCurrentSpeedLevel + (generalroadperformance - 80)*((lastNegativeLvl - gCurrentSpeedLevel)/(generalroadperformance - lastNegativePerf));
							extropIndexRoad = extropIndexRoad + 1;
							
							//store new 'above 80' value
							lastPositivePerf = generalroadperformance;	//store new positive values
							lastPositiveLvl = gCurrentSpeedLevel;
							lastNeg = false;
						}
					}
					
					else if (!lastNeg)	//if the prev value was above 80%
					{
						if (generalroadperformance > 80 && generalroadperformance < lastPositivePerf)
						{
							//is the current value above 80% but closer to 80% than previous stored value? then replace					
							lastPositivePerf = generalroadperformance;
							lastPositiveLvl = gCurrentSpeedLevel;
							lastNeg = false;
						}
						if (generalroadperformance < 80) //is current value below 80 while prev was above?
						{
							extrapolatedRoad[extropIndexRoad] = gCurrentSpeedLevel + (generalroadperformance - 80)*((lastPositiveLvl - gCurrentSpeedLevel)/(generalroadperformance - lastPositivePerf));
							extropIndexRoad = extropIndexRoad + 1;
							
							//store new 'below 80' value
							lastNegativePerf = generalroadperformance;	//store new positive values
							lastNegativeLvl = gCurrentSpeedLevel;
							lastNeg = true;
						}
					}
				}
				else if (!storedValRoad)  // handles first run
				{
					if (generalroadperformance > 80)
					{
						storedValRoad = true;
						lastPositivePerf = generalroadperformance;	//store new positive values
						lastPositiveLvl = gCurrentSpeedLevel;
						lastNeg = false;
					}
					else if (generalroadperformance < 80)
					{
						lastNegativePerf = generalroadperformance;
						lastNegativeLvl = gCurrentSpeedLevel;
						lastNeg = true;
						storedValRoad = true;
					}
					else if (generalroadperformance == 80)
					{
						extrapolatedRoad[extropIndexRoad] = gCurrentSpeedLevel;
						extropIndexRoad = extropIndexRoad + 1;
						storedValRoad = false;
					}
				}
				//				printf("lastNegPerf: %d lastPosPerf: %d \n generalroadperf: %d \n index: %d \n",lastNegativePerf, lastPositivePerf, generalroadperformance, extropIndexRoad);
				//				printf("gStoryBoardMode: %d \n", gStoryBoardMode);
				//				printf("roadThreshCount : %d \n", roadThreshCount);
				//				printf("storedValRoad: %d \n", storedValRoad);
				//				printf("lastneg: %d \n", lastNeg);
			}
			
			//Sign thresholder that extrapolates based on historical performance, skipping the first three runs
			if (gStoryBoardMode == 5)
			{
				if (signThreshCount < 3)  //first three runs
				{
					signThreshCount++;
				}
				else if (storedValSign)
				{
					if (percentSignAccuracy == 80)		// if its 80%, just take the value
					{
						extrapolatedSign[extropIndexSign] = gCurrentSignLevel;
						extropIndexSign = extropIndexSign + 1;
					}
					else if (lastNeg)	//if the prev value was below 80%
					{
						if (percentSignAccuracy < 80 && percentSignAccuracy > lastNegativePerf)
						{
							//is the current value below 80% but closer to 80% than previous stored value? then replace					
							lastNegativePerf = percentSignAccuracy;
							lastNegativeLvl = gCurrentSignLevel;
							lastNeg = true;
						}
						if (percentSignAccuracy > 80) //is current value over 80 while prev was below?
						{
							extrapolatedSign[extropIndexSign] = gCurrentSignLevel + (percentSignAccuracy - 80)*((lastNegativeLvl - gCurrentSignLevel)/(percentSignAccuracy - lastNegativePerf));
							extropIndexSign = extropIndexSign + 1;
							
							//store new 'above 80' value
							lastPositivePerf = percentSignAccuracy;	//store new positive values
							lastPositiveLvl = gCurrentSignLevel;
							lastNeg = false;
						}
					}
					
					else if (!lastNeg)	//if the prev value was above 80%
					{
						if (percentSignAccuracy > 80 && percentSignAccuracy < lastPositivePerf)
						{
							//is the current value above 80% but closer to 80% than previous stored value? then replace					
							lastPositivePerf = percentSignAccuracy;
							lastPositiveLvl = gCurrentSignLevel;
							lastNeg = false;
						}
						if (percentSignAccuracy < 80) //is current value below 80 while prev was above?
						{
							extrapolatedSign[extropIndexSign] = gCurrentSignLevel + (percentSignAccuracy - 80)*((lastPositiveLvl - gCurrentSignLevel)/(percentSignAccuracy - lastPositivePerf));
							extropIndexSign = extropIndexSign + 1;
							
							//store new 'below 80' value
							lastNegativePerf = percentSignAccuracy;	//store new positive values
							lastNegativeLvl = gCurrentSignLevel;
							lastNeg = true;
						}
					}
				}
				else if (!storedValSign)  // handles first run
				{
					if (percentSignAccuracy > 80)
					{
						storedValSign = true;
						lastPositivePerf = percentSignAccuracy;	//store new positive values
						lastPositiveLvl = gCurrentSignLevel;
						lastNeg = false;
					}
					else if (percentSignAccuracy < 80)
					{
						lastNegativePerf = percentSignAccuracy;
						lastNegativeLvl = gCurrentSignLevel;
						lastNeg = true;
						storedValSign = true;
					}
					else if (percentSignAccuracy == 80)
					{
						extrapolatedSign[extropIndexSign] = gCurrentSignLevel;
						extropIndexSign = extropIndexSign + 1;
						storedValSign = false;
					}
				}
				printf("lastNegPerf: %d lastPosPerf: %d \n percentSignAcc: %d \n index: %d \n",lastNegativePerf, lastPositivePerf, percentSignAccuracy, extropIndexSign);
				printf("gStoryBoardMode: %d \n", gStoryBoardMode);
				printf("signThreshCount : %d \n", signThreshCount);
				printf("storedValSign: %d \n", storedValSign);
				printf("lastneg: %d \n", lastNeg);
			}
			
			
			
			
			
			// Signs
			int signAdjustment = 0;
			float jumpPerStep = (100 - SignTargetPerformanceRange_MAX)/SignLevelRange_MAX;
			for (int i= 1; i <= SignLevelRange_MAX - SignLevelRange_MIN + 1; i++){
				if(((percentSignAccuracy <= (100 - (jumpPerStep*(i-1)))) && i <= (SignLevelRange_MAX + 1))){
					signAdjustment = SignLevelRange_MAX - i + 1;
				}
				if((i > (SignLevelRange_MAX + 1)) && (percentSignAccuracy <= (SignTargetPerformanceRange_MIN - (jumpPerStep*(i - SignLevelRange_MAX - 2)))) ){
					signAdjustment = SignLevelRange_MAX - i + 1;
				}
			}
			//			printf("Sign Accuracy: %d \n", percentSignAccuracy);			
			
			
			// Road
			int roadAdjustment = 0;
			jumpPerStep = (100 - RoadTargetPerformanceRange_MAX)/RoadLevelRange_MAX;
			for (int i= 1; i <= RoadLevelRange_MAX - RoadLevelRange_MIN + 1; i++){
				if(((generalroadperformance <= (100 - (jumpPerStep*(i-1)))) && i <= (RoadLevelRange_MAX + 1))){
					roadAdjustment = RoadLevelRange_MAX - i + 1;
				}
				if((i > (RoadLevelRange_MAX + 1)) && (generalroadperformance <= (RoadTargetPerformanceRange_MIN - (jumpPerStep*(i - RoadLevelRange_MAX - 2)))) ){
					roadAdjustment = RoadLevelRange_MAX - i + 1;
				}
			}
			//			printf("Road Accuracy: %d \n", generalroadperformance);
			
			// Mode display adjustment. 					
			if (gStoryBoardMode == 0) {  // Road accuracy only, no signs. 
				// gOverallAccuracy = generalroadperformance;
				gOverallAccuracy = 80.0;
			}
			else if (gStoryBoardMode == 1) { // Road Accuracy only, but show signs. 
				// gOverallAccuracy = percentSignAccuracy;
				gOverallAccuracy = 80.0;
			}
			else if (gStoryBoardMode == 2) { // Sign Accuracy only. 
				// gOverallAccuracy = percentSignAccuracy;
				gOverallAccuracy = 80.0;
			}
			else if (gStoryBoardMode == 3 ||  gStoryBoardMode == 4){ // Use both sign and road accuracy gStoryBoardMode == 3 
				if (generalroadperformance < percentSignAccuracy)
					gOverallAccuracy = generalroadperformance;
				else
					gOverallAccuracy = percentSignAccuracy;
			}
			else {
				gOverallAccuracy = 80.0;
			}
			
			// Overall difficulty 
			int OverallAdjustment = 0;
			jumpPerStep = (100 - OverallTargetPerformanceRange_MAX)/OverallLevelRange_MAX;
			for (int i= 1; i <= OverallLevelRange_MAX - OverallLevelRange_MIN + 1; i++){
				if(((gOverallAccuracy <= (100 - (jumpPerStep*(i-1)))) && i <= (OverallLevelRange_MAX + 1))){
					OverallAdjustment = OverallLevelRange_MAX - i + 1;
				}
				if((i > (OverallLevelRange_MAX + 1)) && (gOverallAccuracy <= (OverallTargetPerformanceRange_MIN - (jumpPerStep*(i - OverallLevelRange_MAX - 2)))) ){
					OverallAdjustment = OverallLevelRange_MAX - i + 1;
				}
			}
			//			printf("Overall Accuracy: %d \n", gOverallAccuracy);
			
			// 
			// This is to record the prvious road speed level so we can display it later. 
			gLastSpeedAdjustment = gCurrentSpeedLevel;
			gLastSignAdjustment  = gCurrentSignLevel;
			gLastDifficultyAdjustment = OverallAdjustment;	
			
			
			
			// Now we only adjust the difficulty level, in certain modes. Mode dependent difficulty adjustment. 
			// We seem to be resetting some values to zero
			if (gStoryBoardMode == 0) {  // Road accuracy only, no signs. 
				AdjustAllDifficultyLevels(0, roadAdjustment, 0, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 1) { // Road accuracy, but show signs. 
				AdjustAllDifficultyLevels(0, roadAdjustment, 0, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 2) { // Sign Accuracy only. 
				AdjustAllDifficultyLevels(0, 0, signAdjustment, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode ==3 ) { // Use both sign and road accuracy
				AdjustAllDifficultyLevels(OverallAdjustment, roadAdjustment, signAdjustment, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 4) {  // count both roads and sigs but never implement any difficulty adjustment. 
				AdjustAllDifficultyLevels(0,0,0, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 5) { //road accuracy, show signs, no driving
				AdjustAllDifficultyLevels(0, 0, signAdjustment, gCurrentGamePlayCount);
			}	//same as mode 2, but no driving
			else if (gStoryBoardMode == 6) { //sign testing, all signs relevant
				AdjustAllDifficultyLevels(0, 0, 0, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 7) { //sign thresholding, all signs relevant
				AdjustAllDifficultyLevels(0, 0, signAdjustment, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 8) { //sign testing, all signs relevant, no driving
				AdjustAllDifficultyLevels(0, 0, 0, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 9) { //sign thresholding, all signs relevant, no driving
				AdjustAllDifficultyLevels(0, 0, signAdjustment, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 10) { //sign testing, go signs relevant
				AdjustAllDifficultyLevels(0, 0, 0, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 11) { //sign testing, all signs relevant, no driving
				AdjustAllDifficultyLevels(0, 0, 0, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 12) { //no signs, no driving
				AdjustAllDifficultyLevels(0, 0, 0, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 13) { //no signs, user driving
				AdjustAllDifficultyLevels(0, 0, 0, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 14) { //signs, no driving, respond to none
				AdjustAllDifficultyLevels(0, 0, 0, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 15) { //signs, user driving, respond to none
				AdjustAllDifficultyLevels(0, 0, 0, gCurrentGamePlayCount);
			}
			else if (gStoryBoardMode == 16) { //road accuracy, show signs, no driving
				AdjustAllDifficultyLevels(0, 0, 0, gCurrentGamePlayCount);
			}	//same as mode 5, but no  level changes
			else if (gStoryBoardMode == 17) { //blackout, signs on, no signs are relevant
				AdjustAllDifficultyLevels(0, 0, 0, gCurrentGamePlayCount);
			}
			
			
			// here is the storyboard level override.  
			int overalllevelchange = gCurrentDifficultyLevel;
			int roadchange = gCurrentSpeedLevel;
			int signchange = gCurrentSignLevel;
			
			if (gStoryBoardMasterLevelChange != 0) {
				overalllevelchange = ceil(gCurrentDifficultyLevel*gStoryBoardMasterLevelChange/100);
			}
			if (gStoryBoardRoadLevelChange != 0) {
				roadchange =  ceil(gCurrentSpeedLevel*gStoryBoardRoadLevelChange/100);
			}
			if (gStoryBoardSignLevelChange != 0) {
			    signchange =  ceil(gCurrentSignLevel*gStoryBoardSignLevelChange/100);
			}
			
			// Story Board Level Override. 
			//printf("Difficulty before override Overall: %d Road: %d Sign: %d\n", gCurrentDifficultyLevel, gCurrentSpeedLevel,gCurrentSignLevel);
			SetDifficultyLevel(overalllevelchange);
			SetSpeedLevel(roadchange);
			SetSignLevel(signchange);			
			gCurrentGamePlayCount++;
			
			SetStoryBoardLevel(gCurrentGamePlayCount);
			//printf("Difficulty after override Overall: %d Road: %d Sign: %d\n", gCurrentDifficultyLevel, gCurrentSpeedLevel,gCurrentSignLevel);
			
			//	if (gStoryBoardMode == 0) {  // This turns the signs off. 	//NEW DWD SHUFFLE ADDITION; MAY NEED TWEAKING
			
			// if there's any extrapolated thresholding data and we're done thresholding, overwrite the speed level with the extropolated level
			float sumgrandtotalroad;
			int finalExtrapLvlRoad;
			sumgrandtotalroad = 0;
			
			
			if (gStoryBoardMode != 0 && gPrevStoryBoardMode == 0 && extropIndexRoad > 0) // make sure road thresh is done and it crossed 80% at some point
				//	DWD SHUFFLE CHANGE, SHOULD IT BE gPrevStoryBoardMode?? - if (gStoryBoardMode != 0 && prevStoryBoardMode == 0 && extropIndexRoad > 0) // make sure road thresh is done and it crossed 80% at some point
				
				
			{
				for (int i = 0; i < extropIndexRoad; i++)
				{
					sumgrandtotalroad = sumgrandtotalroad + extrapolatedRoad[i];
					printf("Extr Road: %4.2f \n", extrapolatedRoad[i]);
				}
				finalExtrapLvlRoad = sumgrandtotalroad/extropIndexRoad;
				printf("Final Extr Road: %d \n", finalExtrapLvlRoad);
				SetSpeedLevel(finalExtrapLvlRoad);
			}
			// if there's any extrapolated thresholding data and we're done thresholding, overwrite the sign level with the extropolated level
			int finalExtrapLvlSign;
			int sumgrandtotalsign;
			sumgrandtotalsign = 0;
			if (gStoryBoardMode != 5 && gPrevStoryBoardMode == 5 && extropIndexSign > 0) // make sure road thresh is done and it crossed 80% at some point
			{
				for (int i = 0; i < extropIndexSign; i++)
				{
					sumgrandtotalsign = sumgrandtotalsign + extrapolatedSign[i];
					printf("Extr Sign: %4.2f \n", extrapolatedSign[i]);
				}
				finalExtrapLvlSign = sumgrandtotalsign/extropIndexSign;
				printf("Final Extr Sign: %d \n", finalExtrapLvlSign);
				SetSignLevel(finalExtrapLvlSign);
			}
			
			
			if (gStoryBoardMode == 0 || gStoryBoardMode == 12 || gStoryBoardMode == 13) {  // This turns the signs off. 	
				TimeBeforeFirstSign_MIN = gStoryBoardPlayLengthSeconds +10 ;
				TimeBeforeFirstSign_MAX = gStoryBoardPlayLengthSeconds +10 ;
			}	
			else { // turn the signs back on !
				TimeBeforeFirstSign_MIN = 5;
				TimeBeforeFirstSign_MAX = 5;
			}
			
			
			// 
			// 
			printf("Time next: %.2f\n", gStoryBoardPlayLengthSeconds);
			
			// NEW DWD SHUFFLE ADDITION: May need to have the 'int' like this for it to work 	printf("Time next: %d\n", (int)gStoryBoardPlayLengthSeconds);
			
			WriteDataLog();
		}
			
			
			break;
		default:
			break;
	}
}

void Road::Update(float deltaTime)
{
	mGameScreenTotalTime += deltaTime;
	
	UpdateControls(deltaTime);
	
	switch (mCurrentGameScreen)
	{
		case GAMESCREEN_START:
			UpdateStartScreen(deltaTime);
			break;
		case GAMESCREEN_PLAY:
			UpdatePlayScreen(deltaTime);
			break;
		case GAMESCREEN_FINISH:
			UpdateEndScreen(deltaTime);
			break;
		case GAMESCREEN_PAUSE:
			break;
		case GAMESCREEN_SPLASH:
			break;			
		default:
			break;
	}
	
	CheckForScreenTransition(deltaTime);
}

void Road::UpdateStartScreen(float deltaTime)
{
}

void Road::UpdateEndScreen(float deltaTime)
{
}

void Road::DoAutopilot()
{
	float newAccel = 0.0f;
	float newSteer = 0.0f;
	float paramSpeed = (mCar.mCurrentSpeed - gSpeed_MIN) / (gSpeed_MAX - gSpeed_MIN);
	float paramSteer = mCar.mCarRoadCenterOffset;
	
	if (paramSpeed < 0.45f)
		newAccel =  1.0f;
	else if (paramSpeed > 0.55f)
		newAccel = -1.0f;
	
	if (paramSteer < -0.5f)
		newSteer = -1.0f;
	else if (paramSteer > 0.5f)
		newSteer =  1.0f;
	
	mCar.mJoystickY = newAccel;
	mCar.mJoystickX = newSteer;
}

void Road::UpdatePlayScreen(float deltaTime)
{
	bool bCountdown = false;
	
	
	// Determine where the car is
	{
		mCar.mOdometer += mCar.mCurrentSpeed * deltaTime;
		int32 markerNumber = (int32)(mCar.mOdometer / mSectionDistance);
		mCurrentCarSection = markerNumber % NUM_ROAD_SECTIONS;
		mFractionalSection = (mCar.mOdometer / mSectionDistance) - (float)markerNumber;
	}
//	printf("Msection: %d     mOdometer: %.2f     totalTime: %.2f\n", mCurrentCarSection, mCar.mOdometer, mCar.mOdometer/mCar.mCurrentSpeed);

	// This will need to be changed to adjust the end point limit. 
	// HERE! HERE! HERE!
	// 
	float speedLineMult = SpeedBumpStretch_MIN;
	
	float midPoint = speedLineMult * ((0.5f * (gSpeed_MAX + gSpeed_MIN)) - mCar.mCurrentSpeed);
	
	float halfGray = (0.5f * (gSpeed_MAX - gSpeed_MIN)) - (SpeedYellowWidth_MIN + SpeedRedWidth_MIN);
	mSpeedLineOffset[0] = midPoint + speedLineMult * (halfGray + SpeedYellowWidth_MIN + SpeedRedWidth_MIN);
	mSpeedLineOffset[1] = midPoint + speedLineMult * (halfGray + SpeedYellowWidth_MIN);
	mSpeedLineOffset[2] = midPoint + speedLineMult * (halfGray);
	mSpeedLineOffset[3] = midPoint - speedLineMult * (halfGray);
	mSpeedLineOffset[4] = midPoint - speedLineMult * (halfGray + SpeedYellowWidth_MIN);
	mSpeedLineOffset[5] = midPoint - speedLineMult * (halfGray + SpeedYellowWidth_MIN + SpeedRedWidth_MIN);
	// place the speed lines
	for (int i = 0; i < 6; ++i)
	{
		mSpeedLineOdometer[i] = mCar.mOdometer + mSpeedLineOffset[i];
		int32 markerNumber = (int32)(mSpeedLineOdometer[i] / mSectionDistance);
		mCurrentSpeedLineSection[i] = markerNumber % NUM_ROAD_SECTIONS;
		mFractionalSpeedLineSection[i] = (mSpeedLineOdometer[i] / mSectionDistance) - (float)markerNumber;
	}
	
	////////////////////////////////////////
	// Are we still in the countdown?
	if (mGameScreenTotalTime < LeadTime321Go_MAX)
	{
		bCountdown = true;
	}
	else
	{
		bCountdown = false;
	}
	//Add acceleration
	// Joystick speed
	//	if (mCar.mJoystickY < previousJoyY)
	mCar.mCurrentSpeed += mCar.mJoystickY * CarAcceleration_MIN * deltaTime;
	
	// Keyboard speed
	if (mCar.mbKeyboardAccel && !mCar.mbKeyboardBrake)
	{
		mCar.mCurrentSpeed += CarAcceleration_MIN * deltaTime;
	}
	else if (mCar.mbKeyboardBrake && !mCar.mbKeyboardAccel)
	{
		mCar.mCurrentSpeed -= CarBraking_MIN * deltaTime;
	}
	if (mCar.mCurrentSpeed < gSpeed_MIN)
		mCar.mCurrentSpeed = gSpeed_MIN;
	if (mCar.mCurrentSpeed > gSpeed_MAX)
		mCar.mCurrentSpeed = gSpeed_MAX;
	
	
	if (mbAutopilot)
	{
		DoAutopilot();
	}
	
	float joystickX = mCar.mJoystickX;
	// Parabolic smoothing for the joystick
	if (1)
	{
		// "sign" is just -1 or 1
		float sign = (joystickX >= 0.0f) ? 1.0f : -1.0f;
		// Curve the value
		joystickX *= joystickX;
		// Put the sign back in
		joystickX *= sign;
	}
	
	// Joystick steering
	mCar.mCarRoadCenterOffset -= joystickX * CarSteering_MIN * deltaTime;
	
	// Keyboard steering
	if (mCar.mbKeyboardLeft && !mCar.mbKeyboardRight)
	{
		mCar.mCarRoadCenterOffset += CarSteering_MIN * deltaTime;
	}
	else if (mCar.mbKeyboardRight && !mCar.mbKeyboardLeft)
	{
		mCar.mCarRoadCenterOffset -= CarSteering_MIN * deltaTime;
	}
	
	// printf("Keyboard Left/Right: %4.2f Speed: %4.2f\n",mCar.mCarRoadCenterOffset, mCar.mCurrentSpeed);
	
	if (bCountdown)
	{
		mCar.mJoystickX = 0.0f;
		mCar.mJoystickY = 0.0f;
		mRoadSign.mTimer = RANDOM_IN_RANGE(TimeBeforeFirstSign_MIN, TimeBeforeFirstSign_MAX);
		mRoadSign.mbVisible = false;
		mRoadSign.mbNewlyVisible = false;
		mCar.mCarRoadCenterOffset = 0.0f;
		mCar.mCurrentSpeed = (gSpeed_MIN + gSpeed_MAX) * 0.5f;
		mbTurning = false;
		mbOnHill = false;
		mCurrentTurnTimer = RANDOM_IN_RANGE(TimeBetweenCurves_MIN, TimeBetweenCurves_MAX);
		mCurrentHillTimer = RANDOM_IN_RANGE(TimeBetweenHills_MIN, TimeBetweenHills_MAX);
//		mCurrentTurnStrength = 0.0f;
//		mCurrentHillStrength = 0.0f;
		
		gCurrentSignPressDecision = PRESS_NOT_READY;
		mbFixation = 0;
	}
	
	
	
	//////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////
	int newSign = SIGN_NONE;
	bool randtrigger;
	if (Behav0EEG1fMRI2_MIN == 2)
		randtrigger = false;
	else
		randtrigger = true;  //a 'true' here = randomize the pieces 
	GetCurrentTrack(deltaTime, randtrigger, &mCurrentTurnStrength, &mCurrentHillStrength, &newSign);// 
	
	/*  // all this is no longer needed, now that we shuffle. Keep it just in case.	
	 // Handle road turns
	 mCurrentTurnTimer -= deltaTime;
	 if (mCurrentTurnTimer <= 0.0f)
	 {
	 if (mbTurning)
	 {
	 // stop turning and go straight for a while
	 mCurrentTurnTimer = RANDOM_IN_RANGE(TimeBetweenCurves_MIN, TimeBetweenCurves_MAX);
	 mCurrentTurnStrength = 0.0f;
	 mbTurning = false;
	 }
	 else
	 {
	 // start turning
	 mCurrentTurnTimer = RANDOM_IN_RANGE(CurveDuration_MIN, CurveDuration_MAX);
	 mCurrentTurnStrength = RANDOM_IN_RANGE(CurveSharpness_MIN, CurveSharpness_MAX);
	 if (RANDOM0TO1 < 0.5f)
	 mCurrentTurnStrength = -mCurrentTurnStrength;
	 mbTurning = true;
	 }
	 }
	 
	 
	 // Handle road hills
	 mCurrentHillTimer -= deltaTime;
	 if (mCurrentHillTimer <= 0.0f)
	 {
	 if (mbOnHill)
	 {
	 // stop hills and go straight for a while
	 mCurrentHillTimer = RANDOM_IN_RANGE(TimeBetweenHills_MIN, TimeBetweenHills_MAX);
	 mCurrentHillStrength = 0.0f;
	 mbOnHill = false;
	 }
	 else
	 {
	 // start hills
	 mCurrentHillTimer = RANDOM_IN_RANGE(HillDuration_MIN, HillDuration_MAX);
	 mCurrentHillStrength = RANDOM_IN_RANGE(HillSteepness_MIN, HillSteepness_MAX);
	 if (RANDOM0TO1 < 0.5f)
	 mCurrentHillStrength = -mCurrentHillStrength;
	 mbOnHill = true;
	 }
	 }
	 */
	
	//ej curve testing
	//mCurrentTurnTimer = 1.0f;
	//mCurrentTurnStrength = CurveSharpness_MIN;
	//mbTurning = true;
	//mCurrentHillTimer = HillDuration_MAX;
	//mCurrentHillStrength = -HillSteepness_MAX;
	//mbOnHill = true;
	//mCurrentTurnStrength = 0.001f * mCar.mCarRoadCenterOffset;
	
	
	// Build the new road sections
	int32 sectionToRebuild = (mCurrentCarSection + mLookAheadSections) % NUM_ROAD_SECTIONS;
	int32 thisSection = mNewRebuiltSection;
	while (thisSection != sectionToRebuild)
	{//oba
		int32 prevSection = thisSection;
		thisSection = (thisSection + 1) % NUM_ROAD_SECTIONS;
		RoadSection *pPrevSec = &(mSections[prevSection]);
		RoadSection *pThisSec = &(mSections[thisSection]);
		pThisSec->mMatrix = pPrevSec->mMatrix;
		pThisSec->mMatrix.t = pPrevSec->mMatrix.t + (pPrevSec->mMatrix.z * this->mSectionDistance);
		// Turns and hills!
		{
			pThisSec->mTurn = mCurrentTurnStrength;
			// Set the turn
			pThisSec->mMatrix.z = pThisSec->mMatrix.z
			+ (pThisSec->mMatrix.x * mCurrentTurnStrength);
			
			float riseRate = HillChangeRate_MAX;
			pThisSec->mRise = pPrevSec->mRise;
			
			if (pThisSec->mRise < mCurrentHillStrength)
			{
				pThisSec->mRise += riseRate;
				if (pThisSec->mRise > mCurrentHillStrength)
				{
					pThisSec->mRise = mCurrentHillStrength;
				}
			}
			else if (pThisSec->mRise > mCurrentHillStrength)
			{
				pThisSec->mRise -= riseRate;
				if (pThisSec->mRise < mCurrentHillStrength)
				{
					pThisSec->mRise = mCurrentHillStrength;
				}
			}
			
			pThisSec->mMatrix.z.y = pThisSec->mRise;
			
			Vec3Cross(pThisSec->mMatrix.x, pThisSec->mMatrix.y, pThisSec->mMatrix.z);
			Vec3Normalize(pThisSec->mMatrix.x, pThisSec->mMatrix.x);
			Vec3Normalize(pThisSec->mMatrix.y, pThisSec->mMatrix.y);
			Vec3Normalize(pThisSec->mMatrix.z, pThisSec->mMatrix.z);
		}
		
		mNewRebuiltSection = thisSection;
	}
	
	
	// Handle roadsigns
	mRoadSign.mbNewlyVisible = false;
	
	if (mRoadSign.mTimer > 0.0f)
	{
		mRoadSign.mTimer -= deltaTime;
	}
	
	if (mRoadSign.mTimer <= 0.0f)
	{
		if (mRoadSign.mbVisible)
		{
			// Turn off the sign
			mRoadSign.mTimer = RANDOM_IN_RANGE(TimeBetweenSigns_MIN, TimeBetweenSigns_MAX);
			mRoadSign.mbVisible = false;
		}
	}
	
	if (newSign != SIGN_NONE)
	{
		{
			// turn on the sign
			// mRoadSign.mBlinkerCountdown = 1;
			mRoadSign.mTimer = RANDOM_IN_RANGE(SignDuration_MIN, SignDuration_MAX);
			mRoadSign.mbVisible = true;
			mRoadSign.mbNewlyVisible = true;
			//place the sign
			float signAheadDistance = RANDOM_IN_RANGE(SignAheadTime_MIN, SignAheadTime_MAX) * mCar.mCurrentSpeed;
			int32 sectionsAhead = (signAheadDistance / mSectionDistance);
			if (sectionsAhead > mLookAheadSections - 1)
			{
				sectionsAhead = mLookAheadSections - 1;
			}
			int32 sectionToPlaceSign = (mCurrentCarSection + sectionsAhead) % NUM_ROAD_SECTIONS;
			RoadSection *pThisSec = &(mSections[sectionToPlaceSign]);
			mRoadSign.mMatrix = pThisSec->mMatrix;
			
			mRoadSign.mCurrentSignTexture = newSign;
			
			/*			// and pick the sign
			 if (RANDOM_IN_RANGE(0.0f, 1.0f) <= RelevantSignProbability_MIN)
			 {				// this is the sign we told them to watch for
			 mRoadSign.mCurrentSignTexture = SIGN_GO;
			 }
			 else
			 {
			 int randomSign = (int)RANDOM_IN_RANGE(1.0f, ((float)NUM_DIFFERENT_ROADSIGNS) - 0.1f);
			 
			 if (randomSign < 1)
			 randomSign = 1;
			 else if (randomSign > (NUM_DIFFERENT_ROADSIGNS - 1))
			 randomSign = (NUM_DIFFERENT_ROADSIGNS - 1);
			 mRoadSign.mCurrentSignTexture = randomSign;
			 }
			 */
		}
	}
	
	
	RoadSection *pSec1 = &(mSections[mCurrentCarSection]);
	
	// This is totall a number. 
	//printf("Road center offset: %4.5f\n",mCar.mCarRoadCenterOffset);
	
	
	// Make turns affect it
	float maxCarSideTravel = mRoadWidth * 0.5f + mYellowWidth + mRedWidth - (mCar.mWidth * 0.5f);
	
	mCar.mCarRoadCenterOffset += pSec1->mTurn * mCar.mCurrentSpeed * -CarTurnSlip_MIN * deltaTime;
	
	// Use this only for PERFECT autopilot. It will be smooth and fake, like a game show host.
	if (mbAutopilot)  // steering
	{
		mCar.mCarRoadCenterOffset = 0.0f;
	}
	
	if (mCar.mCarRoadCenterOffset < -maxCarSideTravel)
		mCar.mCarRoadCenterOffset = -maxCarSideTravel;
	else if (mCar.mCarRoadCenterOffset > maxCarSideTravel)
		mCar.mCarRoadCenterOffset = maxCarSideTravel;
	
	// Can't modify mCarRoadCenterOffset after this because the value is used here to construct the car matrix
	
	// Put the car on the track
	RoadSection *pSec2 = &(mSections[(mCurrentCarSection + 1) % NUM_ROAD_SECTIONS]);
	mCar.mMatrix.t = (pSec2->mMatrix.t * mFractionalSection) + (pSec1->mMatrix.t * (1.0f - mFractionalSection));
	mCar.mMatrix.t = mCar.mMatrix.t + (pSec1->mMatrix.x * mCar.mCarRoadCenterOffset);
	mCar.mMatrix.t = mCar.mMatrix.t + (pSec1->mMatrix.y * 0.5f);
	// Turn it to face
	mCar.mMatrix.z = (pSec2->mMatrix.z * mFractionalSection) + (pSec1->mMatrix.z * (1.0f - mFractionalSection));
	// 
	mCar.mMatrix.y = (pSec2->mMatrix.y * mFractionalSection) + (pSec1->mMatrix.y * (1.0f - mFractionalSection));
	
	Vec3Cross(mCar.mMatrix.x, mCar.mMatrix.y, mCar.mMatrix.z);
	Vec3Normalize(mCar.mMatrix.x, mCar.mMatrix.x);
	Vec3Normalize(mCar.mMatrix.y, mCar.mMatrix.y);
	Vec3Normalize(mCar.mMatrix.z, mCar.mMatrix.z);
	
	
	
	
	
	// Make hills affect it
	mCar.mCurrentSpeed -= pSec1->mRise * HillAffectsCarSpeed_MIN * (CarTurnSlip_MIN)/4 * deltaTime;
	if (mCar.mCurrentSpeed < gSpeed_MIN)
		mCar.mCurrentSpeed = gSpeed_MIN;
	else if (mCar.mCurrentSpeed > gSpeed_MAX)
		mCar.mCurrentSpeed = gSpeed_MAX;
	
	// Use this only for PERFECT autopilot. It will be smooth and fake, like a game show host.
	if (mbAutopilot)  // apeed
	{
		mCar.mCurrentSpeed = 0.5f * (gSpeed_MIN + gSpeed_MAX);
	}
	
	// Place the speed lines
	for (int i = 0; i < 6; ++i)
	{
		Mat44MakeIdentity(mSpeedLineMat[i]);
		RoadSection *pSec1b = &(mSections[mCurrentSpeedLineSection[i]]);
		RoadSection *pSec2b = &(mSections[(mCurrentSpeedLineSection[i] + 1) % NUM_ROAD_SECTIONS]);
		mSpeedLineMat[i].t = (pSec2b->mMatrix.t * mFractionalSpeedLineSection[i]) + (pSec1b->mMatrix.t * (1.0f - mFractionalSpeedLineSection[i]));
		mSpeedLineMat[i].t = mSpeedLineMat[i].t + (pSec1b->mMatrix.y * 0.5f);
		// Turn it to face
		mSpeedLineMat[i].z = (pSec2b->mMatrix.z * mFractionalSpeedLineSection[i]) + (pSec1b->mMatrix.z * (1.0f - mFractionalSpeedLineSection[i]));
		Vec3Cross(mSpeedLineMat[i].x, mSpeedLineMat[i].y, mSpeedLineMat[i].z);
		Vec3Normalize(mSpeedLineMat[i].x, mSpeedLineMat[i].x);
		Vec3Normalize(mSpeedLineMat[i].y, mSpeedLineMat[i].y);
		Vec3Normalize(mSpeedLineMat[i].z, mSpeedLineMat[i].z);
	}
	
	///////////////////////////////////////////////////////////
	// Steering power adjustment is here
	///////////////////////////////////////////////////////////
	// Now make absolutely sure that the car's controls are strong enough to stay on the road!
	{
		float powerFactor = PowerTurnAndAccel_MIN;	// Adjust the steering here! This should be > 1
		
		float slipMag = sqrt(mCar.mCurrentSpeed) * CurveSharpness_MAX * CarTurnSlip_MIN;
		CarSteering_MIN = slipMag * powerFactor;
		//		CarSteering_MAX = slipMag * powerFactor;
	}
	///////////////////////////////////////////////////////////
	// Speed power adjustment is here
	///////////////////////////////////////////////////////////
	// Now make absolutely sure that the car's controls are strong enough to stay on the road!
	{
		float powerFactor = PowerTurnAndAccel_MAX;	// Adjust the steering here! This should be > 1
		float speedMag = (HillSteepness_MAX) * CarTurnSlip_MIN;     //make it constant regardless of level
		//		float speedMag = mCar.mCurrentSpeed * (HillSteepness_MAX)/10 * CarTurnSlip_MIN;    //Joaq made this 
		//float speedMag = (HillSteepness_MAX) * HillAffectsCarSpeed_MIN * CarTurnSlip_MIN;    
		
		CarAcceleration_MIN = speedMag * powerFactor;
		//		CarAcceleration_MAX = speedMag * powerFactor;
		CarBraking_MIN = speedMag * powerFactor;
		//		CarBraking_MAX = speedMag * powerFactor;
		
	}
	
	// Determine red/yellow
	float yellowBounds = mRoadWidth * 0.5f - (mCar.mWidth * 0.5f);
	float redBounds = mRoadWidth * 0.5f + mYellowWidth - (mCar.mWidth * 0.5f);
	
	bool bLeftRed = false;
	bool bRightRed = false;
	bool bLeftYellow = false;
	bool bRightYellow = false;
	bool bFastRed = false;
	bool bSlowRed = false;
	bool bFastYellow = false;
	bool bSlowYellow = false;
	
	// This is to make it so that any part of the car touching yellow/red counts.
	float halfCarWidth = 1.0f;
	redBounds -= halfCarWidth;
	yellowBounds -= halfCarWidth;
	
	if (mCar.mCarRoadCenterOffset < -redBounds) {
		bLeftRed = true;
	}
	else if (mCar.mCarRoadCenterOffset < -yellowBounds) {
		bLeftYellow = true;
	}	
	else if (mCar.mCarRoadCenterOffset > redBounds) {
		bRightRed = true;
	}
	else if (mCar.mCarRoadCenterOffset > yellowBounds) {
		bRightYellow = true;
	}
	
	// This is to make it so that any part of the car outside of the speedometer counts. 
	// 
	if (mCar.mCurrentSpeed > (gSpeed_MAX - SpeedRedWidth_MIN)-halfCarWidth) {
		bFastRed = true;
	}
	else if (mCar.mCurrentSpeed > (gSpeed_MAX - (SpeedYellowWidth_MIN + SpeedRedWidth_MIN) - halfCarWidth)) {
		bFastYellow = true;
	}	
	else if (mCar.mCurrentSpeed < (gSpeed_MIN + SpeedRedWidth_MIN)-halfCarWidth) {
		bSlowRed = true;
	}
	else if (mCar.mCurrentSpeed < (gSpeed_MIN + (SpeedYellowWidth_MIN + SpeedRedWidth_MIN)+halfCarWidth)) {
		bSlowYellow = true;
	}
	
	// end speed bound update. 
	
	
	// Place the camera
	extern Vec3 gCameraPos;
	extern Vec3 gCameraLookAt;
	// Old Camera View from over the top. 
	//	gCameraPos = mCar.mMatrix.t - (mCar.mMatrix.z * 8.0f) + (mCar.mMatrix.y * 4.0f);
	//	gCameraLookAt = gCameraPos + (mCar.mMatrix.z * 8.0f);
	
	gCameraPos = mCar.mMatrix.t - (mCar.mMatrix.z * 15.0f);//current camera
	gCameraPos.y += 8.0f;//current camera
    gCameraLookAt = gCameraPos + (mCar.mMatrix.z * 4.0f);	//current camera
	
	
	// Level it out, so we can tell when we're on a hill.
	float levelOutFactor = 0.0001f; // 1.0 is "keep the camera level" but the car might go offscreen.//current camera
	// 0.0 is "watch the car" but hills won't feel hilly.
	gCameraLookAt.y = (gCameraLookAt.y * (1.0f - (levelOutFactor))) + (gCameraPos.y * (levelOutFactor));//current camera
	
	// Crazy camera!
	//gCameraPos.x += 5.0f;
	
	// Bounce/Bump/Shake the car if it's off the course
	gBumpLevel = 0.0f;
	if (bLeftRed || bRightRed
		|| bFastRed || bSlowRed
		)
	{
		gBumpLevel = RANDOM_IN_RANGE(-RedShake_MIN, RedShake_MIN);
	}
	else if (bLeftYellow || bRightYellow
			 || bFastYellow || bSlowYellow
			 )
	{
		gBumpLevel = RANDOM_IN_RANGE(-YellowShake_MIN, YellowShake_MIN);
	}
	
	
	//////////////////////////////////////////////////
	// Rock the car back and forth!
	
	//if (mRoadSign.mbVisible)//Omar put this in to change the rocking
	if (1)//Road signs will
	{
		bool bSquash = false;		// This decides whether we roll it ot 'squash' it
		float rockSmooth = 0.3f;	// how quickly does it rock? (0 - 1)
		float rockMagnitudeLR = RockingSideAndForward_MIN; // how far does it rock left/right?
		float rockMagnitudeFB = RockingSideAndForward_MAX; // how far does it rock front-back?
		
		static float smoothLeanLeftRight = 0.0f;
		static float smoothLeanFrontBack = 0.0f;
		float leftRight = -mCar.mJoystickX;
		float frontBack = mCar.mJoystickY;
		
		if (mCar.mbKeyboardLeft)
			leftRight = 1.0f;
		else if (mCar.mbKeyboardRight)
			leftRight = -1.0f;
		
		if (mCar.mbKeyboardAccel)
			frontBack = 1.0f;
		else if (mCar.mbKeyboardBrake)
			frontBack = -1.0f;
		
		// Smooth it out
		smoothLeanLeftRight = (smoothLeanLeftRight * (1.0f - rockSmooth)) + (leftRight * rockSmooth);
		smoothLeanFrontBack = (smoothLeanFrontBack * (1.0f - rockSmooth)) + (frontBack * rockSmooth);
		
		//Vec3Set(mCar.mMatrix.y, 0, 1, 0);
		Vec3Cross(mCar.mMatrix.x, mCar.mMatrix.y, mCar.mMatrix.z);
		Vec3Cross(mCar.mMatrix.y, mCar.mMatrix.z, mCar.mMatrix.x);
		
		mCar.mMatrix.y =  mCar.mMatrix.y + mCar.mMatrix.x * rockMagnitudeLR * smoothLeanLeftRight;
		mCar.mMatrix.y =  mCar.mMatrix.y + mCar.mMatrix.z * rockMagnitudeFB * smoothLeanFrontBack;
		
		if (!bSquash)
		{
			Vec3Cross(mCar.mMatrix.x, mCar.mMatrix.y, mCar.mMatrix.z);
			Vec3Cross(mCar.mMatrix.z, mCar.mMatrix.x, mCar.mMatrix.y);
		}
		Vec3Normalize(mCar.mMatrix.x, mCar.mMatrix.x);
		Vec3Normalize(mCar.mMatrix.y, mCar.mMatrix.y);
		Vec3Normalize(mCar.mMatrix.z, mCar.mMatrix.z);
	}
	
	
	
	
	//////////////////////////////////////////////
	// Log the data
	static float totalTime = 0.0f;   // This needs to be replaced with a real timestamper.
	totalTime += deltaTime;
	DataLogEntry entry;
	float lookAheadTime = (NUM_ROAD_SECTIONS / ((gSpeed_MIN + gSpeed_MAX) * 0.5f)) * 0.75f;
	entry.mTimeStampSECONDS = totalTime + lookAheadTime;
//	printf("totalTime: %.2f   deltaTime = %.2f\n", totalTime, deltaTime);
	entry.mRoadCenterOffset = mCar.mCarRoadCenterOffset;
	entry.mCurrentSpeed = mCar.mCurrentSpeed - ((gSpeed_MAX + gSpeed_MIN)/2);//Omar fix to fix make posY variable be consistent
	
	entry.mJoystickX = mCar.mJoystickX;
	entry.mJoystickY = mCar.mJoystickY;
	entry.mVisibleSign = -1;
	entry.mFlags = 0;

	entry.mLeftRed=0;
	entry.mLeftYellow=0;	
	entry.mRightRed=0;
	entry.mRightYellow=0;	
	entry.mJoystickButton=0;	
	entry.mCurrentTurn = mbCurrentTurnLog; 
	entry.mCurrentHill = mbCurrentHillLog; 
	

	
//#ifdef _WIN32  // #if WINDOWS
//	OpenPortTalk();
//	outportb(0x378, 0x0000);
//#endif
	
	if (mCar.mbButtonDown)
	{
		entry.mFlags |= DATALOG_FLAG_JOYSTICKBUTTON;
		entry.mJoystickButton = 1;
//#ifdef _WIN32  // #if WINDOWS		
//	    outportb(0x378, 0x000a);
//#endif
	}
	// This is a placeholder yellow/red detection. It'll be hooked into the user feedback system
	if (bLeftRed) {
		entry.mFlags |= DATALOG_FLAG_LEFTRED;
		entry.mLeftRed = 1;
//#ifdef _WIN32  // #if WINDOWS	
//	    outportb(0x378, 0x0001);
//#endif
	}
	else if (bLeftYellow) {
		entry.mFlags |= DATALOG_FLAG_LEFTYELLOW;
		entry.mLeftYellow=1;
//#ifdef _WIN32  // #if WINDOWS		
//	    outportb(0x378, 0x0002);
//#endif
	}	
	else if (bRightRed) {
		entry.mFlags |= DATALOG_FLAG_RIGHTRED;
		entry.mRightRed =1;
//#ifdef _WIN32  // #if WINDOWS	
//	    outportb(0x378, 0x0004);
//#endif		
	}
	else if (bRightYellow) {
		entry.mFlags |= DATALOG_FLAG_RIGHTYELLOW;
	    entry.mRightYellow = 1;
//#ifdef _WIN32  // #if WINDOWS			
//	    outportb(0x378, 0x0003);
//#endif			
	}
	
	if (bFastRed) {
		entry.mFlags |= DATALOG_FLAG_FASTRED;
		entry.mFastRed = 1;
//#ifdef _WIN32  // #if WINDOWS	
//	    outportb(0x378, 0x0001);
//#endif
	}
	else if (bFastYellow) {
		entry.mFlags |= DATALOG_FLAG_FASTYELLOW;
		entry.mFastYellow=1;
//#ifdef _WIN32  // #if WINDOWS		
//	    outportb(0x378, 0x0002);
//#endif
	}	
	else if (bSlowRed) {
		entry.mFlags |= DATALOG_FLAG_SLOWRED;
		entry.mSlowRed =1;
//#ifdef _WIN32  // #if WINDOWS	
//	    outportb(0x378, 0x0004);
//#endif		
	}
	else if (bSlowYellow) {
		entry.mFlags |= DATALOG_FLAG_SLOWYELLOW;
	    entry.mSlowYellow = 1;
//#ifdef _WIN32  // #if WINDOWS			
//	    outportb(0x378, 0x0003);
//#endif			
	}
	
	
	
	// Calculate Sign Accuracy Information. 
	SignAccuracy(entry);
	
	// TTL Pulse output. 
	if (mRoadSign.mbVisible) 
	{
		entry.mVisibleSign = mRoadSign.mCurrentSignTexture;
/* #ifdef _WIN32  // #if WINDOWS	
		if (entry.mVisibleSign == 0) {
			outportb(0x378, 0x003C);	// 60  // this is the relevant sign? 
		}
		else if (entry.mVisibleSign == 1) {
			outportb(0x378, 0x003D);	// 61		
		}
		else if (entry.mVisibleSign == 2) {
			outportb(0x378, 0x003E);	// 62				
		}
		else if (entry.mVisibleSign == 3) {
			outportb(0x378, 0x003F);	// 63				
		}
		else if (entry.mVisibleSign == 4) {
			outportb(0x378, 0x0040);	// 64			
		}
		else if (entry.mVisibleSign == 5) {
			outportb(0x378, 0x0041);	// 65			
		}		
		else if (entry.mVisibleSign == 6) {
			outportb(0x378, 0x0042);	// 66			
		}		
		else if (entry.mVisibleSign == 7) {
			outportb(0x378, 0x0043);	// 67			
		}		
		else if (entry.mVisibleSign == 8) {
			outportb(0x378, 0x0044);	// 68			
		}				
		else if (entry.mVisibleSign == 9) {
			outportb(0x378, 0x0045);	// 69			
		}		
		else if (entry.mVisibleSign == 10) {
			outportb(0x378, 0x0046);	// 70			
		}
		else if (entry.mVisibleSign == 11) {
			outportb(0x378, 0x0047);	// 71			
		}		
		else if (entry.mVisibleSign == 12) {
			outportb(0x378, 0x0048);	// 72			
		}				
		else if (entry.mVisibleSign == 13) {
			outportb(0x378, 0x0049);	// 73			
		}		
		else if (entry.mVisibleSign == 14) {
			outportb(0x378, 0x004A);	// 74			
		}				
		else if (entry.mVisibleSign == 15) {
			outportb(0x378, 0x004B);	// 75			
		}		
		else if (entry.mVisibleSign == 16) {
			outportb(0x378, 0x004C);	// 76			
		}			
		
		
#endif	*/		
	}
	
	
	// Keep track of our overall percentages
	gTotalSamples++;
	if (entry.mFlags & DATALOG_FLAG_ALLRED)
	{
		gTotalRedSamples++;
	}
	else if (entry.mFlags & DATALOG_FLAG_ALLYELLOW)
	{
		gTotalYellowSamples++;
	}
	else
	{
		gTotalGraySamples++;
	}
	
	
	// Keep track of separate Speed and Road Accuracy Percentages, instead of combined percentage. 
	//	Speed
	if (entry.mFlags & (DATALOG_FLAG_FASTRED | DATALOG_FLAG_SLOWRED))
	{
		gRedSpeedSamples++; 	
	}
	else if (entry.mFlags & (DATALOG_FLAG_SLOWYELLOW | DATALOG_FLAG_FASTYELLOW))
	{
		gYellowSpeedSamples++; 
	}
	else
	{
		gGraySpeedSamples++; 
	}
	
	// Road Accuracy
	if (entry.mFlags & (DATALOG_FLAG_RIGHTRED | DATALOG_FLAG_LEFTRED))
	{
		gRedAccuracySamples++; 	
	}
	else if (entry.mFlags & (DATALOG_FLAG_RIGHTYELLOW | DATALOG_FLAG_LEFTYELLOW))
	{
		gYellowAccuracySamples++; 
	}
	else
	{
		gGrayAccuracySamples++; 
	}		
	//
	
	SetDataLogEntry(&entry);
#ifdef _WIN32  // #if WINDOWS			
	ClosePortTalk();
#endif
}

static float floorAlt = -10.0f;

// Sign Accuracy
void Road::SignAccuracy(DataLogEntry entry)
{
	bool goSign = (mRoadSign.mCurrentSignTexture == SIGN_GO);
//	printf("goSign %d\n", goSign);
//	float elapsed = entry.mTimeStampSECONDS - gSignStartTime;	

	// Sign Turned On
	if (mRoadSign.mbNewlyVisible)
	{
		alreadyHitNontargetSign = false;
		alreadyMisshitNontargetSign = false;
		alreadyHitTargetSign = false;
		alreadyMisshitTargetSign = false;
		alreadyIgnoredNontargetSign = false;
		alreadyIgnoredTargetSign = false;
		gSignsStarted = true;
		entry.mVisibleSign = mRoadSign.mCurrentSignTexture;
		gSignStartTime = mGameScreenTotalTime;
		printf("entry.mTimeStampSECONDS: %.2f\n", entry.mTimeStampSECONDS);
		gCurrentSignPressDecision = PRESS_READY;
		
		// diode markers
		if (goSign) {
			//blinker for diode - relevant sign measurement. 
			mRoadSign.mBlinkerCountdown2 = 1; 
		} 
		else if (mRoadSign.mCurrentSignTexture == 1 || mRoadSign.mCurrentSignTexture == 2 || mRoadSign.mCurrentSignTexture == 1 || mRoadSign.mCurrentSignTexture == 15)  {
			// blinker for diode - irrelevant signs
			mRoadSign.mBlinkerCountdown3 = 1; 

//			// Test to have two diodes at the top on at once. 
//			mRoadSign.mBlinkerCountdown = 1; 		
//			mRoadSign.mBlinkerCountdown2 = 1; 

		}
		else {
			//blinker for diode -- all others
			mRoadSign.mBlinkerCountdown4 = 1;
		}
		
		// Count only the Go Signs
		if (goSign && (gStoryBoardMode <= 5 || gStoryBoardMode >= 10)) {
			gGoSignsTotal++;
			printf("Sign %d (GO)...\n", gAllSignsTotal);
		}
		else if (!goSign && (gStoryBoardMode <= 5 || gStoryBoardMode >= 10))
		{
			printf("Sign %d (no-go)...\n", gAllSignsTotal);		
		}
/*		else if (gStoryBoardMode >= 6  && gStoryBoardMode <=9)
		{
			printf("Sign %d...\n", gAllSignsTotal);
			gGoSignsTotal++;
		}
*/		// Also count all the signs for the false positive calculation. 
		gAllSignsTotal++;
	}
	
		if (goSign){
			lastSignGo = 1;
		}
		else if ((gStoryBoardMode <= 5 || gStoryBoardMode >= 10))
		{
			lastSignGo = 0;
		}
		float elapsed = mGameScreenTotalTime - gSignStartTime;
		
		if (elapsed > gReactionTime && goSign && ((gStoryBoardMode >= 2 && gStoryBoardMode <= 5) || (gStoryBoardMode >= 10 && gStoryBoardMode <= 13) || (gStoryBoardMode == 16)) 
			&& !alreadyIgnoredTargetSign && !alreadyHitTargetSign && !alreadyMisshitTargetSign && gSignsStarted) 
		{
			gCurrentSignPressDecision = DID_NOT_PRESS;
			printf("  Wrong: Missed the target sign\n");
			printf("ELAPSED TIME: %.2f  REACTION TIME: %.2f\n", elapsed, gReactionTime);
			mbFixation = 2;
			gSignsIncorrect++;
			alreadyIgnoredTargetSign = true;
		}
		if (elapsed > gReactionTime && !goSign && ((gStoryBoardMode >= 2 && gStoryBoardMode <= 5) || (gStoryBoardMode >= 10 && gStoryBoardMode <= 13) || (gStoryBoardMode == 16)) 
			&& !alreadyIgnoredNontargetSign && !alreadyHitNontargetSign && !alreadyMisshitNontargetSign && gSignsStarted) 
		{
			gCurrentSignPressDecision = DID_NOT_PRESS;
			printf("  Wrong: Missed the non-target sign\n");
			printf("ELAPSED TIME: %.2f  REACTION TIME: %.2f\n", elapsed, gReactionTime);
			mbFixation = 2;
			gSignsIncorrect++;
			alreadyIgnoredNontargetSign = true;
		}
	/*	if (elapsed < 1.0 && !goSign && (gStoryBoardMode <= 5 || (gStoryBoardMode >= 10 && gStoryBoardMode <= 13) || (gStoryBoardMode == 16)) 
			&& !alreadyIgnoredNontargetSign && !alreadyHitNontargetSign && !alreadyMisshitNontargetSign && gSignsStarted)
		{
			mbFixation = 2;
			printf(" Incorrect: Didn't get the non-target sign\n");
			printf(" Time is currently: %.2f\n", elapsed);
			alreadyIgnoredNontargetSign = true;
			gSignsIncorrect++;
		}
*/
		
		if (mCar.mbJoystickRightPressed && !alreadyIgnoredTargetSign && !alreadyIgnoredNontargetSign && elapsed < 1.0 && gSignsStarted) 
		{
			gCurrentSignPressDecision = DID_PRESS;
			if (((gStoryBoardMode >= 2 && gStoryBoardMode <= 5) || (gStoryBoardMode >= 10 && gStoryBoardMode <= 13) || (gStoryBoardMode == 16)))
			{
				if (elapsed < gReactionTime && !alreadyHitTargetSign && !alreadyMisshitTargetSign && goSign)
				{
					printf("  Correct: Pressed on a target sign\n");
					alreadyHitTargetSign = true;
					//blinker for diode
					mRoadSign.mBlinkerCountdown = 1;
					gGoSignsTriggered++;
					mbFixation = 1;
					gSignLatency =  (mGameScreenTotalTime - gSignStartTime);
					// Now we need to add all the sign latencies together so we can get the average to display at the end
					gTotalSignLatency += gSignLatency;
					gSignsCorrect++;
				}
				else if (!alreadyMisshitNontargetSign && !goSign){
					printf("  Wrong: Pressed target button on a non-target sign\n");
					gSignFalsePositives++;
					mbFixation = 2;
					alreadyMisshitNontargetSign = true;
					mRoadSign.mBlinkerCountdown = 1; // any irrelevant sign incorrectly hit
					gSignsIncorrect++;
					if (alreadyHitNontargetSign)
						gSignsCorrect--;
				}
			}
/*			if (!goSign && ((gStoryBoardMode >= 2 && gStoryBoardMode <= 5) || (gStoryBoardMode >= 10 && gStoryBoardMode <= 13) || (gStoryBoardMode == 16)) && !alreadyMisshitNontargetSign)
			{
				printf("  Incorrect: Hit target button on non-target sign\n");
				gSignFalsePositives++;
				mbFixation = 2;
				printf(" Time is currently: %.2f/n", elapsed);
				mRoadSign.mBlinkerCountdown = 2; // any irrelevant sign incorrectly hit
				alreadyMisshitNontargetSign = true;
				gSignsIncorrect++;
				if (alreadyHitNontargetSign)
					gSignsCorrect--;			}
*/		}

		if (mCar.mbJoystickLeftPressed && !alreadyIgnoredTargetSign && !alreadyIgnoredNontargetSign && elapsed < 1.0 && gSignsStarted) 
		{
			gCurrentSignPressDecision = DID_PRESS;
			if (((gStoryBoardMode >= 2 && gStoryBoardMode <= 5) || (gStoryBoardMode >= 10 && gStoryBoardMode <= 13) || (gStoryBoardMode == 16)))
			{
				if (!alreadyMisshitTargetSign && goSign)
				{
					printf("  Incorrect: Pressed non-target button on target sign\n");
					alreadyMisshitTargetSign = true;
					//blinker for diode
					mRoadSign.mBlinkerCountdown = 1;
					mbFixation = 2;
					gSignLatency =  (mGameScreenTotalTime - gSignStartTime);
					// Now we need to add all the sign latencies together so we can get the average to display at the end
					gTotalSignLatency += gSignLatency;
					gSignsIncorrect++;
					gSignFalsePositives++;
					if (alreadyHitTargetSign)
						gSignsCorrect--;
				}
				else if (elapsed < gReactionTime && !alreadyHitNontargetSign && !goSign && !alreadyMisshitNontargetSign){
					printf("  Correct: Pressed non-target button on a non-target sign\n");
					gGoSignsTriggered++;
					mbFixation = 1;
					alreadyHitNontargetSign = true;
					mRoadSign.mBlinkerCountdown = 1; // any irrelevant sign incorrectly hit
					gSignsCorrect++;
				}
			}
/*			if (!goSign && ((gStoryBoardMode >= 2 && gStoryBoardMode <= 5) || (gStoryBoardMode >= 10 && gStoryBoardMode <= 13) || (gStoryBoardMode == 16)) && !alreadyMisshitNontargetSign)
			{
				printf("  Incorrect: Hit target button on non-target sign\n");
				gSignFalsePositives++;
				mbFixation = 2;
				printf(" Time is currently: %.2f\n", elapsed);
				mRoadSign.mBlinkerCountdown = 2; // any irrelevant sign incorrectly hit
				alreadyMisshitNontargetSign = true;
				gSignsIncorrect++;
			}
*/		}	
}   //  end sign accuracy.

// Now we need to find the level change and reset the 
// gSignFalsePositives, gGoSignsTriggered, gGoSignsTotal, gAllSignsTotal


void Road::Draw()
{
	// Adjust the floor height
	const float maxUnder = 15.0f;
	const float minUnder = 6.0f;
	if (floorAlt < mCar.mMatrix.t.y - maxUnder)
		floorAlt = mCar.mMatrix.t.y - maxUnder;
	if (floorAlt > mCar.mMatrix.t.y - minUnder)
		floorAlt = mCar.mMatrix.t.y - minUnder;
	
	DrawBackdrop();
	
	Vec3	v1, v2, n1, n2;
	Vec4	c1, c2;
	Vec2	t1, t2;
	
	Vec4Set(c1, 1.0f, 1.0f, 1.0f, 1.0f);
	//Vec4Set(c2, 0.8f, 0.8f, 0.8f, 1.0f);// Joaq changed this to make it so the car didn't 'light up' when a sign flashes; 
	Vec4Set(c2, 1.0f, 1.0f, 1.0f, 1.0f);
	
	Vec3Set(n1, 0.0f, 1.0f, 0.0f);
	Vec3Set(n2, 0.0f, 1.0f, 0.0f);
	Vec2Set(t1, 0.0f, 0.0f);
	Vec2Set(t2, 1.0f, 0.0f);
	
	if (blackout)
	{
		Vec4Set(c1, 0.0f, 0.0f, 0.0f, 1.0f);
		Vec4Set(c2, 0.0f, 0.0f, 0.0f, 1.0f);
	}
	// Draw the road CENTER
	glBindTexture(GL_TEXTURE_2D, mTexture_Road);
	
	
	
	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i < NUM_ROAD_SECTIONS; ++i)
	{
		int32 secNum = (i + mNewRebuiltSection + 1) % NUM_ROAD_SECTIONS;
		RoadSection *pSec = &(mSections[secNum]);
		v1 = pSec->mMatrix.t - (pSec->mMatrix.x * mRoadWidth * 0.5);
		v2 = pSec->mMatrix.t + (pSec->mMatrix.x * mRoadWidth * 0.5);
		t1.y = pSec->mRoadTextureOffset;
		t2.y = pSec->mRoadTextureOffset;
		
		if (pSec->mTurn > 0.0f)
		{
			Vec4Set(c1, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		else
		{
			Vec4Set(c1, 0.5f, 0.5f, 0.5f, 1.0f);
		}
		if (blackout)
		{
			Vec4Set(c1, 0.0f, 0.0f, 0.0f, 1.0f);
			Vec4Set(c2, 0.0f, 0.0f, 0.0f, 1.0f);
		}
		
		glColor4fv(c1.f);
		glTexCoord2fv(t1.f);
		glNormal3fv(n1.f);
		glVertex3fv(v1.f);
		
		glColor4fv(c2.f);
		glTexCoord2fv(t2.f);
		glNormal3fv(n2.f);
		glVertex3fv(v2.f);
	}
	glEnd();
	
	float curbOffset1[4] = {-(mRoadWidth * 0.5f + mYellowWidth),
		-(mRoadWidth * 0.5f + mYellowWidth + mRedWidth),
		(mRoadWidth * 0.5f),
		(mRoadWidth * 0.5f + mYellowWidth)};
	float curbOffset2[4] = {-(mRoadWidth * 0.5f),
		-(mRoadWidth * 0.5f + mYellowWidth),
		(mRoadWidth * 0.5f + mYellowWidth),
		(mRoadWidth * 0.5f + mYellowWidth + mRedWidth)};
	
	// Draw the road CURBS
	glBindTexture(GL_TEXTURE_2D, mTexture_Curb);
	for (int stripe = 0; stripe < 4; ++stripe)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (int i = 0; i < NUM_ROAD_SECTIONS; ++i)
		{
			int32 secNum = (i + mNewRebuiltSection + 1) % NUM_ROAD_SECTIONS;
			RoadSection *pSec = &(mSections[secNum]);
			v1 = pSec->mMatrix.t + (pSec->mMatrix.x * curbOffset1[stripe]);
			v2 = pSec->mMatrix.t + (pSec->mMatrix.x * curbOffset2[stripe]);
			t1.y = pSec->mRoadTextureOffset;
			t2.y = pSec->mRoadTextureOffset;
			
			if (stripe == 0 || stripe == 2)
			{
				Vec4Set(c1, 1.0f, 1.0f, 0.0f, 1.0f);	// yellow
			}
			else
			{
				Vec4Set(c1, 1.0f, 0.0f, 0.0f, 1.0f);	// red
			}
			if (blackout)
			{
				Vec4Set(c1, 0.0f, 0.0f, 0.0f, 1.0f);
				Vec4Set(c2, 0.0f, 0.0f, 0.0f, 1.0f);
			}
			
			glColor4fv(c1.f);
			glTexCoord2fv(t1.f);
			glNormal3fv(n1.f);
			glVertex3fv(v1.f);
			
			glColor4fv(c2.f);
			glTexCoord2fv(t2.f);
			glNormal3fv(n2.f);
			glVertex3fv(v2.f);
		}
		glEnd();
	}
	///////////////////////////////////////
	// Draw the grassy hill
	// Draw the road CURBS
	float hillScale = 0.3f;	// 0.0 - 1.0 This controls how much of the hills are drawn.
	// Too much, and the car drives through the ground. Too little, and it looks bad.
	glBindTexture(GL_TEXTURE_2D, mTexture_Grass);
	glDisable(GL_LIGHTING);
	
	
	
	glEnable(GL_BLEND); // EJ Fix 3.1.1.2 to Fade out the edges
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // EJ Fix 3.1.1.2 to Fade out the edges
    glDisable(GL_ALPHA_TEST); // EJ Fix 3.1.1.2 to Fade out the edges
	
	if (1)// EJ Fix 3.1.1.3 CutOff Edges so that it looks like there are no sides to hills going up or down, the car is on a 'magic carpet'
	{
		
		glBegin(GL_TRIANGLE_STRIP);
		for (int i = 0; i < NUM_ROAD_SECTIONS; ++i)
		{
			int32 secNum = (i + mNewRebuiltSection + 1) % NUM_ROAD_SECTIONS;
			RoadSection *pSec = &(mSections[secNum]);
			v1 = pSec->mMatrix.t - (pSec->mMatrix.x * (mRoadWidth * 0.5f + mYellowWidth + mRedWidth));
			v2 = pSec->mMatrix.t - (pSec->mMatrix.x * 100.0f);
			v2.y = floorAlt - 22;
			// Now rein in the hill so it looks the same but doesn't go as far down.
			// This should help keep the road from going through the hill
			v2 = (v2 * hillScale) + (v1 * (1.0f - hillScale));
			
			t1.y = pSec->mRoadTextureOffset;
			t2.y = pSec->mRoadTextureOffset;
			
			float tsec = (float)i / (float)(NUM_ROAD_SECTIONS - 1);
			Vec4Set(c1, 1.0f, 1.0f, 1.0f, 1.0f);
			//Vec4Set(c2, tsec, tsec, tsec, 0.0f);	// red EJ trans road
			Vec4Set(c2, tsec, tsec, tsec, 0.80f);	// red Joaq 80% transparent road
			
			if (blackout)
			{
				Vec4Set(c1, 0.0f, 0.0f, 0.0f, 1.0f);
				Vec4Set(c2, 0.0f, 0.0f, 0.0f, 1.0f);
			}
			
			glColor4fv(c1.f);
			glTexCoord2fv(t1.f);
			glNormal3fv(n1.f);
			glVertex3fv(v1.f);
			
			glColor4fv(c2.f);
			glTexCoord2fv(t2.f);
			glNormal3fv(n2.f);
			glVertex3fv(v2.f);
		}
		glEnd();
		glBegin(GL_TRIANGLE_STRIP);
		for (int i = 0; i < NUM_ROAD_SECTIONS; ++i)
		{
			int32 secNum = (i + mNewRebuiltSection + 1) % NUM_ROAD_SECTIONS;
			RoadSection *pSec = &(mSections[secNum]);
			v1 = pSec->mMatrix.t + (pSec->mMatrix.x * (mRoadWidth * 0.5f + mYellowWidth + mRedWidth));
			v2 = pSec->mMatrix.t + (pSec->mMatrix.x * 100.0f);
			v2.y = floorAlt - 22;
			// Now rein in the hill so it looks the same but doesn't go as far down.
			// This should help keep the road from going through the hill
			v2 = (v2 * hillScale) + (v1 * (1.0f - hillScale));
			
			t1.y = pSec->mRoadTextureOffset;
			t2.y = pSec->mRoadTextureOffset;
			
			float tsec = (float)i / (float)(NUM_ROAD_SECTIONS - 1);
			Vec4Set(c1, 1.0f, 1.0f, 1.0f, 1.0f);
			//Vec4Set(c2, tsec, tsec, tsec, 0.0f);	// red EJ trans road
			Vec4Set(c2, tsec, tsec, tsec, 0.80f);	// red Joaq 80% transparent road
			
			if (blackout)
			{
				Vec4Set(c1, 0.0f, 0.0f, 0.0f, 1.0f);
				Vec4Set(c2, 0.0f, 0.0f, 0.0f, 1.0f);
			}
			
			glColor4fv(c1.f);
			glTexCoord2fv(t1.f);
			glNormal3fv(n1.f);
			glVertex3fv(v1.f);
			
			glColor4fv(c2.f);
			glTexCoord2fv(t2.f);
			glNormal3fv(n2.f);
			glVertex3fv(v2.f);
		}
		glEnd();
	}
	
	glDisable(GL_BLEND);    // ej trans roads
	
	glEnable(GL_LIGHTING);
	
	DrawRoadSign();
	DrawCar();
	DrawCarSpeedBars();
}

void Road::DrawBackdrop()
{
	Vec3	v1, v2, v3, v4, n1;
	Vec4	c1;
	Vec2	t1, t2, t3, t4;
	float scale = 1000.0f;
	
	glDepthMask(false);
	
	Vec4Set(c1, 1.0f, 1.0f, 1.0f, 1.0f);
	Vec3Set(n1, 0.0f, 1.0f, 0.0f);
	Vec2Set(t1, 0.0f, 0.0f);
	Vec2Set(t2, 0.0f, 0.0f);
	Vec2Set(t3, 0.0f, 0.0f);
	Vec2Set(t4, 0.0f, 0.0f);
	if (blackout)
	{
		Vec4Set(c1, 0.0f, 0.0f, 0.0f, 1.0f);
	}
	
	v1 = mCar.mMatrix.t + ( mCar.mMatrix.x * ( 0.5f * scale)) + (mCar.mMatrix.z * (-0.5f * scale));
	v2 = mCar.mMatrix.t + ( mCar.mMatrix.x * (-0.5f * scale)) + (mCar.mMatrix.z * (-0.5f * scale));
	v3 = mCar.mMatrix.t + ( mCar.mMatrix.x * (-0.5f * scale)) + (mCar.mMatrix.z * ( 0.5f * scale));
	v4 = mCar.mMatrix.t + ( mCar.mMatrix.x * ( 0.5f * scale)) + (mCar.mMatrix.z * ( 0.5f * scale));
	
	// Level it out in case the car is on a hill
	v1.y = v2.y = v3.y = v4.y = mCar.mMatrix.t.y;
	
	float dropUnder = 5.0f;
	v1.y -= dropUnder;
	v2.y -= dropUnder;
	v3.y -= dropUnder;
	v4.y -= dropUnder;
	v1.y = v2.y = v3.y = v4.y = floorAlt - 20.0f;
	
	glBindTexture(GL_TEXTURE_2D, mTexture_Grass);
	
	glBegin(GL_QUADS);
	{
		glColor4fv(c1.f);
		glTexCoord2fv(t1.f);
		glNormal3fv(n1.f);
		glVertex3fv(v1.f);
		
		glTexCoord2fv(t2.f);
		glVertex3fv(v2.f);
		
		glTexCoord2fv(t3.f);
		glVertex3fv(v3.f);
		
		glTexCoord2fv(t4.f);
		glVertex3fv(v4.f);
	}
	glEnd();
	glDepthMask(true);
}

void Road::DrawCar()	// move this to the car class when it exists
{
	bool bumpCar = false;	// Set this true to make the CAR shake when it goes off-road
	bool bumpCross = true;	// Set this true to make the CROSS shake when it goes off-road
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, mCar.mTexture);
	glEnable(GL_LIGHTING);
	
	if (blackout)
	{
		glDisable(GL_LIGHTING);
		glColor4f(0,0,0,1);
	}
	glMatrixMode(GL_MODELVIEW_MATRIX);
	glPushMatrix();
	Mat44 mat;
	mat.x = mCar.mMatrix.x * 1.0;
	mat.y = mCar.mMatrix.y;
	mat.z = mCar.mMatrix.z * 1.0;
	mat.t = mCar.mMatrix.t;
	mat.t.y += 1.0f;
	
	if (bumpCar)
		mat.t.y += gBumpLevel;	// bump when we go off-road
	
	glMultMatrixf((float*)&(mat));
	glmDraw(mCar.mpModel, GLM_SMOOTH|GLM_TEXTURE);
	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
	
	
	//Fixation Cross
	Vec3	v1, v2, v3, v4;
	//	float scale = 5.0;
	float elapsed = mGameScreenTotalTime - gSignStartTime;
//	printf("elapsed: %.2f     mGameScreenTime: %.2f     gSignStartTime: %.2f \n", elapsed, mGameScreenTotalTime, gSignStartTime);
	if (elapsed < 1.0)
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	else if (mbFixation == 2 && elapsed <= 1.25)
		glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	else if (mbFixation == 1 && elapsed <= 1.25)
		glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
	else if (elapsed > 1.25)
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	else if (gStoryBoardMode == 17)
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	Mat44 gmathorz;
	gmathorz = mCar.mMatrix;
	Vec3 up;
	Vec3Set(up, 0, 1, 0);
	Vec3 left = mCar.mMatrix.x;
	left.y = 0;
	Vec3Normalize(left, left);
	
	if (bumpCross)
		gmathorz.t.y += gBumpLevel;	// Shaking when we go off-road
	
	v1 = gmathorz.t + ( left * 0.8) + ( up * 3.9) + (mCar.mMatrix.z * 0);
	v2 = gmathorz.t + ( left * -0.8) + ( up * 3.9) + (mCar.mMatrix.z * 0);
	v3 = gmathorz.t + ( left * -0.8) + ( up * 4.1) + (mCar.mMatrix.z * 0);
	v4 = gmathorz.t + ( left * 0.8) + ( up * 4.1) + (mCar.mMatrix.z * 0);
	
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
	
	glBegin(GL_QUADS);
	{
		glVertex3fv(v1.f);
		glVertex3fv(v2.f);
		glVertex3fv(v3.f);
		glVertex3fv(v4.f);
	}
	
	Mat44 gmatvert;
	gmatvert = mCar.mMatrix;
	
	if (bumpCross)
		gmatvert.t.y += gBumpLevel;	// Shaking when we go off-road
	
	v1 = gmatvert.t + ( left * 0.1) + ( up * 3.2) + (mCar.mMatrix.z * 0);
	v2 = gmatvert.t + ( left * -0.1) + ( up * 3.2) + (mCar.mMatrix.z * 0);
	v3 = gmatvert.t + ( left * -0.1) + ( up * 4.8) + (mCar.mMatrix.z * 0);
	v4 = gmatvert.t + ( left * 0.1) + ( up * 4.8) + (mCar.mMatrix.z * 0);
	
	glBegin(GL_QUADS);
	{
		glVertex3fv(v1.f);
		glVertex3fv(v2.f);
		glVertex3fv(v3.f);
		glVertex3fv(v4.f);
	}
	glEnd();
	glColor4f(1, 1, 1, 1);
	glEnable(GL_TEXTURE_2D);
	
}


void Road::DrawCarSpeedBars()	// move this to the car class when it exists
{
	if (mbSpeedometerOnRoad)
	{
		Vec3	n1;
		Vec4	cRed, cYellow, cMiddle;
		Vec2	t1, t2, t3, t4;
		float scale = mCar.mWidth;
		
		glEnable(GL_BLEND); //EJ transparent speed barriers
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //EJ transparent speed barriers
        glDisable(GL_ALPHA_TEST); //EJ transparent speed barriers
        float alpha = 0.4f; //EJ transparent speed barriers; adjust this to make the barrier more/less transparent
		
		//DWD SHUFFLE CHANGE - EJ has this different, may relate to cross smashing top of car    //float scale = mCar.mWidth;
		
		Vec4Set(cRed, 1.0f, 0.0f, 0.0f, alpha);
        Vec4Set(cYellow, 1.0f, 1.0f, 0.0f, alpha);
        Vec3Set(n1, 0.0f, 1.0f, 0.0f);
		//Vec4Set(cRed, 1.0f, 0.0f, 0.0f, 1.0f); old speed barriers before EJ solution to fix
		//Vec4Set(cYellow, 1.0f, 1.0f, 0.0f, 1.0f); old speed barriers before EJ solution to fix
		//Vec3Set(n1, 0.0f, 1.0f, 0.0f); old speed barriers before EJ solution to fix
		Vec2Set(t1, 0.0f, 0.0f);
		Vec2Set(t2, 1.0f, 0.0f);
		Vec2Set(t3, 1.0f, 1.0f);
		Vec2Set(t4, 0.0f, 1.0f);
		
		if (blackout)
		{
			Vec4Set(cRed, 0.0f, 0.0f, 0.0f, 1.0f);
			Vec4Set(cYellow, 0.0f, 0.0f, 0.0f, 1.0f);
			Vec4Set(cMiddle, 0.0f, 0.0f, 0.0f, 1.0f);
		}
		
		Mat44 gmat[6];
		Vec3 vLeft[6], vRight[6];
		
		for (int i = 0; i < 6; ++i)
		{
			gmat[i] = mSpeedLineMat[i];
			vLeft[i] = gmat[i].t - ( gmat[i].x * RoadGrayWidth_MIN * 0.5f);
			vRight[i] = gmat[i].t + ( gmat[i].x * RoadGrayWidth_MIN * 0.5f);
		}
		
		glDisable(GL_LIGHTING);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GEQUAL, 0.1f);
		glCullFace(GL_BACK);
		
		glBindTexture(GL_TEXTURE_2D, mTexture_SpeedBump);
		
		glBegin(GL_QUADS);
		{
			int seg;
			Vec3 diff;
			//			float safetyDist = 20.0f * 20.0f;
			// Forward red line
			seg = 0;
			diff = vLeft[0] - vLeft[seg+1];
			if (Vec3Dot(diff, gmat[seg].z) > 0)
			{
				glColor4fv(cRed.f);
				glTexCoord2fv(t1.f);
				glNormal3fv(n1.f);
				glVertex3fv(vLeft[seg].f);			
				glTexCoord2fv(t2.f);
				glVertex3fv(vRight[seg].f);
				glTexCoord2fv(t3.f);
				glVertex3fv(vRight[seg+1].f);
				glTexCoord2fv(t4.f);
				glVertex3fv(vLeft[seg+1].f);
			}
			
			// Forward yellow line
			seg = 1;
			diff = vLeft[0] - vLeft[seg+1];
			if (Vec3Dot(diff, gmat[seg].z) > 0)
			{
				glColor4fv(cYellow.f);
				glTexCoord2fv(t1.f);
				glNormal3fv(n1.f);
				glVertex3fv(vLeft[seg].f);			
				glTexCoord2fv(t2.f);
				glVertex3fv(vRight[seg].f);
				glTexCoord2fv(t3.f);
				glVertex3fv(vRight[seg+1].f);
				glTexCoord2fv(t4.f);
				glVertex3fv(vLeft[seg+1].f);
			}
			
			// Backward yellow line
			seg = 3;
			diff = vLeft[0] - vLeft[seg+1];
			if (Vec3Dot(diff, gmat[seg].z) > 0)
			{
				glColor4fv(cYellow.f);
				glTexCoord2fv(t1.f);
				glNormal3fv(n1.f);
				glVertex3fv(vLeft[seg].f);			
				glTexCoord2fv(t2.f);
				glVertex3fv(vRight[seg].f);
				glTexCoord2fv(t3.f);
				glVertex3fv(vRight[seg+1].f);
				glTexCoord2fv(t4.f);
				glVertex3fv(vLeft[seg+1].f);
			}
			
			// Backward red line
			seg = 4;
			diff = vLeft[0] - vLeft[seg+1];
			if (Vec3Dot(diff, gmat[seg].z) > 0)
			{
				glColor4fv(cRed.f);
				glTexCoord2fv(t1.f);
				glNormal3fv(n1.f);
				glVertex3fv(vLeft[seg].f);			
				glTexCoord2fv(t2.f);
				glVertex3fv(vRight[seg].f);
				glTexCoord2fv(t3.f);
				glVertex3fv(vRight[seg+1].f);
				glTexCoord2fv(t4.f);
				glVertex3fv(vLeft[seg+1].f);
			}
		}
		glEnd();
		
		
		// Draw the speedbump middle
		//		glEnable(GL_BLEND);
		//float middleOpacity = alpha;//New EJ transparent speed barrriers
		float middleOpacity = .750f;// Old EJ transparent speed barriers
		Vec4Set(cMiddle, 1.0f, 1.0f, 1.0f, middleOpacity); // This is the color
		
		
		if (blackout)
		{
			Vec4Set(cRed, 0.0f, 0.0f, 0.0f, 1.0f);
			Vec4Set(cYellow, 0.0f, 0.0f, 0.0f, 1.0f);
			Vec4Set(cMiddle, 0.0f, 0.0f, 0.0f, 1.0f);
		}
		glBindTexture(GL_TEXTURE_2D, mTexture_SpeedBumpMiddle);
		glBegin(GL_QUADS);
		{
			int seg;
			Vec3 diff;
			// Forward red line
			seg = 2;
			diff = vLeft[0] - vLeft[seg+1];
			if (Vec3Dot(diff, gmat[seg].z) > 0)
			{
				glColor4fv(cMiddle.f);
				glTexCoord2fv(t1.f);
				glNormal3fv(n1.f);
				glVertex3fv(vLeft[seg].f);			
				glTexCoord2fv(t2.f);
				glVertex3fv(vRight[seg].f);
				glTexCoord2fv(t3.f);
				glVertex3fv(vRight[seg+1].f);
				glTexCoord2fv(t4.f);
				glVertex3fv(vLeft[seg+1].f);
			}
		}
		glEnd();
		glDisable(GL_BLEND);// EJ Transparency Speed Barriers Blend : was previously commented here
		
		
		
		glColor4f(1, 1, 1, 1);
		glCullFace(GL_NONE);
	}
	else
	{
		Vec3	v1, v2, v3, v4, n1;
		Vec4	cRed, cYellow, cNeedle;
		Vec2	t1, t2, t3, t4;
		float hscale = mCar.mWidth * 0.7;
		float vscale = mCar.mWidth * 0.4;
		float alt1 = 0.7f;
		float alt2 = 0.2f;
		
		Vec4Set(cRed, 1.0f, 0.0f, 0.0f, 1.0f);
		Vec4Set(cYellow, 1.0f, 1.0f, 0.0f, 1.0f);
		Vec4Set(cNeedle, 0.0f, 0.0f, 0.0f, 1.0f);
		Vec3Set(n1, 0.0f, 1.0f, 0.0f);
		Vec2Set(t1, 0.0f, 0.0f);
		Vec2Set(t2, 1.0f, 0.0f);
		Vec2Set(t3, 1.0f, 1.0f);
		Vec2Set(t4, 0.0f, 1.0f);
		
		if (blackout)
		{
			Vec4Set(cRed, 0.0f, 0.0f, 0.0f, 1.0f);
			Vec4Set(cYellow, 0.0f, 0.0f, 0.0f, 1.0f);
			Vec4Set(cNeedle, 0.0f, 0.0f, 0.0f, 1.0f);
		}
		v1 = mCar.mMatrix.t + ( mCar.mMatrix.x * ( 0.5f * hscale)) + (mCar.mMatrix.z * (0.0f * vscale)) + (mCar.mMatrix.y * alt1);
		v2 = mCar.mMatrix.t + ( mCar.mMatrix.x * (-0.5f * hscale)) + (mCar.mMatrix.z * (0.0f * vscale)) + (mCar.mMatrix.y * alt1);
		v3 = mCar.mMatrix.t + ( mCar.mMatrix.x * (-0.5f * hscale)) + (mCar.mMatrix.z * (-1.0f * vscale)) + (mCar.mMatrix.y * alt2);
		v4 = mCar.mMatrix.t + ( mCar.mMatrix.x * ( 0.5f * hscale)) + (mCar.mMatrix.z * (-1.0f * vscale)) + (mCar.mMatrix.y * alt2);
		
		v1.y += vscale;
		v2.y += vscale;
		
		float thick = 0.05f;
		float speedT = (mCar.mCurrentSpeed - gSpeed_MIN) / (gSpeed_MAX - gSpeed_MIN);
		float st0 = speedT + thick;
		float st1 = speedT - thick;
		Vec3 v1Needle = (v2 * st0) + (v3 * (1.0f - st0));
		Vec3 v2Needle = (v1 * st0) + (v4 * (1.0f - st0));
		Vec3 v3Needle = (v1 * st1) + (v4 * (1.0f - st1));
		Vec3 v4Needle = (v2 * st1) + (v3 * (1.0f - st1));
		
		float trt0 = ((gSpeed_MAX - SpeedRedWidth_MAX) - gSpeed_MIN) / (gSpeed_MAX - gSpeed_MIN);
		float trt1 = 1.0f;
		Vec3 v1TRed = (v2 * trt0) + (v3 * (1.0f - trt0));
		Vec3 v2TRed = (v1 * trt0) + (v4 * (1.0f - trt0));
		Vec3 v3TRed = (v1 * trt1) + (v4 * (1.0f - trt1));
		Vec3 v4TRed = (v2 * trt1) + (v3 * (1.0f - trt1));
		trt0 = 1.0f - trt0;
		trt1 = 1.0f - trt1;
		Vec3 v1BRed = (v2 * trt0) + (v3 * (1.0f - trt0));
		Vec3 v2BRed = (v1 * trt0) + (v4 * (1.0f - trt0));
		Vec3 v3BRed = (v1 * trt1) + (v4 * (1.0f - trt1));
		Vec3 v4BRed = (v2 * trt1) + (v3 * (1.0f - trt1));
		trt0 = ((gSpeed_MAX - (SpeedYellowWidth_MAX+SpeedRedWidth_MAX)) - gSpeed_MIN) / (gSpeed_MAX - gSpeed_MIN);
		trt1 = ((gSpeed_MAX - SpeedRedWidth_MAX) - gSpeed_MIN) / (gSpeed_MAX - gSpeed_MIN);
		Vec3 v1TYel = (v2 * trt0) + (v3 * (1.0f - trt0));
		Vec3 v2TYel = (v1 * trt0) + (v4 * (1.0f - trt0));
		Vec3 v3TYel = (v1 * trt1) + (v4 * (1.0f - trt1));
		Vec3 v4TYel = (v2 * trt1) + (v3 * (1.0f - trt1));
		trt0 = 1.0f - trt0;
		trt1 = 1.0f - trt1;
		Vec3 v1BYel = (v2 * trt0) + (v3 * (1.0f - trt0));
		Vec3 v2BYel = (v1 * trt0) + (v4 * (1.0f - trt0));
		Vec3 v3BYel = (v1 * trt1) + (v4 * (1.0f - trt1));
		Vec3 v4BYel = (v2 * trt1) + (v3 * (1.0f - trt1));
		
		glDisable(GL_LIGHTING);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GEQUAL, 0.5f);
		//	glBlendFunc(GL_ONE, GL_ONE);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		
		glBindTexture(GL_TEXTURE_2D, mTexture_SpeedBar);
		
		glBegin(GL_QUADS);
		{
			glColor4fv(cRed.f);
			glTexCoord2fv(t1.f);
			glNormal3fv(n1.f);
			glVertex3fv(v1TRed.f);		
			glTexCoord2fv(t2.f);
			glVertex3fv(v2TRed.f);
			glTexCoord2fv(t3.f);
			glVertex3fv(v3TRed.f);		
			glTexCoord2fv(t4.f);
			glVertex3fv(v4TRed.f);
			
			glColor4fv(cRed.f);
			glTexCoord2fv(t1.f);
			glNormal3fv(n1.f);
			glVertex3fv(v1BRed.f);		
			glTexCoord2fv(t2.f);
			glVertex3fv(v2BRed.f);
			glTexCoord2fv(t3.f);
			glVertex3fv(v3BRed.f);		
			glTexCoord2fv(t4.f);
			glVertex3fv(v4BRed.f);
			
			glColor4fv(cYellow.f);
			glTexCoord2fv(t1.f);
			glNormal3fv(n1.f);
			glVertex3fv(v1TYel.f);		
			glTexCoord2fv(t2.f);
			glVertex3fv(v2TYel.f);
			glTexCoord2fv(t3.f);
			glVertex3fv(v3TYel.f);		
			glTexCoord2fv(t4.f);
			glVertex3fv(v4TYel.f);
			
			glColor4fv(cYellow.f);
			glTexCoord2fv(t1.f);
			glNormal3fv(n1.f);
			glVertex3fv(v1BYel.f);		
			glTexCoord2fv(t2.f);
			glVertex3fv(v2BYel.f);
			glTexCoord2fv(t3.f);
			glVertex3fv(v3BYel.f);		
			glTexCoord2fv(t4.f);
			glVertex3fv(v4BYel.f);
			
			glColor4fv(cNeedle.f);
			glTexCoord2fv(t1.f);
			glNormal3fv(n1.f);
			glVertex3fv(v1Needle.f);		
			glTexCoord2fv(t2.f);
			glVertex3fv(v2Needle.f);
			glTexCoord2fv(t3.f);
			glVertex3fv(v3Needle.f);		
			glTexCoord2fv(t4.f);
			glVertex3fv(v4Needle.f);
		}
		glEnd();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}
}


void Road::DrawRoadSign()	// move this to the car class when it exists
{
	if (! mRoadSign.mbVisible)
	{
		return;
	}
	
	glDisable(GL_LIGHTING);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GEQUAL, 0.5f);
	
	Vec3	v1, v2, v3, v4, n1;
	Vec4	c1;
	Vec2	t1, t2, t3, t4;
	
	
	float hScale = 0.0f;	// For static signs, This is the WIDTH of the roadsign
	float vScale = 0.0f;	// For static signs, This is the HEIGHT
	
	//float hScale = 16.0f;	// This is the WIDTH of the roadsign
	//float vScale = 10.0f;	// This is the HEIGHT
	
	Vec4Set(c1, 1.0f, 1.0f, 1.0f, 1.0f);
	Vec3Set(n1, 0.0f, 1.0f, 0.0f);
	Vec2Set(t1, 0.0f, 0.0f);
	Vec2Set(t2, 1.0f, 0.0f);
	Vec2Set(t3, 1.0f, 1.0f);
	Vec2Set(t4, 0.0f, 1.0f);
	
	//	v1 = mRoadSign.mMatrix.t + ( mRoadSign.mMatrix.x * ( 0.5f * hScale)) + (mRoadSign.mMatrix.y * ( 0.0f * vScale));
	//	v2 = mRoadSign.mMatrix.t + ( mRoadSign.mMatrix.x * (-0.5f * hScale)) + (mRoadSign.mMatrix.y * ( 0.0f * vScale));
	//	v3 = mRoadSign.mMatrix.t + ( mRoadSign.mMatrix.x * (-0.5f * hScale)) + (mRoadSign.mMatrix.y * ( 1.0f * vScale));
	//	v4 = mRoadSign.mMatrix.t + ( mRoadSign.mMatrix.x * ( 0.5f * hScale)) + (mRoadSign.mMatrix.y * ( 1.0f * vScale));
	
	v1 = mCar.mMatrix.t + ( mCar.mMatrix.x * ( 0.5f * hScale)) + (mCar.mMatrix.y * ( 0.0f * vScale));
	v2 = mCar.mMatrix.t + ( mCar.mMatrix.x * (-0.5f * hScale)) + (mCar.mMatrix.y * ( 0.0f * vScale));
	v3 = mCar.mMatrix.t + ( mCar.mMatrix.x * (-0.5f * hScale)) + (mCar.mMatrix.y * ( 1.0f * vScale));
	v4 = mCar.mMatrix.t + ( mCar.mMatrix.x * ( 0.5f * hScale)) + (mCar.mMatrix.y * ( 1.0f * vScale));
	
	glBindTexture(GL_TEXTURE_2D, mRoadSign.mTextures[mRoadSign.mCurrentSignTexture]);
	
	glBegin(GL_QUADS);
	{
		glColor4fv(c1.f);
		glTexCoord2fv(t1.f);
		glNormal3fv(n1.f);
		glVertex3fv(v1.f);
		
		glTexCoord2fv(t2.f);
		glVertex3fv(v2.f);
		
		glTexCoord2fv(t3.f);
		glVertex3fv(v3.f);
		
		glTexCoord2fv(t4.f);
		glVertex3fv(v4.f);
	}
	glEnd();
}

