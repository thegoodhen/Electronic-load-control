
#include "BatteryTest.h"
#include "TestScheduler.h"

#include <functional>
#include "NTPManager.h"
#include "SerialManager.h"

using namespace std::placeholders;	

int BatteryTest::getBatteryNo()
{
	return batteryNo;
}

char* BatteryTest::getTextResults()
{
	return textResults;
}

boolean BatteryTest::autorunEnabled()
{
	return this->config.autorun;
}

void BatteryTest::setFirstScheduledStartTime(int day, int month, int year, int hour, int min)
{
	
	year = CalendarYrToTm(year);
	tmElements_t startDateElems = { 0,min,hour,1, day,month,year};
	this->firstScheduledStartTime = makeTime(startDateElems);
	this->scheduledStartTime = this->firstScheduledStartTime;
	SerialManager::debugPrintln("scheduled start time set...");
}

void BatteryTest::setSchedulingPeriod(int days, int hours, int minutes)
{
	tmElements_t periodElems = { 0,minutes,hours,1, days+1, 1, 0};
	this->period = makeTime(periodElems);
}

void BatteryTest::beginTest(boolean scheduled)
{
	BatteryTest* bt = this->scheduler->getCurrentTest();
	if (this->scheduler->getStatus() != 0)//testing not permitted
	{
		return;
	}
	
	if (scheduled && !this->config.autorun)//if the attempt to run this test was made by the scheduler, but the test cannot be ran automatically, disregard
	{
		return;
	}

	if (bt != NULL)//some test already running
	{
		if (!scheduled)
		{
			GUI* gui = this->cont->getGUI();
			gui->showError(-1, "Test already running!");
		}
		return;
	}

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
		fastForwardScheduling();

	}
}

void BatteryTest::printHistoricalResults()
{
	char fname[50];
    sprintf(fname, "%s.data", getId().c_str());
	File f = SPIFFS.open(fname, "r");
	while (f.available())
	{
		SerialManager::sendToOutput(f.read());
		//Serial.write(f.read());
	}
	f.close();
}

void BatteryTest::fastForwardScheduling()
{

		while (this->scheduledStartTime < now())//skip all the missed scheduled times
		{
			this->scheduledStartTime += this->period;//schedule the next run of this test
		}
}

/*
mode 0 - pass
mode 1 - fail
mode 2 - error
mode 3 - interrupted by user
*/
void BatteryTest::endTest(int endMode)
{

	this->scheduler->notifyAboutTestEnd(endMode);
	ElectronicLoad::connectBattery(0);
	ElectronicLoad::setI(0);
	ElectronicLoad::setUpdatePeriod(0);
	this->lastRunDuration = now() - this->lastRunStart;

	if (endMode == 2)//error
	{
		this->state = STATE_ERROR;//TODO: report error using email
		sprintf(textResults, "FATAL ERROR");
	}
	if (endMode == 3)//interrupted by user
	{
		this->state = STATE_INTERRUPTED_BY_USER;
		sprintf(textResults, "INTERRUPTED BY USER");
	}

	if (endMode == 0 || endMode == 1)
	{
		generateTextResults();
		saveResults();
		//reportResultsOnGUI();
		if ((emailReport == REPORT_MAIL_ONFAIL && testFailed) || emailReport == REPORT_MAIL_ONFINISHED)
		{
			sendEmailReport();
		}
		SerialManager::debugPrintln("end of test");
		if (canRunAutomatically)
		{
			state = STATE_SCHEDULED;
		}
		else
		{
			state = STATE_STOPPED;
		}
	}


	SerialManager::sendToOutputln("TEST FINISHED. RESULTS:");
	printResultsToSerial();

	//SerialManager::debugPrintln((unsigned long)this->scheduler);
	//SerialManager::debugPrintln(textResults);

}


