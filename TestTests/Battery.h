// Battery.h

#ifndef _BATTERY_h
#define _BATTERY_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class Battery
{
private:
	int batteryNo;
	double OCV=-1;
	double Ri=-1;
	double c=-1;
	time_t OCVDate=0;
	time_t RiDate=0;
	time_t cDate=0;
	static Battery* bat1;
	static Battery* bat2;
	Battery(int _batteryNo);
 protected:


 public:
	 void updateProperthies(double _OCV, double _Ri, double _c);
	 void printHistory();
	 int getNumber();
	 static Battery* get(int _batteryNo);
	 void printPartialHistory();
	 void printProperthies();
};


#endif

