#pragma once

#include <TGH_GUI.h>

#include <TimeLib.h>
#include "ElectronicLoad.h"
#include "Communicator.h"
#include "SpiffsManager.h"
#include "Container.h"
#include "Arduino.h"



class NTPManager
{
private:
public:
	static void begin();
	static void loop();
	static void generateGUI(Container* c);
	static boolean isDateValid(String theDate);

	static boolean isPeriodValid(String thePeriod);


	struct ntpConfig {
		char ntpServer[50]="us.pool.ntp.org";
		int gmtOffset;
	};
	static ntpConfig config;
	static time_t stringToDate(String dateStr);
	static void slightlyAdjustTime();
	static String dateToString(time_t _theDate);

	static String periodToString(time_t _theDate);

protected:

	static Container* cont;

	//virtual void start(boolean scheduled);
	static void saveSettingsCallback(int user);
	static void saveSettingsToSpiffs();
	static void loadSettingsFromSpiffs();
	//fill a container with a GUI







	static void saveSchSettingsCallback(int user);
	static void saveSchSettingsToSpiffs();
	static void loadSchSettingsFromSpiffs();

	void parseLoadedSettings();

	

	


	//time_t getNtpTime();

	static time_t getNtpTime();

	static void sendNTPpacket(IPAddress & address);



	//String schedulingGUIPrefix;// = "";//since the scheduling is the same for all tests, it is handled in the generic BatteryTest class; but since GUI elements are placed and their IDs have to be unique,
	//char* slepice="";// = "";//since the scheduling is the same for all tests, it is handled in the generic BatteryTest class; but since GUI elements are placed and their IDs have to be unique,
	
	//there needs to be some sort of naming convention to differentiate the GUI elements belonging to the individual tests; this is what the prefix is for

};