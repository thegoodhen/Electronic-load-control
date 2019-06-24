#include "FastTest.h"
#include <functional>
#include "NTPManager.h"
#include "SerialManager.h"
using namespace std::placeholders;

FastTest::FastTest(int _batteryNo, TestScheduler* ts, Communicator* comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute)
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


	tmElements_t periodElems = { 0,periodMinute,periodHour,1, periodDay + 1, 1, 0 };
	this->period = makeTime(periodElems);


	this->loadSettingsFromSpiffs();
	this->loadSchSettingsFromSpiffs();
	this->canRunAutomatically = scheduled;
	this->fastForwardScheduling();
}


void FastTest::updateChart()
{
	if (ElectronicLoad::areNewReadingsReady())
	{

		float currentU;
		float currentI;
		ElectronicLoad::getU(&currentU, batteryNo);
		ElectronicLoad::getI(&currentI);
		lastMeasuredU = currentU;
		lastMeasuredI = currentI;
		//Chart* ch = (Chart*)cont->getGUI()->find(getId()+"chLastTestData");//TODO: optimize
		double currTime = this->lastRunStart + ((millis() - startMillis) / (double)1000);
		double arr[] = {currTime,currentU};
		//ch->addPoint(ALL_CLIENTS, arr,2);
	}
}

String FastTest::getName()
{
	return (String)"Fast test of battery " + this->batteryNo;
}

void FastTest::handle()
{
	
	/*
	if (state == STATE_SCHEDULED)
	{
		if (now() > this->scheduledStartTime)
		{
			SerialManager::debugPrintln("now");
			SerialManager::debugPrintln(now());
			SerialManager::debugPrintln("then");
			SerialManager::debugPrintln(this->scheduledStartTime);
			beginTest(true);
		}
	}
	*/
	if (state == STATE_RUNNING)
	{
		static int it;
		if (phase == PHASE_PREPARATION)
		{
			SerialManager::debugPrintln("entering the loading phase...");
			//Chart* ch = (Chart*)cont->getGUI()->find(getId()+"chLastTestData");
			//ch->clear();
			//TODO: handle errors
			failOnError(ElectronicLoad::connectBattery(this->batteryNo));
			failOnError(ElectronicLoad::setUpdatePeriod(0.25));
			phase = PHASE_NOLOAD;
			return;
		}
		if (phase == PHASE_NOLOAD)
		{
			updateChart();
			if (millis() - startMillis > 5000)
			{
				voltageAtStart = this->lastMeasuredU;
				phase = PHASE_LOADING;
				failOnError(ElectronicLoad::setI(loadCurrent));
			}
		}
		if (phase == PHASE_LOADING)
		{
			updateChart();

			if (millis() - startMillis > 20000)//rollover-safe; see https://arduino.stackexchange.com/questions/12587/how-can-i-handle-the-millis-rollover
			{
				this->voltageWhenLoaded= this->lastMeasuredU;
				this->currentWhenLoaded = this->lastMeasuredI;
				SerialManager::debugPrintln("entering the recovery phase...");
				failOnError(ElectronicLoad::connectBattery(0));
				phase = PHASE_RECOVERY;
				return;
			}
		}
		if (phase == PHASE_RECOVERY)
		{

			float currentI;
			float currentU;
			//TODO: handle errors
			updateChart();
			

			if (millis() - startMillis > 35000)//rollover-safe; see https://arduino.stackexchange.com/questions/12587/how-can-i-handle-the-millis-rollover
			{
				//end of the test
				this->voltageAtEnd = this->lastMeasuredU;
				double loadingResistance = voltageWhenLoaded / currentWhenLoaded;
				this->internalResistance = ((this->voltageAtStart*loadingResistance)/voltageWhenLoaded)-loadingResistance;

				this->testFailed = false;
				if (internalResistance > this->maxRiBeforeFail)
				{
					this->testFailed = true;
				}

				Battery::get(batteryNo)->updateProperthies(voltageAtStart, internalResistance, -1);
				endTest(testFailed);



			}
		}

	}
	
}


void FastTest::reportResultsOnGUI()
{

	double arr[] = { now(),voltageAtStart,voltageWhenLoaded,voltageAtEnd,internalResistance };

	Chart* ch = (Chart*)cont->getGUI()->find(this->getId() + "chLast");
	ch->addPoint(ALL_CLIENTS, arr, 5);
	Text* t = (Text*)cont->getGUI()->find(this->getId() + "lastResults");
	delay(500);
	t->setDefaultText(this->getTextResults());
	//SerialManager::debugPrintln("getTextResults()");
	//SerialManager::debugPrintln(strlen(getTextResults()));
	//SerialManager::debugPrintln(getTextResults());

	//t->setText(ALL_CLIENTS, getTextResults());
    //t->setText(ALL_CLIENTS, (String)""+getTextResults());

}


int FastTest::getType()
{
	return 1;
}


