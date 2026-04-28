#include "ConfigManager.h"
#include "src/SDManager.h"

#include <ArduinoJson.h>

SDManager sd;
ConfigManager cfgManager(&sd);

void setup() {
  Serial.begin(115200);

  if (!sd.begin()) {
    Serial.println(F("Error: No se pudo iniciar la SD"));
    while(1); //bloqueamos el sistema ya que si no hay SD no se puede hacer nada. 
  }

  DeviceConfig devcfg = cfgManager.getDeviceConfig(); 

  //DeserializationError error = deserializeJson(doc, configFile);

    Serial.println("--- Configuración Cargada ---");
  Serial.print("Dirección inicio: "); Serial.println(devcfg.start_addr);
  Serial.print("Longitud: ");         Serial.println(devcfg.length);
  Serial.print("Intervalo Log: ");    Serial.println(devcfg.log_interval);
  Serial.print("Intervalo Medida: "); Serial.println(devcfg.meas_interval);


}

void loop() {
  // Tu lógica de Modbus y Logs usaría las variables aquí
}

/*
#include "ConfigManager.h"
#include "src/SDManager.h"

#include <ArduinoJson.h>

SDManager sd;


int start_addr;
int length;
long log_interval;
long measure_interval;

void miLogicaJson(Stream& data, void* context) {
    JsonDocument* doc = (JsonDocument*)context;
    deserializeJson(*doc, data);
}

void setup() {
  Serial.begin(115200);

  if (!sd.begin()) {
    Serial.println(F("Error: No se pudo iniciar la SD"));
    while(1); //bloqueamos el sistema ya que si no hay SD no se puede hacer nada. 
  }
  //File configFile = SD.open("/config.jsn");

  JsonDocument doc;
  sd.withFile(String("/config.jsn").c_str(), miLogicaJson, &doc);

  start_addr       = doc["start_addr"] | 0;
  length           = doc["length"]     | 1;
  log_interval     = doc["log_interval"] | 300;
  measure_interval = doc["measure_interval"] | 1000;

  //DeserializationError error = deserializeJson(doc, configFile);

    Serial.println("--- Configuración Cargada ---");
  Serial.print("Dirección inicio: "); Serial.println(start_addr);
  Serial.print("Longitud: ");         Serial.println(length);
  Serial.print("Intervalo Log: ");    Serial.println(log_interval);
  Serial.print("Intervalo Medida: "); Serial.println(measure_interval);


}

void loop() {
  // Tu lógica de Modbus y Logs usaría las variables aquí
}
*/

/*
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

// Variables donde guardaremos los datos leídos
int start_addr;
int length;
long log_interval;
long measure_interval;

void setup() {
  Serial.begin(115200);

  // 1. Inicializar SD
  if (!SD.begin()) {
    Serial.println("Error: No se pudo montar la tarjeta SD");
    return;
  }

  // 2. Abrir el archivo
  File configFile = SD.open("/config.jsn");
  if (!configFile) {
    Serial.println("Error: No se encontró config.json");
    return;
  }

  // 3. Crear el documento JSON (v7)
  JsonDocument doc;

  // 4. Deserializar (leer y parsear)
  DeserializationError error = deserializeJson(doc, configFile);

  // 5. Cerrar el archivo (ya no lo necesitamos, los datos están en 'doc')
  configFile.close();

  if (error) {
    Serial.print("Error en JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // 6. Asignar valores a nuestras variables
  // El operador "|" define un valor por defecto si el JSON está mal
  start_addr       = doc["start_addr"] | 0;
  length           = doc["length"]     | 1;
  log_interval     = doc["log_interval"] | 300;
  measure_interval = doc["measure_interval"] | 1000;

  // 7. Mostrar resultados
  Serial.println("--- Configuración Cargada ---");
  Serial.print("Dirección inicio: "); Serial.println(start_addr);
  Serial.print("Longitud: ");         Serial.println(length);
  Serial.print("Intervalo Log: ");    Serial.println(log_interval);
  Serial.print("Intervalo Medida: "); Serial.println(measure_interval);
}

void loop() {
  // Tu lógica de Modbus y Logs usaría las variables aquí
}

*/
