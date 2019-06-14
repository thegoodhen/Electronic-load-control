#pragma once

#include "BatteryTest.h"
#include "parserUtils.h"

#define PHASE_PREPARATION 0
#define PHASE_NOLOAD 1
#define PHASE_LOADING 2
#define PHASE_RECOVERY 3


class FastTest:public BatteryTest
{
	
public:
	//FastTest(Communicator* comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute);
	void start(boolean scheduled, float loadCurrent);
	FastTest(int _batteryNo, TestScheduler * ts, Communicator * comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute);
	void updateChart();
	String getName() override;
	void handle() override;
	void saveResults() override;
	String setOptions(String opt1, String opt2, String opt3, String opt4, String opt5) override;
	void reportResultsOnGUI() override;
	void loadSettingsFromSpiffs();
	void generateTextResults() override;
	String getId() override;
	//get the textual representation of the test results
	void generateGUI(Container* c) override;
	void saveSettingsToSpiffs();
private:
	//Communicator* comm;
	Container* cont;
	float loadCurrent=20;
	float scheduledLoadCurrent;
	//int sendEmailReport() override;
	int getType() override;
	void startTestCallback(int user);
	void saveSettingsCallback(int user);
	String getSettings() override;
	double voltageAtStart;
	double voltageWhenLoaded;
	double currentWhenLoaded;
	double voltageAtEnd;
	double internalResistance;
	float maxRiBeforeFail=0.7;
};
