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

	#define MAX_SRV_CLIENTS 1
	static WiFiServer server;
    static  WiFiClient serverClients[MAX_SRV_CLIENTS];
	static boolean telnetEnabled;
	static unsigned long lastInputMillis;
	char dataIn[400];
public:
	SerialManager(TestScheduler* ts, Communicator* _comm);
	static int readAvailableInput();
	static void scanForTelnetConnections();
	//Communicator(WiFiClient theClient, String smtpServer, String sourceAddr, String sourcePassword, String targetAddr, int portNumber, unsigned long phoneNumber);
	void begin();
	void handleData();
	static void sendToOutput(String str);
	static void sendToOutput(char ch);
	static void sendToOutputln(String str);
	void loop();
	void parseCommand(char * str);

	static void removeTrailingNewlines(char * str);

	static void processCommand(char ** arguments);
	static int getArgsCount(char ** arguments);
	static int getFunctionIndex(char * theName);

	static void showHelp(char * topic);

	static void showSetOptionsHelp();

	static void printValidTestTypes();

static void printValidMailSettings();

static int getMailSettings(char * str);

static boolean telnetClientsOnline();


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
static void scanWiFi(char ** params, int argCount);
static void lastResult(char ** params, int argCount);
static void schedule(char ** params, int argCount);
static void history(char ** params, int argCount);
static void timeSet(char ** params, int argCount);

static void handleStatusPrintout();

static void status(char ** params, int argCount);
static void resetStatus(char ** params, int argCount);

static void configureEmail(char ** params, int argCount);

static void testEmail(char ** params, int argCount);

static void setOptions(char ** params, int argCount);
static void getOptions(char ** params, int argCount);

static void enableTelnet(char ** params, int argCount);

static void disableTelnet(char ** params, int argCount);

static void format(char ** params, int argCount);

static void batteryHistory(char ** params, int argCount);

static void enableAuto(char ** params, int argCount);

static void disableAuto(char ** params, int argCount);

static int getBatteryNo(char * input);

static int getTestType(char * theName);


static struct command cmds[]; 
};


