// 
// 
// 

#include "SpiffsManager.h"
#include "SerialManager.h"

void SpiffsManager::begin()
{
	SPIFFS.begin();
}

JsonObject& SpiffsManager::loadSettings(StaticJsonBuffer <1000> *jb, char * filename)
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

void SpiffsManager::saveSettings(JsonObject& obj, char * filename)
{
	File f = SPIFFS.open(filename, "w");
	obj.printTo(f);
	f.write(0);
	f.close();
}


void SpiffsManager::appendLineTo(char* filename, char* line)
{
	File f = SPIFFS.open(filename, "a");
	f.println(line);
	f.close();
}

void SpiffsManager::deleteData()
{
	SerialManager::debugPrintln("GEKITY GEK");
	Dir dir = SPIFFS.openDir("");
	while (dir.next()) {
		String s=dir.fileName();
		if (s.endsWith(".data"))
		{
			SPIFFS.remove(s);
		}
	}
}
