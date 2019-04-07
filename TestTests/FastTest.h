#pragma once

#include "BatteryTest.h"
#include "parserUtils.h"

#define PHASE_PREPARATION 0
#define PHASE_LOADING 1
#define PHASE_RECOVERY 2


class FastTest:public BatteryTest
{
	
public:
	FastTest(Communicator* comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute);
	void start(boolean scheduled, float loadCurrent);
	void handle() override;
	String getTextResults() override;
	String getId() override;
	//get the textual representation of the test results
	void generateGUI(Container* c) override;
private:
	Communicator* comm;
	Container* cont;
	float loadCurrent;
	float scheduledLoadCurrent;
	int reportResults() override;
	void startTestCallback(int user);
	void saveSettingsCallback(int user);
};
