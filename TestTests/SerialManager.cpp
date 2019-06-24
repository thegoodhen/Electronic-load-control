// 
// 
// 

#include "SerialManager.h"
#include <functional>

#include "SpiffsManager.h"
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include "BatteryTest.h"
#include "NTPManager.h"

 using namespace std::placeholders; 
 //void SerialManager::help(char** params, int argCount);
 SerialManager::command SerialManager::cmds[] = { {help, "HELP"}, {startTest,"STARTTEST"}, {stopTest,"STOPTEST"}, {connectWiFi, "CONNECTWIFI" }, {disconnectWiFi,"DISCONNECTWIFI"},{scanWiFi, "SCANWIFI"},{lastResult,"LASTRESULT"},{schedule,"SCHEDULE"},{timeSet,"SETTIME"},{status, "STATUS"}, {configureEmail,"CONFIGUREEMAIL"},{testEmail,"TESTEMAIL"}, {setOptions, "SETOPTIONS"},{getOptions,"GETOPTIONS"}, {resetStatus, "RESETSTATUS"}, {history,"HISTORY"}, {enableTelnet, "ENABLETELNET"},{disableTelnet,"DISABLETELNET"}, {format,"FORMAT"},{batteryHistory, "BATTERYHISTORY"},{enableAuto, "ENABLEAUTO"} ,{disableAuto,"DISABLEAUTO"},{enableDebug, "ENABLEDEBUG"},{disableDebug, "DISABLEDEBUG"} };
 TestScheduler* SerialManager::ts;
 Communicator * SerialManager::comm;
//how many clients should be able to telnet to this ESP8266
WiFiServer SerialManager::server(23);
WiFiClient SerialManager::serverClients[MAX_SRV_CLIENTS];
unsigned long SerialManager::lastInputMillis = 0;

SerialManager::Config SerialManager::config;



void SerialManager::startTest(char** params, int argCount)
{
	if (ts->getStatus() != 0)
	{
		sendToOutputln("Running of tests is not permitted right now! Type STATUS for more information!");
	}
  if (argCount != 2)
  {
    sendToOutputln("Incorrect usage.");
	showHelp("STARTTEST");
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
	  sendToOutputln("A test is already running. Use STOPTEST to interrupt the current test, if you so desire.");
	  sendToOutput("Currently running test: ");
	  sendToOutput(ts->getCurrentTest()->getName());
	  return;
  }
  if (bt == NULL)
  {
	  sendToOutputln("The selected test is not implemented yet.");
	  return;
  }
  bt->beginTest(false);
  sendToOutput("Starting test: ");
  sendToOutputln(bt->getName());
  //sendToOutput("GEKITY GEK");
  //sendToOutput(argCount);
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
		sendToOutputln("No test is running, so no test can be stopped.");
	}
}

void SerialManager::help(char** params, int argCount)
{
  //TODO: handle contextual help
	if (argCount == 0)
	{
		sendToOutputln("Use HELP|COMMAND to view the usage of a certain command. What follows is the list of available commands.");
		for (int i = 0;i < CMDS_COUNT;i++)
		{
			sendToOutputln(cmds[i].commandString);
		}
	}
	else if (argCount == 1)
	{
		showHelp(params[0]);
	}
}

void SerialManager::connectWiFi(char** params, int argCount)
{
	if (argCount > 2)
	{
		sendToOutputln("Incorrect usage. Correct usage:");
		showHelp("CONNECTWIFI");
	}
	WiFi.disconnect();
	if (argCount == 0)
	{
		connectToDefaultWiFi();
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
		sendToOutputln("connecting...");
		delay(1000);
		if (millis() - startMillis > 10000)
		{
			sendToOutputln("Unable to connect! Aborting.");
			connectToDefaultWiFi();
			return;
		}
	}
	SerialManager::debugPrint("Connected to: ");
	sendToOutputln(WiFi.SSID());
}

