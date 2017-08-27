
#ifndef BRROAD_H
#define BRROAD_H

#include "cbMath.h"
#include "glm.h"
#include "DataLog.h"

class Car
	{
	public:
		Car();
		~Car();
		float mCurrentSpeed;
		float mOdometer;
		Mat44 mMatrix;
		uint32 mTexture;
		float mCarRoadCenterOffset;	// how far off center are we?
		float mWidth;
		GLMmodel *mpModel;
	
		float mJoystickX;
		float mJoystickY;
		bool mbJoystickButton;
		bool mbJoystickRight;
		bool mbJoystickLeft;
		bool mbJoystickRight_Prev;
		bool mbJoystickLeft_Prev;
		bool mbJoystickRightPressed;
		bool mbJoystickLeftPressed;
		bool mbKeyboardAccel;
		bool mbKeyboardBrake;
		bool mbKeyboardLeft;
		bool mbKeyboardRight;
		bool mbKeyboardButton;

		bool mbKeyboardButtonPressed; //was keyboard pressed this frame?
		bool mbKeyboardButton_Prev; //was keyboard pressed in previous frame?

		bool mbButtonDown;  // the button state on this frame
		bool mbButtonDown_Prev;  // the button state on the previous frame
		bool mbButtonPressed; // was the button pressed this frame?
		bool mbButtonReleased; // was the button released this frame?
};

#define NUM_DIFFERENT_ROADSIGNS 10
#define NUM_AVERAGED_VALUES 12
#define NUM_AVERAGED_SIGN_VALUES 3

#define SIGN_GO	0	// sign 0 is the "go" sign

class RoadSign
	{
	public:
		RoadSign();
		~RoadSign();
		Mat44 mMatrix;
		uint32 mTextures[NUM_DIFFERENT_ROADSIGNS];
		int32 mCurrentSignTexture;
		float mTimer;
		bool mbVisible;
		bool mbNewlyVisible; // the sign appeared this frame
		int32 mBlinkerCountdown;
		int32 mBlinkerCountdown2;		
		int32 mBlinkerCountdown3;		
		int32 mBlinkerCountdown4;		
		uint32 mTexture_StaticSign;
	};

struct RoadSection
{
	int32 mSerialNumber;	// Zero is the first section
	float mTurn;		// How much are we turning left or right?
	float mRise;		// How much are we rising up or down?
	float mRoadTextureOffset;	// What's our TexCoord here?
	Mat44 mMatrix;
};

#define NUM_ROAD_SECTIONS	200

enum GameScreen
{
	GAMESCREEN_START,
	GAMESCREEN_PLAY,
	GAMESCREEN_FINISH,
	GAMESCREEN_TRANSITION,
	GAMESCREEN_FINAL_TRANSITION,
	GAMESCREEN_PAUSE,
	GAMESCREEN_SPLASH,
};

class Road

//#define NUM_DIFFERENT_ROADSIGNS 10//Joaq attempt to make static signs

{
public:
	Road();
	~Road();
	void Draw();
	void DrawBackdrop();
	void DrawCar();
	void DrawCarSpeedBars();
	void DrawRoadSign();
	void Update(float deltaTime);
	void UpdateControls(float deltaTime);
	void UpdateStartScreen(float deltaTime);
	void UpdatePlayScreen(float deltaTime);
	void UpdateEndScreen(float deltaTime);
	void TransitionToScreen(GameScreen toScreen);
	void TogglePause();
	void CheckForScreenTransition(float deltaTime);
	void SignAccuracy(DataLogEntry entry);
	void DoAutopilot();

	uint32 mTexture_Road;
	uint32 mTexture_Grass;
	uint32 mTexture_SpeedBar;
	uint32 mTexture_SpeedBump;
	uint32 mTexture_SpeedBumpMiddle;  // The texture for the "gray area" between the speedbumps
	uint32 mTexture_Curb;
	uint32 mTexture_StartScreen;
	uint32 mTexture_SplashScreen;	
	uint32 mTexture_FinishScreen;
	uint32 mTexture_PauseScreen;
	uint32 mBlinkerTexture;
	uint32 mBlinkerTexture2;		
	uint32 mBlinkerTexture3;
	uint32 mBlinkerTexture4;	
	uint32 mTexture_StaticSign;
	uint32 mTextures[NUM_DIFFERENT_ROADSIGNS];	
	
	
	uint32 mTexture_TransitionScreen;
	uint32 mTexture_FinalTransitionScreen;
	uint32 mTexture_TransCheck;
	uint32 mTexture_TransX;
	uint32 mTexture_TransDash;
	uint32 mTexture_Count3;
	uint32 mTexture_Count2;
	uint32 mTexture_Count1;
	uint32 mTexture_CountGo;
	uint32 mTexture_Digit[10];
	uint32 mTexture_DigitSize[10][2];

	bool mbAutopilot;			// Is autopilot engaged?
	bool mbOverrideSpeed; // Override the speed?

	Car mCar;
	RoadSign mRoadSign;
	RoadSection mSections[NUM_ROAD_SECTIONS];
	float mSectionDistance;     // How long is each tiny roadSection?
	int32 mCurrentCarSection;	// What section is the car over now?
	float mFractionalSection;	// Where are we in between sections?
	int32 mLookAheadSections;	// How many sections ahead can we see?
	int32 mNewRebuiltSection;	// Which section just got rebuilt?
	float mRoadWidth;			// The width of the "ok" gray section
	float mRedWidth;			// The width of the red section
	float mYellowWidth;			// The width of the yellow section
	float mTextureScaleAlongRoad; // How much do we stretch the texture along the road?
	float performanceHistory[NUM_AVERAGED_VALUES]; 
	int lastPositivePerf;
	int lastNegativePerf;
	int lastPositiveLvl;
	int lastNegativeLvl;
	float extrapolatedRoad[12];
	float extrapolatedSign[12];
	int extropIndexRoad;
	int extropIndexSign;
	int signThreshCount;
	int roadThreshCount;
	bool lastNeg;
	bool storedValRoad;
	bool storedValSign;

	bool mbSpeedometerOnRoad;   // Is the speedometer on the road, or on the car? Press 's' to switch
	int mbFixation;       // Is the fixation white, red or green?
	bool mbTurning;	// We're on a curve
	bool mbOnHill;  // We're on a hill
	float mCurrentTurnStrength;
	float mCurrentHillStrength;
	float mCurrentTurnTimer;
	float mCurrentHillTimer;

	float mSpeedLineOffset[6]; // this is the distance fwd/back for each section
	float mSpeedLineOdometer[6];
	int32 mCurrentSpeedLineSection[6];
	float mFractionalSpeedLineSection[6];
	Mat44 mSpeedLineMat[6];

	GameScreen mCurrentGameScreen;
	float mGameScreenTotalTime;
	float mTransitionTime;
};

#ifdef DWD_QT
unsigned int LoadTexture(const char* filename,bool compressed=true, bool makeMips=true, int *outWidth=NULL, int *outHeight=NULL);
#endif

#endif // BRROAD_H
