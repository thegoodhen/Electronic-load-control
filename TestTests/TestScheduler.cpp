
#include "TestScheduler.h"
#include "BatteryTest.h"

void TestScheduler::addTest(BatteryTest * bt)
{
	tests.push_back(bt);
	bt->setScheduler(this);
}

void TestScheduler::handle()
{
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

void TestScheduler::notifyAboutTestEnd()
{
	Serial.println("SCHEDULER KNOWS: TEST END!");
	this->currentTest = NULL;
}

void TestScheduler::notifyAboutTestStart(BatteryTest* _bt)
{
	this->currentTest = _bt;	
}
