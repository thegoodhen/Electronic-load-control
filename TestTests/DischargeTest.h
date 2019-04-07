#pragma once

#include "BatteryTest.h"
#include "parserUtils.h"

#define PHASE_PREPARATION 0
#define PHASE_LOADING 1


class DischargeTest:public BatteryTest
{
	
public:
	 DischargeTest(Communicator* comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute);
	void start(boolean scheduled);
	void handle() override;
	String getTextResults() override;//get the textual representation of the test results
	void generateGUI(Container* c) override;
	String getId();
private:
	Communicator* comm;
	Container* cont;
	float voltageSum;
	float averageVoltage;
	int reportResults() override;
	void startTestCallback(int user);
	void saveSettingsCallback(int user);
	const int MEASURMENTS_COUNT = 10;
	float testEndVoltage = 10;

	float minCapacity =		30;
	float extractedEnergy = 0;
	float batteryCapacity = 0;
};