void SerialManager::connectToDefaultWiFi()
{
	sendToOutput("Connecting to a default WiFi network: ");
	sendToOutputln(config.SSID);
	WiFi.begin(config.SSID, config.pass);

	unsigned long startMillis = millis();

	while (WiFi.status() != WL_CONNECTED)
	{
		sendToOutputln("connecting...");
		delay(1000);
		if (millis() - startMillis > 10000)
		{
			sendToOutputln("Unable to connect! Aborting.");
			return;
		}

	}
}

void SerialManager::disconnectWiFi(char** params, int argCount)
{
	if (argCount > 1)
	{
		sendToOutputln("Incorrect usage. Correct usage:");
		showHelp("DISCONNECTWIFI");
	}
	if (telnetClientsOnline())
	{
		if (argCount == 0)
		{
			sendToOutputln("Some clients (you?) are connected over telnet. If you disconnect WiFi, this will also interrupt their sessions. Furthermore, you will need physical access to the device to reenable the connection. type DISCONNECTWIFI|CONFIRM to proceed.");
			return;
		}
		if (argCount == 1 && strcmp(params[0], "CONFIRM") != 0)
		{
			sendToOutputln("Unknown parameter, expected nothing or CONFIRM (all caps).");
			return;
		}
	}
	WiFi.disconnect(true);
	sendToOutputln("WiFi disconnected.");
}

