
//#include <SD.h>
//#include "f_map_register_sd.h"
#include <Ethernet.h>
#include <ArduinoModbus.h>
#include <RTClib.h>
//#include "EM_750_Datalogger.h"
#include "src/EnergyMeter/EnergyMeter750.h"
#include "src/SDManager.h"

#include "src/EnergyMeter/EnergyMeterRegInterpreter.h"

//#include <ArduinoRS485.h>
//#include <String>

// --- Configuración de Red ---
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
IPAddress ip(192, 168, 0, 10);
IPAddress server(192, 168, 0, 100); // update with the IP Address of your Modbus server

EthernetClient ethClient;
ModbusTCPClient modbusTCPClient(ethClient);
RTC_DS3231 rtc;

//---Objetos de Gestion 
SDManager sd;
EnergyMeterRegInterpreter regInterpreter(&sd);

//EM750_Datalogger data_logger(&sd,"/example2.txt", "/tabla.txt");  
EnergyMeter750 energy_meter(1); // ID de esclavo 1c:\Users\wm04082\Documents\Arduino\modbus_SD_energi_meter\src\EnergyMeter\EnergyMeter750.cpp

//----codigo a revisar 
#define SLAVE_ADDRESS 1 // A eliminar posiblemente 

std::vector<String> nombres; 

unsigned long anteriorMillisModbus = 0; // Almacena la última vez que leíste
const long intervaloModbus = 5000;      // Intervalo de 5 segundos
long contadorPrueba = 0;                // El contador que quieres incrementar

//RegRequest reg_request_modbus; // estructura con base adress y size para lectura de registros 

void printErrorNoRTC();
void ejecutarLecturaModbus();


void procesarRegistroIndividual(float  valor) {
    // Aquí le decimos al datalogger que añada un valor a la fila actual
    // Nota: Deberás implementar "addValueToSession" en tu Datalogger
    //data_logger.addValueToSession(String(valor));
    
    // Opcional: ver en tiempo real por Serial
    Serial.print("resultado: ");
    Serial.println(valor);
}

void setup() {
  
  Serial.begin(115200);
 
  if(!regInterpreter.begin()){
      Serial.print("malas noticias");
  }
  
  if (! rtc.begin()) { // todo while(1) bloquea al micro, cambiar
  while (1) printErrorNoRTC(); 
  }

  if(!energy_meter.begin(&modbusTCPClient)){ // se le pasa el tipo de conexion y acceso a funciones de la SD 
    Serial.println("error al iniciar el energy meter");
  }

  Ethernet.init(ETHERNET_CS);
  if (Ethernet.linkStatus() == LinkOFF) Serial.println("Ethernet Cable is not connected");
  
  Ethernet.begin(mac, ip); 

  delay(5000);

  Serial.print("ultimas señales de vida");

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

  Serial.println("--- Iniciando Captura de Datos ---");


  EM_request req = regInterpreter.startRequest(19000, 1);
  energy_meter.readRegisters(req);

  uint16_t* datos_brutos = energy_meter.getData();
  uint16_t cantidad_datos_brutos = energy_meter.getLastSize(); 

  float* datos_float = regInterpreter.getFloatValues(datos_brutos, cantidad_datos_brutos);
  uint16_t cantidad_datos_float = regInterpreter.getSizeData();

  Serial.println("final");

}

unsigned long prevTime = 0;
void printErrorNoRTC(){
  if(millis()-prevTime>3000){
    Serial.println("No RTC Module");
    prevTime = millis();
  }    
}
