
#include <RTClib.h>

//Librerias mias 
#include "src/transport/ModbusTCPManager.h" // bajo pruebas
//#include "src/transport/ModbusRTUManager.h"

#include "src/Datalogger.h"
#include "src/devices/EnergyMeter750.h"
#include "src/SDManager.h"

#include "src/EnergyMeterRegInterpreter.h"
//#include "src/ConfigManager.h"
//#include "src/ModbusRequestCSV.h"

#define SLAVE_ADDRESS 1 // A eliminar posiblemente MAL
#define MODBUS_PORT 502

// Configuración de red
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
IPAddress ip(192, 168, 0, 10);
IPAddress server(192, 168, 0, 200);

RTC_DS3231 rtc;

//---Objetos de Gestion 
SDManager sd;  // gestion SD compartido
Datalogger datalogger(&sd); // usa SD para hacer LOGs
EnergyMeterRegInterpreter regInterpreter(&sd); // usa SD para interpretar mapa de memoria de EM
EnergyMeter750 energy_meter; //TODO usa EM para leer por modbus TCP o RTU Slave (de momento solo modbus)

ModbusRequestCSV mb_csv(&sd);
ModbusTCPManager modbus(server, SLAVE_ADDRESS, MODBUS_PORT); // IP del servidor y Slave ID 1

unsigned long anteriorMillisModbus = 0; // Almacena la última vez que leíste
unsigned long anteriorMillisArchivo = 0;

EM_request req;              // El contador que quieres incrementar

void lectura_modbus();
void crear_nueva_sesion_log();

nameColValues misTitulos;

uint16_t log_interval = 5000; 

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

// para iniciar el modbus del energymeter necesitamos algunos datos-> los recuperamos por parte de EnergyMeterRegInterpreter
  IPAddress otra_ip_server(192, 168, 0, 100);

  modbus.setIpServer(otra_ip_server);
  modbus.begin(mac, ip); // esto va a aqui de momento

  if(!energy_meter.begin(&modbus)){ // se le pasa el tipo de conexion y acceso a funciones de la SD 
    Serial.println("Error al iniciar el energy meter");
    while(1);
  }

// quiza no es mala idea esto... y dependiendo del tipo de puntero que reciba hacer una cosa u otra...
  regInterpreter.advancedDatalogger(&datalogger, &energy_meter,  &rtc); // funcion bloqueante

}

void loop() {
 
}

