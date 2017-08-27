// generic GLUT main setup

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>


// 
// #define DWD_QT  // Take this out!
// 
#ifdef DWD_QT
#include <QGLWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QPixmap>
#include <QImage>
#include <QRgb>
#include "DWD_Qt/dwdwindow.h"
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

bool blackout = false;		

// #include "Texfont.h"
// #include <OGLFT.h>


#include "brTypes.h"
#include "brRoad.h"
#include "AdjustableVar.h"
#include "DataLog.h"

Road *gpRoad;

float gDesiredFrameRate = 60.0f;


uint32 gDesiredFrameMilliseconds = (uint32)(1000.0f/gDesiredFrameRate);

int32 gGLUTWindow = 0;
Vec3 gCameraPos;
Vec3 gCameraLookAt;
Vec3 gCameraUp;
float gScreenWidth = 1;
float gScreenHeight = 1;
float gbFullscreen = false;

void SetCameraPosition(void)
{
}

void gCameraReset(void)
{
	Vec3Set(gCameraPos, 0.0, 6.0, 0.0);
	Vec3Set(gCameraLookAt, 0.0, 6.0, 1.0);
	Vec3Set(gCameraUp, 0.0, 1.0, 0.0);
}

void
drawGLString(GLfloat x, GLfloat y, char const *string)
{
#ifndef DWD_QT
    glDisable(GL_TEXTURE_2D);
    glRasterPos2f(x, y);
    int len = (int) strlen(string);
    for (int i = 0; i < len; i++)
    {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, string[i]);
    }
    glEnable(GL_TEXTURE_2D);
#endif
}


////////////////////////////////////////////
// Digit-drawing code is here

float DigitDrawX = 100;
float DigitDrawY = 100;
float DigitDrawScale = 1.0f;
float DigitDrawTopColor[4] = {1, 1, 1, 1};
float DigitDrawBottomColor[4] = {1, 1, 1, 1};

void DrawOneDigit(int digit)
{
	glDisable(GL_DEPTH_TEST);
	float x1 = DigitDrawX;
	float y1 = DigitDrawY;
	float scale = DigitDrawScale / 60;
	float w = gpRoad->mTexture_DigitSize[digit][0];
	float h = gpRoad->mTexture_DigitSize[digit][1];
	float x2 = x1 + w * scale;
	float y2 = y1 + h * scale;

	DigitDrawX = x2;	// move for the next digit

	// Scale it to the stretched screen
	float xScreenScale = gScreenWidth / 640.0f;
	float yScreenScale = gScreenHeight / 480.0f;
	x1 *= xScreenScale;
	x2 *= xScreenScale;
	y1 *= yScreenScale;
	y2 *= yScreenScale;

	{
		glBindTexture(GL_TEXTURE_2D, gpRoad->mTexture_Digit[digit]);
		Vec3	v1, v2, v3, v4;
		Vec2	t1, t2, t3, t4;

		Vec2Set(t1, 0.0f, 1.0f);
		Vec2Set(t2, 1.0f, 1.0f);
		Vec2Set(t3, 1.0f, 0.0f);
		Vec2Set(t4, 0.0f, 0.0f);
		Vec3Set(v1, x1, y1, 0.0f);
		Vec3Set(v2, x2, y1, 0.0f);
		Vec3Set(v3, x2, y2, 0.0f);
		Vec3Set(v4, x1, y2, 0.0f);
		glBegin(GL_QUADS);
		{
			glColor4fv(DigitDrawTopColor);
			glTexCoord2fv(t1.f);
			glVertex3fv(v1.f);
			
			glTexCoord2fv(t2.f);
			glVertex3fv(v2.f);
			
			glColor4fv(DigitDrawBottomColor);
			glTexCoord2fv(t3.f);
			glVertex3fv(v3.f);
			
			glTexCoord2fv(t4.f);
			glVertex3fv(v4.f);
		}
		glEnd();
	}
}

void DrawDigitString(char const *str)
{
	for (int i = 0; str[i] != 0; ++i)
	{
		char ch = str[i];
		if (ch == '-') {
			DrawOneDigit(ch - '0' -2);
		}
		else if (ch >= '0' && ch <= '9')
		{
			DrawOneDigit(ch - '0');
		}

		
	}
}

void DrawNumber(int number, float x, float y, float size)
{
	char str[256];
	sprintf(str, "%d", number);

	DigitDrawX = x;
	DigitDrawY = y;
	DigitDrawScale = size;

	DrawDigitString(str);
}

void SetDrawTopColor(float r, float g, float b, float a)
{
	DigitDrawTopColor[0] = r;
	DigitDrawTopColor[1] = g;
	DigitDrawTopColor[2] = b;
	DigitDrawTopColor[3] = a;
}

void SetDrawBottomColor(float r, float g, float b, float a)
{
	DigitDrawBottomColor[0] = r;
	DigitDrawBottomColor[1] = g;
	DigitDrawBottomColor[2] = b;
	DigitDrawBottomColor[3] = a;
}

///////////////////////////////////////////////////


void SetLighting(unsigned int mode)
{
	GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
	GLfloat mat_shininess[] = {90.0};

	GLfloat position[4] = {7.0,-7.0,12.0,0.0};
	GLfloat ambient[4]  = {0.5,0.5,0.5,1.0};
	GLfloat diffuse[4]  = {1.0,1.0,1.0,1.0};
	GLfloat specular[4] = {1.0,1.0,1.0,1.0};
	
	glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
	
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);

	switch (mode) {
		case 0:
			break;
		case 1:
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
			glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_FALSE);
			break;
		case 2:
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
			glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_TRUE);
			break;
		case 3:
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
			glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_FALSE);
			break;
		case 4:
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
			glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_TRUE);
			break;
	}
	
	glLightfv(GL_LIGHT0,GL_POSITION,position);
	glLightfv(GL_LIGHT0,GL_AMBIENT,ambient);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse);
	glLightfv(GL_LIGHT0,GL_SPECULAR,specular);
	glEnable(GL_LIGHT0);
}

void DrawScreenQuad(uint32 texture, float x1, float y1, float x2, float y2, float red, float green, float blue, float alpha)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	Vec3	v1, v2, v3, v4, n1;
	Vec4	c1;
	Vec2	t1, t2, t3, t4;

	Vec4Set(c1, red, green, blue, alpha);
	Vec3Set(n1, 0.0f, 1.0f, 0.0f);
	Vec2Set(t1, 0.0f, 1.0f);
	Vec2Set(t2, 1.0f, 1.0f);
	Vec2Set(t3, 1.0f, 0.0f);
	Vec2Set(t4, 0.0f, 0.0f);
	Vec3Set(v1, x1, y1, 0.0f);
	Vec3Set(v2, x2, y1, 0.0f);
	Vec3Set(v3, x2, y2, 0.0f);
	Vec3Set(v4, x1, y2, 0.0f);
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

