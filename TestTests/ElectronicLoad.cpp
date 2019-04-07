#include "ElectronicLoad.h"
int ElectronicLoad::connectedBattery = -1;
unsigned long ElectronicLoad::lastQueryTimestamp =0;

int ElectronicLoad::connectBattery(int batteryNo)
{
	pinMode(D2, OUTPUT);
	if (batteryNo == -1)
	{
		digitalWrite(D2, LOW);
		return 0;
	}
	digitalWrite(D2, HIGH);
	connectedBattery = batteryNo;
	return 0;
}

int ElectronicLoad::setI(float theI)
{
	return 0;
}

int ElectronicLoad::setUpdatePeriod(float thePeriod)
{
	return 0;
}

int ElectronicLoad::getI(float * target)
{
	*target = analogRead(A0);
	//*target = random(100);
	return 0;
}

int ElectronicLoad::getU(float * target)
{
	static float returnVal;


	int state = getState();
	if (state == 0)//we got a fresh new result, that is, not one that we got previously
	{
		recordLastQueryTimestamp();
		returnVal = random(24);
	}

	*target = returnVal;

	return state;
}

int ElectronicLoad::getState()
{
	if (millis()-lastQueryTimestamp<250)
	{
		return RESULT_OBSOLETE;
	}
	return 0;

}

void ElectronicLoad::recordLastQueryTimestamp()
{
	lastQueryTimestamp = millis();
}