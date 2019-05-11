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
public:
	void addTest(BatteryTest* bt);
	BatteryTest* getCurrentTest();
	void handle(); 
	void notifyAboutTestEnd();
	BatteryTest* getLastTest(int batteryNo);
	void notifyAboutTestStart(BatteryTest * _bt);
};