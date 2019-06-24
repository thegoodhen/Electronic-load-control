/*
 * TimeNTP_ESP8266WiFi.ino
 * Example showing time sync to NTP time source
 *
 * This sketch uses the ESP8266WiFi library
 */

 #include "Battery.h"
#include "TestScheduler.h"
#include "SpiffsManager.h"
#include "parserUtils.h"
#include "Communicator.h"
#include "TestScheduler.h"

#include <base64.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
//#include "HovnoTest.h"
#include "FastTest.h"
#include "VoltageTest.h"
#include "DischargeTest.h"
#include <WiFiClient.h>
#include <TGH_GUI.h>
#include "NTPManager.h"
#include "StatusDisplay.h"
#include "SerialManager.h"

GUI gui;


const char ssid[] = "iPhone";  //  your network SSID (name)
const char pass[] = "kokodak1";       // your network password

// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

const int timeZone = 1;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)


WiFiClient wfc;

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);
FastTest* ft;
VoltageTest* vt;
DischargeTest* dt;
FastTest* ftb2;
VoltageTest* vtb2;
DischargeTest* dtb2;
Communicator* comm;
TestScheduler* ts;
StatusDisplay* sd;
SerialManager* sm;

NTPManager* ntpm;

void setup()
{
  ElectronicLoad::begin();
  Serial.begin(9600);
  while (!Serial) ; // Needed for Leonardo only
  delay(250);
  SerialManager::debugPrintln("TimeNTP Example");
  SerialManager::debugPrint("Connecting to ");
  SerialManager::debugPrintln(ssid);

  WiFi.begin();
  //WiFi.begin(ssid, pass);

  unsigned long startMillis = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    SerialManager::debugPrint(".");
	if (millis() - startMillis > 5000)
	{
		break;
	}
  }

  SpiffsManager::begin();

  comm = new Communicator(wfc, "smtp.seznam.cz", "batterymanagement@email.cz", "BMS2019", "dsibrava@seznam.cz", 25, 0);
  ts = new TestScheduler();

  comm->begin();
  Battery* b1 = Battery::get(1);
  Battery* b2 = Battery::get(2);

  ft = new FastTest(1, ts, comm, true, 2019, 12, 19, 16, 04, 0, 2, 2);
  vt = new VoltageTest(b1, ts, comm, true, 2019, 12, 19, 16, 04, 0, 2, 2);
  dt = new DischargeTest(1, ts, comm, true, 2019, 12, 19, 16, 04, 0, 2, 2);
  ftb2 = new FastTest(2, ts, comm, true, 2019, 12, 19, 16, 04, 0, 2, 2);
  vtb2 = new VoltageTest(b2, ts, comm, true, 2019, 12, 19, 16, 04, 0, 2, 2);
  dtb2 = new DischargeTest(2, ts, comm, true, 2019, 12, 19, 16, 04, 0, 2, 2);
  sd = new StatusDisplay(ts);
  sm = new SerialManager(ts, comm);
  /*
  comm->login();
  comm->sendHeader("slepice");
  comm->printText("<h1>kokodak</h1>");
  comm->exit();
  */
  //StaticJsonBuffer<1000> jb;
  //JsonObject& obj = jb.createObject();
  //obj["slepice"] = 3;

  //SpiffsManager::saveSettings(obj, "slepice.txt");
  //JsonObject& obj2 = SpiffsManager::loadSettings("slepice.txt");
  //int s = obj2["slepice"];
  //SerialManager::debugPrintln("Ted to ma vypsat tu trojku...");
  //SerialManager::debugPrintln(s);

  NTPManager::begin();
  initGUI();
}

void initGUI()
{
	gui.begin();//this is a necessary call! You need to call this in setup() to properly initialize the GUI!!

	
	//TabbedPane* tp = new TabbedPane("tp1");//We first need to create a tabbed pane in order to add tabs!
	//gui.add(tp);//We need to attach it to the GUI

	//Tab* tab2 = new Tab("Tests");
	//tp->addTab(tab2);//We add the tab to the tabPane

	////Tab* tab3 = new Tab("Settings");
	//tp->addTab(tab3);//We add the tab to the tabPane


	//dtb2->generateGUI(tab2);
	//ntpm->generateGUI(tab3);

	

	//SerialManager::debugPrintln("koko");
	//delay(500);



	//comm->generateGUI(tab3);
	//Tab* tab1 = new Tab("Overview");//We create the first tab
	//tp->addTab(tab1);//We add the tab to the tabPane

	//sd->generateGUI(tab1);
	

	//hBox* hb = new hBox("hb");
	//ft->generateGUI(hb);
	//gui.add(hb);
}




time_t prevDisplay = 0; // when the digital clock was displayed

void loop()
{
	
	/*
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
    }
  }
  */
  
  ts->handle();
  //ft->handle();
  //vt->handle();

	gui.loop();//you have to call this function in loop() for this library to work!
	//sd->loop();
	sm->loop();
	ElectronicLoad::heartBeat();
	NTPManager::loop();
}

void digitalClockDisplay()
{
  // digital clock display of the time
  SerialManager::debugPrint(hour());
  printDigits(minute());
  printDigits(second());
  SerialManager::debugPrint(" ");
  SerialManager::debugPrint(day());
  SerialManager::debugPrint(".");
  SerialManager::debugPrint(month());
  SerialManager::debugPrint(".");
  SerialManager::debugPrint(year());
  SerialManager::debugPrintln("");
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  SerialManager::debugPrint(":");
  if (digits < 10)
    SerialManager::debugPrint('0');
  SerialManager::debugPrint(digits);
}

