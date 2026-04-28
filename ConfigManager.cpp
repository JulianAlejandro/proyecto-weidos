#include "ConfigManager.h"
#include <ArduinoJson.h>

ConfigManager::ConfigManager(SDManager* sdManager){
  _filename = "/config.jsn";
  _sd = sdManager;
}


void ConfigManager::staticCallback(Stream& data, void* context) {
    JsonDocument* doc = (JsonDocument*)context; 
    DeserializationError error = deserializeJson(*doc, data);
    if(error){
      Serial.println("hay un error en el desentralizedError");
    }
    //deserializeJson(*doc, data);
    Serial.print("Tamaño del chorro de datos: ");
    Serial.println(data.available());
}

DeviceConfig ConfigManager::getDeviceConfig() {
    // 1. Asegurarse de que la SD esté lista
    if (!_sd || !_sd->isReady()) {
        Serial.println(F("Error: SD no lista en ConfigManager"));
        return _config; 
    }

    // 2. Usar una estructura interna para capturar los datos
    // En ArduinoJson 7, JsonDocument es dinámico y suele bastar así:
    JsonDocument doc;

    // 3. Definir una lambda o usar el callback para llenar el doc
    bool success = _sd->withFile(_filename, [](Stream& data, void* context) {
        JsonDocument* d = (JsonDocument*)context;
        DeserializationError error = deserializeJson(*d, data);
        if (error) {
            Serial.print(F("Error deserializando JSON: "));
            Serial.println(error.c_str());
        }
    }, &doc);

    if (!success) {
        Serial.println(F("No se pudo abrir el archivo de config."));
        return _config;
    }

    // 4. Mapeo de datos con validación
    // Nota: Si doc["campo"] no existe, se usará el valor tras el pipe '|'
    _config.start_addr = doc["start_addr"] | 1;
    _config.length = doc["length"] | 1;
    _config.log_interval_s = (doc["log_interval_sec"] | 60); // Guardamos en segundos
    _config.meas_interval_ms = doc["measure_interval_ms"] | 1000;

    return _config;
}
/*
 DeviceConfig ConfigManager::getDeviceConfig(){

  if (!_sd || !_sd->isReady()) {
        Serial.println(F("Error: SD no lista en ConfigManager"));
        return _config; 
    }

    JsonDocument doc;
    //DeserializationError error = deserializeJson(doc, file);
    _sd->withFile(_filename, staticCallback, &doc);

   //if (error) return false;

        // Mapeo de JSON a la estructura C++

  _config.start_addr = doc["start_addr"] | 1;
  _config.length = doc["length"] | 1;
  _config.log_interval_s = (doc["log_interval_sec"] | 60) * 1000UL;
  _config.meas_interval_ms = doc["measure_interval_ms"] | 1000;

    return _config; 
 }
 */