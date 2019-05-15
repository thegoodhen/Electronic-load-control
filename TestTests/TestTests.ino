/*
 * TimeNTP_ESP8266WiFi.ino
 * Example showing time sync to NTP time source
 *
 * This sketch uses the ESP8266WiFi library
 */

 #include "TestScheduler.h"
#include "SpiffsPersistentSettingsUtils.h"
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
#include <WiFiClient.h>
#include <TGH_GUI.h>
#include "NTPManager.h"
#include "StatusDisplay.h"
#include "SerialManager.h"

GUI gui;


const char ssid[] = "TGH_network";  //  your network SSID (name)
const char pass[] = "r0ut3rp@$$";       // your network password

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
//HovnoTest* HOVNOUS;
FastTest* ft;
VoltageTest* vt;
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
  Serial.println("TimeNTP Example");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  SpiffsPersistentSettingsUtils::begin();

  comm = new Communicator(wfc, "smtp.seznam.cz", "batterymanagement@seznam.cz", "BMS2019", "dsibrava@seznam.cz", 25, 0);
  ts = new TestScheduler();

  //comm->begin();
  ft = new FastTest(ts, comm, true, 2019, 12, 19, 16, 04, 0, 2, 2);
  vt = new VoltageTest(ts, comm, true, 2019, 12, 19, 16, 04, 0, 2, 2);
  sd = new StatusDisplay(ts);
  sm = new SerialManager(ts, comm);
  /*
  comm->login();
  comm->sendHeader("slepice");
  comm->printText("<h1>kokodak</h1>");
  comm->exit();
  */
  StaticJsonBuffer<1000> jb;
  JsonObject& obj = jb.createObject();
  obj["slepice"] = 3;

  //SpiffsPersistentSettingsUtils::saveSettings(obj, "slepice.txt");
  //JsonObject& obj2 = SpiffsPersistentSettingsUtils::loadSettings("slepice.txt");
  //int s = obj2["slepice"];
  //Serial.println("Ted to ma vypsat tu trojku...");
  //Serial.println(s);

  NTPManager::begin();
  initGUI();
}

void initGUI()
{
	gui.begin();//this is a necessary call! You need to call this in setup() to properly initialize the GUI!!

	
	TabbedPane* tp = new TabbedPane("tp1");//We first need to create a tabbed pane in order to add tabs!
	gui.add(tp);//We need to attach it to the GUI
	Tab* tab1 = new Tab("Overview");//We create the first tab
	tp->addTab(tab1);//We add the tab to the tabPane

	Tab* tab2 = new Tab("Tests");
	tp->addTab(tab2);//We add the tab to the tabPane

	Tab* tab3 = new Tab("Settings");
	tp->addTab(tab3);//We add the tab to the tabPane


	ft->generateGUI(tab2);
	vt->generateGUI(tab2);

	




	comm->generateGUI(tab3);
	ntpm->generateGUI(tab3);
	sd->generateGUI(tab1);
	

	//hBox* hb = new hBox("hb");
	//ft->generateGUI(hb);
	//gui.add(hb);
}



void buttonCB(int user)
{
	USE_SERIAL.println("User clicked the button! User number: ");
	USE_SERIAL.println(user);
	USE_SERIAL.println("First text input:");
	USE_SERIAL.println(gui.find("ti1")->retrieveText(user));
	USE_SERIAL.println("Second text input:");
	USE_SERIAL.println(gui.find("ti2")->retrieveText(user));
	USE_SERIAL.println("Checkbox:");
	USE_SERIAL.println(gui.find("cb1")->retrieveIntValue(user));
	USE_SERIAL.println("Slider:");
	USE_SERIAL.println(gui.find("sl1")->retrieveIntValue(user));
	USE_SERIAL.println("ListBox");
	USE_SERIAL.println(gui.find("lb1")->retrieveText(user));
	USE_SERIAL.println(gui.find("lb1")->retrieveIntValue(user));
}

time_t prevDisplay = 0; // when the digital clock was displayed

void loop()
{
	
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
    }
  }
  
  ts->handle();
  //ft->handle();
  //vt->handle();

	gui.loop();//you have to call this function in loop() for this library to work!
	//sd->loop();
	sm->loop();
	ElectronicLoad::heartBeat();
}

void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

