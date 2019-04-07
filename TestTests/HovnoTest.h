#pragma once

#include "BatteryTest.h"
#include "parserUtils.h"
#include "Communicator.h"


class HovnoTest:public BatteryTest
{
	
public:
	String getId() override;
	void handle() override;
	int reportResults();
	void generateGUI(Container* c) override;
	String getTextResults() override;
	//get the textual representation of the test results
};
