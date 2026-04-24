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


#include <Ethernet.h>
#include <ArduinoModbus.h>
#include <RTClib.h>

//Librerias mias 
#include "src/Datalogger.h"
#include "src/EnergyMeter/EnergyMeter750.h"
#include "src/SDManager.h"

#include "src/EnergyMeter/EnergyMeterRegInterpreter.h"
// --- Configuración de Red ---
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
IPAddress ip(192, 168, 0, 10);
IPAddress server(192, 168, 0, 100); // update with the IP Address of your Modbus server

EthernetClient ethClient;
ModbusTCPClient modbusTCPClient(ethClient);
RTC_DS3231 rtc;

//---Objetos de Gestion 
SDManager sd;
Datalogger datalogger(&sd);
EnergyMeterRegInterpreter regInterpreter(&sd);
EnergyMeter750 energy_meter(1); // TODO ID ES DE MODBUS MAL 

//----codigo a revisar 
#define SLAVE_ADDRESS 1 // A eliminar posiblemente 

unsigned long anteriorMillisModbus = 0; // Almacena la última vez que leíste
const long intervaloModbus = 5000;      // Intervalo de 5 segundos
long contadorPrueba = 0;   

EM_request req;              // El contador que quieres incrementar

void printErrorNoRTC();
void ejecutarLecturaModbus();

void setup() {
  
  Serial.begin(115200);

  if (!sd.begin()) {
    Serial.println(F("Error: No se pudo iniciar la SD"));
    while(1); //bloqueamos el sistema ya que si no hay SD no se puede hacer nada. 
  }

  if(! datalogger.begin()){
    Serial.println("Error en encendido de datalogger");
  }
 
  if(! regInterpreter.begin()){
    Serial.println("Fallo reg interpretert");
  }
  
  if (! rtc.begin()) { // todo while(1) bloquea al micro, cambiar
     Serial.print("Fallo RTC");
  }

  if(!energy_meter.begin(&modbusTCPClient)){ // se le pasa el tipo de conexion y acceso a funciones de la SD 
    Serial.println("error al iniciar el energy meter");
  }


  Ethernet.init(ETHERNET_CS);
  if (Ethernet.linkStatus() == LinkOFF) Serial.println("Ethernet Cable is not connected");
  
  Ethernet.begin(mac, ip); 

  delay(5000);

  req = regInterpreter.startNewRequest(19000, 10);
  titlesBuffer misTitulos =  regInterpreter.getTitles();

  // 2. Creamos un nuevo buffer temporal con espacio para el Timestamp (+1)
  uint16_t totalTitulos = misTitulos.size + 1;
  const char* cabeceraCompleta[totalTitulos];

  cabeceraCompleta[0] = "Timestamp";

  for (uint16_t i = 0; i < misTitulos.size; i++) {
    cabeceraCompleta[i + 1] = misTitulos.buffer[i];
  }

  datalogger.clearLogFile();
  if(!datalogger.writeHeader(cabeceraCompleta, totalTitulos)){
    Serial.println("error al poner los titulos");
  }

Serial.print("Empieza el loop");
}

void loop() {

  

  unsigned long actualMillis = millis();

    if (actualMillis - anteriorMillisModbus >= intervaloModbus) {
        anteriorMillisModbus = actualMillis;

        if (!modbusTCPClient.connected()) {
            Serial.println("Reconectando Modbus...");
            modbusTCPClient.begin(server, 502);
        } else {
            ejecutarLecturaModbus();
        }
    }
    
}


void ejecutarLecturaModbus() {
   
    if (energy_meter.executeRequest(req)) {
        RegDataBuffer raw = energy_meter.readDataBuffer();
        netFloatDataBuffer res = regInterpreter.getFloatValues(raw.buffer, raw.size);

        DateTime now = rtc.now();

        // Creamos el string con el formato deseado
        char buffer[20];
        sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 
                now.year(), now.month(), now.day(), 
                now.hour(), now.minute(), now.second());

        // LLAMADA CORREGIDA: Pasamos el buffer de texto y los floats
        if(!datalogger.writeRow(buffer, res.buffer, res.size)){
            Serial.println("Error escribiendo un nuevo dato en el data_logger");
        } 

        datalogger.printLogToSerial();
    }else{

      Serial.println("creo que salta el error aqui");
    }
}

unsigned long prevTime = 0;
void printErrorNoRTC(){
  if(millis()-prevTime>3000){
    Serial.println("No RTC Module");
    prevTime = millis();
  }    
}

