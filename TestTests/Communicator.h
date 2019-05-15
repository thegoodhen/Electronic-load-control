// Communicator.h

#ifndef _COMMUNICATOR_h
#define _COMMUNICATOR_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "esp_base64.h"//hmmmm... is this the standard directory though?

#include <WiFiClient.h>
#include <TGH_GUI.h>
#include "parserUtils.h"

class Communicator
{
private: 
	WiFiClient theClient;
	Container* cont;
	struct Config {
		char smtpServer[50];
		char sourceAddr[50];
		char targetAddr[50];
		char sourcePass[50];
		long portNumber;
		char phoneNumber[50];
	};
	Config config;
public:
	//Communicator(WiFiClient theClient, String smtpServer, String sourceAddr, String sourcePassword, String targetAddr, int portNumber, unsigned long phoneNumber);
	int printText(String theText);
	Communicator(WiFiClient theClient, char * smtpServer, char * sourceAddr, char * sourcePassword, char * targetAddr, int portNumber, unsigned long phoneNumber);
	void begin();
	int login();
	int sendHeader(String subject);
	int exit();
	byte emailResp();
	void sendTestEmail();
	void generateGUI(Container * c);
	void saveSettingsCallback(int user);
	void saveSettings(String smtpServer, String thePort, String sourceAddr, String sourcePass, String targetAddr);
	void saveSettingsToSpiffs();
	void loadSettingsFromSpiffs();
};

#endif

