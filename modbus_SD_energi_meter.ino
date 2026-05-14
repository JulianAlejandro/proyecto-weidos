#include <RTClib.h>

// Transport Layer implementations
#include "src/transport/ModbusTCPManager.h" 
#include "src/transport/ModbusRTUManager.h"

// Core Services
#include "src/services/Datalogger.h"
#include "src/devices/EnergyMeter750.h"
#include "src/services/SDManager.h"

// Logic and Configuration Managers
#include "src/EnergyMeterRegInterpreter.h"
#include "src/ModbusRequestCSV.h"

// Default Modbus Settings
#define SLAVE_ADDRESS 1 
#define MODBUS_PORT 502

// Global Instances
RTC_DS3231 rtc;
SDManager sd;  
Datalogger datalogger(&sd,30); //30 ficheros por defecto
EnergyMeterRegInterpreter regInterpreter(&sd); 
EnergyMeter750 energy_meter; 
ModbusRequestCSV mb_csv(&sd);
ModbusTransport* modbus = nullptr; // Polymorphic pointer for TCP or RTU


/**
 * @brief Función auxiliar para manejar errores críticos de inicialización.
 * Imprime el código de error en hexadecimal y bloquea el sistema.
 */
void check_critical_error(esp_err_t err, const char* msg) {
    if (err != ESP_OK) {
        Serial.printf("\n[CRITICO] %s | Error: 0x%X\n", msg, err);
        // Aquí podrías añadir un parpadeo de LED de error
        while(1) { delay(1000); } 
    }
}

void setup() {
    Serial.begin(115200);
    while(!Serial); // Esperar a la consola en MKR/ESP32-S3
    Serial.println("\n--- WEIDOS SYSTEM STARTUP ---");

    esp_err_t err;

    // 1. Hardware Base: Tarjeta SD
    err = sd.begin();
    check_critical_error(err, "Fallo en Hardware SD");

    // 2. Servicios: Datalogger y RTC
    if(datalogger.begin()) {
        check_critical_error(ESP_FAIL, "Directorio /LOGS no accesible");
    }

    if (!rtc.begin()) {
        check_critical_error(ESP_FAIL, "Hardware RTC DS3231 no encontrado");
    }

    // 3. Configuración: Carga de Mapas y Peticiones desde SD
    err = regInterpreter.begin();
    check_critical_error(err, "Fallo al leer Mapa de Registros (EM750map.csv)");

    err = mb_csv.begin();
    check_critical_error(err, "Fallo al acceder a configuracion (MBReq.csv)");

    // Intentamos cargar parámetros (no crítico, usamos valores por defecto si falla)
    if(mb_csv.loadFromSDParameters() == ESP_OK) {
        Serial.printf("Dispositivo: %s | Servidor: %s\n", mb_csv.getDeviceName(), mb_csv.getIpAdress());
    } else {
        Serial.println("Aviso: Usando parámetros de conexión por defecto.");
    }

    // 4. Capa de Transporte: Conexión Modbus
    Struct_MBRequest req;  
    err = mb_csv.loadFromSDMbrequest(&req);
    check_critical_error(err, "Peticion Modbus invalida en CSV");

    // Selección de transporte (TCP/RTU)
    // En el futuro, este 'true' vendrá de un parámetro en el CSV
    bool useTCP = true; 

    if(useTCP) {
        byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
        IPAddress local_ip(192, 168, 0, 10);
        IPAddress server_ip;

        if(server_ip.fromString(mb_csv.getIpAdress())) {
            ModbusTCPManager* tcp = new ModbusTCPManager(server_ip, SLAVE_ADDRESS, MODBUS_PORT);
            tcp->begin(mac, local_ip);
            modbus = tcp;
        } else {
            check_critical_error(ESP_ERR_CONFIG_INVALID_DATA, "IP del Servidor invalida");
        }
    } else {
        ModbusRTUManager* rtu = new ModbusRTUManager(19200, 1, SERIAL_8E1);
        rtu->begin();
        modbus = rtu;
    }

    // 5. Driver de Dispositivo y Sesión de Log
    err = energy_meter.begin(modbus);
    check_critical_error(err, "Error al vincular Driver EM750");

    err = regInterpreter.prepareAdvanceDatalogger(req, &datalogger, &rtc);
    check_critical_error(err, "No se pudo iniciar la sesion de Datalogging");

    Serial.println("--- SISTEMA LISTO Y CORRIENDO ---\n");
}

