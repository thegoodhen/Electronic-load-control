#include "VoltageTest.h"
#include "NTPManager.h"
#include "SerialManager.h"
#include <functional>
 using namespace std::placeholders; 

 VoltageTest::VoltageTest(Battery* b, TestScheduler* ts, Communicator* comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute)
{
	 this->bat = b;
	 this->batteryNo = b->getNumber();
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


	this->loadSettingsFromSpiffs();
	this->loadSchSettingsFromSpiffs();
	
	this->canRunAutomatically = scheduled;
	this->fastForwardScheduling();
}

 void VoltageTest::generateTextResults()
 {
	sprintf(textResults, "%s\nOpen-circuit voltage (out of %d samples): \t%.4fV\r\n", this->getGenericLastTestInfo().c_str(), MEASURMENTS_COUNT, averageVoltage);
 }

 void VoltageTest::reportResultsOnGUI()
 {
	 /*
	 double arr[] = { now(),averageVoltage };
	 Chart* ch = (Chart*)cont->getGUI()->find(this->getId() + "chLast");//TODO: optimize
	 ch->addPoint(ALL_CLIENTS, arr, 2);

	 Text* t = (Text*)cont->getGUI()->find(this->getId() + "lastResults");
	 t->setDefaultText(this->getTextResults());
	 t->setText(ALL_CLIENTS, getTextResults());
	 */
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
			failOnError(ElectronicLoad::setUpdatePeriod(updatePeriod));
			SerialManager::debugPrintln("prep phase");
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

			if (ElectronicLoad::getU(&currentU, batteryNo) == 0)
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


				bat->updateProperthies(averageVoltage, -1, -1);
				endTest(testFailed);

			}
		}
		

	}
}


void VoltageTest::generateGUI(Container * c)
{
	

	this->cont = c;
	vBox* vb = new vBox(getId()+"vb");//we create a new vertical box - the things in this box will go one under another
	//c->add(vb);//we add the vertical box inside the horizontal box we created
	
	
	
	Heading* h = new Heading(getId()+"h1", 1, "Voltage test of battery "+(String)batteryNo);//the heading
	//vb->add(h);//Always remember to actually add the elements somewhere!
	Text* t = new Text(getId()+"desc", R"(This test simply measures the no-load voltage of the battery; it only takes a few seconds and can be used to estimate the state of charge.)");//We add some explanation
	//vb->add(t);
	
	TextInput* ti1 = new TextInput(getId() + (String)"failVoltage", "Minimum measured voltage before test fails");
	//vb->add(ti1);

	auto fStoreSettings = std::bind(&VoltageTest::saveSettingsCallback, this, _1);
	Button* btnStoreSettings = new Button(getId()+"btnStoreSettings", "Store settings as default" , fStoreSettings);
	//vb->add(btnStoreSettings);


	Text* lastResultsText = new Text(getId()+"lastResults", R"(Last results are something something)");
	//vb->add(lastResultsText);


	Chart* ch = new Chart(getId()+"chLast", "Last test results",true);
	//ch->setPersistency(true);
	//vb->add(ch);

	
	auto f1 = std::bind(& VoltageTest::startTestCallback, this, _1);
	Button* btnStartTest = new Button(getId()+"bStart", "Start test now" , f1);
	//vb->add(btnStartTest);

	//vb->add(btnStartTest);
	//generateSchedulingGUI(vb, this->getId());

	//loadSettingsFromSpiffs();



}
int VoltageTest::getType()
{
	return 0;
}

String VoltageTest::getName()
{
	return (String)"Voltage test of battery " + this->batteryNo;
}

void VoltageTest::saveSettingsToSpiffs()
{
	//SerialManager::debugPrintln("ten prefix je:");
		//SerialManager::debugPrintln(prefix);

	char fname[50];
	sprintf(fname, "%s.cfg", getId().c_str());

	//char* fname = (char*)((String)prefix+".cfg").c_str();
	SerialManager::debugPrintln(fname);

	StaticJsonBuffer<50> jsonBuffer;

	// Parse the root object
	JsonObject &root = jsonBuffer.createObject();

	// Set the values
	root["minU"] = failVoltageThreshold;
	SpiffsManager::saveSettings(root, fname);
}


	String VoltageTest::setOptions(String opt1, String opt2="", String opt3="", String opt4="", String opt5="")
	{
		if (opt1=="" || opt2 != "" || opt3 != "" || opt4 != "" || opt4 != "")
		{
			return (String)"Usage: SETOPTIONS|VOLTAGE|" + batteryNo + "|(minimum open circuit voltage before the test fails)";
		}
		if (parserUtils::retrieveFloat(opt1.c_str(), &failVoltageThreshold)<0)
		{
			return "Not a valid value for minimum voltage.";
		}
		saveSettingsToSpiffs();
		return "";

	}

void VoltageTest::saveSettingsCallback(int user)
{
	GUI* gui = cont->getGUI();
	String s = gui->find(getId()+"failVoltage")->retrieveText(user);
	setOptions(s);
}

void VoltageTest::loadSettingsFromSpiffs()
{

	SerialManager::debugPrintln("nacitam nastaveni...");
	StaticJsonBuffer<1000> jb;
	StaticJsonBuffer<1000> *jbPtr = &jb;



		char fname[50];
		sprintf(fname, "%s.cfg", getId().c_str());
	SerialManager::debugPrintln(fname);

	JsonObject& root = SpiffsManager::loadSettings(jbPtr, fname);
	if (root["success"] == false)
	{
		SerialManager::debugPrintln("failnulo to nacitani konkretniho nastaveni toho testu...");
	return;
	}
	SerialManager::debugPrintln("nacetlo se konkretni nastaveni...");

	failVoltageThreshold= root["minU"];
}


String VoltageTest::getId()
{
	return "vt_b" + (String)this->batteryNo;
}



void VoltageTest::saveResults()
{
	char theLine[200];
	sprintf(theLine, "%s\t%.3f", NTPManager::dateToString(now()).c_str(),averageVoltage);

	char fname[50];
    sprintf(fname, "%s.data", getId().c_str());
	SpiffsManager::appendLineTo(fname, theLine);
}

void VoltageTest::startTestCallback(int user)
{
	GUI* gui = cont->getGUI();

	if (this->state == STATE_RUNNING)
	{
		SerialManager::debugPrintln("stopping");
		processRequestToStopTest(user);
	}
	else
	{
		SerialManager::debugPrintln("beginning");
		beginTest(false);
	}

}

String VoltageTest::getSettings()
{
	char returnStr[200];
	sprintf(returnStr,"Minimum open-circuit vtg. before failure: %.2fV", failVoltageThreshold);
	return getSchedulingSettings()+String(returnStr);
}

String VoltageTest::getIntermediateResults()
{
	return "Voltage test in progress...";
}

void VoltageTest::printHistoricalResults()
{
	SerialManager::sendToOutputln("Date\t\tU_noload");
	BatteryTest::printHistoricalResults();
}
