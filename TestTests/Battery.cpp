// 
// 
// 

#include "Battery.h"
#include "SerialManager.h"
#include "NTPManager.h"

Battery* Battery::bat1 = NULL;
Battery* Battery::bat2 = NULL;

Battery::Battery(int _batteryNo)
{
	batteryNo = _batteryNo;
}

void Battery::updateProperthies(double _OCV, double _Ri, double _c)
{
	time_t timeNow = now();
	String lineStr = NTPManager::dateToString(timeNow)+"\t";
	
	if (_OCV > 0)//new value
	{
		OCV = _OCV;
		lineStr+=(String(OCV, 3));
		this->OCVDate = timeNow;
	}
	else
	{
		lineStr+=("(");
		lineStr+=(String(OCV, 3));
		lineStr+=(")");
	}

	lineStr+=("\t");



	if (_Ri > 0)//new value
	{
		Ri = _Ri;
		lineStr+=(String(Ri, 3));
		this->RiDate = timeNow;
	}
	else
	{
		lineStr+=("(");
		lineStr+=(String(Ri, 3));
		lineStr+=(")");
	}

	lineStr+=("\t");

	if (_c > 0)//new value
	{
		c = _c;
		lineStr+=(String(c, 3));
		this->cDate = timeNow;
	}
	else
	{
		lineStr+=("(");
		lineStr+=(String(c, 3));
		lineStr+=(")");
	}

	char fName[50];
	sprintf(fName, "b%d.data", this->batteryNo);
	SpiffsManager::appendLineTo(fName, (char* )(lineStr.c_str()));
}


void Battery::printProperthies()
{
	char tempStr[300];
	sprintf(tempStr, "Open-circuit voltage:\t\t%.2fV \t\t(%s)\r\nInternal resistance: \t\t%.2fOhms \t(%s) \r\nCapacity: \t\t\t%.2fAh \t\t(%s) \r\n", OCV,NTPManager::dateToString(OCVDate).c_str(), Ri,
		NTPManager::dateToString(RiDate).c_str(), c, NTPManager::dateToString(cDate).c_str());

	if (OCV > 0)
	{
		SerialManager::sendToOutputln("Open-circuit voltage: \t\t" + String(OCV, 3) + "V\t\t("+NTPManager::dateToString(this->OCVDate)+")");
	}
	else
	{
		SerialManager::sendToOutputln("Open-circuit voltage: \t\t (not measured yet)");
	}

	if (Ri> 0)
	{
		SerialManager::sendToOutputln("Internal resistance: \t\t" + String(Ri, 3) + "Ohms\t\t("+NTPManager::dateToString(this->RiDate)+")");
	}
	else
	{
		SerialManager::sendToOutputln("Internal resistance: \t\t (not measured yet)");
	}

	if (c>0)
	{
		SerialManager::sendToOutputln("Capacity: \t\t" + String(c, 3) + "Ah\t\t("+NTPManager::dateToString(this->cDate)+")");
	}
	else
	{
		SerialManager::sendToOutputln("Capacity: \t\t (not measured yet)");
	}

}


void Battery::printHistory()
{
	char fname[50];
    sprintf(fname, "b%d.data", this->batteryNo);
	File f = SPIFFS.open(fname, "r");
	SerialManager::sendToOutputln(F("Date\tU_oc\tR_i\tC"));
	while (f.available())
	{
		SerialManager::sendToOutput(f.read());
		//Serial.write(f.read());
	}
	f.close();
}

int Battery::getNumber()
{
	return this->batteryNo;
}

Battery* Battery::get(int _batteryNo)
{
	if (bat1 == NULL)
	{
		bat1 = new Battery(1);
	}

	if (bat2 == NULL)
	{
		bat2 = new Battery(2);
	}

	if (_batteryNo == 1)
	{
		return bat1;
	}

	if (_batteryNo == 2)
	{
		return bat2;
	}
	return 0;
}

