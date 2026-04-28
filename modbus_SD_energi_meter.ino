



#include <Ethernet.h>
#include <ArduinoModbus.h>
#include <RTClib.h>

//Librerias mias 
#include "src/Datalogger.h"
//#include "src/EnergyMeter/EnergyMeter750.h"
#include "src/SDManager.h"

#include "src/EnergyMeter/EnergyMeterRegInterpreter.h"
#include "ConfigManager.h"

// --- Configuración de Red ---
/*
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
IPAddress ip(192, 168, 0, 10);
IPAddress server(192, 168, 0, 100); // update with the IP Address of your Modbus server

EthernetClient ethClient;
ModbusTCPClient modbusTCPClient(ethClient);
*/
RTC_DS3231 rtc;

//---Objetos de Gestion 
SDManager sd;
Datalogger datalogger(&sd);
EnergyMeterRegInterpreter regInterpreter(&sd);
ConfigManager cfgManager(&sd);

//EnergyMeter750 energy_meter(1); // TODO ID ES DE MODBUS MAL 

//----codigo a revisar 
//#define SLAVE_ADDRESS 1 // A eliminar posiblemente 

unsigned long anteriorMillisModbus = 0; // Almacena la última vez que leíste

unsigned long anteriorMillisArchivo = 0;

//EM_request req;              // El contador que quieres incrementar

void lectura_modbus();
void crear_nueva_sesion_log();

titlesBuffer misTitulos;
DeviceConfig devcfg; 

void setup() {
  
  Serial.begin(115200);

  if (!sd.begin()) {
    Serial.println(F("Error: No se pudo iniciar la SD"));
    while(1); //bloqueamos el sistema ya que si no hay SD no se puede hacer nada. 
  }

  if(! datalogger.begin()){ // mira si esta creada el directorio de log si no esta creado , lo crea
    Serial.println("Error en encendido de datalogger");
  }
 
  if(! regInterpreter.begin()){
    Serial.println("Fallo reg interpretert");
  }
  
  if (! rtc.begin()) { // todo while(1) bloquea al micro, cambiar
     Serial.print("Fallo RTC");
  }

  devcfg = cfgManager.getDeviceConfig(); 

/*
  Serial.println("--- Configuración Cargada ---");
  Serial.print("Dirección inicio: "); Serial.println(devcfg.start_addr);
  Serial.print("Longitud: ");         Serial.println(devcfg.length);
  Serial.print("Intervalo Log: ");    Serial.println(devcfg.log_interval_s);
  Serial.print("Intervalo Medida: "); Serial.println(devcfg.meas_interval_ms);
*/

  //req = regInterpreter.startNewRequest(19000, 10);
  regInterpreter.startNewRequest(devcfg.start_addr, devcfg.length);
  misTitulos = regInterpreter.getTitles();
  /*
  if(!datalogger.newSesion(String("inicial").c_str(), misTitulos.buffer, misTitulos.size)){
    Serial.println("algo salio regular");
  }
  */
  crear_nueva_sesion_log(); // creamos un nuevo log con el timestamp de ese momento

  Serial.println("Empieza el loop");
}

void loop() {
    unsigned long actualMillis = millis();

    // --- Bucle de lectura Modbus ---
    // Usamos devcfg.meas_interval_ms para controlar la frecuencia de muestreo
    if (actualMillis - anteriorMillisModbus >= devcfg.meas_interval_ms) {
        anteriorMillisModbus = actualMillis;
        lectura_modbus();       
    }

    // --- Bucle de nueva sesión de log ---
    // Convertimos log_interval_s (segundos) a milisegundos para la comparación
    if (actualMillis - anteriorMillisArchivo >= (devcfg.log_interval_s * 1000UL)) {
        anteriorMillisArchivo = actualMillis;
        crear_nueva_sesion_log();
    }
}

void lectura_modbus() {
    // SIMULACIÓN DE DATOS
    Serial.println("Escribiendo un nuevo valor en el fichero");
    const char* datosPrueba[] = {"230.5", "1.25", "285.0", "50.01", "0.98"};

    DateTime now = rtc.now();
    char bufferTime[20];
    sprintf(bufferTime, "%04d-%02d-%02d %02d:%02d:%02d", 
            now.year(), now.month(), now.day(), 
            now.hour(), now.minute(), now.second());

    if(!datalogger.writeRow(bufferTime, datosPrueba, 5)){
        Serial.println("Error escribiendo en SD"); 
    } 
}

void crear_nueva_sesion_log() {
    
    DateTime ahora = rtc.now();
    char nombreFichero[25]; 

   // sprintf(nombreFichero, "%02d%02d%02d%02d%02d%02d.txt", 
    
    sprintf(nombreFichero, "%02d%02d%02d.txt", 
            //ahora.year() % 100, // Usamos % 100 para obtener solo "26" de "2026"
            //ahora.month(), 
            //ahora.day(), 
            ahora.hour(), 
            ahora.minute(), 
            ahora.second());
    
    Serial.print("Cambiando a nueva sesion: ");
    Serial.println(nombreFichero);
       
    if(!datalogger.newSesion(nombreFichero, misTitulos.buffer, misTitulos.size)){
        Serial.println("Error al crear el archivo por timestamp");
    }
}


/*
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

// Variables globales
int start_addr;
int length;
long log_interval;
long measure_interval;

void setup() {
  Serial.begin(115200);
  while (!Serial) continue; // Esperar a que el monitor serial esté listo

  // 1. Inicializar SD
  if (!SD.begin()) {
    Serial.println("Error: No se pudo montar la tarjeta SD");
    return;
  }

  // 2. Abrir el archivo (Asegúrate que en la SD se llame exactamente config.jsn)
  File configFile = SD.open("/config.jsn");
  if (!configFile) {
    Serial.println("Error: No se encontró el archivo config.jsn");
    return;
  }

  // --- BLOQUE DE DIAGNÓSTICO ---
  Serial.print("Tamaño detectado en SD: ");
  Serial.println(configFile.size());
  Serial.println("Contenido crudo del archivo:");
  
  while (configFile.available()) {
    Serial.write(configFile.read());
  }
  Serial.println("\n----------------------------");
  
  configFile.seek(0); // IMPORTANTE: Volver al inicio para que ArduinoJson pueda leerlo
  // -----------------------------

  // 3. Crear el documento JSON (v7)
  JsonDocument doc;

  // 4. Deserializar
  DeserializationError error = deserializeJson(doc, configFile);

  // 5. Cerrar el archivo
  configFile.close();

  if (error) {
    Serial.print("Error al parsear JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // 6. Asignar valores (Nombres exactos según tu último JSON)
  start_addr       = doc["start_addr"]       | 0;
  length           = doc["length"]           | 1;
  log_interval     = doc["log_interval"]     | 300;
  measure_interval = doc["measure_interval"] | 1000;

  // 7. Confirmación final
  Serial.println("--- Configuración Cargada con Éxito ---");
  Serial.print("Dirección inicio: "); Serial.println(start_addr);
  Serial.print("Longitud: ");         Serial.println(length);
  Serial.print("Intervalo Log: ");    Serial.println(log_interval);
  Serial.print("Intervalo Medida: "); Serial.println(measure_interval);
}

void loop() {
  // Tu lógica aquí
}
*/