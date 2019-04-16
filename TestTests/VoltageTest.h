#pragma once

#include "BatteryTest.h"
#include "parserUtils.h"

#define PHASE_PREPARATION 0
#define PHASE_MEASURING 1


class VoltageTest:public BatteryTest
{
	
public:
	//VoltageTest(Communicator* comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute);
	VoltageTest(TestScheduler * ts, Communicator * comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute);
	void handle() override;
	String getTextResults() override;//get the textual representation of the test results
	void generateGUI(Container* c) override;
	void saveSettingsToSpiffs();
	void loadSettingsFromSpiffs();
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
	float failVoltageThreshold = 10.0;
};