/*
Print the contents of the textResults variable, removing the html tags
*/
void BatteryTest::printResultsToSerial()
{
	int len = strlen(textResults);
	boolean skipPrinting = false;
	for (int i = 0;i < len;i++)
	{
		if (textResults[i] == '<')//beginning of the html tag
		{
			skipPrinting = true;
		}

		if (!skipPrinting)
		{
			SerialManager::sendToOutput(String(textResults[i]));
		}


		if (textResults[i] == '>')//ending of the html tag
		{
			skipPrinting = false;
		}

	}
}

void BatteryTest::failOnError(int status)
{
	if (status != 0)
	{
		endTest(2);
	}
}

int BatteryTest::sendEmailReport()
{
	
	if (comm->login()) { return 1 ;}
	if (comm->sendHeader("TEST RESULTS: " + getName())) {return 1; }//TODO: make sure that this changes when we failed
		comm->printText(this->getTextResults());
		comm->exit();
		
	return 0;
	
}


void BatteryTest::generateSchedulingGUI(Container* c, String _prefix)
{
	_prefix.toCharArray(prefix, 50);
	//this->schedulingGUIPrefix = prefix;
	this->cont = c;

	Heading* hSchedulingProps = new Heading(_prefix+"hSch", 2, "Scheduling settings");
	//c->add(hSchedulingProps);

	TextInput* tiFirstRun = new TextInput(_prefix+"tifr", "the time and date of the first scheduled run (DD.MM.YYYY HH:MM)");
	//c->add(tiFirstRun);


	TextInput* tiPeriod= new TextInput(_prefix+"tiP", "the period between two consecutive scheduled runs (DD:HH:MM)");
	//c->add(tiPeriod);

	Checkbox* cbIncludeResult= new Checkbox(_prefix+"cbir", "Store historical test results");
	//c->add(cbIncludeResult);



	
	ListBox* lbMailSettings = new ListBox(_prefix+"lbMail", "email settings");
	//CAUTION: the items are in a specific order; reodering the following lines will result in a fatal malfunction
	lbMailSettings->addItem(new ListItem("Do not send an email"));
	lbMailSettings->addItem(new ListItem("Only notify on fail"));
	lbMailSettings->addItem(new ListItem("Notify when finished"));
	lbMailSettings->addItem(new ListItem("Notify about start and finish"));
	//c->add(lbMailSettings);

	
	
	/*
	
	

	auto f2 = std::bind(& BatteryTest::saveSchSettingsCallback, this, _1);

	

	Button* btnSaveSettings= new Button(_prefix+"_saveScSettings", "Save", f2);

	
	c->add(btnSaveSettings);
	
	
	Button* btnRecallSettings= new Button(_prefix+"_recallSchSettings", "Recall stored settings", NULL);
	c->add(btnRecallSettings);

	loadSchSettingsFromSpiffs();
	*/


	//the following lines fix everything... how very bizzare indeed!
	//Text* slepicelastResultsText = new Text("slepicelastResults", R"(Last results are something something)");
	//vb->add(slepicelastResultsText);
	
}

	void BatteryTest::saveSchSettingsCallback(int user)
	{

		GUI* gui = this->cont->getGUI();

		
		String firstRun = gui->find((String)prefix+ "tifr")->retrieveText(user);


		String period = gui->find((String)prefix + "tiP")->retrieveText(user);

		int storeResults = gui->find((String)prefix + "cbir")->retrieveIntValue(user);
		config.storeResults = storeResults;

		int mailSettings = gui->find((String)prefix + "lbMail")->retrieveIntValue(user);
		if (!schedule(firstRun, period, mailSettings))
		{
			gui->showError(user, "Invalid scheduling settings.");
		}
		
	}

	boolean BatteryTest::schedule(String firstRun, String period, int mailSettings)
	{
		if (!NTPManager::isDateValid(firstRun))
		{
			return false;
		}
		if (!NTPManager::isPeriodValid(period))
		{
			return false;
		}
		firstRun.toCharArray(config.firstRun, 50);
		period.toCharArray(config.runPeriod, 50);
		config.mailSettings = mailSettings;
		saveSchSettingsToSpiffs();
		return true; 
	}

	String BatteryTest::setOptions(String opt1, String opt2, String opt3, String opt4, String opt5)
	{
		return "";

	}
	
