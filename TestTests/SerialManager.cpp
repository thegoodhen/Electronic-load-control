// 
// 
// 

#include "SerialManager.h"
#include <functional>

#include "SpiffsPersistentSettingsUtils.h"
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include "BatteryTest.h"
#include "NTPManager.h"

 using namespace std::placeholders; 
 //void SerialManager::help(char** params, int argCount);
 SerialManager::command SerialManager::cmds[] = { {help, "HELP"}, {startTest,"STARTTEST"}, {connectWiFi, "CONNECTWIFI" }, {disconnectWiFi,"DISCONNECTWIFI"},{lastResult,"LASTRESULT"},{schedule,"SCHEDULE"},{timeSet,"SETTIME"} };
 TestScheduler* SerialManager::ts;



void SerialManager::startTest(char** params, int argCount)
{
  if (argCount != 2)
  {
    Serial.println("Incorrect usage. Correct usage: \"STARTTEST|TESTTYPE|BATTERYNO\" - i.e. \"STARTTEST|VOLTAGE|1\"");
    return;
  }
  int testType = getTestType(params[0]);
  if (testType==-1)
  {
    return;
  }

  int bNo = getBatteryNo(params[1]);
  if (bNo == -1)
  {
	  return;
  }
  BatteryTest* bt=ts->findTest(testType,bNo);
  if (ts->getCurrentTest() != NULL)
  {
	  Serial.println("A test is already running. Use STOPTEST to interrupt the current test, if you so desire.");
	  return;
  }
  if (bt == NULL)
  {
	  Serial.println("The selected test is not implemented yet.");
	  return;
  }
  bt->beginTest(false);
  //Serial.println("GEKITY GEK");
  //Serial.println(argCount);
}

void SerialManager::help(char** params, int argCount)
{
  //TODO: handle contextual help
  Serial.println("HELP\tthis command. Use HELP|COMMANDNAME to see contextual help.");
  Serial.println("STARTTEST\tstart a test. Correct usage: \"STARTTEST|TESTTYPE|BATTERYNO\" - i.e. \"STARTTEST|VOLTAGE|1\"");
}

void SerialManager::connectWiFi(char** params, int argCount)
{
	if (argCount > 2 )
	{
		Serial.println("Incorrect usage. Correct usage:");
		Serial.println("CONNECTWIFI or CONNECTWIFI|SSID or CONNECTWIFI|SSID|PASSWORD");
	}
	WiFi.disconnect();
	if (argCount == 0)
	{
		WiFi.begin();
	}
	if (argCount == 1)
	{
		WiFi.begin(params[0]);
	}
	if (argCount == 2)
	{
		WiFi.begin(params[0], params[1]);
	}
	unsigned long startMillis = millis();

	while (WiFi.status() != WL_CONNECTED) 
	{
		Serial.println("connecting...");
		delay(1000);
		if (millis() - startMillis > 10000)
		{
			Serial.println("Unable to connect! Aborting.");
			return;
		}
	}
	Serial.print("Connected to: ");
	Serial.println(WiFi.SSID());
}

void SerialManager::disconnectWiFi(char** params, int argCount)
{
	if (argCount != 0)
	{
		Serial.println("Usage: DISCONNECTWIFI");
	}
	WiFi.disconnect(true);
	Serial.println("WiFi disconnected.");
}

void SerialManager::schedule(char** params, int argCount)
{
	Serial.println(argCount);
	if (argCount != 5)
	{
		Serial.println("Usage: SCHEDULE|TESTTYPE|BATTERYNO|STARTDATE|PERIOD|MAILSETTINGS, that is SCHEDULE|TESTTYPE|BATTERYNO|DD.MM.YYYY HH:MM|DD:HH:MM|MAILSETTINGS");
		return;
	}

  int testType = getTestType(params[0]);
  if (testType==-1)
  {
    return;
  }

  int bNo = getBatteryNo(params[1]);
  if (bNo == -1)
  {
	  return;
  }
  BatteryTest* bt=ts->findTest(testType,bNo);

  if (bt != NULL)
  {
	  if (!bt->schedule(params[2], params[3], 0))
	  {
		  Serial.println("Invalid scheduling settings. Correct format for start date: DD.MM.YYYY HH:MM and for period DD:HH:MM.");
	  }
  }
}