void DrawScreenArc(float x1, float y1, float radiusx, float radiusy, float startAngle, float endAngle, float red, float green, float blue, float alpha)
{
//	glBindTexture(GL_TEXTURE_2D, texture);
	glDisable(GL_TEXTURE_2D);
	Vec3	v1, v2, n1;
	Vec4	c1, c2;

	float tint = 0.7f;
	Vec4Set(c1, red * tint, green * tint, blue * tint, alpha);
	Vec4Set(c2, red, green, blue, alpha);
	Vec3Set(n1, 0.0f, 1.0f, 0.0f);
	Vec3Set(v1, x1, y1, 0.0f);

	glBegin(GL_TRIANGLE_FAN);

	glColor4fv(c1.f);
	glVertex3fv(v1.f);

	while (endAngle < startAngle)
	{
		endAngle += 360.0f;
	}

	bool done = false;
	float theta = startAngle;
	while (! done)
	{
		v2 = v1;
		float sval = sin(theta * M_PI / 180.0f);
		float cval = cos(theta * M_PI / 180.0f);
		v2.x += radiusx * sval;
		v2.y += radiusy * cval;
		glColor4fv(c2.f);
		glVertex3fv(v2.f);

		if (theta >= endAngle)
			done = true;
		theta += 5.0f;
		if (theta > endAngle)
			theta = endAngle;
	}
	glEnd();
	glEnable(GL_TEXTURE_2D);
}



