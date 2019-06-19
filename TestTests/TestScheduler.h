#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class BatteryTest;

class TestScheduler
{
private:
	std::vector<BatteryTest*> tests;
	BatteryTest* currentTest = 0;
	BatteryTest* lastTestBat1 = 0;
	BatteryTest* lastTestBat2 = 0;
	int status = 0;//0 = OK, 1 = FAIL
public:
	std::vector<BatteryTest*> getTests();
	void addTest(BatteryTest* bt);
	BatteryTest* getCurrentTest();
	void handle();
	BatteryTest* findTest(int testType, int batteryNo);

	void notifyAboutTestEnd(int endMode);

	BatteryTest* getLastTest(int batteryNo);
	void notifyAboutTestStart(BatteryTest * _bt);
	void resetStatus();
	int getStatus();
};