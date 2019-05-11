// StatusDisplay.h

#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "esp_base64.h"//hmmmm... is this the standard directory though?

#include <WiFiClient.h>
#include <TGH_GUI.h>
#include "parserUtils.h"
#include "TestScheduler.h"

class StatusDisplay
{
private: 
	WiFiClient theClient;
	Container* cont;
	TestScheduler* ts;
public:
	StatusDisplay(TestScheduler* ts);
	//Communicator(WiFiClient theClient, String smtpServer, String sourceAddr, String sourcePassword, String targetAddr, int portNumber, unsigned long phoneNumber);
	void begin();
	void loop();
	void generateGUI(Container * c);
};


