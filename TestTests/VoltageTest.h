#pragma once

#include "BatteryTest.h"
#include "parserUtils.h"

#define PHASE_PREPARATION 0
#define PHASE_MEASURING 1


class VoltageTest:public BatteryTest
{
	
public:
	//VoltageTest(Communicator* comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute);
	VoltageTest(Battery * b, TestScheduler * ts, Communicator * comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute);
	void generateTextResults() override;
	void reportResultsOnGUI() override;
	void handle() override;
	void generateGUI(Container* c) override;
	int getType() override;
	String getName() override;
	void saveSettingsToSpiffs();
	void loadSettingsFromSpiffs();
	String getId() override;
private:
	//Communicator* comm;
	Container* cont;
	float voltageSum;
	float averageVoltage;
	void saveResults();
	void startTestCallback(int user);
	String setOptions(String opt1, String opt2, String opt3, String opt4, String opt5) override;
	void saveSettingsCallback(int user);
	const int MEASURMENTS_COUNT = 10;
	float failVoltageThreshold = 20.0;
	String getSettings() override;
	String getIntermediateResults() override;
	void printHistoricalResults() override;
};
