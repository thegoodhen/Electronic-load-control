// 
// 

#include "Communicator.h"
#include <functional>

#include "SpiffsPersistentSettingsUtils.h"

 using namespace std::placeholders; 

//taken partially from: https://www.electronicshub.org/send-an-email-using-esp8266/

Communicator::Communicator(WiFiClient theClient, char* smtpServer, char* sourceAddr, char* sourcePassword, char* targetAddr, int portNumber, unsigned long phoneNumber)
{
	strcpy(config.smtpServer, smtpServer);
	this->theClient = theClient;
	strcpy(config.sourceAddr, sourceAddr);
	strcpy(config.sourcePass, sourcePassword);
	strcpy(config.targetAddr, targetAddr);
	strcpy(config.targetAddr, targetAddr);
	config.portNumber = portNumber;
}

void Communicator::begin()
{
	this->loadSettingsFromSpiffs();
}

int Communicator::login()
{
	if (theClient.connect(config.smtpServer, config.portNumber) == 1)
	{
		Serial.println(F("connected"));
	}
	else
	{
		Serial.println(F("connection failed"));
		return 1;
	}

	if (!emailResp())
		return 1;

	Serial.println(F("Sending EHLO"));
	theClient.println("EHLO www.example.com");
	if (!emailResp())
		return 0;
	//
	/*Serial.println(F("Sending TTLS"));
	  espClient.println("STARTTLS");
	  if (!emailResp())
	  return 0;*/
	  //
	Serial.println(F("Sending auth login"));
	theClient.println("AUTH LOGIN");
	if (!emailResp())
		return 0;
	//
	Serial.println(F("Sending User"));
	// Change this to your base64, ASCII encoded username
	/*
	  For example, the email address test@gmail.com would be encoded as dGVzdEBnbWFpbC5jb20=
	*/
	char output[200];

	b64_encode(output, config.sourceAddr, strlen(config.sourceAddr));
	theClient.println(output);//print the source email address

	Serial.println(F("Sending Password"));
	b64_encode(output, config.sourcePass, strlen(config.sourcePass));
	theClient.println(output);//print the source password
	if (!emailResp()) 
		return 1;


	//espClient.println("ZHNpYnJhdmE="); //base64, ASCII encoded Username

	// change to your base64, ASCII encoded password
	/*
	  For example, if your password is "testpassword" (excluding the quotes),
	  it would be encoded as dGVzdHBhc3N3b3Jk
	*/




	return 0;
}

int Communicator::sendHeader(String subject)
{
	Serial.println(F("Sending From"));
	// change to sender email address
	theClient.print(F("MAIL From: <")); 
	theClient.print(config.sourceAddr);
	theClient.println(">");

	if (!emailResp())
		return 1;
	// change to recipient address
	Serial.println(F("Sending To"));
	theClient.print(F("RCPT To: <"));
	theClient.print(config.targetAddr);
	theClient.println(">");

	if (!emailResp())
		return 1;
	//

	Serial.println(F("Sending DATA"));
	theClient.println(F("DATA"));
	if (!emailResp())
		return 1;
	Serial.println(F("Sending email"));
	// change to recipient address
	theClient.print(F("To:  "));
	theClient.println(config.targetAddr);
	// change to your address
	theClient.print(F("From: "));
	theClient.println(config.sourceAddr);
	theClient.print(F("Subject: "));
	theClient.println(subject);

	theClient.println("Mime-Version: 1.0;");
	theClient.println("Content-Type: text/html; charset=\"ISO - 8859 - 1\";");
	theClient.print("Content-Transfer-Encoding: 7bit;");
	theClient.println("\r\n");
	theClient.println("<html>");
	theClient.println("<body>");


	

	return 0;
}

int Communicator::printText(String text)
{
	theClient.print(text);
	return 0;
}

int Communicator::exit()
{

  theClient.println("</body>");
  theClient.println("</html>");
  theClient.println();
  theClient.println(F("."));
  if (!emailResp())
    return 0;
  //
  Serial.println(F("Sending QUIT"));
  theClient.println(F("QUIT"));
  if (!emailResp())
    return 0;
  //
   theClient.stop();
  Serial.println(F("disconnected"));
	return 0;
}

