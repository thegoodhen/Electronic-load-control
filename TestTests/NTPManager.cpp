
#include "NTPManager.h"

#include <functional>

#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "SerialManager.h"

using namespace std::placeholders;	
WiFiUDP Udp;

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
unsigned int localPort = 8888;  // local port to listen for UDP packets

NTPManager::ntpConfig NTPManager::config;
Container* NTPManager::cont=0;


void NTPManager::begin()
{
  SerialManager::debugPrint("IP number assigned by DHCP is ");
  SerialManager::debugPrintln(WiFi.localIP().toString());
  SerialManager::debugPrintln("Starting UDP");
  Udp.begin(localPort);
  SerialManager::debugPrint("Local port: ");
  SerialManager::debugPrintln(String(Udp.localPort()));
  SerialManager::debugPrintln("waiting for sync");
  //setSyncProvider(getNtpTime);
  //setSyncInterval(300);
}

void NTPManager::loop()
{
	unsigned static long lastMillis;
	if (millis() - lastMillis > (unsigned long)1 * 60 * 1000)
	{
		slightlyAdjustTime();
		lastMillis = millis();
	}

}

void NTPManager::generateGUI(Container* c)
{
	//this->schedulingGUIPrefix = prefix;
	vBox* vb = new vBox("ntmpVb");
	cont = c;

	c->add(vb);
	Heading* theHeader= new Heading("theHeader", 2, "NTP Settings");
	vb->add(theHeader);

	TextInput* tiNTPServer= new TextInput("tiNTPServer", "NTP server address:");
	vb->add(tiNTPServer);


	TextInput* tiOffset = new TextInput("tiOffset", "GMT offset");
	vb->add(tiOffset);


	

	Button* btnSaveSettings= new Button("saveNTPSettings", "Save NTP settings", saveSettingsCallback);

	
	vb->add(btnSaveSettings);
	
loadSettingsFromSpiffs();


	//the following lines fix everything... how very bizzare indeed!
	//Text* slepicelastResultsText = new Text("slepicelastResults", R"(Last results are something something)");
	//vb->add(slepicelastResultsText);
	
}

	void NTPManager::saveSettingsCallback(int user)
	{

		GUI* gui = cont->getGUI();
		SerialManager::debugPrintln("Saving scheduling settings");

		
		String theServer= gui->find("tiNTPServer")->retrieveText(user);
		theServer.toCharArray(NTPManager::config.ntpServer, 50);


		String gmtOffset= gui->find("tiOffset")->retrieveText(user);
		float offset = 0;
		parserUtils::retrieveFloat(gmtOffset.c_str(), &offset);
		NTPManager::config.gmtOffset = (int)offset;

		saveSettingsToSpiffs();
		


	}

	
void NTPManager::saveSettingsToSpiffs()
{

	/*
	StaticJsonBuffer<350> jsonBuffer;

	// Parse the root object
	JsonObject &root = jsonBuffer.createObject();

	// Set the values
	root["NTPServer"] = config.ntpServer;
	root["GMTOffset"] = config.gmtOffset;

	//parseLoadedSettings();
	char* fname = "ntp.cfg";
	SpiffsManager::saveSettings(root, fname);
	*/
}

void NTPManager::loadSettingsFromSpiffs()
{

	//SerialManager::debugPrintln("nacitam nastaveni...");
	StaticJsonBuffer<1000> jb;
	StaticJsonBuffer<1000> *jbPtr = &jb;




	JsonObject& root = SpiffsManager::loadSettings(jbPtr, "ntp.cfg");
	if (root["success"] == false)
	{
		SerialManager::debugPrintln("failnulo to nacitani toho testu...");
	return;
	}
	SerialManager::debugPrintln("nacetlo se nastaveni...");

	strlcpy(config.ntpServer,                   // <- destination
	root["NTPServer"],
	sizeof(config.ntpServer));

		
	config.gmtOffset= root["GMTOffset"];


	GUI* gui = cont->getGUI();

	gui->find("tiNTPServer")->setDefaultText(config.ntpServer);
	gui->find("tiOffset")->setDefaultText((String)config.gmtOffset);
	//gui->find((String)prefix+"cbir")
	//gui->find((String)prefix+"lbMail")->setDefaultText(config.mailSettings);
}


