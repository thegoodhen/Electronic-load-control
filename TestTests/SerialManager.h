// SerialManager.h

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
#include "Communicator.h"

#define CMDS_COUNT sizeof(cmds)/sizeof(cmds[0])

class SerialManager
{
private: 
	WiFiClient theClient;
	Container* cont;
	static TestScheduler* ts;
	static Communicator* comm;
public:
	SerialManager(TestScheduler* ts, Communicator* _comm);
	//Communicator(WiFiClient theClient, String smtpServer, String sourceAddr, String sourcePassword, String targetAddr, int portNumber, unsigned long phoneNumber);
	void begin();
	void handleData();
	void loop();
	void parseCommand(char * str);

	static void removeTrailingNewlines(char * str);

	static void processCommand(char ** arguments);
	static int getArgsCount(char ** arguments);
	static int getFunctionIndex(char * theName);


const char* delimiter = "|";

struct command {
  void (* cmd)(char **, int agrCount);
  const char commandString[20];
};
static void startTest(char** params, int argCount);
static void stopTest(char ** params, int argCount);
static void help(char** params, int argCount);

static void connectWiFi(char ** params, int argCount);
static void disconnectWiFi(char ** params, int argCount);
static void lastResult(char ** params, int argCount);
static void schedule(char ** params, int argCount);
static void timeSet(char ** params, int argCount);

static void status(char ** params, int argCount);

static void configureEmail(char ** params, int argCount);

static void testEmail(char ** params, int argCount);

static int getBatteryNo(char * input);

static int getTestType(char * theName);


static struct command cmds[]; 
};


