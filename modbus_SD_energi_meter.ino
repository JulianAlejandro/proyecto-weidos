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

/*
// --- Callback para el procesamiento de datos ---
// Esta función se ejecuta cada vez que el medidor lee UN registro
void procesarRegistroIndividual(uint16_t direccion, uint32_t valor) {
    // Aquí le decimos al datalogger que añada un valor a la fila actual
    // Nota: Deberás implementar "addValueToSession" en tu Datalogger
    //data_logger.addValueToSession(String(valor));
    
    // Opcional: ver en tiempo real por Serial
    Serial.print("Reg "); Serial.print(direccion); Serial.print(": "); Serial.println(valor); // esto cambiara , tenemos otra filosofia 
}
 */
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
  delay(5000);

  Serial.print("señales de vida");
/*
  if (!data_logger.begin()) {
    Serial.println("Error de SD");
    while (true);
  }
*/
  Serial.print("señales de vida");

  if (! rtc.begin()) {
  while (1) printErrorNoRTC(); 
  }

  if(!energy_meter.begin(&sd, modbusTCPClient)){
    Serial.println("error al iniciar el energy meter");
  }

  //reg_request_modbus = data_logger.RegRequestParamsFromRangeAddr(19000, 19020);

  //data_logger.clearLogFile(); // limpiando el fichero de log 

// TODO Obviamente los nombres estan mal, no estan añadidos. 
  // al incio de los nombres, añadir "timestamp"
  //nombres.insert(nombres.begin(), "Timestamp");
  //data_logger.writeHeader(nombres);

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

  /*
  unsigned long actualMillis = millis();

  // --- TAREA A: 
  contadorPrueba++;
  if (contadorPrueba % 10000 == 0) { 
    Serial.print("Haciendo otras cosas... Contador: ");
    Serial.println(contadorPrueba);
  }

  // --- TAREA B cada 5 segundos
  if (actualMillis - anteriorMillisModbus >= intervaloModbus) {
    
    anteriorMillisModbus = actualMillis;

    Serial.println("--- Iniciando ciclo de lectura programada ---");

    if (!modbusTCPClient.connected()) {
      Serial.println("Intentando reconectar Modbus...");
      if (!modbusTCPClient.begin(server, 502)) {
        Serial.println("Fallo al conectar!");
      } else {
        Serial.println("Conectado con éxito");
      }
    } else {
      ejecutarLecturaModbus(); 
    }
  }
  */
}

void ejecutarLecturaModbus() {

  Serial.println("--- Iniciando Captura de Datos ---");
/*
  // 1. Obtener Timestamp
    DateTime now = rtc.now();
    char timestamp[20];
    sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d", 
            now.year(), now.month(), now.day(), 
            now.hour(), now.minute(), now.second());
*/

    // 2. INICIAR SESIÓN EN SD (Abre el archivo y escribe el tiempo)
    // Debes crear estas funciones en tu Datalogger para que no use vectores
    //data_logger.startRowSession(); 
    //data_logger.addValueToSession(String(timestamp));

    // 3. LEER MEDIDOR (El callback irá llenando la SD registro por registro)
    // Usamos el bloque que definimos en la clase

/*
    // aqui primero hay que obtener el rango que se desea 
    //bloque-> tiene que ser una addr y un size y luego si usar esta 
    if (!energy_meter.readAndProcess(energy_meter.BLOCK_BASIC, procesarRegistroIndividual)) { // esto no es asi, aqui hay que hacer un procedimiento 
        Serial.println("Error en lectura Modbus");
    }
*/

  energy_meter.readAndProcess_2(19000, 19120, procesarRegistroIndividual);
    // 4. FINALIZAR SESIÓN (Escribe el salto de línea \n y cierra el archivo)
    //data_logger.endRowSession();

    Serial.println("--- Fila guardada en SD (Sin usar vectores en RAM) ---");


  /*
  Serial.println("Reading Registers...");

  std::vector<long> data; 

  long a [128];

  data.reserve(reg_request_modbus.totalSize);

// obtener los datos y el timestamp
  if (modbusTCPClient.requestFrom(SLAVE_ADDRESS, INPUT_REGISTERS, reg_request_modbus.baseAddress, reg_request_modbus.totalSize)) {
    for(int i = 0; i < reg_request_modbus.totalSize; i++){
      data.push_back(modbusTCPClient.read());
    }
        // Timestamp
    DateTime now = rtc.now();
    char buffer[20];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 
          now.year(), now.month(), now.day(), 
          now.hour(), now.minute(), now.second());

    std::vector<String> filaParaSD = data_logger.obtener_fila (data, reg_request_modbus.baseAddress);
    
    filaParaSD.insert(filaParaSD.begin(), String(buffer));
    
    data_logger.writeRow(filaParaSD);
    Serial.println("Datos guardados en SD.");
    
    data_logger.printLogToSerial(); 
  } else {
    Serial.println("Fallo en la petición Modbus.");
  }
  */
}

