// 
// 
// 

#include "StatusDisplay.h"
#include <functional>

#include "SpiffsManager.h"
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include "BatteryTest.h"

 using namespace std::placeholders; 

 StatusDisplay::StatusDisplay(TestScheduler* _ts)
 {
	 this->ts = _ts;
 }

void StatusDisplay::begin()
{
	//this->loadSettingsFromSpiffs();
}

void StatusDisplay::loop()
{
	static unsigned long lastMillis;
	if (millis() - lastMillis < 2000)
	{
		return;
	}
	lastMillis = millis();
	Label* l = (Label*)(this->cont->getGUI()->find("lblCurrentTime"));
	char timeStr[50];
	sprintf(timeStr, "%d:%d:%d %d.%d.%d", hour(), minute(), second(), day(), month(), year());
	l->setText(ALL_CLIENTS, timeStr);
	Label* l2 = (Label*)(this->cont->getGUI()->find("lblIP"));
	//l2->setText(ALL_CLIENTS, WiFi.localIP().toString());
	Label* l3 = (Label*)(this->cont->getGUI()->find("lblSSID"));
	//l3->setText(ALL_CLIENTS, WiFi.SSID());
	
	
	Text* b1t = (Text*)(this->cont->getGUI()->find("sdB1lastResults"));
	BatteryTest* lastTestB1 = ts->getLastTest(1);
	BatteryTest* lastTestB2 = ts->getLastTest(2);
	if (lastTestB1 != NULL)
	{
		//SerialManager::debugPrintln(lastTestB1->getTextResults());
		b1t->setText(ALL_CLIENTS, lastTestB1->getTextResults());
	}
	/*
	Text* b2t = (Text*)(this->cont->getGUI()->find("sdB2lastResults"));
	if (lastTestB2 != NULL)
	{
		b2t->setText(ALL_CLIENTS, lastTestB2->getTextResults());
	}
	*/


}






void StatusDisplay::generateGUI(Container * c)
{

	this->cont = c;
	
	vBox* vb = new vBox("vbSD");//we create a new vertical box - the things in this box will go one under another
	c->add(vb);//we add the vertical box inside the horizontal box we created

	
	Heading* h = new Heading("hOverview", 1, "Overview");//We create heading of level "1", name it "heading1" and change its text.
	vb->add(h);//Always remember to actually add the elements somewhere!


	
	Label* lCurrentTime = new Label("lblCurrentTime", "current time");
	vb->add(lCurrentTime);
	Label* lIP = new Label("lblIP", "IP: ");
	vb->add(lIP);
	Label* lSSID= new Label("lblSSID", "SSID: ");
	vb->add(lSSID);
	Text* lastResultsB1Text = new Text("sdB1lastResults", R"(Last results are something something)");
	Text* lastResultsB2Text = new Text("sdB2lastResults", R"(Last results are something something)");
	vb->add(lastResultsB1Text);
	vb->add(lastResultsB2Text);
}