void loop() {
    // El intérprete gestiona internamente los tiempos de muestreo
    regInterpreter.advancedDataloggerExec(&datalogger, &energy_meter, &rtc);
    
    // El delay es pequeño para mantener la responsividad
    delay(10); 
}


/*
#include <RTClib.h>

// Transport Layer implementations
#include "src/transport/ModbusTCPManager.h" 
#include "src/transport/ModbusRTUManager.h"

// Core Services
#include "src/services/Datalogger.h"
#include "src/devices/EnergyMeter750.h"
#include "src/services/SDManager.h"

// Logic and Configuration Managers
#include "src/EnergyMeterRegInterpreter.h"
#include "src/ModbusRequestCSV.h"

// Default Modbus Settings
#define SLAVE_ADDRESS 1 
#define MODBUS_PORT 502

// Global Instances
RTC_DS3231 rtc;
SDManager sd;  
Datalogger datalogger(&sd,30); //30 ficheros por defecto
EnergyMeterRegInterpreter regInterpreter(&sd); 
EnergyMeter750 energy_meter; 
ModbusRequestCSV mb_csv(&sd);
ModbusTransport* modbus = nullptr; // Polymorphic pointer for TCP or RTU



void setup() {
  Serial.begin(115200);

  // 1. Initialize SD Card (System Critical)
  if (sd.begin()) {
    Serial.println(F("Error: SD Card could not be initialized. Blocking system..."));
    while(1); // System cannot operate without SD for configuration/logging
  }

  // 2. Initialize Datalogger (Ensures log directory structure exists)
  if(datalogger.begin()){ 
    Serial.println(F("Error: Datalogger initialization failed."));
    while(1);
  }
 
  // 3. Initialize Register Interpreter (Loads parameter mapping)
  if(regInterpreter.begin()){
    Serial.println(F("Error: Register Interpreter failed."));
    while(1);
  }
  
  // 4. Initialize Real Time Clock (RTC)
  if (!rtc.begin()) {
     Serial.print(F("Error: RTC not found."));
     while(1); // Consider implementing a non-blocking fallback if hardware allows
  }

  // NOTE: Manual RTC sync. In production, this should be synced with an NTP server 
  // or only set if the RTC lost power.
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  delay(1000);

  // 5. Load Modbus Requests and Device Parameters from CSV on SD
  if(mb_csv.begin()){
    Serial.println(F("Error: Modbus CSV Manager failed."));
    while(1);
  }

  if(mb_csv.loadFromSDParameters()){
    Serial.println(F("Warning: Failed to load device parameters from SD."));    
  }

  // Display identified device info
  Serial.print(F("Device Identified: "));
  Serial.println(mb_csv.getDeviceName());
  Serial.print(F("Target IP: "));
  Serial.println(mb_csv.getIpAdress());
  
  // Load specific Modbus request structure (start address, length, etc.)
  Struct_MBRequest req;  
  mb_csv.loadFromSDMbrequest(&req); 

  // 6. Transport Layer Selection (TCP vs RTU)
  // Logic currently defaults to TCP. Update condition to switch based on config.
  if(true) { 
    // Ethernet Settings
    byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
    IPAddress ip(192, 168, 0, 10); // Local device IP
    IPAddress server;

    if(server.fromString(mb_csv.getIpAdress())){
      // Instantiate TCP Manager
      ModbusTCPManager* tcpModbus = new ModbusTCPManager(server, SLAVE_ADDRESS, MODBUS_PORT);
      tcpModbus->begin(mac, ip); 
      modbus = tcpModbus; // Assign to generic interface pointer
    }
      
  } else {
    // Instantiate RTU Manager (RS485)
    ModbusRTUManager* rtuModbus = new ModbusRTUManager(19200, 1, SERIAL_8E1);
    rtuModbus->begin();
    modbus = rtuModbus; // Assign to generic interface pointer
  }

  // 7. Attach Transport to Energy Meter Driver
  if(energy_meter.begin(modbus)){
    Serial.println(F("Error: Could not link Modbus to Energy Meter."));
    while(1);
  }

  // 8. Prepare Advanced Datalogging Session
  // This validates the CSV request against the meter's memory map and starts the SD log file
  if(regInterpreter.prepareAdvanceDatalogger(req, &datalogger, &rtc)){
    Serial.println(F("Error: Advanced Datalogger initialization failed. Check request ranges."));
  }
}

void loop() {

  regInterpreter.advancedDataloggerExec(&datalogger, &energy_meter, &rtc);

  // Minor delay to yield to other tasks or background processes
  delay(100); 
}


*/