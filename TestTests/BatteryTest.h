#pragma once

#include <TGH_GUI.h>

#include <TimeLib.h>
#include "ElectronicLoad.h"
#include "Communicator.h"
#include "SpiffsManager.h"
#include "Container.h"
#include "Arduino.h"

#include "TestScheduler.h"
#include "Battery.h"

#define REPORT_MAIL_NEVER 0//do not send an email
#define REPORT_MAIL_ONFAIL 1//send an email only if the battery fails the test 
#define REPORT_MAIL_ONFINISHED 2//send an email report whenever the test finishes
#define REPORT_MAIL_ONSTARTANDFINISH 3//also send the email when the test starts


#define REPORT_SMS_ONFINISHED 0//send an SMS report whenever the test finishes
#define REPORT_SMS_ONFAIL 1//send an SMS only if the battery fails the test 
#define REPORT_SMS_ONSTARTANDFINISH 2//also send the SMS when the test starts

#define STATE_PENDING 0 //the test should already be running, but it is being prevented from doing so by some other events (i.e. error or another test running)
#define STATE_SCHEDULED 1 //the test is scheduled to start some time in the future
#define STATE_RUNNING 2 //the test is currently in progress
#define STATE_INTERRUPTED_BY_USER 3 //the user has interrupted the current test
#define STATE_ERROR 4 //the test was interrupted because an error occured
#define STATE_STOPPED 5 //the test is not scheduled and it will not run automatically in the future


class BatteryTest
{
private:
public:
	virtual String getName();
	char* slepice = "";
	void setScheduler(TestScheduler * _sch);
	time_t getScheduledStartTime();
	String getSchedulingSettings();
	void processRequestToStopTest(int userNo);
	void enableAutorun();
	void disableAutorun();
	virtual String getSettings();
	void beginTest(boolean scheduled);
	virtual void printHistoricalResults();
	virtual void handle();
	virtual int getType();
	virtual void saveResults();
	void fastForwardScheduling();
	int getBatteryNo();
	char* getTextResults();
	boolean autorunEnabled();
	//get the textual representation of the test results
	virtual void generateTextResults();
	boolean schedule(String firstRun, String period, int mailSettings);
	void endTest(int endMode);
	void printResultsToSerial();
	virtual String setOptions(String opt1, String opt2="", String opt3="", String opt4="", String opt5="");
	virtual String getIntermediateResults();
protected:
	Communicator* comm;
	char textResults[400];
	TestScheduler* scheduler=NULL;
	Container* cont=NULL;
	boolean canRunAutomatically=true;//whether the test is scheduled to be run automatically
	time_t firstScheduledStartTime;//the first time the test is scheduled to; the time instants when the test should be run are defined by the first time and the period
	time_t period;//the period (i.e. how often the test is run)
	time_t scheduledStartTime;//the time the next test is scheduled to
	unsigned long startMillis;//the millis() at the time of start, more precise than the startTime... workaround for the fact that the timeLib doesn't have millis...
	time_t lastRunStart;//the time the test was last run
	time_t lastRunDuration;//How much time did it take last time to complete the test
	boolean lastRunPassed;//whether the battery passed the last test
	int phase;//which phase (step) the test is in currently
	int state=0;//which state is the test in
	int batteryNo=1;//which battery should be tested;
	int emailReport = REPORT_MAIL_ONFINISHED;
	bool wasThisRunScheduled;//whether the current run is a result of automatic scheduling (true) or was initiated manually by the user (false)
	boolean testFailed=0;//whether the test succeeded (false) or not (true)
	
	//virtual void start(boolean scheduled);
	virtual int sendEmailReport();
	virtual void generateGUI(Container* c);//fill a container with a GUI
	virtual String getId()=0;

	void setFirstScheduledStartTime(int day, int month, int year, int hour, int min);
	void setSchedulingPeriod(int days, int hours, int minutes);





	void failOnError(int status);

	virtual void reportResultsOnGUI();

	void generateSchedulingGUI(Container * c, String prefix);


	void saveSchSettingsCallback(int user);
	void saveSchSettingsToSpiffs();
	void loadSchSettingsFromSpiffs();

	void parseLoadedSettings();

	
	float updatePeriod = 0.25;

	struct schedulingConfig {
		char firstRun[50];
		char runPeriod[50];
		boolean storeResults;
		boolean autorun = true;
		int mailSettings;
		int SMSSettings;
	};
	schedulingConfig config;

	char prefix[50];
	
	double lastMeasuredU;
	double lastMeasuredI;
	String getGenericLastTestInfo();

	Battery* bat;

	//String schedulingGUIPrefix;// = "";//since the scheduling is the same for all tests, it is handled in the generic BatteryTest class; but since GUI elements are placed and their IDs have to be unique,
	//char* slepice="";// = "";//since the scheduling is the same for all tests, it is handled in the generic BatteryTest class; but since GUI elements are placed and their IDs have to be unique,
	
	//there needs to be some sort of naming convention to differentiate the GUI elements belonging to the individual tests; this is what the prefix is for

};