byte Communicator::emailResp()
{
  byte responseCode;
  byte readByte;
  int loopCount = 0;

  while (!theClient.available())
  {
    delay(1);
    loopCount++;
    // Wait for 20 seconds and if nothing is received, stop.
    if (loopCount > 20000)
    {
      theClient.stop();
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }

  responseCode = theClient.peek();
  while (theClient.available())
  {
    readByte = theClient.read();
    Serial.write(readByte);
  }

  if (responseCode >= '4')
  {
    //  efail();
    return 0;
  }
  return 1;
}

void Communicator::sendTestEmail()
{
	login();
	sendHeader("TEST EMAIL");//TODO: make sure that this changes when we failed
	printText("This is just a test email. If you got it, it means your SMTP server settings are correct.");
	exit();
}

void Communicator::generateGUI(Container * c)
{

	this->cont = c;
	
	vBox* vb = new vBox("vBoxComm");//we create a new vertical box - the things in this box will go one under another
	c->add(vb);//we add the vertical box inside the horizontal box we created

	
	Heading* h = new Heading("hCommSettings", 1, "Communication settings");//We create heading of level "1", name it "heading1" and change its text.
	//vb->add(h);//Always remember to actually add the elements somewhere!
	Text* t = new Text("commDescText", R"(In this section you can access the various settings related to the email and SMS settings)");//We add some explanation
	//vb->add(t);


	

	TextInput* tiSMTPServer = new TextInput("tiSMTPServer", "SMTP server address");
	//vb->add(tiSMTPServer);

	

	TextInput* tiPort = new TextInput("tiPort", "The port");
	//TextInput* tiPort = new TextInput("tiPort", "The port used to communicate with the SMTP server (typically 25)");
	//vb->add(tiPort);


	
	TextInput* tiSourceAddr= new TextInput("tiSourceAddr", "The username (email address) to log with");
	vb->add(tiSourceAddr);

	TextInput* tiPass = new TextInput("tiPass", "The password connected with the given email address");
	//vb->add(tiPass);

	TextInput* tiTargetAddr = new TextInput("tiTargetAddr", "The email address to send the emails to");
	vb->add(tiTargetAddr);

	TextInput* tiTargetNum= new TextInput("tiTargetNum", "The phone number to send the SMS messages to");
	vb->add(tiTargetNum);

	
	auto f2 = std::bind(&Communicator::saveSettingsCallback, this, _1);

	Button* btnSaveSettings= new Button("bscs", "Save settings", f2);


	vb->add(btnSaveSettings);
	Button* btnRecallSettings= new Button("btnRecallCommunicatorSettings", "Recall sstored settings", NULL);
	vb->add(btnRecallSettings);

	this->loadSettingsFromSpiffs();	
	

}

void Communicator::saveSettingsCallback(int user)
{
	GUI* gui;
	gui = this->cont->getGUI();
	Serial.println("Saving communicator settings");

	String smtpServer = gui->find("tiSMTPServer")->retrieveText(user);
	String thePort = gui->find("tiPort")->retrieveText(user);
	String sourceAddr = gui->find("tiSourceAddr")->retrieveText(user);
	String sourcePass = gui->find("tiPass")->retrieveText(user);
	String targetAddr = gui->find("tiTargetAddr")->retrieveText(user);
	//String phoneNum = gui->find("tiTargetNum")->retrieveText(user);

	//phoneNum.toCharArray(config.phoneNumber, 50);
	saveSettings(smtpServer, thePort, sourceAddr, sourcePass, targetAddr);
}

void Communicator::saveSettings(String smtpServer, String thePort, String sourceAddr, String sourcePass, String targetAddr)
{
	smtpServer.toCharArray(config.smtpServer, 50);
	parserUtils::retrieveNLongs(thePort.c_str(), 1, &config.portNumber);//TODO: check for errors
	sourceAddr.toCharArray(config.sourceAddr, 50);
	sourcePass.toCharArray(config.sourcePass, 50);
	targetAddr.toCharArray(config.targetAddr, 50);
	saveSettingsToSpiffs();
	sendTestEmail();
}

void Communicator::saveSettingsToSpiffs()
{
	char* fname = "comm.cfg";

	StaticJsonBuffer<350> jsonBuffer;

	// Parse the root object
	JsonObject &root = jsonBuffer.createObject();

	// Set the values
	root["smtpServer"] = config.smtpServer;
	root["portNumber"] = config.portNumber;
	root["sourceAddr"] = config.sourceAddr;
	root["sourcePass"] = config.sourcePass;
	root["targetAddr"] = config.targetAddr;
	root["phoneNumber"] = config.phoneNumber;
	root["portNumber"] = config.portNumber;
	SpiffsPersistentSettingsUtils::saveSettings(root, fname);
}

void Communicator::loadSettingsFromSpiffs()
{

	StaticJsonBuffer<1000> jb;
	StaticJsonBuffer<1000> *jbPtr = &jb;
	JsonObject& root = SpiffsPersistentSettingsUtils::loadSettings(jbPtr, "comm.cfg");
	if (root["success"] == false)
	{
		Serial.println("ajta, failnulo to...");
	return;
	}

	strlcpy(config.smtpServer,                   // <- destination
		root["smtpServer"],
		sizeof(config.smtpServer));




	Serial.println("smtp server info loaded:");
	Serial.println(config.smtpServer);

	config.portNumber = root["portNumber"];

	strlcpy(config.sourceAddr,                   // <- destination
		root["sourceAddr"],
		sizeof(config.sourceAddr));

	strlcpy(config.sourcePass,                   // <- destination
		root["sourcePass"],
		sizeof(config.sourcePass));

	strlcpy(config.targetAddr,                   // <- destination
		root["targetAddr"],
		sizeof(config.targetAddr));

	strlcpy(config.phoneNumber,                   // <- destination
		root["phoneNumber"],
		sizeof(config.phoneNumber));


	/*
	GUI* gui = this->cont->getGUI();

	gui->find("tiSMTPServer")->setDefaultText(config.smtpServer);
	gui->find("tiPort")->setDefaultText((String)config.portNumber);
	gui->find("tiSourceAddr")->setDefaultText((String)config.sourceAddr);
	gui->find("tiPass")->setDefaultText((String)config.sourcePass);
	gui->find("tiTargetAddr")->setDefaultText((String)config.targetAddr);
	gui->find("tiTargetNum")->setDefaultText((String)config.phoneNumber);
	*/
}