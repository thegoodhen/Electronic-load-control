#include "VoltageTest.h"
#include <functional>
 using namespace std::placeholders; 

 VoltageTest::VoltageTest(Communicator* comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute)
{
	this->comm = comm;
	firstRunYear = CalendarYrToTm(firstRunYear);
	tmElements_t startDateElems = { 0,firstRunMinute,firstRunHour,1, firstRunDay,firstRunMonth,firstRunYear };
	this->firstScheduledStartTime = makeTime(startDateElems);
	this->scheduledStartTime = this->firstScheduledStartTime;
	this->firstScheduledStartTime = makeTime(startDateElems);
	this->state = STATE_SCHEDULED;


	tmElements_t periodElems = { 0,periodMinute,periodHour,1, periodDay+1, 1, 0};
	this->period = makeTime(periodElems);


	this->canRunAutomatically = scheduled;
}

void VoltageTest::start(boolean scheduled)
{
	this->wasThisRunScheduled = scheduled;
	//this->loadCurrent = loadCurrent;
	this->lastRunStart = now();
	this->startMillis = millis();
	this->phase = 0;
	this->state = STATE_RUNNING;

	if (scheduled)
	{

		while (this->scheduledStartTime < now())//skip all the missed scheduled times
		{
			this->scheduledStartTime += this->period;//schedule the next run of this test
		}
	}
}

void VoltageTest::handle()
{
	static int currentMeasurmentNumber = 0;
	if (state == STATE_SCHEDULED)
	{
		if (now() > this->scheduledStartTime)
		{
			start(true);
		}
	}
	if (state == STATE_RUNNING)
	{
		if (phase == PHASE_PREPARATION)
		{
			//TODO: handle errors
			int state=ElectronicLoad::connectBattery(this->batteryNo);
			ElectronicLoad::setUpdatePeriod(updatePeriod);
			Serial.println("prep phase");
			phase = PHASE_MEASURING;
			return;
		}
		if (phase == PHASE_MEASURING)
		{
			float currentU;//current as in immediate, might wanna relabel that...

			if (ElectronicLoad::getU(&currentU) == 0)
			{
				voltageSum += currentU;
			}
			currentMeasurmentNumber++;
			if (currentMeasurmentNumber >= MEASURMENTS_COUNT)
			{
				averageVoltage = voltageSum / currentMeasurmentNumber;
				if (averageVoltage < failVoltageThreshold)
				{
					testFailed = true;
				}
				endTest();
			}
		}
		

	}
}

int VoltageTest::reportResults()
{
		comm->login();
		comm->sendHeader((String)"BATTERY "+batteryNo+(String)" TEST RESULTS");//TODO: make sure that this changes when we failed
		comm->printText("Battery voltage test complete.<br>");
		comm->printText("Voltage average (out of "+(String)MEASURMENTS_COUNT+(String) " samples):  "+averageVoltage);
		comm->exit();
		
		//Serial.println(getTextResults());
	return 0;
}

void VoltageTest::generateGUI(Container * c)
{
	

	this->cont = c;
	vBox* vb = new vBox(getId()+"vb");//we create a new vertical box - the things in this box will go one under another
	c->add(vb);//we add the vertical box inside the horizontal box we created
	
	
	
	Heading* h = new Heading(getId()+"h1", 1, "Voltage test of battery "+(String)batteryNo);//the heading
	vb->add(h);//Always remember to actually add the elements somewhere!
	Text* t = new Text(getId()+"desc", R"(This test simply measures the no-load voltage of the battery; it only takes a few seconds and can be used to estimate the state of charge.)");//We add some explanation
	vb->add(t);
	
	TextInput* ti1 = new TextInput(getId() + (String)"failVoltage", "Minimum measured voltage before test fails");
	vb->add(ti1);


	Text* lastResultsText = new Text(getId()+"lastResults", R"(Last results are something something)");
	vb->add(lastResultsText);


	Chart* ch = new Chart(getId()+"chLast", "Last test results");
	vb->add(ch);

	
	auto f1 = std::bind(& VoltageTest::startTestCallback, this, _1);
	Button* btnStartTest = new Button(getId()+"bStart", "Start test now" , f1);
	vb->add(btnStartTest);

	vb->add(btnStartTest);
	generateSchedulingGUI(vb, this->getId());




}

String VoltageTest::getId()
{
	return "vt_b" + (String)this->batteryNo;
}

String VoltageTest::getTextResults()
{
	return "Vysledek je... SLEPICE! :3";
}

void VoltageTest::startTestCallback(int user)
{
	USE_SERIAL.println("starting test, weeeeeee");
	GUI* gui = cont->getGUI();

	start(false);

}

void VoltageTest::saveSettingsCallback(int user)
{
	USE_SERIAL.println("Saving settings");
	/*
	GUI* gui = cont->getGUI();
	//String s = gui->find("tiLoadCurrent")->retrieveText(user);

	String s = gui->find("tiFirstRun")->retrieveText(user);

	long outArr[5];
	int n = parserUtils::retrieveNLongs(s.c_str(), 5, outArr);
	if (n == 5)
	{
		this->setFirstScheduledStartTime(outArr[0], outArr[1], outArr[2], outArr[3], outArr[4]);
	}


	s = gui->find("tiPeriod")->retrieveText(user);


	n = parserUtils::retrieveNLongs(s.c_str(), 5, outArr);
	if (n == 3)
	{
		this->setSchedulingPeriod(outArr[0], outArr[1], outArr[2]);
	}
	*/
}