#include "FastTest.h"
#include <functional>
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
	Serial.println(year(firstScheduledStartTime));
	Serial.println(month(firstScheduledStartTime));
	Serial.println(day(firstScheduledStartTime));
	Serial.println(hour(firstScheduledStartTime));
	Serial.println(minute(firstScheduledStartTime));

	this->firstScheduledStartTime = makeTime(startDateElems);
	this->state = STATE_SCHEDULED;


	tmElements_t periodElems = { 0,periodMinute,periodHour,1, periodDay + 1, 1, 0 };
	this->period = makeTime(periodElems);


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
		Chart* ch = (Chart*)cont->getGUI()->find(getId()+"chLastTestData");//TODO: optimize
		double currTime = this->lastRunStart + ((millis() - startMillis) / (double)1000);
		double arr[] = {currTime,currentU};
		ch->addPoint(ALL_CLIENTS, arr,2);
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
			Serial.println("now");
			Serial.println(now());
			Serial.println("then");
			Serial.println(this->scheduledStartTime);
			beginTest(true);
		}
	}
	*/
	if (state == STATE_RUNNING)
	{
		static int it;
		if (phase == PHASE_PREPARATION)
		{
			Serial.println("entering the loading phase...");
			Chart* ch = (Chart*)cont->getGUI()->find(getId()+"chLastTestData");
			ch->clear();
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
				Serial.println("entering the recovery phase...");
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
				if(testFailed)
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

	String FastTest::setOptions(String opt1, String opt2="", String opt3="", String opt4="", String opt5="")
	{
		if (opt2 != "" || opt3 != "" || opt4 != "" || opt4 != "")
		{
			return (String)"Usage: SETOPTIONS|FAST|" + batteryNo + "|(maximum Ri before test fails)";
		}
		if (parserUtils::retrieveFloat(opt1.c_str(), &maxRiBeforeFail)<0)
		{
			return "Not a valid value for internal resistance.";
		}
		saveSettingsToSpiffs();
		return "";

	}

void FastTest::reportResultsOnGUI()
{

	double arr[] = { now(),voltageAtStart,voltageWhenLoaded,voltageAtEnd,internalResistance };

	Chart* ch = (Chart*)cont->getGUI()->find(this->getId() + "chLast");
	ch->addPoint(ALL_CLIENTS, arr, 5);
	Text* t = (Text*)cont->getGUI()->find(this->getId() + "lastResults");
	delay(500);
	t->setDefaultText(this->getTextResults());
	//Serial.println("getTextResults()");
	//Serial.println(strlen(getTextResults()));
	//Serial.println(getTextResults());

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
	c->add(vb);//we add the vertical box inside the horizontal box we created
	
	
	
	Heading* h = new Heading(getId()+"heading1", 1, "Fast test of battery 1");//We create heading of level "1", name it "heading1" and change its text.
	vb->add(h);//Always remember to actually add the elements somewhere!
	Text* t = new Text(getId()+"text1", R"(This test works by loading the battery for roughly 15 seconds and then letting it recover; it can be used to estimate the state of charge and also the internal resistance of the battery.)");//We add some explanation
	vb->add(t);


	Text* lastResultsText = new Text(getId()+"lastResults", R"(Last results are something something)");
	vb->add(lastResultsText);

	TextInput* tiMaxRiBeforeFail= new TextInput(getId()+"tiMaxRiBeforeFail", "Max Ri before fail");
	vb->add(tiMaxRiBeforeFail);

	auto fStoreSettings = std::bind(&FastTest::saveSettingsCallback, this, _1);
	Button* btnStoreSettings = new Button(getId()+"btnStoreSettings", "Store settings as default" , fStoreSettings);
	vb->add(btnStoreSettings);

	Chart* ch = new Chart(getId()+"chLastTestData", "Last test results",true,"time","voltage");
	ch->setPersistency(true);
	vb->add(ch);

	Chart* chHist = new Chart(getId()+"chLast", "Historical results",true,"time","OC voltage","Voltage under load","Recovery vtg","Internal resistance");
	chHist->setPersistency(true);
	vb->add(chHist);

	
	auto f1 = std::bind(&FastTest::startTestCallback, this, _1);
	Button* btnStartFastTest = new Button("btnStartFastTest", "Start test now" , f1);
	vb->add(btnStartFastTest);
	yield();
	//generateSchedulingGUI(vb, this->getId());
	yield();
	loadSettingsFromSpiffs();
}

void FastTest::saveSettingsToSpiffs()
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
	root["maxRi"] = maxRiBeforeFail;
	SpiffsPersistentSettingsUtils::saveSettings(root, fname);
}

void FastTest::loadSettingsFromSpiffs()
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

	maxRiBeforeFail = root["maxRi"];



	GUI* gui = this->cont->getGUI();
	gui->find(getId()+"tiMaxRiBeforeFail")->setDefaultText((String)maxRiBeforeFail);
}



void FastTest::generateTextResults()
{

	sprintf(textResults, "%s\nOpen-circuit voltage: \t<b>%.4fV</b>\n<br>Voltage under load: \t<b>%.4fV</b>\n<br>Current under load: %.4f A\n<br>Internal resistance: \t<b>%.4f ohm</b>\n", this->getGenericLastTestInfo().c_str(), this->voltageAtStart, this->voltageWhenLoaded, this->currentWhenLoaded, this->internalResistance);
}

String FastTest::getId()
{
	return (String)"ft_b" + batteryNo;
}

/*
void FastTest::startTestCallback(int user)
{
	USE_SERIAL.println("starting test, weeeeeee");
	GUI* gui = cont->getGUI();
	//String s = gui->find("tiLoadCurrent")->retrieveText(user);
	//USE_SERIAL.println("s");
	//USE_SERIAL.println(s);
	//float loadCurrent;
	//parserUtils::retrieveFloat(s.c_str(), &loadCurrent);

	long outArr[10];
	int n = parserUtils::retrieveNLongs("10:20:30:40:15", 10, outArr);
	beginTest(false);

}
*/


void FastTest::startTestCallback(int user)
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

void FastTest::saveSettingsCallback(int user)
{
	GUI* gui = cont->getGUI();
	String s = gui->find(getId()+"tiMaxRiBeforeFail")->retrieveText(user);
	setOptions(s);
}
