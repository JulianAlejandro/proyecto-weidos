/*
#include "src/Datalogger.h"
#include "src/SDManager.h"
#include <RTClib.h> // Asegúrate de incluir la librería del RTC

// Instancias globales
SDManager sdManager;
Datalogger logger(&sdManager);
RTC_DS3231 rtc;

// Configuración de los datos a loguear
const char* cabeceras[] = {"Voltaje", "Corriente", "Potencia"};
const uint16_t numCampos = 3;

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10); 

    Serial.println(F("\n--- TEST DATALOGGER CON FECHA ---"));

    // 1. Inicializar SD
    if (!sdManager.begin()) {
        Serial.println(F("Error: No se pudo inicializar la SD."));
        while (1); 
    }

    // 2. Inicializar Datalogger
    if (!logger.begin()) {
        Serial.println(F("Error: No se pudo inicializar el Datalogger."));
        while (1);
    }

    // 3. Inicializar RTC
    if (!rtc.begin()) {
        Serial.println(F("Error: No se detectó el RTC."));
        // Aquí podrías decidir si continuar con un nombre genérico o bloquear
    }

    // 4. Generar el nombre del archivo basado en la fecha
    DateTime now = rtc.now();
    char nombre[32]; // Buffer para la ruta completa
    

    // Formato: /LOGS/260424.txt (6 caracteres, dentro del límite)
    snprintf(nombre, sizeof(nombre), "%02d%02d%02d.txt", 
      now.year() % 100, now.month(), now.day());

    Serial.print(F("Intentando crear archivo: "));
    Serial.println(nombre);

    // 5. Usar addAndSetLogFile con el nuevo nombre
    // Gracias a tu lógica interna, si el archivo de hoy ya existe, creará L20260424_1.txt
    if (logger.newLog(nombre)) {
        Serial.println(F("Archivo configurado correctamente."));
    } else {
        Serial.println(F("Error al crear el archivo de log."));
    }

    // 6. Escribir la cabecera
    if (logger.writeHeader(cabeceras, numCampos)) {
        Serial.println(F("Cabecera escrita."));
    }

    // 7. Simular datos
    Serial.println(F("--- Escribiendo datos de prueba ---"));
    for (int i = 0; i < 5; i++) {
        char timeStr[10];
        DateTime tempTime = rtc.now();
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", tempTime.hour(), tempTime.minute(), tempTime.second());

        float datosSimulados[numCampos] = { 220.5, 5.2, 1146.6 };

        logger.writeRow(timeStr, datosSimulados, numCampos);
        delay(500);
    }

    // 8. Ver resultado
    logger.printLogToSerial();
    Serial.println(F("\n--- PROCESO COMPLETADO ---"));
}

void loop() {

}
*/

//#include <ArduinoJson.h>
#include <Ethernet.h>
#include <ArduinoModbus.h>
#include <RTClib.h>

//Librerias mias 
#include "src/Datalogger.h"
//#include "src/EnergyMeter/EnergyMeter750.h"
#include "src/SDManager.h"

#include "src/EnergyMeter/EnergyMeterRegInterpreter.h"
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
//EnergyMeter750 energy_meter(1); // TODO ID ES DE MODBUS MAL 

//----codigo a revisar 
//#define SLAVE_ADDRESS 1 // A eliminar posiblemente 

unsigned long anteriorMillisModbus = 0; // Almacena la última vez que leíste
const long intervaloModbus = 1000;      // Intervalo de 5 segundos
long contadorPrueba = 0; 

unsigned long anteriorMillisArchivo = 0;
const unsigned long intervaloArchivo = 10000; // 10 segundos
int contador_ficheros = 0;

//EM_request req;              // El contador que quieres incrementar

//uint16_t totalTitulos;
//const char* cabeceraCompleta[totalTitulos];

void lectura_modbus();
void crear_nueva_sesion_log();

titlesBuffer misTitulos;

//String nombre = "nombre";


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

  //req = regInterpreter.startNewRequest(19000, 10);
  regInterpreter.startNewRequest(19000, 10);
  misTitulos = regInterpreter.getTitles();
  
  if(!datalogger.newSesion(String("inicial").c_str(), misTitulos.buffer, misTitulos.size)){
    Serial.println("algo salio regular");
  }
  Serial.print("Empieza el loop");
}

void loop() {
  unsigned long actualMillis = millis();

    if (actualMillis - anteriorMillisModbus >= intervaloModbus) { // loop cada 5 m
        anteriorMillisModbus = actualMillis;
           lectura_modbus();       
    }

      if (actualMillis - anteriorMillisArchivo >= intervaloArchivo) {
        anteriorMillisArchivo = actualMillis;
        crear_nueva_sesion_log();
    }
}


void lectura_modbus() {
    // SIMULACIÓN DE DATOS
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