void SerialManager::lastResult(char** params, int argCount)
{
  if (argCount != 2)
  {
    Serial.println("Incorrect usage. Correct usage: LASTRESULT|TYPE|BATTERYNO");
    return;
  }
  int testType = getTestType(params[0]);
  if (testType==-1)
  {
    return;
  }

  int bNo = getBatteryNo(params[1]);
  if (bNo == -1)
  {
	  return;
  }
  BatteryTest* bt=ts->findTest(testType,bNo);
  if (bt != NULL)
  {
	  Serial.println("Last results for "+bt->getName());
	  Serial.println(bt->getTextResults());

  }
  else
  {
	  Serial.println("No such test.");
  }
}

void SerialManager::timeSet(char** params, int argCount)
{
	if (argCount != 1)
	{
		Serial.println("Usage: SETTIME|DD:MM:YYYY HH:MM");
		return;
	}
	time_t timeToSet = NTPManager::stringToDate(params[0]);
	if (timeToSet != 0)
	{
		setTime(timeToSet);
	}

}

int SerialManager :: getBatteryNo(char* input)
{
  float outArg;
  parserUtils::retrieveFloat(input, &outArg);
  int bNo = (int)outArg;
  if (bNo < 1 || bNo>2)
  {
	  Serial.println("Invalid battery number. Valid options are: 1, 2.");
	  return -1;
  }
  return bNo;
}

int SerialManager::getTestType(char* theName)
{
  if (strcmp(theName, "VOLTAGE") == 0)
  {
    return 0;
  }
  if (strcmp(theName, "FAST") == 0)
  {
    return 1;
  }
  if (strcmp(theName, "DISCHARGE") == 0)
  {
    return 2;
  }
  Serial.println("Invalid test type; valid options are: VOLTAGE, FAST, DISCHARGE.");
  return -1;
}



 SerialManager::SerialManager(TestScheduler* _ts)
 {
	ts = _ts;
 }


 void SerialManager::handleData()
 {
	 char inputBuffer[400];
	 static int index = 0;
	 while (Serial.available()) {
		 // get the new byte:
		 char inChar = (char)Serial.read();
		 // add it to the inputString:
		 if (index >= 400)
		 {
			 index = 0;
		 }
		 inputBuffer[index++] = inChar;
		 if (inChar == '\n') {
			 inputBuffer[index - 1] = '\0';
			 Serial.print("Got this: \"");
			 Serial.print(inputBuffer);
			 Serial.print("\"");

			 index = 0;
			 parseCommand(inputBuffer);
		 }
	 }
 }

void SerialManager::loop()
{
	handleData();
}



void SerialManager::parseCommand(char* str)
{
  removeTrailingNewlines(str);
  const int nargs = 10;
  char* arguments[nargs];//max 10 arguments in total;
  for (int i = 0; i < nargs; i++)
  {
    arguments[i] = NULL;
  }

  char* lastToken =  strtok(str, delimiter);
  int  it = 0;


  while (lastToken != NULL)
  {
    arguments[it] = lastToken; //function name
    lastToken = strtok(NULL, delimiter);
    it++;
    //lastToken=arguments[it];
  }
  processCommand(arguments);
}



void SerialManager::removeTrailingNewlines(char* str)
{
  char* endPtr = strchr(str, '\r');
  if (endPtr == NULL)
  {
    endPtr = strchr(str, '\n');
  }
  if (endPtr != NULL)
  {
    int index = (int)(endPtr - str);
    str[index] = '\0';
  }
}

void SerialManager::processCommand(char** arguments)
{
  char* functionName = arguments[0];
  int functionIndex = getFunctionIndex(functionName);
  if (functionIndex != -1)
  {
    void (* fncPtr)(char **, int) ;

    fncPtr = (void(*)(char**, int))(cmds[functionIndex].cmd);
    int argsCount = getArgsCount(arguments);
    Serial.println(argsCount);
    fncPtr(&arguments[1], argsCount - 1);
  }
  else
  {
    Serial.println("Unknown command. Type HELP to see the list of available commands.");
  }

}

int SerialManager::getArgsCount(char** arguments)
{
  int theCount = 0;
  while (arguments[theCount] != NULL)
  {
    theCount++;
  }
  return theCount;
}

int SerialManager::getFunctionIndex(char* theName)
{
  //Serial.println(cmds[0].commandString);
  //Serial.println(strcmp_P("LIST",cmds[0].commandString));
  for (int i = 0; i < CMDS_COUNT; i++)
  {
    if (strcmp(theName, cmds[i].commandString) == 0)
    {
      return i;
    }
  }
  return -1;
}