void SerialManager::scanWiFi(char** params, int argCount)
{
	if (argCount != 0)
	{
		showHelp("SCANWIFI");
		return;
	}
 int n = WiFi.scanNetworks();
  sendToOutputln("scan done");
  if (n == 0)
    sendToOutputln("no networks found");
  else
  {
    sendToOutput(String(n));
    sendToOutputln(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      sendToOutput(String(i + 1));
      sendToOutput(": ");
      sendToOutput(WiFi.SSID(i));
      sendToOutput(" (");
      sendToOutput(String(WiFi.RSSI(i)));
      sendToOutput(")");
      sendToOutputln((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
}

void SerialManager::schedule(char** params, int argCount)
{
	//sendToOutput(argCount);
	if (argCount != 5)
	{
		sendToOutputln("Incorrect usage. Correct usage:");
		showHelp("SCHEDULE");
		return;
	}

	int testType = getTestType(params[0]);
	if (testType == -1)
	{
		return;
	}

	int bNo = getBatteryNo(params[1]);
	if (bNo == -1)
	{
		return;
	}

	int mailS = getMailSettings(params[4]);
	if (mailS == -1)
	{
		sendToOutputln("Invalid email settings.");
		printValidMailSettings();
		return;
	}

	BatteryTest* bt = ts->findTest(testType, bNo);

	if (bt != NULL)
	{
		if (!bt->schedule(params[2], params[3], mailS))
		{
			sendToOutputln("Invalid scheduling settings. Correct format for start date: DD.MM.YYYY HH:MM and for period DD:HH:MM.");
		}
	}
}


void SerialManager::history(char** params, int argCount)
{
	if (argCount != 2)
	{
		sendToOutputln("Incorrect usage. Correct usage:");
		showHelp("HISTORY");
		return;
	}

	int testType = getTestType(params[0]);
	if (testType == -1)
	{
		return;
	}

	int bNo = getBatteryNo(params[1]);
	if (bNo == -1)
	{
		return;
	}
	BatteryTest* bt = ts->findTest(testType, bNo);

	if (bt != NULL)
	{
		bt->printHistoricalResults();
	}
}



void SerialManager::lastResult(char** params, int argCount)
{
	if (argCount != 2)
	{

		sendToOutputln("Incorrect usage. Correct usage:");
		showHelp("LASTRESULT");
		return;
	}
	int testType = getTestType(params[0]);
	if (testType == -1)
	{
		return;
	}

	int bNo = getBatteryNo(params[1]);
	if (bNo == -1)
	{
		return;
	}
	BatteryTest* bt = ts->findTest(testType, bNo);
	if (bt != NULL)
	{
		sendToOutputln("Last results for " + bt->getName());
		bt->printResultsToSerial();
	}
	else
	{
		sendToOutputln("No such test.");
	}
}

void SerialManager::timeSet(char** params, int argCount)
{
	if (argCount != 1)
	{
		sendToOutputln("Incorrect usage. Correct usage:");
		showHelp("SETTIME");
		return;
	}
	time_t timeToSet = NTPManager::stringToDate(params[0]);
	if (timeToSet != 0)
	{
		sendToOutputln("setting time...");
		setTime(timeToSet);
		ElectronicLoad::setTime(timeToSet);
	}
	else
    {
		sendToOutputln("not setting time");
	}

}

void SerialManager::handleStatusPrintout()
{
	static unsigned long lastPrintoutMillis;
	if (millis() - lastPrintoutMillis < 5000)
	{
		return;
	}
		
	if (lastInputMillis>0 && (millis()-lastInputMillis<60000))
	{
		return;
	}
	lastPrintoutMillis = millis();
	status(0,0);

}

void SerialManager::status(char** params, int argCount)
{
	if (ts->getStatus() == 1)//something is wrong, failed test or error
	{
		sendToOutputln("TESTING NOT PERMITTED!!!! SEE BELOW FOR DETAILS, RESOLVE PROBLEMS AND THEN USE THE \"RESETSTATUS\" command to continue");
	}
	sendToOutput("time: \t\t\t");
	sendToOutputln(NTPManager::dateToString(now()));
	
	if (WiFi.status() == WL_CONNECTED)
	{
		sendToOutputln("WiFi status: \t\tconnected");
		sendToOutput("AP: \t\t\t");
		sendToOutputln(WiFi.SSID());
		sendToOutput("IP: \t\t\t");
		sendToOutputln(WiFi.localIP().toString());
	}
	else
	{
		sendToOutputln("WiFi status: \t\tdisconnected");
	}
	sendToOutput("Currently running test: \t");
	BatteryTest* currentTest = ts->getCurrentTest();
	if (currentTest == NULL)
	{
		sendToOutputln("None");
	}
	else
	{
		sendToOutputln(currentTest->getName());
		sendToOutputln(currentTest->getIntermediateResults());
	}
	sendToOutputln("");
	sendToOutputln("");
	sendToOutputln("Battery 1: ");
	sendToOutputln("------------");

	Battery::get(1)->printProperthies();

	sendToOutputln("Battery 2: ");
	sendToOutputln("------------");
	Battery::get(2)->printProperthies();

}

void SerialManager::resetStatus(char ** params, int argCount)
{
	ts->resetStatus();
}

void SerialManager::configureEmail(char** params, int argCount)
{
	if (argCount != 5)
	{
		sendToOutputln("Incorrect usage. Correct usage:");
		showHelp("CONFIGUREEMAIL");
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
  if (argCount <2)
  {
	sendToOutputln("Incorrect usage. Correct usage:");
	showHelp("SETOPTIONS");
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
	  sendToOutputln("The selected test is not implemented yet.");
	  return;
  }
  //sendToOutputln("Params 2 je ");
  String theParams[5] = { "","","","","" };
  for (int i = 0;i < argCount;i++)
  {
	  theParams[i] = params[i + 2];
  }
  String msg = bt->setOptions(theParams[0],theParams[1],theParams[2],theParams[3],theParams[4]);
  if (msg != "")
  {
	  sendToOutputln(msg);
  }
}


void SerialManager::getOptions(char** params, int argCount)
{
	if (argCount == 0)
	{
		std::vector<BatteryTest*> tests = ts->getTests();
		for (std::vector<BatteryTest*>::size_type i = 0; i != tests.size(); i++) {
			BatteryTest* bt = (tests)[i];
			sendToOutputln(" ");
			sendToOutput("Options for: ");
			sendToOutputln(bt->getName());
			sendToOutputln("-------------------------------");
			sendToOutputln(bt->getSettings());	
		}
		return;
	}

  if (argCount !=2)
  {

	sendToOutputln("Incorrect usage. Correct usage:");
	showHelp("GETOPTIONS");
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
	  sendToOutputln("The selected test is not implemented yet.");
	  return;
  }
  sendToOutputln(bt->getSettings());	
}

void SerialManager::enableTelnet(char** params, int argCount)
{
	if (argCount != 0)
	{
		sendToOutputln("Incorrect usage. Correct usage:");
		showHelp("ENABLETELNET");
		return;
	}

	config.telnetEnabled = true;
	
	 server.begin();

	saveSchSettingsToSpiffs();

	if (!WiFi.isConnected())
	{
		sendToOutputln(F("Telnet enabled, but cannot be used yet - not connected to WiFi. Use CONNECTWIFI to allow remote management and then STATUS to see the IP to connect to. Then use telnet IP 23 on the remote computer to connect"));
	}
	else
	{
		SerialManager::sendToOutput("Ready! Use 'telnet ");
		SerialManager::sendToOutput(WiFi.localIP().toString());
		sendToOutputln(" 23' to connect");
	}
}


void SerialManager::disableTelnet(char** params, int argCount)
{

	if (telnetClientsOnline())
	{
		if (argCount == 0)
		{
			sendToOutputln("Some clients (you?) are connected over telnet. Disabling telnet will interrupt their sessions. Furthermore, you will need physical access to the device to reenable the connection. Type DISABLETELNET|CONFIRM to proceed.");
			return;
		}
		if (argCount == 1 && strcmp(params[0], "CONFIRM") != 0)
		{
			sendToOutputln("Unknown parameter, expected nothing or CONFIRM (all caps).");
			return;
		}
	}
	config.telnetEnabled = false;

	WiFiClient serverClient = server.available();
	serverClient.stop();
	server.stop();

	saveSchSettingsToSpiffs();
	sendToOutputln("Telnet disabled.");
}

void SerialManager::format(char** params, int argCount)
{
	if (argCount == 1 && strcmp(params[0], "ALL"))
	{
		if (SPIFFS.format())
		{
			sendToOutputln("Spiffs formatted succesfully.");
			return;
		}
		else
		{
			sendToOutputln("Failed to format SPIFFS.");
			return;
		}
	}
	if (argCount == 0)
	{
		SpiffsManager::deleteData();
		return;
	}
	sendToOutputln("Incorrect usage. Correct usage:");
	showHelp("FORMAT");
	return;
}


void SerialManager::batteryHistory(char** params, int argCount)
{
	if (argCount != 1)
	{
		sendToOutputln("Incorrect usage. Correct usage:");
		showHelp("BATTERYHISTORY");
		return;
	}
	int batteryNo = getBatteryNo(params[0]);
	if (batteryNo <= 0)
	{
		return;
	}
	Battery::get(batteryNo)->printHistory();
}



void SerialManager::enableAuto(char** params, int argCount)
{

	if (argCount != 2)
	{

		sendToOutputln("Incorrect usage. Correct usage:");
		showHelp("ENABLEAUTO");
		return;
	}
	int testType = getTestType(params[0]);
	if (testType == -1)
	{
		return;
	}

	int bNo = getBatteryNo(params[1]);
	if (bNo == -1)
	{
		return;
	}
	BatteryTest* bt = ts->findTest(testType, bNo);

	if (bt == NULL)
	{
		sendToOutputln("The selected test is not implemented yet.");
		return;
	}
	bt->enableAutorun();
}


void SerialManager::disableAuto(char** params, int argCount)
{

	if (argCount != 2)
	{

		sendToOutputln("Incorrect usage. Correct usage:");
		showHelp("DISABLEAUTO");
		return;
	}
	int testType = getTestType(params[0]);
	if (testType == -1)
	{
		return;
	}

	int bNo = getBatteryNo(params[1]);
	if (bNo == -1)
	{
		return;
	}
	BatteryTest* bt = ts->findTest(testType, bNo);

	if (bt == NULL)
	{
		sendToOutputln("The selected test is not implemented yet.");
		return;
	}
	bt->disableAutorun();
}



void SerialManager::enableDebug(char** params, int argCount)
{

	if (argCount != 0)
	{

		sendToOutputln("Incorrect usage. Correct usage:");
		showHelp("ENABLEDEBUG");
		return;
	}
	config.debugEnabled = true;
	saveSchSettingsToSpiffs();
}


void SerialManager::disableDebug(char** params, int argCount)
{

	if (argCount != 0)
	{

		sendToOutputln("Incorrect usage. Correct usage:");
		showHelp("ENABLEDEBUG");
		return;
	}
	config.debugEnabled = false;
	saveSchSettingsToSpiffs();
}


int SerialManager :: getBatteryNo(char* input)
{
  float outArg;
  parserUtils::retrieveFloat(input, &outArg);
  int bNo = (int)outArg;
  if (bNo < 1 || bNo>2)
  {
	  sendToOutputln("Invalid battery number. Valid options are: 1, 2.");
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
  sendToOutputln("Invalid test type; valid options are: VOLTAGE, FAST, DISCHARGE.");
  return -1;
}




 SerialManager::SerialManager(TestScheduler* _ts, Communicator* _comm)
 {
	 ts = _ts;
	 comm = _comm;
	 server.begin();
	 server.setNoDelay(true);

	 SerialManager::debugPrint("Ready! Use 'telnet ");
	 SerialManager::debugPrint(WiFi.localIP());
	 sendToOutputln(" 23' to connect");
	 loadSchSettingsFromSpiffs();
	 if (config.telnetEnabled)
	 {
		 enableTelnet(0, 0);
	 }
	 else
	 {
		 disableTelnet(0, 0);
	 }
 }

 int SerialManager::readAvailableInput()
 {
	 if (Serial.available())
	 {
		 lastInputMillis = millis();
		 return Serial.read();
	 }

	 if (!config.telnetEnabled)
	 {
		 return -1;
	 }
	 //check clients for data
	 for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
		 if (serverClients[i] && serverClients[i].connected()) {
			 if (serverClients[i].available()) {
				 //get data from the telnet client and push it to the UART
				 if (serverClients[i].available())
				 {
					 lastInputMillis = millis();
					 int kokon=serverClients[i].read();
					 return kokon;
				 }
			 }
		 }
	 }
	 return -1;

 }

 void SerialManager::scanForTelnetConnections()
 {
	 uint8_t i;
	 //check if there are any new clients
	 if (server.hasClient()) {
		 for (i = 0; i < MAX_SRV_CLIENTS; i++) {
			 //find free/disconnected spot
			 if (!serverClients[i] || !serverClients[i].connected()) {
				 if (serverClients[i]) serverClients[i].stop();
				 serverClients[i] = server.available();
				 sendToOutput("New client: "); sendToOutput((String)""+i);
				 break;
			 }
		 }
		 //no free/disconnected spot so reject
		 if (i == MAX_SRV_CLIENTS) {
			 WiFiClient serverClient = server.available();
			 serverClient.stop();
			 sendToOutputln("Connection rejected ");
		 }
	 }
 }

 void SerialManager::handleData()
 {
	 static int index = 0;
	 int nextByte = 0;
	 while ((nextByte=readAvailableInput())!=-1) {
		 if (nextByte == 8)//backspace
		 {
			 index--;
			 if (index < 0)
			 {
				 index = 0;
			 }
			 continue;
		 }
		 // get the new byte:
		 char inChar = (char)nextByte;
		 // add it to the inputString:
		 if (index >= 400-1)
		 {
			 index = 0;
		 }
		 dataIn[index++] = inChar;
		 if (inChar == '\n') {
			 dataIn[index - 1] = '\0';
			 SerialManager::debugPrint("Got this: \"");
			 SerialManager::debugPrint(dataIn);
			 SerialManager::debugPrintln("\"");
			 index = 0;

			 sendToOutputln("============================================");
		     sendToOutputln("============================================");
			 parseCommand(dataIn);
			 dataIn[0] = '\0';
		 }
	 }
 }


 void SerialManager::sendToOutput(String str)
 {
	 Serial.print(str);
	 
	 if (!config.telnetEnabled)
	 {
		 return;
	 }
    for(int i = 0; i < MAX_SRV_CLIENTS; i++){
      if (serverClients[i] && serverClients[i].connected()){
        serverClients[i].write(str.c_str(), strlen(str.c_str()));
      }
    }
	ElectronicLoad::heartBeat();//so it doesn't crash with long outputs...
 }

 void SerialManager::sendToOutput(char ch)
 {
	 Serial.write(ch);

	 if (!config.telnetEnabled)
	 {
		 return;
	 }

    for(int i = 0; i < MAX_SRV_CLIENTS; i++){
      if (serverClients[i] && serverClients[i].connected()){
		  char tempStr[1];
		  tempStr[0] = ch;
        serverClients[i].write(tempStr,1);
      }
    }
	ElectronicLoad::heartBeat();//so it doesn't crash with long outputs...
 }

 void SerialManager::sendToOutputln(String str)
 {
	 str += "\r\n";
	 sendToOutput(str);
 }


 void SerialManager::debugPrint(char ch)
 {
	 if (config.debugEnabled)
	 {
		 sendToOutput(ch);
	 }
 }

 void SerialManager::debugPrint(String str)
 {
	 if (config.debugEnabled)
	 {
		 sendToOutput(str);
	 }
 }

 void SerialManager::debugPrintln(String str)
 {
	 if (config.debugEnabled)
	 {
		 sendToOutputln(str);
	 }
 }

void SerialManager::loop()
{
	scanForTelnetConnections();
	handleData();
	handleStatusPrintout(); 
}



void SerialManager::parseCommand(char* str)
{
  removeTrailingNewlines(str);
	if (strlen(str)==0)
	{
		return;
	}
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
    //sendToOutput(argsCount);
    fncPtr(&arguments[1], argsCount - 1);
  }
  else
  {
    sendToOutputln("Unknown command. Type HELP to see the list of available commands.");
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
  //SerialManager::debugPrintln(cmds[0].commandString);
  //SerialManager::debugPrintln(strcmp_P("LIST",cmds[0].commandString));
  for (int i = 0; i < CMDS_COUNT; i++)
  {
    if (strcmp(theName, cmds[i].commandString) == 0)
    {
      return i;
    }
  }
  return -1;
}

void SerialManager::showHelp(char* topic)
{
	if (strcmp(topic, "HELP") == 0)
	{
		sendToOutputln(F("HELP|(-optional- TOPIC) - display this help"));
		return;
	}

	if (strcmp(topic, "STARTTEST") == 0)
	{
		sendToOutputln(F("STARTTEST|test type|battery number - for example STARTTEST|VOLTAGE|1 - start a test immediately"));
		printValidTestTypes();
		return;
	}

	if (strcmp(topic, "ENDTEST") == 0)
	{
		sendToOutputln(F("ENDTEST (no arguments) - stop the current test, discarding the results"));
		return;
	}


	if (strcmp(topic, "CONNECTWIFI") == 0)
	{
		sendToOutputln(F("CONNECTWIFI|(-optional- SSID)|(-optional- password) - connect to a given WiFi network"));
		return;
	}



	if (strcmp(topic, "DISCONNECTWIFI") == 0)
	{
		sendToOutputln(F("DISCONNECTWIFI (no arguments) - Disconnect from the WiFi network and turn the WiFi on"));
		return;
	}

	if (strcmp(topic, "SCANWIFI") == 0)
	{
		sendToOutputln(F("SCANWIFI (no arguments) - scan for the present WiFi networks"));
		return;
	}

	if (strcmp(topic, "LASTRESULT") == 0)
	{
		sendToOutputln(F("LASTRESULT|(type of test)|(batteryNumber) - view the last result of the given test"));
		return;
	}
	if (strcmp(topic, "SCHEDULE") == 0)
	{
		sendToOutputln(F("SCHEDULE|(type of test)|(battery number)|(start date)|(period)|(mail settings)"));
		printValidTestTypes();
		sendToOutputln(F("Format for the start date: DD.MM.YYYY HH:MM"));
		sendToOutputln(F("Format for the period: DD:HH:MM"));
		printValidMailSettings();
		return;
	}
	if (strcmp(topic, "SETTIME") == 0)
	{
		sendToOutputln(F("SETTIME|(time and date as DD.MM.YYYY HH:MM) - set the current time and date"));
	}

	if (strcmp(topic, "STATUS") == 0)
	{
		sendToOutputln(F("STATUS(no arguments) - view the status information, such as whether the device is online, what the results of the last tests are, etc."));
	}

	if (strcmp(topic, "CONFIGUREEMAIL") == 0)
	{

		sendToOutputln(F("CONFIGUREEMAIL|(SMTP server)|PORT|(the source email address to send the email from)|(the password associated with the given address)|(target address to send the email to)"));
	}


	if (strcmp(topic, "TESTEMAIL") == 0)
	{

		sendToOutputln(F("TESTEMAIL (no arguments) - send a test email to make sure the settings are correct."));
	}

	if (strcmp(topic, "SETOPTIONS") == 0)
	{

		sendToOutputln(F("SETOPTIONS|(test type)|(battery number)|(option 1)|(option 2)... - set the options for given test;"));
		printValidTestTypes();
		showSetOptionsHelp();

	}

	if (strcmp(topic, "GETOPTIONS") == 0)
	{

		sendToOutputln(F("GETOPTIONS|(test type)|(battery number) - view the current options for the given test"));
		printValidTestTypes();
	}

	if (strcmp(topic, "RESETSTATUS") == 0)
	{

		sendToOutputln(F("RESETSTATUS (no arguments) - when a test fails, no additional tests are permitted to prevent damage to the batteries and to keep the backup functionality of the system. This allows resets the flag that says no tests are currently permitted."));
	}

	if (strcmp(topic, "HISTORY") == 0)
	{
		sendToOutputln(F("HISTORY|(test type)|(battery number) - display the result history for a given test."));
		printValidTestTypes();
	}

	if (strcmp(topic, "ENABLETELNET") == 0)
	{
		sendToOutputln(F("ENABLETELNET (no arguments) - allow telnet for remote management, making it possible to control the device from a computer on the same network (or from the Internet, provided that a VPN is used)"));
	}

	if (strcmp(topic, "DISABLETELNET") == 0)
	{
		sendToOutputln(F("DISABLETELNET (no arguments) - disable the telnet, not allowing any further remote management"));
	}

	if (strcmp(topic, "FORMAT") == 0)
	{
		sendToOutputln(F("FORMAT (optionally FORMAT|ALL) - format the SPIFFS memory, deleting all historical data. If FORMAT|ALL is used, the memory is formatted, which also deletes all settings, reverting to factory defaults."));
	}

	if (strcmp(topic, "BATTERYHISTORY") == 0)
	{
		sendToOutputln(F("BATTERYHISTORY|(battery number) - print the history of the battery properthies"));
	}
	sendToOutputln(F("Unknown command."));
		
}

void SerialManager::showSetOptionsHelp()
{
	BatteryTest* bt;
	String msg;

	bt = ts->findTest(getTestType("FAST"), 1);

	sendToOutputln("Options for "+bt->getName());
	msg = bt->setOptions("", "", "", "", "");
	if (msg != "")
	{
		sendToOutputln(msg);
	}

	bt = ts->findTest(getTestType("DISCHARGE"), 1);
	sendToOutputln("Options for "+bt->getName());
	msg = bt->setOptions("", "", "", "", "");
	if (msg != "")
	{
		sendToOutputln(msg);
	}

	bt = ts->findTest(getTestType("VOLTAGE"), 1);
	sendToOutputln("Options for "+bt->getName());
	msg = bt->setOptions("", "", "", "", "");
	if (msg != "")
	{
		sendToOutputln(msg);
	}
}

void SerialManager::printValidTestTypes()
{
	sendToOutputln(F("Valid test types are: "));
	sendToOutputln(F("VOLTAGE: Open-circuit voltage of the battery when no load is applied. Very fast."));
	sendToOutputln(F("FAST: Test of the open-circuit voltage, voltage under load and the voltage the battery recovers to when the load is removed. Also calculates the internal resistance of the battery."));
	sendToOutputln(F("DISCHARGE: Loads the battery until a certain (user-adjustable) voltage is reached; measures the capacity and extracted energy."));

}

void SerialManager::printValidMailSettings()
{
	sendToOutputln(F("Valid settings for email are: "));
	sendToOutputln(F("ALWAYS: Always send the email once the test is complete"));
	sendToOutputln(F("ONFAIL: Only send the email if the test fails"));
	sendToOutputln(F("NEVER: never send the email"));

}

int SerialManager::getMailSettings(char* str)
{
	if (strcmp(str, "ALWAYS") == 0)
	{
		return 2;
	}

	if (strcmp(str, "ONFAIL") == 0)
	{
		return 1;
	}

	if (strcmp(str, "NEVER") == 0)
	{
		return 0;
	}
	return -1;
}

boolean SerialManager::telnetClientsOnline()
{
	for (int i = 0;i < MAX_SRV_CLIENTS;i++)
	{
		if (serverClients[i] && serverClients[i].connected())
		{
			return true;
		}
	}
	return false;
}


void SerialManager::saveSchSettingsToSpiffs()
{
	char fname[50];
	sprintf(fname, "%s.cfg", "comms");

	//char* fname = (char*)((String)prefix+".cfg").c_str();
	SerialManager::debugPrintln(fname);

	StaticJsonBuffer<350> jsonBuffer;

	// Parse the root object
	JsonObject &root = jsonBuffer.createObject();

	// Set the values
	root["telnetEnabled"] = config.telnetEnabled;
	root["debugEnabled"] = config.debugEnabled;
	root["SSID"] = config.SSID;
	root["pass"] = config.pass;

	SpiffsManager::saveSettings(root, fname);
}

void SerialManager::loadSchSettingsFromSpiffs()
{

	SerialManager::debugPrintln("nacitam nastaveni...");
	StaticJsonBuffer<1000> jb;
	StaticJsonBuffer<1000> *jbPtr = &jb;



	char fname[50];
	sprintf(fname, "%s.cfg", "comms");
	SerialManager::debugPrintln(fname);

	JsonObject& root = SpiffsManager::loadSettings(jbPtr, fname);
	if (root["success"] == false)
	{
		SerialManager::debugPrintln("failnulo to nacitani nastaveni komunikace...");
	return;
	}
	SerialManager::debugPrintln("nacetlo se nastaveni komunikace...");


	config.telnetEnabled = root["telnetEnabled"];
	config.debugEnabled = root["debugEnabled"];

	strlcpy(config.SSID,                   // <- destination
		root["SSID"],
		sizeof(config.SSID));

	strlcpy(config.pass,                   // <- destination
		root["pass"],
		sizeof(config.pass));


	//gui->find((String)prefix+"lbMail")->setDefaultText(config.mailSettings);
}
