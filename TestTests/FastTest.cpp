#include "FastTest.h"
#include <functional>
using namespace std::placeholders;

FastTest::FastTest(Communicator* comm, boolean scheduled, int firstRunYear, int firstRunMonth, int firstRunDay, int firstRunHour, int firstRunMinute, int periodDay, int periodHour, int periodMinute)
{
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
}

void FastTest::start(boolean scheduled, float loadCurrent)
{
	beginTest(scheduled);
}

void FastTest::updateChart()
{
	if (ElectronicLoad::areNewReadingsReady())
	{

		float currentU;
		ElectronicLoad::getU(&currentU);
		lastMeasuredU = currentU;
		Chart* ch = (Chart*)cont->getGUI()->find("chLastTestData");//TODO: optimize
		double currTime = this->lastRunStart + ((millis() - startMillis) / (double)1000);
		double arr[] = {currTime,currentU};
		ch->addPoint(ALL_CLIENTS, arr,2);
	}
}


void FastTest::handle()
{
	
	if (state == STATE_SCHEDULED)
	{
		if (now() > this->scheduledStartTime)
		{
			Serial.println("now");
			Serial.println(now());
			Serial.println("then");
			Serial.println(this->scheduledStartTime);
			start(true, this->scheduledLoadCurrent);
		}
	}
	if (state == STATE_RUNNING)
	{
		static int it;
		if (phase == PHASE_PREPARATION)
		{
			Serial.println("entering the loading phase...");
			Chart* ch = (Chart*)cont->getGUI()->find("chLastTestData");
			ch->clear();
			//TODO: handle errors
			int state=ElectronicLoad::connectBattery(this->batteryNo);
			ElectronicLoad::setUpdatePeriod(0.25);
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
				ElectronicLoad::setI(loadCurrent);
			}
		}
		if (phase == PHASE_LOADING)
		{
			updateChart();

			if (millis() - startMillis > 20000)//rollover-safe; see https://arduino.stackexchange.com/questions/12587/how-can-i-handle-the-millis-rollover
			{
				this->voltageWhenLoaded= this->lastMeasuredU;
				this->currentWhenLoaded = this->voltageWhenLoaded / 2;//TODO: actually measure the current...
				Serial.println("entering the recovery phase...");
				int state=ElectronicLoad::connectBattery(0);
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
				double loadingResistance = 2;//TODO: calculate from the current and stuff...
				this->internalResistance = ((this->voltageAtStart*loadingResistance)/voltageWhenLoaded)-loadingResistance;
				endTest();
			}
		}

	}
	
}

int FastTest::reportResults()
{
	
	if (this->emailReport == REPORT_MAIL_ONFINISHED || this->emailReport == REPORT_MAIL_ONSTARTANDFINISH)//TODO: or if we failed and should report on fail
	{
		
		comm->login();
		comm->sendHeader("TEST RESULTS");//TODO: make sure that this changes when we failed
		comm->printText(this->getTextResults());
		comm->exit();
		
		//Serial.println(getTextResults());
	}
	return 0;
	
}

void FastTest::generateGUI(Container * c)
{
	

	
	this->cont = c;
	vBox* vb = new vBox("vBox");//we create a new vertical box - the things in this box will go one under another
	c->add(vb);//we add the vertical box inside the horizontal box we created
	
	
	
	Heading* h = new Heading("heading1", 1, "Fast test of battery 1");//We create heading of level "1", name it "heading1" and change its text.
	vb->add(h);//Always remember to actually add the elements somewhere!
	Text* t = new Text("text1", R"(This test works by loading the battery for roughly 15 seconds and then letting it recover; it can be used to estimate the state of charge and also the internal resistance of the battery.)");//We add some explanation
	vb->add(t);


	Text* lastResultsText = new Text("lastResults", R"(Last results are something something)");
	vb->add(lastResultsText);

	TextInput* tiLoadCurrent = new TextInput("tiLoadCurrent", "Load current");
	vb->add(tiLoadCurrent);

	Chart* ch = new Chart("chLastTestData", "Last test results",true);
	ch->setPersistency(true);
	vb->add(ch);

	
	auto f1 = std::bind(&FastTest::startTestCallback, this, _1);
	Button* btnStartFastTest = new Button("btnStartFastTest", "Start test now" , f1);
	vb->add(btnStartFastTest);
	generateSchedulingGUI(vb, this->getId());


	

}


String FastTest::getTextResults()
{

	char specificResults[200];
	sprintf(specificResults, "Open-circuit voltage: \t<b>%.4fV</b><br>Voltage under load: \t<b>%.4fV</b><br>Internal resistance: \t<b>%.4f ohm</b>", this->voltageAtStart, this->voltageWhenLoaded, this->internalResistance);
	return this->getGenericLastTestInfo() + "\n"+specificResults;
}

String FastTest::getId()
{
	return (String)"ft_b" + batteryNo;
}

void FastTest::startTestCallback(int user)
{
	USE_SERIAL.println("starting test, weeeeeee");
	GUI* gui = cont->getGUI();
	String s = gui->find("tiLoadCurrent")->retrieveText(user);
	USE_SERIAL.println("s");
	USE_SERIAL.println(s);
	float loadCurrent;
	parserUtils::retrieveFloat(s.c_str(), &loadCurrent);

	long outArr[10];
	int n = parserUtils::retrieveNLongs("10:20:30:40:15", 10, outArr);
	start(false, loadCurrent);

}

void FastTest::saveSettingsCallback(int user)
{
	USE_SERIAL.println("Saving settings");
	
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
	
}
