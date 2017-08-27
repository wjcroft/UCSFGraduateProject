
// These are the adjustable variables
// This file is #included a few times, with different definitions for ADJUSTABLE_FLOAT_RANGE.

ADJUSTABLE_FLOAT_RANGE(TimeBetweenCurves)
ADJUSTABLE_FLOAT_RANGE(CurveDuration)
ADJUSTABLE_FLOAT_RANGE(CurveSharpness)

ADJUSTABLE_FLOAT_RANGE(TimeBetweenHills)
ADJUSTABLE_FLOAT_RANGE(HillDuration)
ADJUSTABLE_FLOAT_RANGE(HillSteepness)
ADJUSTABLE_FLOAT_RANGE(HillChangeRate)	// larger numbers make more abrupt ("sharp") hill changes
ADJUSTABLE_FLOAT_RANGE(HillAffectsCarSpeed) // larger number means car speeds up or slows down more on hills

ADJUSTABLE_FLOAT_RANGE(CarAcceleration)
ADJUSTABLE_FLOAT_RANGE(CarBraking)
ADJUSTABLE_FLOAT_RANGE(CarSteering)
ADJUSTABLE_FLOAT_RANGE(CarTurnSlip)		// car turn slippage

ADJUSTABLE_FLOAT_RANGE(SignLevelStart) //sign level
ADJUSTABLE_FLOAT_RANGE(SpeedLevelStart) //sign level

ADJUSTABLE_FLOAT_RANGE(TimeBetweenSigns)
ADJUSTABLE_FLOAT_RANGE(SignDuration)
ADJUSTABLE_FLOAT_RANGE(SignAheadTime)   // SECONDS how far ahead of the car does the sign appear?
ADJUSTABLE_FLOAT_RANGE(TimeBeforeFirstSign)

ADJUSTABLE_FLOAT_RANGE(BlinkerPosX)   // min and max X (0 to 1) of white square for the sign
ADJUSTABLE_FLOAT_RANGE(BlinkerPosY)   // min and max Y (0 to 1) of white square for the sign

ADJUSTABLE_FLOAT_RANGE(BlinkerPosX2)   // min and max X (0 to 1) of white square for the sign
ADJUSTABLE_FLOAT_RANGE(BlinkerPosY2)   // min and max Y (0 to 1) of white square for the sign

ADJUSTABLE_FLOAT_RANGE(BlinkerPosX3)   // min and max X (0 to 1) of white square for the sign
ADJUSTABLE_FLOAT_RANGE(BlinkerPosY3)   // min and max Y (0 to 1) of white square for the sign

ADJUSTABLE_FLOAT_RANGE(BlinkerPosX4)   // min and max X (0 to 1) of white square for the sign
ADJUSTABLE_FLOAT_RANGE(BlinkerPosY4)   // min and max Y (0 to 1) of white square for the sign


ADJUSTABLE_FLOAT_RANGE(RelevantSignProbability)

ADJUSTABLE_FLOAT_RANGE(LeadTime321Go)  // The time it takes to say 3,2,1,Go!!
ADJUSTABLE_FLOAT_RANGE(EndScreenTimeSECONDS)   // How long until the Finish screen automatically changes to the Start screen?

ADJUSTABLE_FLOAT_RANGE(RoadGrayWidth)	// The "gray" area
ADJUSTABLE_FLOAT_RANGE(RoadYellowWidth)	// The "yellow" rea
ADJUSTABLE_FLOAT_RANGE(RoadRedWidth)	// The "red" area
ADJUSTABLE_FLOAT_RANGE(CarWidth)	// The width of the car

ADJUSTABLE_FLOAT_RANGE(SpeedYellowWidth)	// The "yellow" speed range
ADJUSTABLE_FLOAT_RANGE(SpeedRedWidth)	// The "red" speed range
    
ADJUSTABLE_FLOAT_RANGE(SignTargetPerformanceRange)
ADJUSTABLE_FLOAT_RANGE(SignLevelRange)

ADJUSTABLE_FLOAT_RANGE(RoadTargetPerformanceRange)
ADJUSTABLE_FLOAT_RANGE(RoadLevelRange)

ADJUSTABLE_FLOAT_RANGE(OverallTargetPerformanceRange)
ADJUSTABLE_FLOAT_RANGE(OverallLevelRange)

ADJUSTABLE_FLOAT_RANGE(PowerTurnAndAccel)

ADJUSTABLE_FLOAT_RANGE(RockingSideAndForward)

ADJUSTABLE_FLOAT_RANGE(SpeedBumpStretch) // this is how spread out the speed-bumps look
ADJUSTABLE_FLOAT_RANGE(RedShake) // How much the car shakes in the red zone
ADJUSTABLE_FLOAT_RANGE(YellowShake) // How much the car shakes in the yellow zone

ADJUSTABLE_FLOAT_RANGE(Behav0EEG1fMRI2) // Are we in the EEG room?  Disallows user to advance level themself
ADJUSTABLE_FLOAT_RANGE(fMRILocalizerTime)
ADJUSTABLE_FLOAT_RANGE(fMRIExperimentTime)