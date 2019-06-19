
#include "TestScheduler.h"
#include "BatteryTest.h"

std::vector<BatteryTest*> TestScheduler::getTests()
{
	return this->tests;
}

void TestScheduler::addTest(BatteryTest * bt)
{
	tests.push_back(bt);
	bt->setScheduler(this);
}

BatteryTest* TestScheduler::getCurrentTest()
{
	return this->currentTest;
}

void TestScheduler::handle()
{
	if (status != 0)
	{
		return;//if something is wrong, do not run any tests!
	}
	if (this->currentTest == NULL)
	{
		for (std::vector<BatteryTest*>::size_type i = 0; i != tests.size(); i++) {
			BatteryTest* bt = (tests)[i];
			if (timeStatus()!=timeNotSet && now() > bt->getScheduledStartTime())
			{
				if (now() - bt->getScheduledStartTime() > 30)//we have missed the last run (for example because the device was off)
				{
					Serial.println("fastforwarding now...");
					bt->fastForwardScheduling();
					return;
				}
				bt->beginTest(true);
				this->currentTest = bt;
				return;
			}
		}
	}
	else
	{
		this->currentTest->handle();
	}

}

BatteryTest* TestScheduler::findTest(int testType, int batteryNo)
{

	for (std::vector<BatteryTest*>::size_type i = 0; i != tests.size(); i++) {
		BatteryTest* bt = (tests)[i];
		if (bt->getType() == testType && bt->getBatteryNo() == batteryNo)
		{
			return bt;
		}
	}
	return NULL;
}

void TestScheduler::notifyAboutTestEnd(int endMode)
{ 
	if (endMode != 0 && endMode != 3)//neither pass, nor interrupted by user (so either an error or fail)
	{
		this->status = 1;
	}
	Serial.println("SCHEDULER KNOWS: TEST END!");
	if (this->currentTest->getBatteryNo() == 1)
	{
		this->lastTestBat1 = currentTest;
	}
	else
	{
		this->lastTestBat2 = currentTest;
	}
	this->currentTest = NULL;
}

BatteryTest* TestScheduler::getLastTest(int batteryNo)
{
	if (batteryNo == 1)
	{
		return lastTestBat1;
	}
	if (batteryNo == 2)
	{
		return lastTestBat2;
	}
	return NULL;
}

void TestScheduler::notifyAboutTestStart(BatteryTest* _bt)
{
	this->currentTest = _bt;	
}

void TestScheduler::resetStatus()
{
	this->status = 0;
}

int TestScheduler::getStatus()
{
	return this->status;
}
