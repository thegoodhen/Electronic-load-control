#include "ElectronicLoad.h"
#include "TimeLib.h"
int ElectronicLoad::connectedBattery = -1;
boolean ElectronicLoad::isResultFresh = false;
unsigned long ElectronicLoad::lastQueryTimestamp =0;
int ElectronicLoad::spiInIndex = 0;
int ElectronicLoad::spiOutIndex = 0;
volatile boolean ElectronicLoad::dataSent= false;

float ElectronicLoad::I = 0;
float ElectronicLoad::U1 = 0;
float ElectronicLoad::U2 = 0;

uint8_t ElectronicLoad::spiDataOut[32];




float ElectronicLoad::parseSPIFloat(uint8_t* data)
{
  int startIndex = spiInIndex;
  byte floatBytes[4];
  for (int i = 0; i < 4; i++)
  {
    floatBytes[i] = data[startIndex + i];
  }
  spiInIndex += 4;
  return *((float*)(floatBytes));
}

unsigned long ElectronicLoad::parseSPIUL(uint8_t* data)
{
  int startIndex = spiInIndex;
  byte ULBytes[4];
  for (int i = 0; i < 4; i++)
  {
    ULBytes[i] = data[startIndex + i];
  }
  spiInIndex += 4;
  return *((unsigned long *)(ULBytes));
}


byte ElectronicLoad::parseSPIByte(uint8_t* data)
{
  return data[spiInIndex++];
}

/**
      Schedule a float to be sent over SPI
*/
void ElectronicLoad::queueFloat(float f)
{
  byte *b = (byte *)&f;

  for (int i = 0; i < 4; i++)
  {
    spiDataOut[spiOutIndex++] = b[i];
  }
}


/**
      Schedule an unsigned long to be sent over SPI
*/
void ElectronicLoad::queueUL(unsigned long f)
{
  byte *b = (byte *)&f;

  for (int i = 0; i < 4; i++)
  {
    spiDataOut[spiOutIndex++] = b[i];
  }
}


/**
  Schedule a byte to be sent over SPI
*/
void ElectronicLoad::queueByte(byte b)
{

  spiDataOut[spiOutIndex++] = b;

}




/**
   @return 0 on success, 1 on timeout

*/

int ElectronicLoad::sendData(uint8_t* data, int len, unsigned long timeout)
{
  unsigned long startMillis = millis();
  for (int i = len; i < 31; i++)
  {
    data[i] = 0; //0-padding
  }
  data[31] = calcCheckSum(data);
  SPISlave.setData(data, 32);
  spiOutIndex = 0;
  dataSent = false;
  digitalWrite(D0, HIGH);
  while (dataSent == false)
  {
    delay(1);
    if (millis() - startMillis > timeout)
    {
      Serial.println("timeout when waiting for the slave to poll data!!");
      return 1;
    }
  }
  return 0;
}

/**
   @return 0 on success, 1 on timeout

*/
int ElectronicLoad::sendData(uint8_t* data, int len)
{
  return sendData(data, len, 1000);
}


void ElectronicLoad::onData(uint8_t* data, size_t len)
{
    spiInIndex = 0;
	isResultFresh = true;

	
    Serial.println(parseSPIByte(data));
    U1=parseSPIFloat(data);
    U2=parseSPIFloat(data);
    I=parseSPIFloat(data);
    ::setTime(parseSPIUL(data));
    //Serial.println(parseSPIFloat(data));
    //Serial.println(parseSPIFloat(data));
    //Serial.println("chksum ok?");
    //delay(1);//just as a test(FAILED, damnit...)

    //Serial.println(isChksumOk(data));
	
}

void ElectronicLoad::onDataSent()
{
    digitalWrite(D0, LOW);
    Serial.println("Answer Sent");
    dataSent = true;
}


boolean ElectronicLoad::areNewReadingsReady()
{
	if (isResultFresh)
	{
		isResultFresh = false;
		return true;
	}
	return false;
}

void ElectronicLoad::begin()
{
  pinMode(D0, OUTPUT);
  digitalWrite(D0, LOW);

  SPISlave.onData([](uint8_t * data, size_t len) {
	  onData(data, len);	
  });

  // The master has read out outgoing data buffer
  // that buffer can be set with SPISlave.setData
  SPISlave.onDataSent([]() {
	  ElectronicLoad::onDataSent();
  });


  // Setup SPI Slave registers and pins
  SPISlave.begin();
}

int ElectronicLoad::connectBattery(int batteryNo)
{
  queueByte(0);//connect battery
  queueByte(batteryNo);
  sendData(spiDataOut, spiOutIndex);
  return 0;
}


int ElectronicLoad::requestTime()
{
  queueByte(4);//request time
  sendData(spiDataOut, spiOutIndex);
  Serial.println("Requesting time...");
  return 0;
}


uint8_t ElectronicLoad::calcCheckSum(uint8_t* data)
{
  uint8_t chksum = 0;
  for (int i = 0; i < 31; i++)
  {
    chksum ^= data[i];
  }
  return chksum;
}

boolean ElectronicLoad::isChksumOk(uint8_t* data)
{
  return calcCheckSum(data) == data[31];
}

int ElectronicLoad::setI(float theI)
{
    queueByte(1);//set current
    queueFloat(theI);
    return sendData(spiDataOut, spiOutIndex);
}

int ElectronicLoad::setTime(time_t theTime)
{
    queueByte(3);//set time
    queueUL(theTime);
    return sendData(spiDataOut, spiOutIndex);
}

int ElectronicLoad::setUpdatePeriod(float thePeriod)
{
	queueByte(2);//set period
    queueFloat(thePeriod);
    return sendData(spiDataOut, spiOutIndex);
}

int ElectronicLoad::getI(float * target)
{
	*target = I;
	//*target = random(100);
	return 0;
}

int ElectronicLoad::getU1(float * target)
{
	static float returnVal;


	*target = U1;

	return 0;
}

int ElectronicLoad::getU(float* target, int batteryNo)
{
	if (batteryNo == 1)
	{
		return getU1(target);
	}
	if (batteryNo == 2)
	{
		return getU2(target);
	}
	return -1;

}

int ElectronicLoad::getU2(float * target)
{
	static float returnVal;


	*target = U2;

	return 0;
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

void ElectronicLoad::heartBeat()
{
	static unsigned long lastMillis;	
	static boolean currentHBState = false;
	if (millis() - lastMillis > 1000)
	{
		if (now() < 10000)
		{
			requestTime();
		}
		currentHBState = !currentHBState;
		pinMode(D1, OUTPUT);
		digitalWrite(D1, currentHBState);
		lastMillis = millis();
	}
}