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
 SerialManager::command SerialManager::cmds[] = { {help, "HELP"}, {startTest,"STARTTEST"}, {stopTest,"STOPTEST"}, {connectWiFi, "CONNECTWIFI" }, {disconnectWiFi,"DISCONNECTWIFI"},{lastResult,"LASTRESULT"},{schedule,"SCHEDULE"},{timeSet,"SETTIME"},{status, "STATUS"}, {configureEmail,"CONFIGUREEMAIL"},{testEmail,"TESTEMAIL"}, {setOptions, "SETOPTIONS"} };
 TestScheduler* SerialManager::ts;
 Communicator * SerialManager::comm;



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

void SerialManager::stopTest(char** params, int argCount)
{
	BatteryTest* bt = ts->getCurrentTest();
	if (bt!= NULL)
	{
		bt->endTest(3);
    }
	else
	{
		Serial.println("No test is running, so no test can be stopped.");
	}
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

void SerialManager::status(char** params, int argCount)
{
	Serial.print("time: ");
	Serial.println(NTPManager::dateToString(now()));
	
	if (WiFi.status() == WL_CONNECTED)
	{
		Serial.println("WiFi status: connected");
		Serial.print("AP:");
		Serial.println(WiFi.SSID());
		Serial.print("IP: ");
		Serial.println(WiFi.localIP().toString());
	}
	else
	{
		Serial.println("WiFi status: disconnected");
	}
	Serial.print("Currently running test:");
	BatteryTest* currentTest = ts->getCurrentTest();
	if (currentTest == NULL)
	{
		Serial.println("None");
	}
	else
	{
		Serial.println(currentTest->getName());
	}
	Serial.println();
	Serial.println();
	Serial.println("Last results:");
	BatteryTest* tb1 = (ts->getLastTest(1));
	BatteryTest* tb2 = (ts->getLastTest(2));
	if (tb1 != NULL)
	{
		Serial.println(tb1->getTextResults());
	}
	else
	{
		Serial.println("(no test for battery 1 yet)");
	}
	Serial.println();
	if (tb2 != NULL)
	{
		Serial.println(tb2->getTextResults());
	}
	else
	{
		Serial.println("(no test for battery 2 yet)");
	}
	Serial.println();

}

void SerialManager::configureEmail(char** params, int argCount)
{
	if (argCount != 5)
	{
		Serial.println("Usage: CONFIGUREEMAIL|SMTPSERVER|PORT|SOURCEADDR|SOURCEPASS|TARGETADDR");
		return;
	}
	comm->saveSettings(params[0], params[1], params[2], params[3], params[4]);

}

void SerialManager::testEmail(char** params, int argCount)
{
	comm->sendTestEmail();
}


void SerialManager::setOptions(char** params, int argCount)
{
  if (argCount <3)
  {
    Serial.println("Incorrect usage. Correct usage: \"SETOPTIONS|TESTTYPE|BATTERYNO|(option 1)|(option 2)|(option 3)|(...)\"");
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

  if (bt == NULL)
  {
	  Serial.println("The selected test is not implemented yet.");
	  return;
  }
  Serial.println("Params 2 je ");
  String theParams[5] = { "","","","","" };
  for (int i = 0;i < argCount;i++)
  {
	  theParams[i] = params[i + 2];
  }
  String msg = bt->setOptions(theParams[0],theParams[1],theParams[2],theParams[3],theParams[4]);
  if (msg != "")
  {
	  Serial.println(msg);
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



 SerialManager::SerialManager(TestScheduler* _ts, Communicator* _comm)
 {
	ts = _ts;
	comm = _comm;
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