void drawGLText (GLint window_width, GLint window_height)
{
	char outString [256] = "";
	GLint matrixMode;
	GLint vp[4];
	GLint lineSpacing = 13;
	GLint line = 0;
	GLint startOffest = 7;

	glGetIntegerv(GL_VIEWPORT, vp);
	glViewport(0, 0, window_width, window_height);
	
	glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glScalef(2.0f / window_width, -2.0f / window_height, 1.0f);
	glTranslatef(-window_width / 2.0f, -window_height / 2.0f, 0.0f);
	
	// Draw 
	glDisable(GL_LIGHTING);
	glColor3f (1.0, 1.0, 1.0);

	if (gpRoad)
	{
		// Draw the banners
		if (gpRoad->mCurrentGameScreen == GAMESCREEN_START)
		{
			// This forces exit of the game after HISTORY_MAX has been reached. 
			// printf("Game Play Count%d\n",gCurrentGamePlayCount);
			
			if (gCurrentGamePlayCount >= gMAXPLAYS) {
				if (gMAXPLAYS != 0) {
					printf("gMAXPLAYS: %d\n",gMAXPLAYS);	
				    exit(1);					
				}
			}
			
			float r = RANDOM_IN_RANGE(0.0f, 0.5f);
			float g = 1.0f;
			float b = RANDOM_IN_RANGE(0.0f, 0.5f);
			float a = 1.0f;
			DrawScreenQuad(gpRoad->mTexture_StartScreen, 0.3 * window_width, 0.3 * window_height, 0.7 * window_width, 0.7 * window_height, r, g, b, a);
		}
		else if (gpRoad->mCurrentGameScreen == GAMESCREEN_FINISH)
		{
			gfeedbackscreencodecalled = 0;
			gtransitioncodecalled = 0;
			float r = 1.0f;
			float g = RANDOM_IN_RANGE(0.0f, 0.5f);
			float b = RANDOM_IN_RANGE(0.0f, 0.5f);
			float a = 1.0f;
			DrawScreenQuad(gpRoad->mTexture_FinishScreen, 0.3 * window_width, 0.3 * window_height, 0.7 * window_width, 0.7 * window_height, r, g, b, a);
		}
		else if (gpRoad->mCurrentGameScreen == GAMESCREEN_SPLASH)
		{
			float r = 1.0f;
			float g = 1.0f;
			float b = 1.0f;
			float a = 1.0f;
			DrawScreenQuad(gpRoad->mTexture_SplashScreen, 0.0 * window_width, 0.0 * window_height, 1.0 * window_width, 1.0 * window_height, r, g, b, a);
		}		
		
		else if (gpRoad->mCurrentGameScreen == GAMESCREEN_PAUSE)
		{
			gfeedbackscreencodecalled = 0;
			gtransitioncodecalled = 0;
			float r = 1.0f;
			float g = RANDOM_IN_RANGE(0.0f, 0.5f);
			float b = RANDOM_IN_RANGE(0.0f, 0.5f);
			float a = 1.0f;
			DrawScreenQuad(gpRoad->mTexture_PauseScreen, 0.3 * window_width, 0.3 * window_height, 0.7 * window_width, 0.7 * window_height, r, g, b, a);
		}
		else if (gpRoad->mCurrentGameScreen == GAMESCREEN_FINAL_TRANSITION)
		{
			//glDisable(GL_DEPTH_TEST);
			float r = 1.0f;
			float g = 1.0f;
			float b = 1.0f;
			float a = 1.0f;
			
			// Replace Final Screen Transition if necessary. 
			  if (strncmp (goldStoryBoardTransitionScreenName,"default",2) != 0) {
				  gtransitioncodecalled++;
//				  printf("Is this getting called? %d", gtransitioncodecalled);
				  if (gtransitioncodecalled == 1) {
					  // Enter the case for the dynamically changing storyboard transition screen loaded in from the storyboard variables. 
	   				  char *gametransitiontexture = strcat(goldStoryBoardTransitionScreenName,".bmp");		
					  char pathname[200] = {0 };
					  
					 
					  strcat(pathname,"images/");
					  strcat(pathname,gametransitiontexture);
				      printf("pathname string: %s\n",pathname);
					  gpRoad->mTexture_FinalTransitionScreen = LoadTexture(pathname, false, true);
				  }
				}		
			
			 DrawScreenQuad(gpRoad->mTexture_FinalTransitionScreen, 0.0 * window_width, 0.0 * window_height, 1.0 * window_width, 1.0 * window_height, r, g, b, a);
			
			// Here are the Graphics for the final transition screens. 
			if (strncmp("DWD_UI_RoadThresholdingFinal.bmp", goldStoryBoardTransitionScreenName,30) == 0){
			
				float x = 0.35f * 800; //window_width;
				float y = 0.425f * 600; //window_height;
				float size = 38;
				// Sign Level
				DrawNumber(gCurrentSpeedLevel, x, y, size); 
				
							
			}
			else if (strncmp("DWD_UI_SignThresholdingFinal.bmp", goldStoryBoardTransitionScreenName,50) == 0) {
				// printf("Sign Thresholding Final Transition Screen\n");
				float x = 0.35f * 800; //window_width;
				float y = 0.425f * 600; //window_height;
				float size = 38;
				// Sign Level
				DrawNumber(gCurrentSignLevel, x, y, size); 
			}



/*			else if (strncmp("DWD_UI_RoadChallengeSignsFinal.bmp", goldStoryBoardTransitionScreenName,50) == 0) {
			}			
			else if (strncmp("DWD_UI_FinalExperimentComplete.bmp", goldStoryBoardTransitionScreenName,50) == 0) {
				// printf("Final Transition Experiment Complete Screen\n");
				float x = 0.25f * 800; //window_width;
				float y = 0.44f * 600; //window_height;
				float size = 40;
				DrawNumber(gCurrentSignLevel, x, y, size); 
				DrawNumber(gCurrentSpeedLevel, x*1.8f, y, size); 
			}	
*/	
			glEnable(GL_DEPTH_TEST);
	
		}
		else if (gpRoad->mCurrentGameScreen == GAMESCREEN_TRANSITION)
		{
			// Here is the feedback screen. 
			glDisable(GL_DEPTH_TEST);
			float r = 1.0f;
			float g = 1.0f;
			float b = 1.0f;
			float a = 1.0f;
			
			// Replace Feedback Transition if necessary. 
			if (strncmp (goldfeedbackScreenName,"default",2) != 0) {
					gfeedbackscreencodecalled++;
				if (gfeedbackscreencodecalled == 1) {
					// Enter the case for the dynamically changing storyboard transition screen loaded in from the storyboard variables. 
					char *gametransitiontexture = strcat(goldfeedbackScreenName,".bmp");					
					char pathname[200] = {0};
					strcat(pathname,"images/");
					strcat(pathname,gametransitiontexture);
					printf("pathname string: %s\n",pathname);
					gpRoad->mTexture_TransitionScreen = LoadTexture(pathname, false, true);

				}
			}		
			
			DrawScreenQuad(gpRoad->mTexture_TransitionScreen, 0.0 * window_width, 0.0 * window_height, 1.0 * window_width, 1.0 * window_height, r, g, b, a);

/////////////////////			
			// These fill in the dynamic graphics for the feedback screens for each GamePlayMode
			// This is is for the road thresholding bit
			if (strncmp("DWD_UI_RoadThresholding.bmp", goldfeedbackScreenName,30) == 0) {
					
			// Draw the history checks
			float checkX1 = 0.145f * window_width;
			float checkY1 = 0.22f * window_height;
			float checkX2 = checkX1 + 0.06f * 0.80 * window_width;
			float checkY2 = checkY1 + 0.1f * 0.75 * window_height;
			float xdiff = 0.060 * window_width;
			
			// Round number feedback area.   
			for (int i = 0; i < gAdjustmentHistoryNum; ++i)
			{
				if (gSpeedAdjustmentHistory[i] > 0)
					DrawScreenQuad(gpRoad->mTexture_TransCheck, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
				else if (gSpeedAdjustmentHistory[i] < 0)
					DrawScreenQuad(gpRoad->mTexture_TransX, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
				else
					DrawScreenQuad(gpRoad->mTexture_TransDash, checkX1, checkY1, checkX2, checkY2, r, g, b, a);

				checkX1 += xdiff;
				checkX2 += xdiff;

			}
			
			// Draw the numerical results. 
			// SetDrawTopColor(0, 1, 0, 1);
			// SetDrawBottomColor(0, 0.75, 0, 1);
			float size = 28.0f;
			float x = 0.19f * 800; //window_width; // 800
			float y = 0.52f * 600; //window_height; // 600

			DrawNumber(gLastSpeedAdjustment, x, y, size); 
			DrawNumber(gLastGrayPercent, x*1.86f, y, size); 
			DrawNumber(gCurrentSpeedLevel, x*2.75f, y, size); 
		
			}
			else if (strncmp("DWD_UI_SignThresholding.bmp", goldfeedbackScreenName,40) == 0) {
				
				// Draw the history checks
				float checkX1 = 0.205f * window_width;
				float checkY1 = 0.225f * window_height;
				float checkX2 = checkX1 + 0.06f * 0.80 * window_width;
				float checkY2 = checkY1 + 0.1f * 0.75 * window_height;
				float xdiff = 0.065 * window_width;
				
				int start =12;
				
				// Round number feedback area.   
				for (int i = start; i < gAdjustmentHistoryNum; ++i)
				{
					if (gSignAdjustmentHistory[i] > 0)
						DrawScreenQuad(gpRoad->mTexture_TransCheck, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					else if (gSignAdjustmentHistory[i] < 0)
						DrawScreenQuad(gpRoad->mTexture_TransX, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					else
						DrawScreenQuad(gpRoad->mTexture_TransDash, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					
					checkX1 += xdiff;
					checkX2 += xdiff;
					
				}
				
				// Draw the numerical results. 
				//SetDrawTopColor(0, 1, 0, 1);
				//SetDrawBottomColor(0, 0.75, 0, 1);
				float size =28;
				float x = 0.21f * 800; //window_width;
				float y = 0.52f * 600; //window_height;
				// Need to record the previous sign level in brRoad. 
				DrawNumber(gLastSignAdjustment, x, y, size); 
				// Sign Performance. 
				DrawNumber(gLastGoSignPercent, x*1.745f, y, size); 
				DrawNumber(gCurrentSignLevel, x*2.50f, y, size); 
				
			}
			else if (strncmp("DWD_Behave_Feedback.bmp", goldfeedbackScreenName,30) == 0) {

//				printf("Time: %d\n",gStoryBoardPlayLengthSeconds);
				
				// Draw the history checks
				float checkX1 = 0.999f * window_width;
				float checkY1 = 0.999f * window_height;
				float checkX2 = checkX1 + 0.06f * 0.80 * window_width;
				float checkY2 = checkY1 + 0.1f * 0.75 * window_height;
				float xdiff = 0.065 * window_width;
				
				// Trying to reference the right elements in the array instead of clearing it. 
				int start = 21;
				
				// Round number feedback area.   
				for (int i = start; i < gAdjustmentHistoryNum; ++i)
				{
					if (gSignAdjustmentHistory[i] > 0)
						DrawScreenQuad(gpRoad->mTexture_TransCheck, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					else if (gSignAdjustmentHistory[i] < 0)
						DrawScreenQuad(gpRoad->mTexture_TransX, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					else
						DrawScreenQuad(gpRoad->mTexture_TransDash, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					
					checkX1 += xdiff;
					checkX2 += xdiff;
										
					
				}
				
				// Draw the numerical results. 
				//SetDrawTopColor(0, 1, 0, 1);
				//SetDrawBottomColor(0, 0.75, 0, 1);
				float size =28;
				float x = 0.21f * 800; //window_width;
				float y = 0.52f * 600; //window_height;
			
			
			// This leaves blanks on conditions in the feedback screen; ex. if there's no sign, you shouldn't get 100%/0% on sign accuracy
			
				if (gPrevStoryBoardMode == 10)  
				{
					DrawNumber(gLastGoSignPercent, x*2.5, y, size); 
					DrawNumber(gLastGrayPercent, x, y, size);
				}
				else if (gPrevStoryBoardMode == 16 || gPrevStoryBoardMode == 11)  
				{
					DrawNumber(gLastGoSignPercent, x*2.5, y, size); 
				}
				else if (gPrevStoryBoardMode == 13  || gPrevStoryBoardMode == 15)  
				{
					DrawNumber(gLastGrayPercent, x, y, size);
				}

				//printf("storyboard: %f\n", gPrevStoryBoardMode);
							
				

			}


// Joaq CONVO begin
			else if (strncmp("DWD_Behave_Feedback_CONVO.bmp", goldfeedbackScreenName,30) == 0) {

//				printf("Time: %d\n",gStoryBoardPlayLengthSeconds);
				
				// Draw the history checks
				float checkX1 = 0.999f * window_width;
				float checkY1 = 0.999f * window_height;
				float checkX2 = checkX1 + 0.06f * 0.80 * window_width;
				float checkY2 = checkY1 + 0.1f * 0.75 * window_height;
				float xdiff = 0.065 * window_width;
				
				// Trying to reference the right elements in the array instead of clearing it. 
				int start = 21;
				
				// Round number feedback area.   
				for (int i = start; i < gAdjustmentHistoryNum; ++i)
				{
					if (gSignAdjustmentHistory[i] > 0)
						DrawScreenQuad(gpRoad->mTexture_TransCheck, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					else if (gSignAdjustmentHistory[i] < 0)
						DrawScreenQuad(gpRoad->mTexture_TransX, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					else
						DrawScreenQuad(gpRoad->mTexture_TransDash, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					
					checkX1 += xdiff;
					checkX2 += xdiff;
										
					
				}
				
				// Draw the numerical results. 
				//SetDrawTopColor(0, 1, 0, 1);
				//SetDrawBottomColor(0, 0.75, 0, 1);
				float size =28;
				float x = 0.37f * 800; //window_width;
				float y = 0.52f * 600; //window_height;
			
			
			// This leaves blanks on conditions in the feedback screen; ex. if there's no sign, you shouldn't get 100%/0% on sign accuracy
			
				if (gPrevStoryBoardMode == 10)  
				{
					DrawNumber(gLastGoSignPercent, x*2.5, y, size); 
					DrawNumber(gLastGrayPercent, x, y, size);
				}
				else if (gPrevStoryBoardMode == 16 || gPrevStoryBoardMode == 11)  
				{
					DrawNumber(gLastGoSignPercent, x*2.5, y, size); 
				}
				else if (gPrevStoryBoardMode == 13  || gPrevStoryBoardMode == 15)  
				{
					DrawNumber(gLastGrayPercent, x, y, size);
				}

				//printf("storyboard: %f\n", gPrevStoryBoardMode);

}

// Joaq CONVO end






			else if (strncmp("DWD_UI_RoadThresholdingFinal.bmp", goldfeedbackScreenName,30) == 0){
			
				float x = 0.35f * 800; //window_width;
				float y = 0.425f * 600; //window_height;
				float size = 38;
				// Sign Level
				DrawNumber(gCurrentSpeedLevel, x, y, size); 
			
				}

			else if (strncmp("DWD_UI_SignThresholdingFinal.bmp", goldfeedbackScreenName,50) == 0) {
				// printf("Sign Thresholding Final Transition Screen\n");
				float x = 0.35f * 800; //window_width;
				float y = 0.425f * 600; //window_height;
				float size = 38;
				// Sign Level
				DrawNumber(gCurrentSignLevel, x, y, size); 
			
				}


			else if (strncmp("DWD_UI_SignChallenge.bmp", goldfeedbackScreenName,40) == 0) {
				// printf("Sign Challenge\n");
				
				// Draw the history checks
				float checkX1 = 0.32f * window_width;
				float checkY1 = 0.23f * window_height;
				float checkX2 = checkX1 + 0.06f * 0.80 * window_width;
				float checkY2 = checkY1 + 0.1f * 0.75 * window_height;
				float xdiff = 0.065 * window_width;
				
				int start = 24;
				
				// Round number feedback area.   
				for (int i = start; i < gAdjustmentHistoryNum; ++i)
				{
					if (gAdjustmentHistory[i] > 0)
						DrawScreenQuad(gpRoad->mTexture_TransCheck, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					else if (gAdjustmentHistory[i] < 0)
						DrawScreenQuad(gpRoad->mTexture_TransX, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					else
						DrawScreenQuad(gpRoad->mTexture_TransDash, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					
					checkX1 += xdiff;
					checkX2 += xdiff;
					
				}
				
				// Draw the numerical results. 
				// SetDrawTopColor(0, 1, 0, 1);
				// SetDrawBottomColor(0, 0.75, 0, 1);
				float size =22;
				float x = 0.18f * 800; // window_width;
				float y = 0.55f * 600; // window_height;
				
				
				// Overall adjustment History value tracking. Temp. 
				for (int i = 0; i < gAdjustmentHistoryNum; ++i)	{
					printf("element: %d\n",gAdjustmentHistory[i]);
				}
				
				// Not road or sign specific information. 
				DrawNumber(gLastDifficultyAdjustment, x, y, size); 
				
				// Draw the history checks
				checkX1 = x*2.1f; //0.32f * window_width;
				checkY1 = x*2.8f; //0.23f * window_height;
				checkX2 = checkX1 + 0.06f * 0.80 * window_width;
				checkY2 = checkY1 + 0.1f * 0.75 * window_height;
				
				// This is the road checkbox.  
					if (gSpeedAdjustmentHistory[gAdjustmentHistoryNum] > 0)
						DrawScreenQuad(gpRoad->mTexture_TransCheck, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					else if (gSpeedAdjustmentHistory[gAdjustmentHistoryNum] < 0)
						DrawScreenQuad(gpRoad->mTexture_TransX, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					else
						DrawScreenQuad(gpRoad->mTexture_TransDash, checkX1, checkY1, checkX2, checkY2, r, g, b, a);		
				
				checkX1 = x*3.1f; //0.32f * window_width;
				checkY1 = x*2.8f; //0.23f * window_height;
				checkX2 = checkX1 + 0.06f * 0.80 * window_width;
				checkY2 = checkY1 + 0.1f * 0.75 * window_height;
				
				// This is the sign checkbox. 
					if (gSignAdjustmentHistory[gAdjustmentHistoryNum] > 0)
						DrawScreenQuad(gpRoad->mTexture_TransCheck, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					else if (gSignAdjustmentHistory[gAdjustmentHistoryNum] < 0)
						DrawScreenQuad(gpRoad->mTexture_TransX, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
					else
						DrawScreenQuad(gpRoad->mTexture_TransDash, checkX1, checkY1, checkX2, checkY2, r, g, b, a);
				 
				//DrawNumber(gOverallAccuracy, x*1.65f, y, size); 
				// we need to insert the sign accuracy here. 
				//DrawNumber(gLastGoSignPercent, x*2.5f, y, size); 			
				DrawNumber(gCurrentDifficultyLevel, x*3.3f, y, size); 
				
			}				
				
			glEnable(GL_DEPTH_TEST);

		}
		else if (gpRoad->mCurrentGameScreen == GAMESCREEN_PLAY)
		{
			if (gpRoad->mGameScreenTotalTime < LeadTime321Go_MAX * 0.25f)
			{
				gfeedbackscreencodecalled = 0;		// During fMRI mode, FINISH turned off;
				gtransitioncodecalled = 0;			// this is here to reset these variables

				float scale = gpRoad->mGameScreenTotalTime - 0.0f;
				scale *= 3.0f;
				if (scale > 1.0f)
					scale = 1.0f;
				float r = 1.0f;
				float g = 1.0f;
				float b = RANDOM_IN_RANGE(0.0f, 0.5f);
				float a = 1.0f;
				float x1 = (0.5f - (0.1f * scale)) * window_width;
				float x2 = (0.5f + (0.1f * scale)) * window_width;
				float y1 = 0.4f * window_height;
				float y2 = 0.6f * window_height;
				DrawScreenQuad(gpRoad->mTexture_Count3, x1, y1, x2, y2, r, g, b, a);
			}
			else if (gpRoad->mGameScreenTotalTime < LeadTime321Go_MAX * 0.5f)
			{
				float scale = gpRoad->mGameScreenTotalTime - 1.0f;
				scale *= 3.0f;
				if (scale > 1.0f)
					scale = 1.0f;
				float r = 1.0f;
				float g = 1.0f;
				float b = RANDOM_IN_RANGE(0.0f, 0.5f);
				float a = 1.0f;
				float x1 = (0.5f - (0.1f * scale)) * window_width;
				float x2 = (0.5f + (0.1f * scale)) * window_width;
				float y1 = 0.4f * window_height;
				float y2 = 0.6f * window_height;
				DrawScreenQuad(gpRoad->mTexture_Count2, x1, y1, x2, y2, r, g, b, a);
			}
			else if (gpRoad->mGameScreenTotalTime < LeadTime321Go_MAX * 0.75f)
			{
				float scale = gpRoad->mGameScreenTotalTime - 2.0f;
				scale *= 3.0f;
				if (scale > 1.0f)
					scale = 1.0f;
				float r = 1.0f;
				float g = 1.0f;
				float b = RANDOM_IN_RANGE(0.0f, 0.5f);
				float a = 1.0f;
				float x1 = (0.5f - (0.1f * scale)) * window_width;
				float x2 = (0.5f + (0.1f * scale)) * window_width;
				float y1 = 0.4f * window_height;
				float y2 = 0.6f * window_height;
				DrawScreenQuad(gpRoad->mTexture_Count1, x1, y1, x2, y2, r, g, b, a);
			}
			else if (gpRoad->mGameScreenTotalTime < LeadTime321Go_MAX * 1.0f)
			{
				float scale = gpRoad->mGameScreenTotalTime - 3.0f;
				scale *= 3.0f;
				if (scale > 1.0f)
					scale = 1.0f;
				float r = RANDOM_IN_RANGE(0.0f, 1.0f);
				float g = RANDOM_IN_RANGE(0.0f, 1.0f);
				float b = RANDOM_IN_RANGE(0.0f, 1.0f);
				float a = 1.0f;
				float x1 = (0.5f - (0.1f * scale)) * window_width;
				float x2 = (0.5f + (0.1f * scale)) * window_width;
				float y1 = 0.4f * window_height;
				float y2 = 0.6f * window_height;
				DrawScreenQuad(gpRoad->mTexture_CountGo, x1, y1, x2, y2, r, g, b, a);
			}
			 //////////////////////////////////////////////
            // Draw the new, stable fixation cross
           /*
			float scale = 0.4f * window_height;
            float thick = 0.062f;
            float thin = 0.01f;
            float posX = 0.5f * window_width;
            float posY = 0.595f * window_height;
            float r = 0;
            float g = 1;
            float b = 0;
            float a = 1;
            if (gpRoad->mbLicenseRed)
            {
                r = 1;
                g = 0;
            }
            
            float x1, y1, x2, y2;
            x1 = (posX - (thin * scale));
            x2 = (posX + (thin * scale));
            y1 = (posY - (thick * scale));
            y2 = (posY + (thick * scale));
            DrawScreenQuad(0, x1, y1, x2, y2, r, g, b, a);
            x1 = (posX - (thick * scale));
            x2 = (posX + (thick * scale));
            y1 = (posY - (thin * scale));
            y2 = (posY + (thin * scale));
            DrawScreenQuad(0, x1, y1, x2, y2, r, g, b, a);
		*/
			}

		
		///////////// Joaq attempt to draw static signs  ///////////////////
		if (gpRoad->mCurrentGameScreen == GAMESCREEN_PLAY)
        if (gpRoad->mRoadSign.mbVisible)
		{
            gfeedbackscreencodecalled = 0;
            gtransitioncodecalled = 0;
            float r = 1.0f;
			float g = 1.0f;
			float b = 1.0f;
			float a = 1.0f;
           
			//DrawScreenQuad(gpRoad->mTexture_StaticSign, 0.3 * window_width, 0.3 * window_height, 0.7 * window_width, 0.7 * window_height, r, g, b, a);
			DrawScreenQuad(gpRoad->mRoadSign.mTextures[gpRoad->mRoadSign.mCurrentSignTexture], 0.3 * window_width, 0.4 * window_height, 0.7 * window_width, 0.8 * window_height, r, g, b, a);
			
		}
		
		//////////////////////////////////////////////////
		// Now enable each of the square white squares for the diode. 
		// 
		// this means the button has been pushed on a relevant sign 
		
		//First we need black boxes for the bottom left and right so that the road doesn't interfere with diode readings
		if (gpRoad->mRoadSign.mBlinkerCountdown3 == 0) {
			float r = 0.0f;
			float g = 0.0f;
			float b = 0.0f;
			float a = 1.0f;
			float	x1 = BlinkerPosX3_MIN * window_width;
			float	x2 = BlinkerPosX3_MAX * window_width;
			float	y1 = BlinkerPosY3_MIN * window_height;
			float	y2 = BlinkerPosY3_MAX * window_height;
			DrawScreenQuad(gpRoad->mBlinkerTexture3, x1, y1, x2, y2, r, g, b, a);
			}
		if (gpRoad->mRoadSign.mBlinkerCountdown4 == 0) {
			float r = 0.0f;
			float g = 0.0f;
			float b = 0.0f;
			float a = 1.0f;
			float x1 = BlinkerPosX4_MIN * window_width;
			float x2 = BlinkerPosX4_MAX * window_width;
			float y1 = BlinkerPosY4_MIN * window_height;
			float y2 = BlinkerPosY4_MAX * window_height;
			DrawScreenQuad(gpRoad->mBlinkerTexture4, x1, y1, x2, y2, r, g, b, a);
			}
		if (gpRoad->mRoadSign.mBlinkerCountdown2 == 0) {
			float r = 0.0f;
			float g = 0.0f;
			float b = 0.0f;
			float a = 1.0f;
			float	x1 = BlinkerPosX2_MIN * window_width;
			float	x2 = BlinkerPosX2_MAX * window_width;
			float	y1 = BlinkerPosY2_MIN * window_height;
			float	y2 = BlinkerPosY2_MAX * window_height;
			DrawScreenQuad(gpRoad->mBlinkerTexture2, x1, y1, x2, y2, r, g, b, a);
			}
		if (gpRoad->mRoadSign.mBlinkerCountdown == 0) {
			float r = 0.0f;
			float g = 0.0f;
			float b = 0.0f;
			float a = 1.0f;
			float x1 = BlinkerPosX_MIN * window_width;
			float x2 = BlinkerPosX_MAX * window_width;
			float y1 = BlinkerPosY_MIN * window_height;
			float y2 = BlinkerPosY_MAX * window_height;
			DrawScreenQuad(gpRoad->mBlinkerTexture, x1, y1, x2, y2, r, g, b, a);
			}

		if (gpRoad->mRoadSign.mBlinkerCountdown > 0)
		{
			gpRoad->mRoadSign.mBlinkerCountdown--;
			float r = 1.0f;
			float g = 1.0f;
			float b = 1.0f;
			float a = 1.0f;
			float	x1 = BlinkerPosX_MIN * window_width;
			float	x2 = BlinkerPosX_MAX * window_width;
			float	y1 = BlinkerPosY_MIN * window_height;
			float	y2 = BlinkerPosY_MAX * window_height;
			DrawScreenQuad(gpRoad->mBlinkerTexture, x1, y1, x2, y2, r, g, b, a);
		}
		// relevant sign appeared
		if (gpRoad->mRoadSign.mBlinkerCountdown2 > 0)
		{
			gpRoad->mRoadSign.mBlinkerCountdown2--;
			float r = 1.0f;
			float g = 1.0f;
			float b = 1.0f;
			float a = 1.0f;
			float	x1 = BlinkerPosX2_MIN * window_width;
			float	x2 = BlinkerPosX2_MAX * window_width;
			float	y1 = BlinkerPosY2_MIN * window_height;
			float	y2 = BlinkerPosY2_MAX * window_height;
			DrawScreenQuad(gpRoad->mBlinkerTexture2, x1, y1, x2, y2, r, g, b, a);
		}
		// lure sign -- anything green and not a circle. 
		if (gpRoad->mRoadSign.mBlinkerCountdown3 > 0)
		{
			gpRoad->mRoadSign.mBlinkerCountdown3--;
			
			float r = 1.0f;
			float g = 1.0f;
			float b = 1.0f;
			float a = 1.0f;
			float	x1 = BlinkerPosX3_MIN * window_width;
			float	x2 = BlinkerPosX3_MAX * window_width;
			float	y1 = BlinkerPosY3_MIN * window_height;
			float	y2 = BlinkerPosY3_MAX * window_height;
			DrawScreenQuad(gpRoad->mBlinkerTexture3, x1, y1, x2, y2, r, g, b, a);
		}
		// Irrelevant sign
		if (gpRoad->mRoadSign.mBlinkerCountdown4 > 0)
		{
			gpRoad->mRoadSign.mBlinkerCountdown4--;
			float r = 1.0f;
			float g = 1.0f;
			float b = 1.0f;
			float a = 1.0f;
			float	x1 = BlinkerPosX4_MIN * window_width;
			float	x2 = BlinkerPosX4_MAX * window_width;
			float	y1 = BlinkerPosY4_MIN * window_height;
			float	y2 = BlinkerPosY4_MAX * window_height;
			DrawScreenQuad(gpRoad->mBlinkerTexture4, x1, y1, x2, y2, r, g, b, a);
		}	
		
	} // end of some bigger function. 

	
	if (0)			// Disable the helpful text
	{
		sprintf (outString, "Camera Position: (%0.1f, %0.1f, %0.1f)", gCameraPos.x, gCameraPos.y, gCameraPos.z);
		drawGLString (10, window_height - (lineSpacing * line++) - startOffest, outString);

		line += 5;

		for (int index = 0; index < GetAdjustableVarCount(); ++index)
		{
			char const *pAdjustString = GetAdjustableVarString(index);
			if (pAdjustString != NULL)
			{
				drawGLString (10, window_height - (lineSpacing * line++) - startOffest, pAdjustString);
			}
		}
		drawGLString (10, window_height - (lineSpacing * line++) - startOffest, "Adjustable Values:");
	}														   
														   
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(matrixMode);
	  
	glViewport(vp[0], vp[1], vp[2], vp[3]);
}

void initGLState (void)
{
	
	glEnable(GL_DEPTH_TEST);

	glShadeModel(GL_SMOOTH);    
	glFrontFace(GL_CCW);

	glColor3f(1.0,1.0,1.0);
	
	glClearColor(0.0,0.0,0.0,0.0);         /* Background recColor */
	gCameraReset ();
	
	glPolygonOffset (1.0, 1.0);
	SetLighting(4);
	glEnable(GL_LIGHTING);
	
}

#ifndef DWD_QT
void reshape (int w, int h)
{
	glViewport(0,0,(GLsizei)w,(GLsizei)h);
	gScreenWidth = w;
	gScreenHeight = h;
	glutPostRedisplay();
}
#endif

void maindisplay(void)
{
	GLdouble xmin, xmax, ymin, ymax;
	// far frustum plane
	GLdouble zFar = 1000.0f;
	// near frustum plane clamped at 1.0
	GLdouble zNear = 0.1f;
	// window aspect ratio
	GLdouble aspect = gScreenWidth / (GLdouble)gScreenHeight; 

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();

	ymax = zNear;
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;

	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
	
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	gluLookAt (gCameraPos.x, gCameraPos.y, gCameraPos.z,
			   gCameraLookAt.x, gCameraLookAt.y, gCameraLookAt.z,
			   gCameraUp.x, gCameraUp.y, gCameraUp.z);

	// Here is the sky color. 
	// r g b and alpha. 
	// 0-191-255 
	glClearColor (0.0f, 0.75f, 1.0f, 1.0f);	
	//glClearColor (0.2f, 0.2f, 0.4f, 1.0f);	// clear the surface
	if (blackout)
	{
		glClearColor (0.0f, 0.0f, 0.0f, 1.0f);	// clear the surface
	}
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_LIGHTING);
	gpRoad->Draw();
	
	drawGLText (gScreenWidth, gScreenHeight);
#ifndef DWD_QT
	glutSwapBuffers();
#endif
}

#ifndef DWD_QT
void specialDownFunc(int key, int px, int py)
{
  switch (key) {
	case GLUT_KEY_UP: // arrow forward, close in on world
		gpRoad->mCar.mJoystickX = gpRoad->mCar.mJoystickY = 0.0f;
		gpRoad->mCar.mbKeyboardAccel = true;
		break;
	case GLUT_KEY_DOWN: // arrow back, back away from world
		gpRoad->mCar.mJoystickX = gpRoad->mCar.mJoystickY = 0.0f;
		gpRoad->mCar.mbKeyboardBrake = true;
		break;
	case GLUT_KEY_LEFT: // arrow left, smaller aperture
		gpRoad->mCar.mJoystickX = gpRoad->mCar.mJoystickY = 0.0f;
		gpRoad->mCar.mbKeyboardLeft = true;
		break;
	case GLUT_KEY_RIGHT: // arrow right, larger aperture
		gpRoad->mCar.mJoystickX = gpRoad->mCar.mJoystickY = 0.0f;
		gpRoad->mCar.mbKeyboardRight = true;
		break;
  }
}

void specialUpFunc(int key, int px, int py)
{
  switch (key) {
	case GLUT_KEY_UP: // arrow forward, close in on world
		gpRoad->mCar.mJoystickX = gpRoad->mCar.mJoystickY = 0.0f;
		gpRoad->mCar.mbKeyboardAccel = false;
		break;
	case GLUT_KEY_DOWN: // arrow back, back away from world
		gpRoad->mCar.mJoystickX = gpRoad->mCar.mJoystickY = 0.0f;
		gpRoad->mCar.mbKeyboardBrake = false;
		break;
	case GLUT_KEY_LEFT: // arrow left, smaller aperture
		gpRoad->mCar.mJoystickX = gpRoad->mCar.mJoystickY = 0.0f;
		gpRoad->mCar.mbKeyboardLeft = false;
		break;
	case GLUT_KEY_RIGHT: // arrow right, larger aperture
		gpRoad->mCar.mJoystickX = gpRoad->mCar.mJoystickY = 0.0f;
		gpRoad->mCar.mbKeyboardRight = false;
		break;
  }
}
#endif

void mouseFunc (int button, int state, int x, int y)
{
}

void mouseMoveFunc (int button, int state, int x, int y)
{
}

void joystickFunc(unsigned int buttonMask, int x, int y, int z)
{
#ifdef _WIN32
	// x goes -1000 to 1000, y goes -1000 to 1000
	gpRoad->mCar.mJoystickX = x / 1000.0f;
	gpRoad->mCar.mJoystickY = -y / 1000.0f;
#else
	// x goes -1000 to 1000, y goes 0 to 255
	gpRoad->mCar.mJoystickX = x / 1000.0f;
	gpRoad->mCar.mJoystickY = -y / 1000.0f;
#endif
	// Uncomment the next line for easy debugging!
//	printf("joystick %d %d --> %.2f %.2f\n", x, y, gpRoad->mCar.mJoystickX, gpRoad->mCar.mJoystickY);
//	printf("buttonmask %d \n", buttonMask);  //32 16
	if (buttonMask != 0)
	{
		gpRoad->mCar.mbJoystickButton = true;
	}
	else
	{
		gpRoad->mCar.mbJoystickButton = false;
	}
	if (buttonMask == 16)
	{
		gpRoad->mCar.mbJoystickLeft = true;
	}
	else
		gpRoad->mCar.mbJoystickLeft = false;
	if (buttonMask == 32)
	{
		gpRoad->mCar.mbJoystickRight = true;
	}
	else
		gpRoad->mCar.mbJoystickRight = false;

	glutJoystickFunc(joystickFunc, gDesiredFrameMilliseconds);
}

#ifndef DWD_QT
void timerFunc(int value)
{
	static float lastTime = 0.0f;
	float nowTime;
#ifdef _WIN32
	// This is a more accurate timer
	uint32 milliseconds = GetMillisecondTimer();
	nowTime = (float)milliseconds * 0.001f;
#else
	// ...but the Mac version isn't done yet.
	nowTime = lastTime + (1.0f / gDesiredFrameRate);
#endif
	float deltaTime = nowTime - lastTime;
	lastTime = nowTime;

	glutForceJoystickFunc();
	gpRoad->Update(deltaTime);
	glutTimerFunc(gDesiredFrameMilliseconds, timerFunc, 0);
	glutPostRedisplay();
}
#endif

void keyDownFunc(unsigned char inkey, int px, int py)
{
  switch (inkey) {
	case 27:		// esc key
		gbFullscreen = !gbFullscreen;
#ifndef DWD_QT
                if (gbFullscreen)
		{
			glutFullScreen();
		}
		else
		{
			glutReshapeWindow(800, 600);
			glutPositionWindow(50, 50);
		}
#endif
		break;
	  case 0x0d:
	  case 0x0a:
		  gpRoad->mCar.mbKeyboardButton = true;
		  break;
	  case '5':
		  if(gpRoad->mCurrentGameScreen == GAMESCREEN_START && Behav0EEG1fMRI2_MIN == 2)
			  gpRoad->TransitionToScreen(GAMESCREEN_PLAY);
		  break;
	  case 'f':
		  gpRoad->TransitionToScreen(GAMESCREEN_FINISH);
		  break;
	  case 'p':
		  gpRoad->TogglePause();
		  break;
	  case 'a':
		  gpRoad->mbAutopilot = ! gpRoad->mbAutopilot;
		  if (gpRoad->mbAutopilot)
			  printf("Autopilot engaged. The computer is driving.\n");
		  else
			  printf("Autopilot disengaged. You are driving.\n");
		  break;
	  case 'b':
		  blackout = ! blackout;
		  if (gpRoad->mbAutopilot)
			  printf("Blackout engaged. Spooky.\n");
		  else
			  printf("Blackout disengaged. Less Spooky.\n");
		  break;
	  case '+':
	  case '=':
		  gpRoad->mbOverrideSpeed = true;
		  gSpeed_MIN += 1.0f;
		  gSpeed_MAX += 1.0f;
		  printf("Override speed: %.1f - %.1f\n", gSpeed_MIN, gSpeed_MAX);
		  break;
	  case '-':
	  case '_':
		  gpRoad->mbOverrideSpeed = true;
		  gSpeed_MIN -= 1.0f;
		  gSpeed_MAX -= 1.0f;
		  printf("Override speed: %.1f - %.1f\n", gSpeed_MIN, gSpeed_MAX);
		  break;
	  case 's':
		  gpRoad->mbSpeedometerOnRoad = ! gpRoad->mbSpeedometerOnRoad;
		   
		  break;
  }
}

void keyUpFunc(unsigned char inkey, int px, int py)
{
  switch (inkey) {
	  case 0x0d:
	  case 0x0a:
		  gpRoad->mCar.mbKeyboardButton = false;
		  break;
	default:
		break;
  }
}

#if !defined(DWD_QT)
int main (int argc, const char * argv[])
{
//	char mydir[512];
//	size_t length = 512;
//	getcwd(mydir, length);
	
	char *glutArgV[] = {"glut", "-syncToVBL"};
	int glutArgC = 2;
//    glutInit(&argc, (char**)argv);
    glutInit(&glutArgC, glutArgV);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); // non-stereo for main window
	glutInitWindowPosition (300, 50);
	glutInitWindowSize (800, 600);
	gGLUTWindow = glutCreateWindow("GazzaleyLab DWD");

	// Now we should have the correct Adjustable Variabls in gAllAdjuster variable
	//SetDifficultyLevel(1);
	// Load all files up to start. 
	LoadAdjustableVars(0);
	SetDifficultyLevel(0);
	SetSpeedLevel(SpeedLevelStart_MIN);
	SetSignLevel(SignLevelStart_MIN);
	SetStoryBoardLevel(0);

    initGLState(); // standard GL init
	gpRoad = new Road();

    glutReshapeFunc (reshape);
    glutDisplayFunc (maindisplay);
	glutKeyboardFunc (keyDownFunc);
	glutKeyboardUpFunc (keyUpFunc);
	glutSpecialFunc (specialDownFunc);
	glutSpecialUpFunc (specialUpFunc);
	glutMouseFunc (mouseFunc);
	// this enables and initialises the peripheral. 
	glutJoystickFunc(joystickFunc, 0);
	
	glutTimerFunc(gDesiredFrameMilliseconds, timerFunc, 0);
    glutMainLoop();
    return 0;
}
#endif // ! DWD_QT

#ifdef DWD_QT
static DWDWindow *gpQTWindow = NULL;

DWDWindow::DWDWindow(QWidget *parent)
    : QGLWidget(parent)
{
    gpQTWindow = this;
    gScreenWidth = width();
    gScreenHeight = height();
    updateGL();

  //  SetDifficultyLevel(1);
	LoadAdjustableVars(1);
	
    initGLState();
    gpRoad = new Road();

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(PeriodicUpdate()));
    timer->start(16);
}

void DWDWindow::keyPressOrRelease(QKeyEvent *pEvt, bool press)
{
    switch (pEvt->key())
    {
        case Qt::Key_Return:
            gpRoad->mCar.mbKeyboardButton = press;
            break;
        case Qt::Key_Up:
			gpRoad->mCar.mJoystickX = gpRoad->mCar.mJoystickY = 0.0f;
            gpRoad->mCar.mbKeyboardAccel = press;
            break;
        case Qt::Key_Down:
			gpRoad->mCar.mJoystickX = gpRoad->mCar.mJoystickY = 0.0f;
            gpRoad->mCar.mbKeyboardBrake = press;
            break;
        case Qt::Key_Left:
			gpRoad->mCar.mJoystickX = gpRoad->mCar.mJoystickY = 0.0f;
            gpRoad->mCar.mbKeyboardLeft = press;
            break;
        case Qt::Key_Right:
			gpRoad->mCar.mJoystickX = gpRoad->mCar.mJoystickY = 0.0f;
            gpRoad->mCar.mbKeyboardRight = press;
            break;
        case Qt::Key_Escape:
            if (press)
            {
                gbFullscreen = !gbFullscreen;
                if (gbFullscreen)
                    this->showFullScreen();
                else
                    this->showNormal();
            }
            break;
          case Qt::Key_1:
          case Qt::Key_2:
          case Qt::Key_3:
          case Qt::Key_4:
          case Qt::Key_5:
          case Qt::Key_6:
          case Qt::Key_7:
          case Qt::Key_8:
          case Qt::Key_9:
                  if (press)
                      SetDifficultyLevel(pEvt->key() - Qt::Key_0);
                  break;
          case Qt::Key_F:
                  if (press)
                      gpRoad->TransitionToScreen(GAMESCREEN_FINISH);
                  break;
          case Qt::Key_P:
                  if (press)
                      gpRoad->TogglePause();
                  break;
		  case Qt::Key_A:
			  gpRoad->mbAutopilot = ! gpRoad->mbAutopilot;
			  if (gpRoad->mbAutopilot)
				  printf("Autopilot engaged. The computer is driving.\n");
			  else
				  printf("Autopilot disengaged. You are driving.\n");
			  break;
		case Qt::Key_B:
			blackout = ! blackout;
			if (gpRoad->mbAutopilot)
				printf("Blackout engaged. Spooky.\n");
			else
				printf("Blackout disengaged.\n");
			break;
		case Qt::Key_Plus:
		  case Qt::Key_Equals:
			  gpRoad->mbOverrideSpeed = true;
			  gSpeed_MIN += 1.0f;
			  gSpeed_MAX += 1.0f;
			  printf("Override speed: %.1f - %.1f\n", gSpeed_MIN, gSpeed_MAX);
			  break;
		  case Qt::Key_Minus:
		  case Qt::Key_Underscore:
			  gpRoad->mbOverrideSpeed = true;
			  gSpeed_MIN -= 1.0f;
			  gSpeed_MAX -= 1.0f;
			  printf("Override speed: %.1f - %.1f\n", gSpeed_MIN, gSpeed_MAX);
			  break;
          case Qt::Key_S:
                  if (press)
                      gpRoad->mbSpeedometerOnRoad = ! gpRoad->mbSpeedometerOnRoad;
                  break;
        default:
            break;
    }
}

void DWDWindow::keyPressEvent(QKeyEvent *pEvt)
{
    keyPressOrRelease(pEvt, true);
}

void DWDWindow::keyReleaseEvent(QKeyEvent *pEvt)
{
    keyPressOrRelease(pEvt, false);
}

void DWDWindow::PeriodicUpdate()
{
    float deltaTime = 1.0f/60.0f;
    gpRoad->Update(deltaTime);
    updateGL();
//    qDebug("Howdy");
}

DWDWindow::~DWDWindow()
{

}

void DWDWindow::paintGL()
{
    if (gpRoad)
    {
        maindisplay();
    }
}

void DWDWindow::resizeGL(int w, int h)
{
    glViewport(0,0,(GLsizei)w,(GLsizei)h);
    gScreenWidth = w;
    gScreenHeight = h;
}

// This is the QT version of LoadTexture
unsigned int LoadTexture(const char* filename, bool compressed, bool makeMips, int *outWidth, int *outHeight)
{
    QImage pix(filename);
    pix = pix.convertToFormat(QImage::Format_ARGB32, Qt::AvoidDither);
    for (int y = 0; y < pix.height(); ++y)
    {
        uint32 *pRow = (uint32*)pix.scanLine(y);
        for (int x = 0; x < pix.width(); ++x)
        {
            uint32 pixel = *pRow;
            if ((pixel & 0x00ffffff) == 0x000000ff)
                *pRow = 0;
            else
                *pRow = pixel | 0xff000000;
            pRow++;
        }
    }
    int texID = gpQTWindow->bindTexture(pix, GL_TEXTURE_2D, GL_RGBA);
    qDebug("TextureID 0x%x %dx%d '%s'", texID, pix.width(), pix.height(), filename);
    
	if (outWidth != NULL)
		*outWidth = pix.width();
	if (outHeight != NULL)
		*outHeight = pix.height();

	return texID;
}
#endif



