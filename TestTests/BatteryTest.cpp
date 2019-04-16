
#include "BatteryTest.h"
#include "TestScheduler.h"

#include <functional>

using namespace std::placeholders;	

void BatteryTest::setFirstScheduledStartTime(int day, int month, int year, int hour, int min)
{
	
	year = CalendarYrToTm(year);
	tmElements_t startDateElems = { 0,min,hour,1, day,month,year};
	this->firstScheduledStartTime = makeTime(startDateElems);
	this->scheduledStartTime = this->firstScheduledStartTime;
	Serial.println("scheduled start time set...");
}

void BatteryTest::setSchedulingPeriod(int days, int hours, int minutes)
{
	tmElements_t periodElems = { 0,minutes,hours,1, days+1, 1, 0};
	this->period = makeTime(periodElems);
}

void BatteryTest::beginTest(boolean scheduled)
{

	this->scheduler->notifyAboutTestStart(this);
	this->wasThisRunScheduled = scheduled;
	//this->loadCurrent = loadCurrent;
	this->lastRunStart = now();
	this->startMillis = millis();
	this->phase = 0;
	this->state = STATE_RUNNING;
	this->testFailed = false;

	if (scheduled)
	{
		Serial.println("prev start time");
		Serial.println(this->scheduledStartTime);
		Serial.println("new start time");
		Serial.println(this->scheduledStartTime+this->period);
		Serial.println("the period");
		Serial.println(period);
		
		fastForwardScheduling();

	}
}

void BatteryTest::fastForwardScheduling()
{

		while (this->scheduledStartTime < now())//skip all the missed scheduled times
		{
			this->scheduledStartTime += this->period;//schedule the next run of this test
		}
}

void BatteryTest::endTest()
{
	this->scheduler->notifyAboutTestEnd();
	ElectronicLoad::connectBattery(0);
	ElectronicLoad::setI(0);
	ElectronicLoad::setUpdatePeriod(0);
	this->lastRunDuration = now() - this->lastRunStart;
	
	if ((emailReport == REPORT_MAIL_ONFAIL && testFailed) || emailReport == REPORT_MAIL_ONFINISHED)
	{
		reportResults();
	}
				Serial.println("end of test");
				if (canRunAutomatically)
				{
					state = STATE_SCHEDULED;
				}
				else
				{
					state = STATE_STOPPED;
				}
}

void BatteryTest::generateSchedulingGUI(Container* c, String _prefix)
{
	_prefix.toCharArray(prefix, 50);
	//this->schedulingGUIPrefix = prefix;
	this->cont = c;

	Heading* hSchedulingProps = new Heading(_prefix+"hSch", 2, "Scheduling settings");
	c->add(hSchedulingProps);

	TextInput* tiFirstRun = new TextInput(_prefix+"tifr", "the time and date of the first scheduled run (DD.MM.YYYY HH:MM)");
	c->add(tiFirstRun);


	TextInput* tiPeriod= new TextInput(_prefix+"tiP", "the period between two consecutive scheduled runs (DD:HH:MM)");
	c->add(tiPeriod);

	Checkbox* cbIncludeResult= new Checkbox(_prefix+"cbir", "Store historical test results");
	c->add(cbIncludeResult);


	//Slider* s = new Slider("sl1", "Some slider: ");
	//vb->add(s);

	
	ListBox* lbMailSettings = new ListBox(_prefix+"lbMail", "email settings");
	//CAUTION: the items are in a specific order; reodering the following lines will result in a fatal malfunction
	lbMailSettings->addItem(new ListItem("Do not send an email"));
	lbMailSettings->addItem(new ListItem("Only notify on fail"));
	lbMailSettings->addItem(new ListItem("Notify when finished"));
	lbMailSettings->addItem(new ListItem("Notify about start and finish"));
	c->add(lbMailSettings);

	
	
	
	

	auto f2 = std::bind(& BatteryTest::saveSchSettingsCallback, this, _1);

	

	Button* btnSaveSettings= new Button(_prefix+"_saveScSettings", "Save", f2);

	
	c->add(btnSaveSettings);
	
	
	Button* btnRecallSettings= new Button(_prefix+"_recallSchSettings", "Recall stored settings", NULL);
	c->add(btnRecallSettings);
	loadSchSettingsFromSpiffs();


	//the following lines fix everything... how very bizzare indeed!
	//Text* slepicelastResultsText = new Text("slepicelastResults", R"(Last results are something something)");
	//vb->add(slepicelastResultsText);
	
}

	void BatteryTest::saveSchSettingsCallback(int user)
	{

		GUI* gui = this->cont->getGUI();
		Serial.println("Saving scheduling settings");

		
		String firstRun = gui->find((String)prefix+ "tifr")->retrieveText(user);
		firstRun.toCharArray(config.firstRun, 50);


		String period = gui->find((String)prefix + "tiP")->retrieveText(user);
		period.toCharArray(config.runPeriod, 50);

		int storeResults = gui->find((String)prefix + "cbir")->retrieveIntValue(user);
		config.storeResults = storeResults;

		int mailSettings = gui->find((String)prefix + "lbMail")->retrieveIntValue(user);
		config.mailSettings = mailSettings;
		saveSchSettingsToSpiffs();
		


	}

	
