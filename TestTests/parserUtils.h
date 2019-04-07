// parserUtils.h

#ifndef _PARSERUTILS_h
#define _PARSERUTILS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class parserUtils
{
public:
	static int retrieveFloat(const char* str, float* outarg);
	static int retrieveNLongs(const char* str, int n, long* outArr);
	static void replaceNonDigitsWithSpaces(char* str);
};

#endif

