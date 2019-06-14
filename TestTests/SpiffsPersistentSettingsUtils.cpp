// 
// 
// 

#include "SpiffsPersistentSettingsUtils.h"

void SpiffsPersistentSettingsUtils::begin()
{
	SPIFFS.begin();
}

JsonObject& SpiffsPersistentSettingsUtils::loadSettings(StaticJsonBuffer <1000> *jb, char * filename)
{
	if (!SPIFFS.exists(filename))
	{
		JsonObject& obj=jb->createObject();
		obj["success"] = false;
		return obj;
	}
	File f = SPIFFS.open(filename, "r");
	//StaticJsonBuffer<1000> jb;
	
	JsonObject& obj = jb->parseObject(f);
	f.close();
	obj["success"] = true;
	obj.printTo(Serial);
	return obj;
}

void SpiffsPersistentSettingsUtils::saveSettings(JsonObject& obj, char * filename)
{
	File f = SPIFFS.open(filename, "w");
	obj.printTo(f);
	f.write(0);
	f.close();
}


void SpiffsPersistentSettingsUtils::appendLineTo(char* filename, char* line)
{
	File f = SPIFFS.open(filename, "a");
	f.println(line);
	f.close();
}
