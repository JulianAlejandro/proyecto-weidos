#include "ConfigManager.h"
#include <ArduinoJson.h>

ConfigManager::ConfigManager(SDManager* sdManager){
  _filename = String("/config.jsn").c_str();
  _sd = sdManager;
}


void ConfigManager::staticCallback(Stream& data, void* context) {
    JsonDocument* doc = (JsonDocument*)context; 
    //DeserializationError error = deserializeJson(*doc, data);
    deserializeJson(*doc, data);
}

 DeviceConfig ConfigManager::getDeviceConfig(){
    JsonDocument doc;
    //DeserializationError error = deserializeJson(doc, file);
    _sd->withFile(_filename, staticCallback, &doc);

   //if (error) return false;

        // Mapeo de JSON a la estructura C++
    _config.start_addr   = doc["modbus"]["start_addr"] | 1;
    _config.length       = doc["modbus"]["length"] | 1;
    _config.log_interval = doc["intervals"]["log_interval_sec"] | 60;
    _config.meas_interval = doc["intervals"]["meas_interval_ms"] | 1000;

    return _config; 
 }