unsigned long prevTime = 0;
void printErrorNoRTC(){
  if(millis()-prevTime>3000){
    Serial.println("No RTC Module");
    prevTime = millis();
  }    
}


/*

//#include <SD.h>
//#include "f_map_register_sd.h"
#include "EM_750_Datalogger.h"

#include <Ethernet.h>

#include <ArduinoRS485.h>
#include <ArduinoModbus.h>
#include <String>

#include <RTClib.h>

#define SLAVE_ADDRESS 1

//#define FREQUENTLY_REQUIRED_READINGS 19000
//#define FREQUENTLY_REQUIRED_READINGS_SIZE 122
//#define FLOAT_SIZE 2

//#define N_DATA_FRR FREQUENTLY_REQUIRED_READINGS_SIZE/FLOAT_SIZE

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
IPAddress ip(192, 168, 0, 10);

EthernetClient ethClient;
ModbusTCPClient modbusTCPClient(ethClient);

IPAddress server(192, 168, 0, 100); // update with the IP Address of your Modbus server

RTC_DS3231 rtc;

std::vector<String> nombres; 

unsigned long anteriorMillisModbus = 0; // Almacena la última vez que leíste
const long intervaloModbus = 5000;      // Intervalo de 5 segundos
long contadorPrueba = 0;                // El contador que quieres incrementar

void printErrorNoRTC();
void ejecutarLecturaModbus();
 

RegRequest reg_request_modbus; // estructura con base adress y size para lectura de registros 

//codigo añadido para cambios para controlar SD 
SDManager sd; 
EM750_Datalogger data_logger(&sd,"/example2.txt", "/tabla.txt"); 

void setup() {
  
  Serial.begin(115200);
  delay(5000);

  Serial.print("señales de vida");
  

  if (!data_logger.begin()) {
    Serial.println("Error de SD");
    while (true);
  }

  Serial.print("señales de vida");

  if (! rtc.begin()) {
  while (1) printErrorNoRTC(); 
  }

  reg_request_modbus = data_logger.RegRequestParamsFromRangeAddr(19000, 19020);

  data_logger.clearLogFile(); // limpiando el fichero de log 

// TODO Obviamente los nombres estan mal, no estan añadidos. 
  // al incio de los nombres, añadir "timestamp"
  nombres.insert(nombres.begin(), "Timestamp");
  data_logger.writeHeader(nombres);

  Ethernet.init(ETHERNET_CS);
  if (Ethernet.linkStatus() == LinkOFF) Serial.println("Ethernet Cable is not connected");
  
  Ethernet.begin(mac, ip); 

  delay(5000);

  Serial.print("ultimas señales de vida");

}

void loop() {
  unsigned long actualMillis = millis();

  // --- TAREA A: 
  contadorPrueba++;
  if (contadorPrueba % 10000 == 0) { 
    Serial.print("Haciendo otras cosas... Contador: ");
    Serial.println(contadorPrueba);
  }

  // --- TAREA B cada 5 segundos
  if (actualMillis - anteriorMillisModbus >= intervaloModbus) {
    
    anteriorMillisModbus = actualMillis;

    Serial.println("--- Iniciando ciclo de lectura programada ---");

    if (!modbusTCPClient.connected()) {
      Serial.println("Intentando reconectar Modbus...");
      if (!modbusTCPClient.begin(server, 502)) {
        Serial.println("Fallo al conectar!");
      } else {
        Serial.println("Conectado con éxito");
      }
    } else {
      ejecutarLecturaModbus(); 
    }
  }
}

void ejecutarLecturaModbus() {
  Serial.println("Reading Registers...");

  std::vector<long> data; 

  long a [128];

  data.reserve(reg_request_modbus.totalSize);

// obtener los datos y el timestamp
  if (modbusTCPClient.requestFrom(SLAVE_ADDRESS, INPUT_REGISTERS, reg_request_modbus.baseAddress, reg_request_modbus.totalSize)) {
    for(int i = 0; i < reg_request_modbus.totalSize; i++){
      data.push_back(modbusTCPClient.read());
    }
        // Timestamp
    DateTime now = rtc.now();
    char buffer[20];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 
          now.year(), now.month(), now.day(), 
          now.hour(), now.minute(), now.second());

    std::vector<String> filaParaSD = data_logger.obtener_fila (data, reg_request_modbus.baseAddress);
    
    filaParaSD.insert(filaParaSD.begin(), String(buffer));
    
    data_logger.writeRow(filaParaSD);
    Serial.println("Datos guardados en SD.");
    
    data_logger.printLogToSerial(); 
  } else {
    Serial.println("Fallo en la petición Modbus.");
  }
}

unsigned long prevTime = 0;
void printErrorNoRTC(){
  if(millis()-prevTime>3000){
    Serial.println("No RTC Module");
    prevTime = millis();
  }    
}

*/
