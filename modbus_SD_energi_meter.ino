
#include "src/services/SDManager.h"
#include "src/services/DataloggerFileManager.h"
#include <RTClib.h>

SDManager sd;  
DataloggerFileManager DatManager(&sd,4); 
RTC_DS3231 rtc;


void check_critical_error(esp_err_t err, const char* msg) {
    if (err != ESP_OK) {
        Serial.printf("\n[CRITICO] %s | Error: 0x%X\n", msg, err);
        Serial.println("Reiniciando sistema en 5 segundos...");
        
        delay(5000); // Tiempo para que el usuario pueda leer el error en el monitor
        while(true); 
        
        //ESP.restart(); // <--- Aquí generas el reset por código
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    esp_err_t err;

    // 1. Hardware Base: Tarjeta SD y RTC
    err = sd.begin();
    check_critical_error(err, "Fallo en Hardware SD");

    if (!rtc.begin()) {
        check_critical_error(ESP_FAIL, "Hardware RTC DS3231 no encontrado");
    }

    // 2. Inicializar Gestor de Datalogger
    err = DatManager.begin(); 
    check_critical_error(err, "Error datalogger");

    // Indexa los archivos existentes y borra los que superen el límite configurado (4)
    DatManager.setCSVLastEnvironment(true);

    // 3. Obtener hora actual para el NOMBRE del nuevo archivo de sesión
    
    DateTime ahora = rtc.now();
    char nombreFichero[16]; 
    // Formato exacto requerido: MMDDHHmm (Ej: 06271240)
    snprintf(nombreFichero, sizeof(nombreFichero), "%02d%02d%02d%02d", 
            ahora.month(), 
            ahora.day(), 
            ahora.hour(),
            ahora.minute());

    // ==========================================
    // SIMULACIÓN: DEFINICIÓN DE CABECERAS (TITLES)
    // ==========================================
    const char* misCabeceras[] = {"TEMP", "HUM", "PRESION"};
    uint16_t numColumnas = sizeof(misCabeceras) / sizeof(misCabeceras[0]);

    // Creamos la nueva sesión pasándole el nombre/timestamp y los títulos
    Serial.print("[SIM] Creando nueva sesion de log: "); Serial.println(nombreFichero);
    err = DatManager.newCSVLogSesion(nombreFichero, misCabeceras, numColumnas);
    check_critical_error(err, "Error al crear nueva sesion CSV");

    // ==========================================
    // SIMULACIÓN: ENTRADA DE DATOS SUCESIVOS
    // ==========================================
    char timestampLectura[32];
    
    // --- LECTURA 1 ---
    ahora = rtc.now(); // Actualizamos marca de tiempo para la fila de datos
    snprintf(timestampLectura, sizeof(timestampLectura), "%04d/%02d/%02d %02d:%02d:%02d", 
             ahora.year(), ahora.month(), ahora.day(), ahora.hour(), ahora.minute(), ahora.second());
    
    const char* valores1[] = {"24.5", "60.2", "1013.2"};
    Serial.println("[SIM] Insertando Lectura 1...");
    DatManager.appendNewDataCSVToLog(timestampLectura, valores1, numColumnas);
    delay(1000); 

    // --- LECTURA 2 ---
    ahora = rtc.now();
    snprintf(timestampLectura, sizeof(timestampLectura), "%04d/%02d/%02d %02d:%02d:%02d", 
             ahora.year(), ahora.month(), ahora.day(), ahora.hour(), ahora.minute(), ahora.second());
             
    const char* valores2[] = {"24.6", "59.8", "1013.1"};
    Serial.println("[SIM] Insertando Lectura 2...");
    DatManager.appendNewDataCSVToLog(timestampLectura, valores2, numColumnas);
    delay(1000); 

    // --- LECTURA 3 ---
    ahora = rtc.now();
    snprintf(timestampLectura, sizeof(timestampLectura), "%04d/%02d/%02d %02d:%02d:%02d", 
             ahora.year(), ahora.month(), ahora.day(), ahora.hour(), ahora.minute(), ahora.second());
             
    const char* valores3[] = {"24.8", "59.5", "1012.9"};
    Serial.println("[SIM] Insertando Lectura 3...");
    DatManager.appendNewDataCSVToLog(timestampLectura, valores3, numColumnas);
    delay(1000); 

    // 4. Forzar el volcado del buffer a la SD físico al terminar el lote
    Serial.println("[SIM] Volcando buffer a la tarjeta SD...");
    DatManager.flushBuffer();
    
}

void loop(){
    delay(1000);
}


/*
#include <RTClib.h>
#include "src/services/SDManager.h"
#include "src/services/DataloggerFileManager.h"


SDManager sd;  
DataloggerFileManager DatManager(&sd,4); 
RTC_DS3231 rtc;


void check_critical_error(esp_err_t err, const char* msg) {
    if (err != ESP_OK) {
        Serial.printf("\n[CRITICO] %s | Error: 0x%X\n", msg, err);
        Serial.println("Reiniciando sistema en 5 segundos...");
        
        delay(5000); // Tiempo para que el usuario pueda leer el error en el monitor
        while(true); 
        
        //ESP.restart(); // <--- Aquí generas el reset por código
    }
}

void setup(){
     Serial.begin(115200);
     delay(1000);

    esp_err_t err;

    // 1. Hardware Base: Tarjeta SD
    err = sd.begin();
    check_critical_error(err, "Fallo en Hardware SD");

    if (!rtc.begin()) {
    check_critical_error(ESP_FAIL, "Hardware RTC DS3231 no encontrado");
    }

    err = DatManager.begin(); 
    check_critical_error(err, "Error datalogger");

    DatManager.setCSVLastEnvironment(true);



}

void loop(){
    delay(1000);
}

*/

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

void check_critical_error(esp_err_t err, const char* msg) {
    if (err != ESP_OK) {
        Serial.printf("\n[CRITICO] %s | Error: 0x%X\n", msg, err);
        Serial.println("Reiniciando sistema en 5 segundos...");
        
        delay(5000); // Tiempo para que el usuario pueda leer el error en el monitor
        while(true); 
        
        ESP.restart(); // <--- Aquí generas el reset por código
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
    esp_err_t err = regInterpreter.advancedDataloggerExec(&datalogger, &energy_meter, &rtc);
    if(err != ESP_OK){
      check_critical_error(err, "No se pudo iniciar la sesion de Datalogging");
    }
    // El delay es pequeño para mantener la responsividad
    delay(1000); 
}
*/
