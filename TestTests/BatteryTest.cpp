
#include "BatteryTest.h"
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

		while (this->scheduledStartTime < now())//skip all the missed scheduled times
		{
			this->scheduledStartTime += this->period;//schedule the next run of this test
		}
	}
}

void BatteryTest::endTest()
{
	ElectronicLoad::connectBattery(-1);
	
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

	TextInput* tiFirstRun = new TextInput(_prefix+"tifr", "the time and date of the first scheduled run");
	c->add(tiFirstRun);


	TextInput* tiPeriod= new TextInput(_prefix+"tiP", "the period between two consecutive scheduled runs");
	c->add(tiPeriod);

	Checkbox* cbIncludeResult= new Checkbox(_prefix+"cbir", "Store historical test results");
	c->add(cbIncludeResult);


	//Slider* s = new Slider("sl1", "Some slider: ");
	//vb->add(s);

	
	ListBox* lbMailSettings = new ListBox(_prefix+"lbMail", "email settings");
	lbMailSettings->addItem(new ListItem("Do not send an email"));
	lbMailSettings->addItem(new ListItem("Notify about start and finish"));
	lbMailSettings->addItem(new ListItem("Notify when finished"));
	lbMailSettings->addItem(new ListItem("Only notify on fail"));
	c->add(lbMailSettings);

	
	
	
	

	auto f2 = std::bind(& BatteryTest::saveSchSettingsCallback, this, _1);

	

	Button* btnSaveSettings= new Button(_prefix+"_saveScSettings", "Save", f2);

	
	c->add(btnSaveSettings);
	
	
	Button* btnRecallSettings= new Button(_prefix+"_recallSchSettings", "Recall stored settings", NULL);
	c->add(btnRecallSettings);
	loadSettingsFromSpiffs();


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
		saveSettingsToSpiffs();
		


	}

	
void BatteryTest::saveSettingsToSpiffs()
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
	SpiffsPersistentSettingsUtils::saveSettings(root, fname);
}

void BatteryTest::loadSettingsFromSpiffs()
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




	GUI* gui = this->cont->getGUI();

	gui->find((String)prefix+"tifr")->setDefaultText(config.firstRun);
	gui->find((String)prefix+"tiP")->setDefaultText(config.runPeriod);
	gui->find((String)prefix + "cbir")->setDefaultIntValue(config.storeResults);
	gui->find((String)prefix + "lbMail")->setDefaultIntValue(config.mailSettings);
	//gui->find((String)prefix+"cbir")
	//gui->find((String)prefix+"lbMail")->setDefaultText(config.mailSettings);
}