String NTPManager::dateToString(time_t _theDate)
{
	char returnString[40];
	sprintf(returnString, "%d.%d. %d %02d:%02d:%02d", day(_theDate), month(_theDate), year(_theDate), hour(_theDate), minute(_theDate), second(_theDate));
	//SerialManager::debugPrintln("returnString");
	//SerialManager::debugPrintln(returnString);
	return (String)returnString;
	
}


String NTPManager::periodToString(time_t _thePeriod)
{
	char returnString[40];
	sprintf(returnString, "%dd:%dh:%dm", day(_thePeriod), hour(_thePeriod),minute(_thePeriod));
	//SerialManager::debugPrintln("returnString");
	//SerialManager::debugPrintln(returnString);
	return (String)returnString;
	
}


time_t NTPManager::stringToDate(String dateStr)
{
	if (!isDateValid(dateStr))
	{
		return 0;
	}
	long outArr[5];
    parserUtils::retrieveNLongs(dateStr.c_str(), 5, outArr);
	byte dd = (byte)outArr[0];
	byte mm = (byte)outArr[1];
	int yy = (int)outArr[2];
	yy = CalendarYrToTm(yy);
	byte hh = (byte)outArr[3];
	byte minmin= (byte)outArr[4];
	tmElements_t startDateElems = {0,minmin,hh,1,dd,mm,yy};
	return makeTime(startDateElems);
}


void NTPManager::slightlyAdjustTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  SerialManager::debugPrintln("Transmitted NTP Request");
  // get a random server from the pool
  WiFi.hostByName(config.ntpServer, ntpServerIP);
  SerialManager::debugPrint(config.ntpServer);
  SerialManager::debugPrint(": ");
  SerialManager::debugPrintln(ntpServerIP.toString());
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      SerialManager::debugPrintln("Received NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
	  time_t NTPTime = secsSince1900 - 2208988800UL;

	  //return 0;
	  // return secsSince1900 - 2208988800UL + config.gmtOffset* SECS_PER_HOUR ;
	  //the following will prevent the time to syncing near the minute rollovers, but who cares? I know I don't...
	  if (abs(minute() - minute(NTPTime)) <= 5)//time isn't off by all that much
	  {
			SerialManager::debugPrintln("Time is close enough, adjusting...");

			int yr = CalendarYrToTm(year());
		    tmElements_t myElements = {second(NTPTime), minute(NTPTime), hour(), weekday(), day(), month(), yr };
			setTime(makeTime(myElements));
			ElectronicLoad::setTime(makeTime(myElements));
			return;
	  }
	  else
	  {
		  SerialManager::debugPrintln("Time is off by too much, adjust it manually.");
		  return ;
	  }
    }
  }
  SerialManager::debugPrintln("No NTP Response :-(");
  return ; // return 0 if unable to get the time

}


time_t NTPManager::getNtpTime()
{
	return 0;//TODO: remove this thingy
}

// send an NTP request to the time server at the given address
void NTPManager::sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

boolean NTPManager::isDateValid(String theDate)
{
	long outArr[5];
	int n = parserUtils::retrieveNLongs(theDate.c_str(), 5, outArr);
	if (n == 5)
	{
		if (outArr[0] > 31|| outArr[0]<1)
		{
			return false;
		}

		if (outArr[1] > 12|| outArr[0]<1)
		{
			return false;
		}

		if (outArr[2] < 2000|| outArr[2]>4000)//futureproofing
		{
			return false;
		}

		if (outArr[3] < 0|| outArr[3]>23)
		{
			return false;
		}

		if (outArr[4] < 0|| outArr[4]>59)
		{
			return false;
		}
		return true;
	}
		SerialManager::debugPrintln(theDate);

	return false;
}


boolean NTPManager::isPeriodValid(String thePeriod)
{
	long outArr[3];
	int n = parserUtils::retrieveNLongs(thePeriod.c_str(), 3, outArr);

	if (n == 3)
	{
		if (outArr[0] < 0)
		{
			return false;
		}

		if (outArr[1] <0 || outArr[1] > 23)
		{
			return false;
		}

		if (outArr[2] < 0 || outArr[2]>59)
		{
			return false;
		}
		return true;
	}
	return false;
}

