#pragma once

#define RESULT_OK 0//everything is fine
#define RESULT_OBSOLETE 1//everything is fine, but the returned result is obsolete, i.e. was already returned before
#define RESULT_ERROR_CRC 2//CRC mismatch
#define RESULT_ERROR_INVALID_REQUEST 3//the secondary processor couldn't process the request, since it had invalid syntax
#define RESULT_ERROR_OVERHEATING 4 // the load is overheating and was shut down forcefully to preven thermal runaway
#define RESULT_ERROR_OUTSIDE_RANGE 5//the setpoint is outside the valid range (e.g. the set current is too high for the given voltage)
#define RESULT_ERROR_CRITICAL_FAILURE 6//some other fault
#define BATTERY_1 1;
#define BATTERY_2 2;
#define BATTERY_NONE -1;

#include "Arduino.h"

class ElectronicLoad
{
protected:
	static boolean isResultFresh;
	static unsigned long lastQueryTimestamp ;
public:
	static int connectedBattery;
	static int setI(float theI);
	static int setUpdatePeriod(float thePeriod);
	static int getI(float* target);
	static int getU(float* target);
	static int getT(float* target);
	static int connectBattery(int batteryNo);
	static int getState();//the current state of the load
    static void recordLastQueryTimestamp();
};

