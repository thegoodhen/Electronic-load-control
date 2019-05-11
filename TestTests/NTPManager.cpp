
#include "NTPManager.h"

#include <functional>

#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

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
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
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
		Serial.println("Saving scheduling settings");

		
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
	SpiffsPersistentSettingsUtils::saveSettings(root, fname);
	*/
}

void NTPManager::loadSettingsFromSpiffs()
{

	//Serial.println("nacitam nastaveni...");
	StaticJsonBuffer<1000> jb;
	StaticJsonBuffer<1000> *jbPtr = &jb;




	JsonObject& root = SpiffsPersistentSettingsUtils::loadSettings(jbPtr, "ntp.cfg");
	if (root["success"] == false)
	{
		Serial.println("failnulo to nacitani toho testu...");
	return;
	}
	Serial.println("nacetlo se nastaveni...");

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
	sprintf(returnString, "%d.%d. %d %d:%d:%d", day(_theDate), month(_theDate), year(_theDate), hour(_theDate), minute(_theDate), second(_theDate));
	Serial.println("returnString");
	Serial.println(returnString);
	return (String)returnString;
	
}


time_t NTPManager::getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(config.ntpServer, ntpServerIP);
  Serial.print(config.ntpServer);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + config.gmtOffset* SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
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