void BatteryTest::saveSchSettingsToSpiffs()
{
	Serial.println("ten prefix je:");
		Serial.println(prefix);

		char fname[50];
		sprintf(fname, "%s_sch.cfg", prefix);

	//char* fname = (char*)((String)prefix+".cfg").c_str();
	Serial.println(fname);

	StaticJsonBuffer<350> jsonBuffer;

	// Parse the root object
	JsonObject &root = jsonBuffer.createObject();

	// Set the values
	root["firstRun"] = config.firstRun;
	root["runPeriod"] = config.runPeriod;
	root["storeResults"] = config.storeResults;
	root["mailSettings"] = config.mailSettings;
	parseLoadedSettings();
	SpiffsPersistentSettingsUtils::saveSettings(root, fname);
}

void BatteryTest::loadSchSettingsFromSpiffs()
{

	Serial.println("nacitam nastaveni...");
	StaticJsonBuffer<1000> jb;
	StaticJsonBuffer<1000> *jbPtr = &jb;



		char fname[50];
		sprintf(fname, "%s_sch.cfg", prefix);
	Serial.println(fname);

	JsonObject& root = SpiffsPersistentSettingsUtils::loadSettings(jbPtr, fname);
	if (root["success"] == false)
	{
		Serial.println("failnulo to nacitani toho testu...");
	return;
	}
	Serial.println("nacetlo se nastaveni...");

	strlcpy(config.firstRun,                   // <- destination
		root["firstRun"],
		sizeof(config.firstRun));

	strlcpy(config.runPeriod,                   // <- destination
		root["runPeriod"],
		sizeof(config.runPeriod));

	config.storeResults = root["storeResults"];
	config.mailSettings= root["mailSettings"];
	parseLoadedSettings();




	GUI* gui = this->cont->getGUI();

	gui->find((String)prefix+"tifr")->setDefaultText(config.firstRun);
	gui->find((String)prefix+"tiP")->setDefaultText(config.runPeriod);
	gui->find((String)prefix + "cbir")->setDefaultIntValue(config.storeResults);
	gui->find((String)prefix + "lbMail")->setDefaultIntValue(config.mailSettings);
	//gui->find((String)prefix+"cbir")
	//gui->find((String)prefix+"lbMail")->setDefaultText(config.mailSettings);
}

void BatteryTest::parseLoadedSettings()
{

	long outArr[5];
	int n = parserUtils::retrieveNLongs(config.firstRun, 5, outArr);
	if (n == 5)
	{
		this->setFirstScheduledStartTime(outArr[0], outArr[1], outArr[2], outArr[3], outArr[4]);
	}

	n = parserUtils::retrieveNLongs(config.runPeriod, 5, outArr);
	if (n == 3)
	{
		this->setSchedulingPeriod(outArr[0], outArr[1], outArr[2]);
	}
	this->emailReport = config.mailSettings;
}

String BatteryTest::dateToString(time_t _theDate)
{
	char returnString[40];
	sprintf(returnString, "%d.%d. %d %d:%d:%d", day(_theDate), month(_theDate), year(_theDate), hour(_theDate), minute(_theDate), second(_theDate));
	Serial.println("returnString");
	Serial.println(returnString);
	return (String)returnString;
	
}

String BatteryTest::getGenericLastTestInfo()
{
	return (String)"start: " + this->dateToString(this->lastRunStart) + "<br>"
		+ "end: " + this->dateToString(this->lastRunStart + this->lastRunDuration) + "<br>" +
		"status: " + ((!this->testFailed)?"PASSED<br>":"<span style=\"color:#FF0000;\">FAILED</span><br>");
}
void BatteryTest::setScheduler(TestScheduler* _sch)
{
	this->scheduler = _sch;
}

time_t BatteryTest::getScheduledStartTime()
{
	return this->scheduledStartTime;
}

void BatteryTest::processRequestToStopTest(int userNo)
{
	static unsigned long lastMillis;
	static boolean wasRequestedAlready=false;
	if (wasRequestedAlready && (millis()-lastMillis<5000))
	{
		wasRequestedAlready = false;
		GUI* gui = cont->getGUI();
		gui->showInfo(userNo, "Test aborted.");
		endTest();
	}
	else
	{
		wasRequestedAlready = true;
		lastMillis = millis();
		GUI* gui = cont->getGUI();
		gui->showInfo(userNo, "Click again to abort test.");

	}
	
}

