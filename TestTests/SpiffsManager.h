// SpiffsManager.h

#ifndef _SpiffsManager_h
#define _SpiffsManager_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <ArduinoJson.h>
#include "FS.h"

class SpiffsManager
{
public:
	static void begin();
	static JsonObject & loadSettings(StaticJsonBuffer<1000>* jb, char * filename);
	static void saveSettings(JsonObject& obj, char* filename);
	static void appendLineTo(char * filename, char * line);
	static void deleteData();
};

#endif