void FastTest::generateGUI(Container * c)
{
	

	
	this->cont = c;
	vBox* vb = new vBox(getId()+"vBox");//we create a new vertical box - the things in this box will go one under another
	//c->add(vb);//we add the vertical box inside the horizontal box we created
	
	
	
	//Heading* h = new Heading(getId()+"heading1", 1, "Fast test of battery 1");//We create heading of level "1", name it "heading1" and change its text.
	//vb->add(h);//Always remember to actually add the elements somewhere!
	Text* t = new Text(getId()+"text1", R"(This test works by loading the battery for roughly 15 seconds and then letting it recover; it can be used to estimate the state of charge and also the internal resistance of the battery.)");//We add some explanation
	//vb->add(t);


	//Text* lastResultsText = new Text(getId()+"lastResults", R"(Last results are something something)");
	//vb->add(lastResultsText);

	TextInput* tiMaxRiBeforeFail= new TextInput(getId()+"tiMaxRiBeforeFail", "Max Ri before fail");
	//vb->add(tiMaxRiBeforeFail);

	auto fStoreSettings = std::bind(&FastTest::saveSettingsCallback, this, _1);
	Button* btnStoreSettings = new Button(getId()+"btnStoreSettings", "Store settings as default" , fStoreSettings);
	//vb->add(btnStoreSettings);

	Chart* ch = new Chart(getId()+"chLastTestData", "Last test results",true,"time","voltage");
	//ch->setPersistency(true);
	//vb->add(ch);

	Chart* chHist = new Chart(getId()+"chLast", "Historical results",true,"time","OC voltage","Voltage under load","Recovery vtg","Internal resistance");
	//chHist->setPersistency(true);
	//vb->add(chHist);

	
	auto f1 = std::bind(&FastTest::startTestCallback, this, _1);
	Button* btnStartFastTest = new Button("btnStartFastTest", "Start test now" , f1);
	//vb->add(btnStartFastTest);
	//yield();
	//generateSchedulingGUI(vb, this->getId());
	//yield();
	//loadSettingsFromSpiffs();
}

void FastTest::saveSettingsToSpiffs()
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
	root["maxRi"] = maxRiBeforeFail;
	SpiffsManager::saveSettings(root, fname);
}

void FastTest::loadSettingsFromSpiffs()
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

	maxRiBeforeFail = root["maxRi"];
	SerialManager::debugPrintln("slepice");



	//GUI* gui = this->cont->getGUI();
	//gui->find(getId()+"tiMaxRiBeforeFail")->setDefaultText((String)maxRiBeforeFail);
}



void FastTest::saveResults()
{
	char theLine[200];
	sprintf(theLine, "%s\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f", NTPManager::dateToString(now()).c_str(),internalResistance, voltageAtStart, voltageWhenLoaded, voltageAtEnd, currentWhenLoaded);

	char fname[50];
    sprintf(fname, "%s.data", getId().c_str());
	SpiffsManager::appendLineTo(fname, theLine);
}


	String FastTest::setOptions(String opt1, String opt2="", String opt3="", String opt4="", String opt5="")
	{
		if (opt1=="" || opt2 != "" || opt3 != "" || opt4 != "" || opt4 != "")
		{
			return (String)"Usage: SETOPTIONS|FAST|" + batteryNo + "|(maximum internal resistance before the test fails)";
		}
		if (parserUtils::retrieveFloat(opt1.c_str(), &maxRiBeforeFail)<0)
		{
			return "Not a valid value for minimum internal resistance.";
		}
		saveSettingsToSpiffs();
		return "";

	}


void FastTest::generateTextResults()
{

	sprintf(textResults, "%s\r\nOpen-circuit voltage: \t<b>%.4fV</b>\r\n<br>Voltage under load: \t\t<b>%.4fV</b>\r\n<br>Current under load: \t\t%.4f A\r\n<br>Internal resistance: \t\t<b>%.4f ohm</b>\r\n", this->getGenericLastTestInfo().c_str(), this->voltageAtStart, this->voltageWhenLoaded, this->currentWhenLoaded, this->internalResistance);
}

String FastTest::getIntermediateResults()
{
	if (phase == PHASE_NOLOAD)
	{
		return "Measuring open-circuit voltage.";
	}

	if (phase == PHASE_LOADING)
	{
		return "Measuring the battery response when loaded...";
	}

	if (phase == PHASE_RECOVERY)
	{
		return "Measuring the battery recovery when the load is released...";
	}
	return "Fast test in progress...";

}


String FastTest::getId()
{
	return (String)"ft_b" + batteryNo;
}

/*
void FastTest::startTestCallback(int user)
{
	USE_SerialManager::debugPrintln("starting test, weeeeeee");
	GUI* gui = cont->getGUI();
	//String s = gui->find("tiLoadCurrent")->retrieveText(user);
	//USE_SerialManager::debugPrintln("s");
	//USE_SerialManager::debugPrintln(s);
	//float loadCurrent;
	//parserUtils::retrieveFloat(s.c_str(), &loadCurrent);

	long outArr[10];
	int n = parserUtils::retrieveNLongs("10:20:30:40:15", 10, outArr);
	beginTest(false);

}
*/


void FastTest::startTestCallback(int user)
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

void FastTest::saveSettingsCallback(int user)
{
	GUI* gui = cont->getGUI();
	String s = gui->find(getId()+"tiMaxRiBeforeFail")->retrieveText(user);
	setOptions(s);
}

String FastTest::getSettings()
{
	char returnStr[200];
	sprintf(returnStr,"Minimum Ri before failure: \t\t%.2fOhms", maxRiBeforeFail);
	return getSchedulingSettings()+String(returnStr);
}

void FastTest::printHistoricalResults()
{
	SerialManager::sendToOutputln("Date\t\tR_i\tU_start\tU_load\tU_rcvr\tI_load");
	BatteryTest::printHistoricalResults();
}