/*
#include <RTClib.h>

//Librerias mias 
#include "src/ModbusTCPManager.h" // bajo pruebas

#include "src/Datalogger.h"
#include "src/EnergyMeter/EnergyMeter750.h"
#include "src/SDManager.h"

#include "src/EnergyMeter/EnergyMeterRegInterpreter.h"
//#include "src/ConfigManager.h"
#include "src/ModbusRequestCSV.h"

#define SLAVE_ADDRESS 1 // A eliminar posiblemente MAL
#define MODBUS_PORT 502

// Configuración de red
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
IPAddress ip(192, 168, 0, 10);
IPAddress server(192, 168, 0, 100);

RTC_DS3231 rtc;

//---Objetos de Gestion 
SDManager sd;  // gestion SD compartido
Datalogger datalogger(&sd); // usa SD para hacer LOGs
EnergyMeterRegInterpreter regInterpreter(&sd); // usa SD para interpretar mapa de memoria de EM
EnergyMeter750 energy_meter; //TODO usa EM para leer por modbus TCP o RTU Slave (de momento solo modbus)

ModbusRequestCSV mb_csv(&sd);
ModbusTCPManager modbus(server, SLAVE_ADDRESS, MODBUS_PORT); // IP del servidor y Slave ID 1

unsigned long anteriorMillisModbus = 0; // Almacena la última vez que leíste
unsigned long anteriorMillisArchivo = 0;

EM_request req;              // El contador que quieres incrementar

void lectura_modbus();
void crear_nueva_sesion_log();

nameColValues misTitulos;

uint16_t log_interval = 5000; 

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

  modbus.begin(mac, ip); // esto va a aqui de momento

//---------------este objeto es necesario inicializarlo y tomar la ip---------------
  
  if(! mb_csv.begin()){
    Serial.println("Fallo reg modbus request");
    while(1);
  }

  if(!mb_csv.loadFromSDParameters()){
    Serial.println("fallo load");    
  }

  Serial.println(mb_csv.getDeviceName());
  Serial.println(mb_csv.getIpAdress());

// en este punto es necesario configurar la ip y ya despues si inicializar el energy meter con el modbus
  //server.fromString(mb_csv.getIpAdress()); 

  if(!energy_meter.begin(&modbus)){ // se le pasa el tipo de conexion y acceso a funciones de la SD 
    Serial.println("Error al iniciar el energy meter");
    while(1);
  }

// una vez todo configurado necesitamos obtener la estructura del modbus request. 

  Struct_MBRequest res = mb_csv.loadFromSDMbrequest(); 

  Serial.print("channel: ");
  Serial.println(res.channel); 

  Serial.print("start_addres: ");
  Serial.println(res.start_addres); 

  Serial.print("length: ");
  Serial.println(res.length); 

  Serial.print("func_code: ");
  Serial.println(res.func_code); 

  Serial.print("req_interval_ms: ");
  Serial.println(res.req_interval_ms); 

// una vez se tienen esos valores pasarselos a la clase regInterpreter que es la que se va a encargar de todo ahora. 

   req = regInterpreter.startNewRequest(res.start_addres, res.length);
  
        Serial.print("valores de req: ");
        Serial.print(req.start_addr); 
        Serial.print(" size: ");
        Serial.println(req.size); 

  misTitulos = regInterpreter.getLastNameValues();// titulos general para las medidas.......aqui hay un cambio de paradigma

  Serial.println("Los titulos: ");
  for(int i = 0; i < misTitulos.size; i++){
    Serial.println(misTitulos.buffer[i]);
  }
  //------------obtener los otros parametros------------
  regInterpreter.loadParametersMapRegister(); 
  Parameters param = regInterpreter.getParameters(); 
  log_interval = atoi(param.log_interval);

  Serial.print("el tiempo de log: "); 
  Serial.println(log_interval);
  
  delay(5000); // esperamos 5 segundos
  Serial.println("Empieza el loop");
  
  //creamos la primera sesion de log 
  crear_nueva_sesion_log();

}

void loop() {
 
  unsigned long actualMillis = millis();

  // ---  ---
  if (actualMillis - anteriorMillisModbus >= log_interval) { // modificar 
    anteriorMillisModbus = actualMillis;
    lectura_modbus();       
  }

  // --- Bucle de nueva sesión de log ---
  if (actualMillis - anteriorMillisArchivo >= (60 * 1000UL)) { // preguntar a xavi como va 
    anteriorMillisArchivo = actualMillis;

    datalogger.clearAllLogs();
    crear_nueva_sesion_log();
  }    
  
}

void lectura_modbus() {
  Serial.println("Escribiendo una linea de datos en LOG...");

  // obtenemos datos modbus y los interpretamos
  if (!energy_meter.executeRequest(req)) { 
      Serial.println("error al ejecutar la solicitud de lectura de registros");
  }else{

    rawDataBuffer raw = energy_meter.readDataBuffer();// lectura de registros 
    regInterpreter.getBufferDataRaw(raw.buffer, raw.size);
      

    netDataString res = regInterpreter.getBufNetDataString(); 

    DateTime now = rtc.now();
    char bufferTime[20];
    sprintf(bufferTime, "%04d-%02d-%02d %02d:%02d:%02d", 
    now.year(), now.month(), now.day(), 
    now.hour(), now.minute(), now.second());

    if(!datalogger.writeRow(bufferTime, res.buffer, res.size)){
        Serial.println("Error escribiendo en SD"); 
    } 

    datalogger.printLogToSerial();
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
*/
