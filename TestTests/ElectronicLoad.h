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
#include <SPISlave.h>

class ElectronicLoad
{
protected:
	static boolean isResultFresh;
	static unsigned long lastQueryTimestamp ;
	static int spiInIndex ;//index for the incoming data
	static int spiOutIndex ; //index for the outgoing data
	static uint8_t spiDataOut[32];
	static volatile boolean dataSent ;
	static float I;
	static float U1;
	static float U2;
	static float updatePeriod;
public:
	static int connectedBattery;
	static int setI(float theI);
	static int setTime(time_t theTime);
	static int setUpdatePeriod(float thePeriod);
	static int getI(float* target);
	static int getU1(float* target);
	static int getU(float * target, int batteryNo);
	static int getU2(float* target);
	static int getT(float* target);
	static float parseSPIFloat(uint8_t * data);
	static unsigned long parseSPIUL(uint8_t * data);
	static byte parseSPIByte(uint8_t * data);
	static void queueFloat(float f);
	static void queueUL(unsigned long f);
	static void queueByte(byte b);
	static int sendData(uint8_t * data, int len, unsigned long timeout);
	static int sendData(uint8_t * data, int len);
	static void onData(uint8_t * data, size_t len);
	static void onDataSent();
	static boolean areNewReadingsReady();
	static void begin();
	static int connectBattery(int batteryNo);
	static int requestTime();
	static uint8_t calcCheckSum(uint8_t * data);
	static boolean isChksumOk(uint8_t * data);
	static int getState();//the current state of the load
    static void recordLastQueryTimestamp();
	static void heartBeat();
};

