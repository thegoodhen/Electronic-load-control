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
public:
	void addTest(BatteryTest* bt);
	void handle(); 
	void notifyAboutTestEnd();
	void notifyAboutTestStart(BatteryTest * _bt);
};