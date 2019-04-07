// 
// 
// 

#include "parserUtils.h"

/**


**/
//>=0 is good,  <0 is bad.
int parserUtils::retrieveFloat(const char * str, float* outArg)
{
	char* endptr;
	*outArg = strtof(str, &endptr);
	if (endptr == str)//no number in the input
	{
		return -1;
	}

	if (endptr == '\0')//the input contains a number and only the number
	{
		return 0;
	}

	return 1;//the input contains a number, but also something else...
}

int parserUtils::retrieveNLongs(const char * str, int n, long* outArr)
{

	char str2[100];//kinda unsafe, oof
	strncpy(str2, str, 100);
	replaceNonDigitsWithSpaces(str2);

	char* endptr=(char*)str2;
	for (int i = 0;i < n;i++)
	{

		outArr[i] = strtol(endptr, &endptr, 10);
		Serial.println(endptr);
		if (endptr == str2)//no number in the input
		{
			return 0;
		}
		if (*endptr == '\0')
		{
			return i + 1;
		}
	}
	return n;
}

void parserUtils::replaceNonDigitsWithSpaces(char* str)
{
	for (int i = 0;i < strlen(str);i++)
	{
		if (!isdigit(str[i]))
		{
			str[i] = ' ';
		}
	}
}