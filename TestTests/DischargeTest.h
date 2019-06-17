#pragma once

#include "BatteryTest.h"
#include "parserUtils.h"

#define PHASE_PREPARATION 0
#define PHASE_LOADING 1


class DischargeTest:public BatteryTest
{

public:
	//DischargeTest(TestScheduler * ts, Communicator * comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute);
	DischargeTest(int _batteryNo, TestScheduler * ts, Communicator * comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute);
	void start(boolean scheduled);
	void handle() override;
	String getIntermediateResults() override;
	String getName() override;
	void generateTextResults() override;
	void reportResultsOnGUI() override;
	void saveResults() override;
	void generateGUI(Container* c) override;
	String getId() override;
private:
	//Communicator* comm;
	Container* cont;
	float voltageSum;
	float averageVoltage;
	//int sendEmailReport() override;
	int getType() override;
	void startTestCallback(int user);
	void saveSettingsCallback(int user);
	void saveSettingsToSpiffs();
	String setOptions(String opt1, String opt2, String opt3, String opt4, String opt5) override;
	void loadSettingsFromSpiffs();
	String getSettings() override;
	void printHistoricalResults() override;
	const int MEASURMENTS_COUNT = 10;
	float testEndVoltage = 22;

	float minCapacity = 30;
	float extractedEnergy = 0;
	float batteryCapacity = 0;

	float currentI;//current as in immediate, might wanna relabel that...
	float currentU;//current as in immediate, might wanna relabel that...
};
