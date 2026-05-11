#include <RTClib.h>


#include "src/transport/ModbusTCPManager.h" 
#include "src/transport/ModbusRTUManager.h"

#include "src/services/Datalogger.h"
#include "src/devices/EnergyMeter750.h"
#include "src/services/SDManager.h"

#include "src/EnergyMeterRegInterpreter.h"
//#include "src/ConfigManager.h"
#include "src/ModbusRequestCSV.h"

#define SLAVE_ADDRESS 1 // A eliminar posiblemente MAL
#define MODBUS_PORT 502

// Configuración de red

RTC_DS3231 rtc;

//---Objetos de Gestion 
SDManager sd;  
Datalogger datalogger(&sd); 
EnergyMeterRegInterpreter regInterpreter(&sd); 
EnergyMeter750 energy_meter; 

ModbusRequestCSV mb_csv(&sd);

ModbusTransport* modbus = nullptr;

void setup() {
  
  Serial.begin(115200);

  if (!sd.begin()) {
    Serial.println(F("Error: No se pudo iniciar la SD"));
    while(1); //bloqueamos el sistema ya que si no hay SD no se puede hacer nada. 
  }

  if(! datalogger.begin()){ // mira si esta creada el directorio de log si no esta creado , lo crea
    Serial.println("Error en encendido de datalogger");
    while(1);
  }
 
  if(! regInterpreter.begin()){
    Serial.println("Fallo reg interpretert");
    while(1);
  }
  
  if (! rtc.begin()) { // todo while(1) bloquea al micro, cambiar
     Serial.print("Fallo RTC");
     while(1);
  }
  rtc.adjust(DateTime(2026, 5, 11, 15, 26, 0)); // En este punto se tiene que actualizar el RTC al valor real, sino dara errores

  if(! mb_csv.begin()){
    Serial.println("Fallo reg modbus request");
    while(1);
  }

  if(!mb_csv.loadFromSDParameters()){
        Serial.println("fallo load");    
  }

   // datos importantes a saber por parte de EM y su comunicacion Modbus TCP  
  Serial.println(mb_csv.getDeviceName());
  Serial.println(mb_csv.getIpAdress());
  
  Struct_MBRequest req = mb_csv.loadFromSDMbrequest(); 

  if(true){ // in this case modbusTCP

    byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
    IPAddress ip(192, 168, 0, 10);
    IPAddress server;

    if(server.fromString(mb_csv.getIpAdress())){

      ModbusTCPManager* tcpModbus = new ModbusTCPManager(server, SLAVE_ADDRESS, MODBUS_PORT);
      tcpModbus ->begin(mac, ip); // Aquí sí funciona porque tcpModbus es de tipo ModbusTCPManager*
      modbus = tcpModbus;  

    }
      
  } else {
    ModbusRTUManager* rtuModbus = new ModbusRTUManager(19200, 1, 1);
    rtuModbus->begin();        // Configuración específica de RTU si la tuviera
    modbus = rtuModbus;
  }

  if(!energy_meter.begin(modbus)){
    Serial.println("Error al iniciar el energy meter");
    while(1);
  }

  if(!regInterpreter.prepareAdvanceDatalogger(req, &datalogger, &rtc)){
    Serial.print("error en inicializacion de funcion");
  } 
}

void loop() {
  regInterpreter.advancedDataloggerExec(&datalogger, &energy_meter, &rtc); 
  delay(100); //otras funcionalidades
}

