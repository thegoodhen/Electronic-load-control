#include "ElectronicLoad.h"
int ElectronicLoad::connectedBattery = -1;
unsigned long ElectronicLoad::lastQueryTimestamp =0;
int ElectronicLoad::spiInIndex = 0;
int ElectronicLoad::spiOutIndex = 0;
volatile boolean ElectronicLoad::dataSent= false;

uint8_t ElectronicLoad::spiDataOut[32];



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
      Serial.println("timeout!!");
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

	/*
    Serial.println(parseSPIByte(data));
    Serial.println(parseSPIFloat(data));
    Serial.println(parseSPIFloat(data));
    Serial.println("chksum ok?");
    //delay(1);//just as a test(FAILED, damnit...)

    Serial.println(isChksumOk(data));
	*/
}

void ElectronicLoad::onDataSent()
{
    digitalWrite(D0, LOW);
    Serial.println("Answer Sent");
    dataSent = true;
}

void ElectronicLoad::begin()
{
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
  pinMode(D0, OUTPUT);
}

int ElectronicLoad::connectBattery(int batteryNo)
{
  queueByte(0);//connect battery
  queueByte(batteryNo);
  sendData(spiDataOut, spiOutIndex);
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