void BatteryTest::saveSchSettingsToSpiffs()
{
	SerialManager::debugPrint("ten prefix je: ");
	SerialManager::debugPrintln(this->getId());

		char fname[50];
		sprintf(fname, "%s_sch.cfg", this->getId().c_str());

	//char* fname = (char*)((String)prefix+".cfg").c_str();
	SerialManager::debugPrintln(fname);

	StaticJsonBuffer<350> jsonBuffer;

	// Parse the root object
	JsonObject &root = jsonBuffer.createObject();

	// Set the values
	root["firstRun"] = config.firstRun;
	root["runPeriod"] = config.runPeriod;
	root["storeResults"] = config.storeResults;
	root["mailSettings"] = config.mailSettings;
	root["autorun"] = config.autorun;

	parseLoadedSettings();//this will actually apply the settings...
	SpiffsManager::saveSettings(root, fname);
}

void BatteryTest::loadSchSettingsFromSpiffs()
{

	SerialManager::debugPrintln("nacitam nastaveni...");
	StaticJsonBuffer<1000> jb;
	StaticJsonBuffer<1000> *jbPtr = &jb;



	char fname[50];
	sprintf(fname, "%s_sch.cfg", this->getId().c_str());
	SerialManager::debugPrintln(fname);

	JsonObject& root = SpiffsManager::loadSettings(jbPtr, fname);
	if (root["success"] == false)
	{
		SerialManager::debugPrintln("failnulo to nacitani toho testu...");
	return;
	}
	SerialManager::debugPrintln("nacetlo se nastaveni...");

	strlcpy(config.firstRun,                   // <- destination
		root["firstRun"],
		sizeof(config.firstRun));

	strlcpy(config.runPeriod,                   // <- destination
		root["runPeriod"],
		sizeof(config.runPeriod));

	config.storeResults = root["storeResults"];
	config.mailSettings= root["mailSettings"];
	config.autorun= root["autorun"];
	parseLoadedSettings();

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


String BatteryTest::getGenericLastTestInfo()
{
	return "\r\n"+(String)getName()+ " results: \r\n-----------------------\r\n<br>start:\t\t" + NTPManager::dateToString(this->lastRunStart) + "<br>\r\n"
		+ "end:\t\t" + NTPManager::dateToString(this->lastRunStart + this->lastRunDuration) + "<br>\r\n" +
		"status:\t\t" + ((!this->testFailed)?"PASSED<br>\r\n":"<span style=\"color:#FF0000;\">FAILED</span><br>\r\n");
}
void BatteryTest::setScheduler(TestScheduler* _sch)
{
	this->scheduler = _sch;
}

time_t BatteryTest::getScheduledStartTime()
{
	return this->scheduledStartTime;
}

String BatteryTest::getSchedulingSettings()
{
	String returnStr = "";
	returnStr+=("Initial scheduled time:\t\t"+(String)config.firstRun+"\r\n");
	returnStr+=(("Period between test runs:\t\t"+(String)config.runPeriod+"\r\n"));
	returnStr+=("Next run in:\t\t\t"+NTPManager::dateToString(this->scheduledStartTime)+"\r\n");
	return returnStr;
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
		endTest(3);
	}
	else
	{
		wasRequestedAlready = true;
		lastMillis = millis();
		GUI* gui = cont->getGUI();
		gui->showInfo(userNo, "Click again to abort test.");

	}
	
}


void BatteryTest::enableAutorun()
{
	this->config.autorun = true;
	this->saveSchSettingsToSpiffs();
}

void BatteryTest::disableAutorun()
{
	this->config.autorun = false;
	this->saveSchSettingsToSpiffs();
}
