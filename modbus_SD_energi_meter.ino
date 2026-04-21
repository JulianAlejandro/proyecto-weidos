//#include <SD.h>
//#include "f_map_register_sd.h"
#include <Ethernet.h>
#include <ArduinoModbus.h>
#include <RTClib.h>
//#include "EM_750_Datalogger.h"
#include "EnergyMeter750.h"
#include "SDManager.h"

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
//EM750_Datalogger data_logger(&sd,"/example2.txt", "/tabla.txt");  
EnergyMeter750 energy_meter(1); // ID de esclavo 1

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
 
  if (! rtc.begin()) { // todo while(1) bloquea al micro, cambiar
  while (1) printErrorNoRTC(); 
  }

//iniciamos el objeto de energy_meter 
//necesita de SD manager para leer los registros de de setup y leer correctamente los registros con modbus
// necesita modbus para leer los registros. 

  if(!energy_meter.begin(&sd, &modbusTCPClient)){
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


  energy_meter.readAndProcess_2(19000, 19120, procesarRegistroIndividual); // TODO cambiar lo de los rangos 
    // 4. FINALIZAR SESIÓN (Escribe el salto de línea \n y cierra el archivo)
    //data_logger.endRowSession();

  Serial.println("--- Fila guardada en SD (Sin usar vectores en RAM) ---");

}

unsigned long prevTime = 0;
void printErrorNoRTC(){
  if(millis()-prevTime>3000){
    Serial.println("No RTC Module");
    prevTime = millis();
  }    
}
