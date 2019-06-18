#include "DischargeTest.h"
#include "NTPManager.h"
#include "SerialManager.h"
#include <functional>
 using namespace std::placeholders; 

 DischargeTest::DischargeTest(int _batteryNo, TestScheduler* ts, Communicator* comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute)
{
    this->batteryNo = _batteryNo;
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
}

void DischargeTest::start(boolean scheduled)
{
	this->wasThisRunScheduled = scheduled;
	this->updatePeriod = 10;
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

void DischargeTest::handle()
{
	static int currentMeasurmentNumber = 0;

	if (state == STATE_RUNNING)
	{
		if (phase == PHASE_PREPARATION)
		{
			//Chart* ch = (Chart*)cont->getGUI()->find(getId()+"chLast");
			//ch->clear();
			failOnError(ElectronicLoad::connectBattery(this->batteryNo));
			failOnError(ElectronicLoad::setUpdatePeriod(10));
			failOnError(ElectronicLoad::setI(20));//TODO: pick from two
			Serial.println("prep phase");
			phase = PHASE_LOADING;
			return;
		}
		if (phase == PHASE_LOADING)
		{

			if (ElectronicLoad::areNewReadingsReady())
			{
				ElectronicLoad::getI(&currentI);
				ElectronicLoad::getU(&currentU, batteryNo);
				//current in amps times the number of seconds = capacity in ampseconds
				batteryCapacity += currentI * updatePeriod;//TODO: make sure the updatePeriod is correct even when the device is under heavy load
				extractedEnergy += updatePeriod * (currentI*currentU);

				//Chart* ch = (Chart*)cont->getGUI()->find(getId() + "chLast");//TODO: optimize
				double currTime = this->lastRunStart + ((millis() - startMillis) / (double)1000);
				double arr[] = { currTime,currentU };
				//ch->addPoint(ALL_CLIENTS, arr, 2);
				if (currentU < testEndVoltage||currentU<20)
				{
					batteryCapacity = batteryCapacity / 3600;
					extractedEnergy = extractedEnergy / 3600;
					if (batteryCapacity < minCapacity)
					{
						testFailed = true;
					}
					if (testFailed)
					{
						endTest(1);
					}
					else
					{
						endTest(0);
					}
				}

			}
		}
		

	}
}

String DischargeTest::getIntermediateResults()
{
	char res[200];
	sprintf(res, "Extracted energy:%.2fWh; Extracted capacity: %.2fAh; Last voltage: %.2f; Last current: %.2f", extractedEnergy/3600,batteryCapacity/3600,currentU,currentI);
	return String(res);

}

String DischargeTest::getName()
{
	return (String)"Discharge test of battery " + this->batteryNo;
}

void DischargeTest::generateTextResults()
{
	sprintf(textResults, "%sBattery capacity: <b>%.2f Ah</b>\r\n<br>Extracted energy: <b>%.2f Wh</b>\r\n<br>", getGenericLastTestInfo().c_str(), batteryCapacity, extractedEnergy);
}

void DischargeTest::reportResultsOnGUI()//TODO: do this
{
}

void DischargeTest::saveResults()
{
	char theLine[200];
	sprintf(theLine, "%s\t%.2f\t%.2f", NTPManager::dateToString(now()).c_str(), batteryCapacity, extractedEnergy);

	char fname[50];
    sprintf(fname, "%s.data", getId().c_str());
	SpiffsPersistentSettingsUtils::appendLineTo(fname, theLine);
}



int DischargeTest::getType()
{
	return 2;
}


void DischargeTest::generateGUI(Container * c)
{
	

	this->cont = c;
	vBox* vb = new vBox(getId()+"vb");//we create a new vertical box - the things in this box will go one under another
	//c->add(vb);//we add the vertical box inside the horizontal box we created
	
	
	
	Heading* h = new Heading(getId()+"h1", 1, "Discharge test of battery "+(String)batteryNo);//the heading
	//vb->add(h);//Always remember to actually add the elements somewhere!
	//Text* t = new Text(getId()+"desc", R"(This test simply measures the no-load voltage of the battery; it only takes a few seconds and can be used to estimate the state of charge.)");//We add some explanation
	//vb->add(t);
	
	TextInput* ti1 = new TextInput(getId() + (String)"failVoltage", "Minimum measured voltage before test fails");
	//vb->add(ti1);


	Text* lastResultsText = new Text(getId()+"lastResults", R"(Last results are something something)");
	//vb->add(lastResultsText);


	Chart* ch = new Chart(getId()+"chLast", "Last test results",true);
	//ch->setPersistency(true);

	//vb->add(ch);

	
	auto f1 = std::bind(&DischargeTest::startTestCallback, this, _1);
	Button* btnStartTest = new Button(getId()+"bStart", "Start test now" , f1);
	//vb->add(btnStartTest);

	//vb->add(btnStartTest);
	generateSchedulingGUI(vb, this->getId());

}

String DischargeTest::getId()
{
	return "dt_b" + (String)this->batteryNo;
}




void DischargeTest::startTestCallback(int user)
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


void DischargeTest::saveSettingsCallback(int user)
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


	String DischargeTest::setOptions(String opt1, String opt2="", String opt3="", String opt4="", String opt5="")
	{
		if ((opt1=="" ||opt2=="")|| opt3 != "" || opt4 != "" || opt4 != "")
		{
			return (String)"Usage: SETOPTIONS|DISCHARGE|" + batteryNo + "|(minimum capacity before the test fails)|(target voltage to discharge the battery to)";
		}
		if (parserUtils::retrieveFloat(opt1.c_str(), &minCapacity)<0)
		{
			return "Not a valid value for minimum capacity.";
		}

		float temp;
		int isFloatValid = !(parserUtils::retrieveFloat(opt2.c_str(), &temp) < 0);
		if (!isFloatValid || temp<20)
		{
			return "Not a valid value for target voltage. For safety reasons, the allowable hardcoded minimum is 20V.";
		}
		saveSettingsToSpiffs();
		return "";
	}


void DischargeTest::saveSettingsToSpiffs()
{
	char fname[50];
	sprintf(fname, "%s.cfg", getId().c_str());
	//char* fname = (char*)((String)prefix+".cfg").c_str();
	Serial.println(fname);
	StaticJsonBuffer<50> jsonBuffer;
	// Parse the root object
	JsonObject &root = jsonBuffer.createObject();
	// Set the values
	root["minC"] = minCapacity;
	root["minU"] = testEndVoltage;
	SpiffsPersistentSettingsUtils::saveSettings(root, fname);
}


void DischargeTest::loadSettingsFromSpiffs()
{

	Serial.println("nacitam nastaveni...");
	StaticJsonBuffer<1000> jb;
	StaticJsonBuffer<1000> *jbPtr = &jb;



		char fname[50];
		sprintf(fname, "%s.cfg", getId().c_str());
	Serial.println(fname);

	JsonObject& root = SpiffsPersistentSettingsUtils::loadSettings(jbPtr, fname);
	if (root["success"] == false)
	{
		Serial.println("failnulo to nacitani konkretniho nastaveni toho testu...");
	return;
	}
	Serial.println("nacetlo se konkretni nastaveni...");

	minCapacity = root["minC"];
	testEndVoltage = root["minU"];
}

String DischargeTest::getSettings()
{
	char returnStr[200];
	sprintf(returnStr,"Minimum capacity before failure: %.2fAh\nTarget voltage: %.2fV", minCapacity, testEndVoltage);
	return getSchedulingSettings()+String(returnStr);
}

void DischargeTest::printHistoricalResults()
{
	SerialManager::sendToOutputln("Date\t\tE_extr\tC_bat");
	BatteryTest::printHistoricalResults();
}
