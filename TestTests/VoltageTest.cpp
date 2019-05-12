#include "VoltageTest.h"
#include <functional>
 using namespace std::placeholders; 

 VoltageTest::VoltageTest(TestScheduler* ts, Communicator* comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute)
{
	ts->addTest(this);
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
	this->fastForwardScheduling();
}

void VoltageTest::handle()
{
	static int currentMeasurmentNumber = 0;
	/*
	if (state == STATE_SCHEDULED)
	{
		if (now() > this->scheduledStartTime)
		{
			beginTest(true);
		}
	}
	*/

	if (state == STATE_RUNNING)
	{
		if (phase == PHASE_PREPARATION)
		{
			//TODO: handle errors
			//int state=ElectronicLoad::connectBattery(this->batteryNo);
			currentMeasurmentNumber = 0;
			voltageSum = 0;
			ElectronicLoad::setUpdatePeriod(updatePeriod);
			Serial.println("prep phase");
			phase = PHASE_MEASURING;
			return;
		}
		if (phase == PHASE_MEASURING)
		{
			float currentU;//current as in immediate, might wanna relabel that...
			if (!ElectronicLoad::areNewReadingsReady())
			{
				return;
			}

			if (ElectronicLoad::getU(&currentU) == 0)
			{
				voltageSum += currentU;
			}
			currentMeasurmentNumber++;
			if (currentMeasurmentNumber >= MEASURMENTS_COUNT)
			{
				averageVoltage = voltageSum / currentMeasurmentNumber;

				testFailed = false;
				if (averageVoltage < failVoltageThreshold)
				{
					testFailed = true;
				}

				double arr[] = { now(),averageVoltage };

				Chart* ch = (Chart*)cont->getGUI()->find(this->getId()+"chLast");//TODO: optimize
				ch->addPoint(ALL_CLIENTS, arr,2);
				endTest();

				Text* t= (Text*)cont->getGUI()->find(this->getId() + "lastResults");
				t->setDefaultText(this->getTextResults());
				t->setText(ALL_CLIENTS, getTextResults());
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

	auto fStoreSettings = std::bind(&VoltageTest::saveSettingsCallback, this, _1);
	Button* btnStoreSettings = new Button(getId()+"btnStoreSettings", "Store settings as default" , fStoreSettings);
	vb->add(btnStoreSettings);


	Text* lastResultsText = new Text(getId()+"lastResults", R"(Last results are something something)");
	vb->add(lastResultsText);


	Chart* ch = new Chart(getId()+"chLast", "Last test results",true);
	ch->setPersistency(true);
	vb->add(ch);

	
	auto f1 = std::bind(& VoltageTest::startTestCallback, this, _1);
	Button* btnStartTest = new Button(getId()+"bStart", "Start test now" , f1);
	vb->add(btnStartTest);

	vb->add(btnStartTest);
	generateSchedulingGUI(vb, this->getId());

	loadSettingsFromSpiffs();



}
int VoltageTest::getType()
{
	return 0;
}


void VoltageTest::saveSettingsToSpiffs()
{
	//Serial.println("ten prefix je:");
		//Serial.println(prefix);

	char fname[50];
	sprintf(fname, "%s.cfg", getId().c_str());

	//char* fname = (char*)((String)prefix+".cfg").c_str();
	Serial.println(fname);

	StaticJsonBuffer<50> jsonBuffer;

	// Parse the root object
	JsonObject &root = jsonBuffer.createObject();

	// Set the values
	root["minU"] = failVoltageThreshold;
	SpiffsPersistentSettingsUtils::saveSettings(root, fname);
}

void VoltageTest::saveSettingsCallback(int user)
{
	GUI* gui = cont->getGUI();
	String s = gui->find(getId()+"failVoltage")->retrieveText(user);
	parserUtils::retrieveFloat(s.c_str(), &failVoltageThreshold);
	saveSettingsToSpiffs();
}

void VoltageTest::loadSettingsFromSpiffs()
{

	Serial.println("nacitam nastaveni...");
	StaticJsonBuffer<1000> jb;
	StaticJsonBuffer<1000> *jbPtr = &jb;



		char fname[50];
		sprintf(fname, "%s.cfg", prefix);
	Serial.println(fname);

	JsonObject& root = SpiffsPersistentSettingsUtils::loadSettings(jbPtr, fname);
	if (root["success"] == false)
	{
		Serial.println("failnulo to nacitani konkretniho nastaveni toho testu...");
	return;
	}
	Serial.println("nacetlo se konkretni nastaveni...");

	failVoltageThreshold= root["minU"];



	GUI* gui = this->cont->getGUI();
	gui->find(getId()+"failVoltage")->setDefaultText((String)failVoltageThreshold);
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

	if (this->state == STATE_RUNNING)
	{
		Serial.println("stopping");
		processRequestToStopTest(user);
	}
	else
	{
		Serial.println("beginning");
		beginTest(false);
	}

}

