

#include <RTClib.h>
#include "SDManager.h"
#include "TimeLogFileManager.h"
#include "Datalogger.h"
#include "EMRegInterpreter.h"
#include "EnergyMeter750.h"
#include "ModbusTCPManager.h"
#include "ModbusRequestCSV.h"
#include "AdvancedDatalogger.h" // <-- El nuevo Orquestador

// Instancias globales de infraestructura básica
RTC_DS3231 rtc;
SDManager sd; 
TimeLogFileManager fileManager(&sd, 4);
Datalogger datalogger(&sd, &fileManager);
EMRegInterpreter regInterpreter(&sd); 
EnergyMeter750 energy_meter;
ModbusRequestCSV mb_csv(&sd);
IModbusTransport* modbus = nullptr;

// Instancia única de nuestro orquestador
AdvancedDatalogger loggerEngine(&sd, &datalogger, &rtc, &regInterpreter, &energy_meter);

void check_critical_error(esp_err_t err, const char* msg); 

void setup() {
    Serial.begin(115200);
    while(!Serial);

    if (!rtc.begin()) while(true);
    rtc.adjust(DateTime(2026, 5, 28, 16, 33, 1));

    // Inicializaciones obligatorias de HW y archivos
    check_critical_error(sd.begin(), "Fallo en Hardware SD");
    check_critical_error(datalogger.begin(), "Datalogger no inicializado");
    check_critical_error(regInterpreter.begin(), "Fallo al leer Mapa");
    check_critical_error(mb_csv.begin(), "Fallo al acceder a config");
    check_critical_error(mb_csv.loadFromSDParameters(), "Fallo al obtener modbus req");

    Struct_MBRequest req;  
    check_critical_error(mb_csv.loadFromSDMbrequest(&req), "Petición Modbus inválida");

    // Red / Transporte Modbus
    byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
    IPAddress local_ip(192, 168, 0, 10);
    IPAddress server_ip;

    if(server_ip.fromString(mb_csv.getIpAdress())) {
        ModbusTCPManager* tcp = new ModbusTCPManager(server_ip, 1, 502);
        tcp->begin(mac, local_ip);
        modbus = tcp;
    } else {
        check_critical_error(ESP_ERR_CONFIG_INVALID_DATA, "IP inválida");
    }

    check_critical_error(energy_meter.begin(modbus), "Error al vincular Driver");

    // ¡Arrancamos el motor! El orquestador toma el control
    check_critical_error(loggerEngine.begin(req), "No se pudo iniciar la sesión");
}

void loop() {
    // El ciclo de ejecución queda reducido a una única y limpia llamada
    esp_err_t err = loggerEngine.execute();
    if(err != ESP_OK) {
        check_critical_error(err, "Fallo en la ejecución del bucle de telemetría");
    }
    delay(1000); 
}

void check_critical_error(esp_err_t err, const char* msg) {
    if (err != ESP_OK) {
        Serial.printf("\n[CRITICO] %s | Error: 0x%X\n", msg, err);

        /*
        if (sd.isReady()) {
            // Aumentamos a 32 por seguridad para evitar truncamientos en el stack
            char timestamp[128]; 
            strcpy(timestamp, "SYSTEM_PANIC");
            
            //DateTime now(2026, 5, 21, 13, 19, 1); // = rtc.now();
            DateTime now = rtc.now();
            if (now.year() >= 2026) { 
                snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d", 
                         now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
            }
            Serial.print("Esto es: ");
            Serial.println(timestamp);

            char err_payload[128];
            snprintf(err_payload, sizeof(err_payload), "%s (Cod: 0x%X)", msg, err);

            Serial.println("Escribiendo reporte de fallo en la SD...");
            datalogger.appendErrorLog(timestamp, err_payload);
            
            //datalogger.flushBuffer();
        } else {
            Serial.println("[AVISO] SD no lista. Imposible guardar el reporte.");
        }
        */
         Serial.println("BLOQUEAMOS EL MICRO");
        delay(1000);
        sd.end(); 
        while(true); 
        Serial.println("Reiniciando sistema en 5 segundos...");
        delay(5000); 
        ESP.restart(); 
    }
}

//-------------------------------------------------------------
//#include <RTClib.h>
//
//// Transport Layer implementations
//#include "ModbusTCPManager.h" 
////#include "ModbusRTUManager.h"
//
//// Core Services
////#include "BasicLogFileManager.h"
//#include "TimeLogFileManager.h"
//#include "Datalogger.h"
//
//#include "EMRegInterpreter.h"
//#include "EnergyMeter750.h"
//
//#include "SDManager.h"
//
//// Logic and Configuration Managers
////#include "EnergyMeterRegInterpreter.h"
//#include "ModbusRequestCSV.h"
//#include "f_getParameters.h"
//
//#include "LogMsgGlobal.h"
//
//// Default Modbus Settings
//#define SLAVE_ADDRESS 1 
//#define MODBUS_PORT 502
//
//// Global Instances
//RTC_DS3231 rtc;
//SDManager sd; 
//
////BasicLogFileManager fileManager(&sd, 4);
//TimeLogFileManager fileManager(&sd, 4);
//Datalogger datalogger(&sd, &fileManager);
//
////EnergyMeterRegInterpreter regInterpreter(&sd);
//
//EMRegInterpreter regInterpreter(&sd); 
//EnergyMeter750 energy_meter;
//IModbusTransport* modbus = nullptr; // Polymorphic pointer for TCP or RTU
//
//ModbusRequestCSV mb_csv(&sd);
//
//bool _advancedIsInitialized = false; 
//int ultimaUnidadTiempo;
//Parameters param;
//unsigned long anteriorMillisModbus = 0;
//int int_log_interval = 0; 
//int int_max_files = 0; 
//
//void check_critical_error(esp_err_t err, const char* msg); 
//void onEnergyMeterError(const char* tag, const char* mensaje);
//
//esp_err_t crear_nueva_sesion_log();
//esp_err_t lectura_modbus();
//
//esp_err_t prepareAdvanceDatalogger(Struct_MBRequest MB_req){
//    
//    // Validaciones de negocio
//    if (MB_req.channel <= 0) return ESP_ERR_INVALID_ARG;
//    if (MB_req.length == 0 || MB_req.length > MAX_MODBUS_REGS_REQUEST) return ESP_ERR_INVALID_SIZE;
//   
//    // Intentar cargar mapa
//    esp_err_t err = regInterpreter.startNewRequest(MB_req.start_addres, MB_req.length);
//    if (err != ESP_OK) return err;
//
//    nameColValues misTitulos = regInterpreter.getLastNameValues();
//    if(misTitulos.size == 0) return ESP_ERR_INTERPRETER_MAP_MISS;
//   
//   // Cargar parámetros adicionales
//    //loadParametersMapRegister(); 
////    Parameters param = getParameters();
//    
//    int_log_interval = atoi(param.log_interval);
//    int_max_files = atoi(param.max_files);
//
//
//   if (int_max_files <= 0 || int_max_files >= MAX_LOG_CAPACITY) {
//       return ESP_ERR_INTERPRETER_BAD_CONF;
//    }
//
//    datalogger.setMaxFiles(int_max_files);
//    //datalogger->clearAllLogs(); // Optional: clears folder on every reboot
//    
//    _advancedIsInitialized = true;
//
//    //err = crear_nueva_sesion_log(datalogger, rtc, &_misTitulos);
// 
//    return err; 
//}
//
//
///**
// * @brief Main execution loop for timed logging and file rotation.
// */
// esp_err_t advancedDataloggerExec() {
//    if (!_advancedIsInitialized){ 
//        //Serial.println("error de que no se inicializo el advanced"); 
//        return ESP_ERR_INTERPRETER_NOT_INIT;
//    };
//
//    unsigned long actualMillis = millis();
//    DateTime ahora = rtc.now();
//
//    // --- CAMBIO DE SESIÓN (Basado en RTC) ---
//    bool debeCambiarSesion = false;
//    esp_err_t err; 
//
//    // Comparamos el tiempo actual con la última vez que se cambió de archivo
//    if (ahora.minute() != ultimaUnidadTiempo) { 
//        // Ejemplo para cambio cada MINUTO
//        if (strcasecmp(param.new_file, "minute") == 0) debeCambiarSesion = true;
//        
//        // Ejemplo para cambio cada HORA (si el minuto es 0 y cambió la hora)
//        if (strcasecmp(param.new_file, "hour") == 0 && ahora.minute() == 0) debeCambiarSesion = true;
//        
//        // Ejemplo para cambio cada DÍA (si es medianoche)
//        if (strcasecmp(param.new_file, "day") == 0 && ahora.hour() == 0 && ahora.minute() == 0) debeCambiarSesion = true;
//
//        if (debeCambiarSesion) {
//            ultimaUnidadTiempo = ahora.minute(); // Actualizamos bandera
//            err = crear_nueva_sesion_log();
//
//            if (err != ESP_OK){
//                //Serial.println("error 2");  
//            return err;
//            }
//        }
//    }
//
//    // --- MUESTREO (Basado en millis) ---
//    if (actualMillis - anteriorMillisModbus >= (unsigned long)int_log_interval) {
//        anteriorMillisModbus += int_log_interval;
//        
//        err = lectura_modbus();
//       
//        if (err != ESP_OK){
//            //Serial.println("error 3");  
//            return err;
//        };
//    }
//    return ESP_OK;
//}
//
//void setup() {
//
//    Serial.begin(115200);
//    while(!Serial); 
//    
//   
//    if (!rtc.begin()) {
//        Serial.println("En esta aplicacion el RTC es vital..."); 
//        while(true);
//    }
//
//    rtc.adjust(DateTime(2026, 5, 28, 15, 32, 1));
//
//    esp_err_t err;
//
//    // 1. Hardware Base: Tarjeta SD
//    err = sd.begin();
//    check_critical_error(err, "Fallo en Hardware SD");
//
//    // 2. Servicios: Datalogger (CORREGIDO: Ahora propaga y evalúa correctamente el código err)
//    err = datalogger.begin();
//    check_critical_error(err, "Directorio /LOGS no accesible o Datalogger no inicializado");
//
//    // 3. Configuración: Carga de Mapas y Peticiones desde SD
//    err = regInterpreter.begin();
//    check_critical_error(err, "Fallo al leer Mapa de Registros (EM750map.csv)");
//
//    err = mb_csv.begin();
//    check_critical_error(err, "Fallo al acceder a configuración (MBReq.csv)");
//
//    err = mb_csv.loadFromSDParameters();
//    check_critical_error(err, "Fallo al obtener el modbus request"); 
//
//    // 4. Capa de Transporte: Conexión Modbus
//    Struct_MBRequest req;  
//    err = mb_csv.loadFromSDMbrequest(&req);
//    check_critical_error(err, "Petición Modbus inválida en CSV");
//
//    param = SDgetParameters(&sd); 
//
//    bool useTCP = true; 
//
//    
//    byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
//    IPAddress local_ip(192, 168, 0, 10);
//    IPAddress server_ip;
//
//    if(server_ip.fromString(mb_csv.getIpAdress())) {
//        ModbusTCPManager* tcp = new ModbusTCPManager(server_ip, SLAVE_ADDRESS, MODBUS_PORT);
//        tcp->begin(mac, local_ip);
//        modbus = tcp;
//    } else {
//        check_critical_error(ESP_ERR_CONFIG_INVALID_DATA, "IP del Servidor inválida");
//    }
//     
//    /*else {
//        ModbusRTUManager* rtu = new ModbusRTUManager(19200, 1, SERIAL_8E1);
//        rtu->begin();
//        modbus = rtu;
//    }*/
//
//    // 5. Driver de Dispositivo y Sesión de Log
//    err = energy_meter.begin(modbus);
//    check_critical_error(err, "Error al vincular Driver EM750");
//
//    Log_msg::registerCallback(onEnergyMeterError); 
//
//    err = prepareAdvanceDatalogger(req); 
//
//    //err = regInterpreter.prepareAdvanceDatalogger(req, &datalogger, &rtc);
//    check_critical_error(err, "No se pudo iniciar la sesión de Datalogging");
//
//}
//
//
//void loop() {
//    // El intérprete gestiona internamente los tiempos de muestreo
//    esp_err_t err = advancedDataloggerExec();
//    if(err != ESP_OK){
//      check_critical_error(err, "No se pudo iniciar la sesion de Datalogging");
//    }
//    // El delay es pequeño para mantener la responsividad
//    delay(1000); 
//}
//
//void check_critical_error(esp_err_t err, const char* msg) {
//    if (err != ESP_OK) {
//        Serial.printf("\n[CRITICO] %s | Error: 0x%X\n", msg, err);
//
//        /*
//        if (sd.isReady()) {
//            // Aumentamos a 32 por seguridad para evitar truncamientos en el stack
//            char timestamp[128]; 
//            strcpy(timestamp, "SYSTEM_PANIC");
//            
//            //DateTime now(2026, 5, 21, 13, 19, 1); // = rtc.now();
//            DateTime now = rtc.now();
//            if (now.year() >= 2026) { 
//                snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d", 
//                         now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
//            }
//            Serial.print("Esto es: ");
//            Serial.println(timestamp);
//
//            char err_payload[128];
//            snprintf(err_payload, sizeof(err_payload), "%s (Cod: 0x%X)", msg, err);
//
//            Serial.println("Escribiendo reporte de fallo en la SD...");
//            datalogger.appendErrorLog(timestamp, err_payload);
//            
//            //datalogger.flushBuffer();
//        } else {
//            Serial.println("[AVISO] SD no lista. Imposible guardar el reporte.");
//        }
//        */
//         Serial.println("BLOQUEAMOS EL MICRO");
//        delay(1000);
//        sd.end(); 
//        while(true); 
//        Serial.println("Reiniciando sistema en 5 segundos...");
//        delay(5000); 
//        ESP.restart(); 
//    }
//}
//
//void onEnergyMeterError(const char* tag, const char* mensaje) {
//    // 1. Mostrar el error en el monitor serial (se queda igual)
//    Serial.printf("[ERROR CONTROLADO] [%s] -> %s\n", tag, mensaje);
//
//    // 2. Si la SD está lista, guardar la información completa
//    if (sd.isReady()) {
//        char timestamp[64];
//        DateTime now = rtc.now();
//        
//        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d", 
//                 now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
//
//        // 🛠️ SOLUCIÓN: Creamos un nuevo buffer para empaquetar "[TAG] Mensaje"
//        char mensajeCompleto[192]; // 64 (para el TAG) + 128 (del mensaje original) = 192 bytes seguro
//        
//        // Formateamos el mensaje incluyendo el TAG de forma idéntica a como lo ves en consola
//        snprintf(mensajeCompleto, sizeof(mensajeCompleto), "[%s] %s", tag, mensaje);
//
//        // Pasamos el nuevo mensaje empaquetado al datalogger
//        datalogger.appendErrorLog(timestamp, mensajeCompleto);
//    }
//}
//
//
//esp_err_t crear_nueva_sesion_log() {
//    DateTime ahora = rtc.now();
//    char nombreFichero[16]; 
//
//    // Formato: MMDDHHmm
//    snprintf(nombreFichero, sizeof(nombreFichero), "%02d%02d%02d%02d%02d%02d", 
//        ahora.year(), // Extrae los últimos dos dígitos del año (ej: 2026 -> 26)
//        ahora.month(), 
//        ahora.day(), 
//        ahora.hour(),
//        ahora.minute(),
//        ahora.second());
//    
//    //const char* actual = datalogger->getCurrentLogFile();
//
//    // --- DEBUG LOGS ---
//   //Serial.println(F("--- Comparación de Sesión ---"));
//   //Serial.print(F("Nueva sugerencia (nombreFichero): ")); 
//   //Serial.println(nombreFichero);
//   //Serial.print(F("Sesión activa (actual): ")); 
//   //Serial.println(actual[0] == '\0' ? "[VACÍO]" : actual);
//    // ------------------
//
//    // Verificamos si nombreFichero está contenido en la ruta actual
//    /*
//    if (actual[0] != '\0' && strstr(actual, nombreFichero) != nullptr) {
//        //Serial.println(F(">> COINCIDENCIA DETECTADA: Manteniendo sesión actual.")); 
//        return ESP_OK; 
//    }
//*/
//    esp_err_t err; 
//    //Serial.println(F(">> NO COINCIDE: Creando nueva sesión...")); 
//
//    //Serial.print("nueva sesion: "); 
//    //Serial.println(nombreFichero);
//    nameColValues misTitulos = regInterpreter.getLastNameValues(); 
//    
//    err = datalogger.newCSVLogSesion(nombreFichero, misTitulos.buffer, misTitulos.size);
//    if(err != ESP_OK){
//        return err; 
//    }
//
///*
//     if(datalogger->newCSVLogSesion(nombreFichero, misTitulos->buffer, misTitulos->size)){
//            Serial.println("nueva session correcta");
//     }else{
//            Serial.println("nueva sesion fracaso");
//     }
//*/
//     return ESP_OK;
//}
//
//esp_err_t lectura_modbus(){
//    
//    // 1. Ejecutar Modbus y capturar error
//    EM_request req = regInterpreter.getLastEMRequest(); 
//    esp_err_t err = energy_meter.executeRequest(req);
//    
//    if (err != ESP_OK) {
//        ESP_LOGE("INTERP", "Fallo Modbus: 0x%X", err);
//        return err; // No intentamos procesar datos basura
//    }
//
//    // 2. Procesar datos (esto es interno, confiamos en el buffer)
//    rawDataBuffer raw = energy_meter.readDataBuffer();
//    regInterpreter.getBufferDataRaw(raw.buffer, raw.size);
//    netDataString res = regInterpreter.getBufNetDataString(); 
//
//    // 3. Obtener tiempo
//    DateTime now = rtc.now();
//    char bufferTime[20];
//    snprintf(bufferTime, sizeof(bufferTime), "%04d-%02d-%02d %02d:%02d:%02d", 
//            now.year(), now.month(), now.day(), 
//            now.hour(), now.minute(), now.second());
//
//    // 4. Intentar escribir en SD y capturar error
//    // (Asumiendo que writeRow de Datalogger también se actualiza a esp_err_t)
//
//    err = datalogger.appendNewDataCSVToLog(bufferTime, res.buffer, res.size);
//    if(err != ESP_OK){
//        if((err == ESP_ERR_INVALID_STATE) && datalogger.isFileLimitReached()){
//            return ESP_OK; // simplemente no se hace nada, se prosigue. de momento
//        }
//        //Serial.println("el error tiene pinta de que es aqui"); 
//        return err; 
//    }
///*
//    if(datalogger->appendNewDataCSVToLog(bufferTime, res.buffer, res.size)){
//         Serial.println("se escribe un nuevo dato en el buffer"); 
//    }else{
//        Serial.println("algo va mal mal");
//    }
//*/
//    return ESP_OK; 
//}


//---------------------------------------------------------------------
//#include <RTClib.h>
//
//// Transport Layer implementations
//#include "ModbusTCPManager.h" 
////#include "ModbusRTUManager.h"
//
//// Core Services
////#include "BasicLogFileManager.h"
//#include "TimeLogFileManager.h"
//#include "Datalogger.h"
//#include "EnergyMeter750.h"
//#include "SDManager.h"
//
//// Logic and Configuration Managers
////#include "EnergyMeterRegInterpreter.h"
//#include "ModbusRequestCSV.h"
//
//#include "LogMsgGlobal.h"
//
//// Default Modbus Settings
//#define SLAVE_ADDRESS 1 
//#define MODBUS_PORT 502
//
//// Global Instances
//RTC_DS3231 rtc;
//SDManager sd; 
//
////BasicLogFileManager fileManager(&sd, 4);
//TimeLogFileManager fileManager(&sd, 4);
//Datalogger datalogger(&sd, &fileManager);
//
////EnergyMeterRegInterpreter regInterpreter(&sd); 
//EnergyMeter750 energy_meter; 
//ModbusRequestCSV mb_csv(&sd);
//IModbusTransport* modbus = nullptr; // Polymorphic pointer for TCP or RTU
//
//
//void check_critical_error(esp_err_t err, const char* msg) {
//    if (err != ESP_OK) {
//        Serial.printf("\n[CRITICO] %s | Error: 0x%X\n", msg, err);
//
//        /*
//        if (sd.isReady()) {
//            // Aumentamos a 32 por seguridad para evitar truncamientos en el stack
//            char timestamp[128]; 
//            strcpy(timestamp, "SYSTEM_PANIC");
//            
//            //DateTime now(2026, 5, 21, 13, 19, 1); // = rtc.now();
//            DateTime now = rtc.now();
//            if (now.year() >= 2026) { 
//                snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d", 
//                         now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
//            }
//            Serial.print("Esto es: ");
//            Serial.println(timestamp);
//
//            char err_payload[128];
//            snprintf(err_payload, sizeof(err_payload), "%s (Cod: 0x%X)", msg, err);
//
//            Serial.println("Escribiendo reporte de fallo en la SD...");
//            datalogger.appendErrorLog(timestamp, err_payload);
//            
//            //datalogger.flushBuffer();
//        } else {
//            Serial.println("[AVISO] SD no lista. Imposible guardar el reporte.");
//        }
//        */
//         Serial.println("BLOQUEAMOS EL MICRO");
//        delay(1000);
//        sd.end(); 
//        while(true); 
//        Serial.println("Reiniciando sistema en 5 segundos...");
//        delay(5000); 
//        ESP.restart(); 
//    }
//}
//
//void onEnergyMeterError(const char* tag, const char* mensaje) {
//    // 1. Mostrar el error en el monitor serial (se queda igual)
//    Serial.printf("[ERROR CONTROLADO] [%s] -> %s\n", tag, mensaje);
//
//    // 2. Si la SD está lista, guardar la información completa
//    if (sd.isReady()) {
//        char timestamp[64];
//        DateTime now = rtc.now();
//        
//        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d", 
//                 now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
//
//        // 🛠️ SOLUCIÓN: Creamos un nuevo buffer para empaquetar "[TAG] Mensaje"
//        char mensajeCompleto[192]; // 64 (para el TAG) + 128 (del mensaje original) = 192 bytes seguro
//        
//        // Formateamos el mensaje incluyendo el TAG de forma idéntica a como lo ves en consola
//        snprintf(mensajeCompleto, sizeof(mensajeCompleto), "[%s] %s", tag, mensaje);
//
//        // Pasamos el nuevo mensaje empaquetado al datalogger
//        datalogger.appendErrorLog(timestamp, mensajeCompleto);
//    }
//}
//
//void setup() {
//
//    Serial.begin(115200);
//    while(!Serial); 
//    
//   
//    if (!rtc.begin()) {
//        Serial.println("En esta aplicacion el RTC es vital..."); 
//        while(true);
//    }
//
//    rtc.adjust(DateTime(2026, 5, 26, 17, 11, 1));
//
//    esp_err_t err;
//
//    // 1. Hardware Base: Tarjeta SD
//    err = sd.begin();
//    check_critical_error(err, "Fallo en Hardware SD");
//
//    // 2. Servicios: Datalogger (CORREGIDO: Ahora propaga y evalúa correctamente el código err)
//    err = datalogger.begin();
//    check_critical_error(err, "Directorio /LOGS no accesible o Datalogger no inicializado");
//
//    // 3. Configuración: Carga de Mapas y Peticiones desde SD
//    //err = regInterpreter.begin();
//    check_critical_error(err, "Fallo al leer Mapa de Registros (EM750map.csv)");
//
//    err = mb_csv.begin();
//    check_critical_error(err, "Fallo al acceder a configuración (MBReq.csv)");
//
//    err = mb_csv.loadFromSDParameters();
//    check_critical_error(err, "Fallo al obtener el modbus request"); 
//
//    // 4. Capa de Transporte: Conexión Modbus
//    Struct_MBRequest req;  
//    err = mb_csv.loadFromSDMbrequest(&req);
//    check_critical_error(err, "Petición Modbus inválida en CSV");
//
//    bool useTCP = true; 
//
//    
//    byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
//    IPAddress local_ip(192, 168, 0, 10);
//    IPAddress server_ip;
//
//    if(server_ip.fromString(mb_csv.getIpAdress())) {
//        ModbusTCPManager* tcp = new ModbusTCPManager(server_ip, SLAVE_ADDRESS, MODBUS_PORT);
//        tcp->begin(mac, local_ip);
//        modbus = tcp;
//    } else {
//        check_critical_error(ESP_ERR_CONFIG_INVALID_DATA, "IP del Servidor inválida");
//    }
//     
//    /*else {
//        ModbusRTUManager* rtu = new ModbusRTUManager(19200, 1, SERIAL_8E1);
//        rtu->begin();
//        modbus = rtu;
//    }*/
//
//    // 5. Driver de Dispositivo y Sesión de Log
//    err = energy_meter.begin(modbus);
//    check_critical_error(err, "Error al vincular Driver EM750");
//
//    Log_msg::registerCallback(onEnergyMeterError); 
//
//    //err = regInterpreter.prepareAdvanceDatalogger(req, &datalogger, &rtc);
//    check_critical_error(err, "No se pudo iniciar la sesión de Datalogging");
//
//}
//
//void loop() {
//    
//    //// El intérprete gestiona internamente los tiempos de muestreo
//    //esp_err_t err = regInterpreter.advancedDataloggerExec(&datalogger, &energy_meter, &rtc);
//    //if(err != ESP_OK){
//    //  check_critical_error(err, "No se pudo iniciar la sesion de Datalogging");
//    //}
//    //// El delay es pequeño para mantener la responsividad
//    delay(1000); 
//}

//--------------------------------------------------------------------------------------------------

//#include <RTClib.h>
//
//// Transport Layer implementations
//#include "ModbusTCPManager.h" 
////#include "ModbusRTUManager.h"
//
//// Core Services
////#include "BasicLogFileManager.h"
//#include "TimeLogFileManager.h"
//#include "Datalogger.h"
//#include "EnergyMeter750.h"
//#include "SDManager.h"
//
//// Logic and Configuration Managers
//#include "EnergyMeterRegInterpreter.h"
//#include "ModbusRequestCSV.h"
//
//#include "LogMsgGlobal.h"
//
//// Default Modbus Settings
//#define SLAVE_ADDRESS 1 
//#define MODBUS_PORT 502
//
//// Global Instances
//RTC_DS3231 rtc;
//SDManager sd; 
//
////BasicLogFileManager fileManager(&sd, 4);
//TimeLogFileManager fileManager(&sd, 4);
//Datalogger datalogger(&sd, &fileManager);
//
//EnergyMeterRegInterpreter regInterpreter(&sd); 
//EnergyMeter750 energy_meter; 
//ModbusRequestCSV mb_csv(&sd);
//IModbusTransport* modbus = nullptr; // Polymorphic pointer for TCP or RTU
//
//
//void check_critical_error(esp_err_t err, const char* msg) {
//    if (err != ESP_OK) {
//        Serial.printf("\n[CRITICO] %s | Error: 0x%X\n", msg, err);
//
//        /*
//        if (sd.isReady()) {
//            // Aumentamos a 32 por seguridad para evitar truncamientos en el stack
//            char timestamp[128]; 
//            strcpy(timestamp, "SYSTEM_PANIC");
//            
//            //DateTime now(2026, 5, 21, 13, 19, 1); // = rtc.now();
//            DateTime now = rtc.now();
//            if (now.year() >= 2026) { 
//                snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d", 
//                         now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
//            }
//            Serial.print("Esto es: ");
//            Serial.println(timestamp);
//
//            char err_payload[128];
//            snprintf(err_payload, sizeof(err_payload), "%s (Cod: 0x%X)", msg, err);
//
//            Serial.println("Escribiendo reporte de fallo en la SD...");
//            datalogger.appendErrorLog(timestamp, err_payload);
//            
//            //datalogger.flushBuffer();
//        } else {
//            Serial.println("[AVISO] SD no lista. Imposible guardar el reporte.");
//        }
//        */
//         Serial.println("BLOQUEAMOS EL MICRO");
//        delay(1000);
//        sd.end(); 
//        while(true); 
//        Serial.println("Reiniciando sistema en 5 segundos...");
//        delay(5000); 
//        ESP.restart(); 
//    }
//}
//
//void onEnergyMeterError(const char* tag, const char* mensaje) {
//    // 1. Mostrar el error en el monitor serial (se queda igual)
//    Serial.printf("[ERROR CONTROLADO] [%s] -> %s\n", tag, mensaje);
//
//    // 2. Si la SD está lista, guardar la información completa
//    if (sd.isReady()) {
//        char timestamp[64];
//        DateTime now = rtc.now();
//        
//        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d", 
//                 now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
//
//        // 🛠️ SOLUCIÓN: Creamos un nuevo buffer para empaquetar "[TAG] Mensaje"
//        char mensajeCompleto[192]; // 64 (para el TAG) + 128 (del mensaje original) = 192 bytes seguro
//        
//        // Formateamos el mensaje incluyendo el TAG de forma idéntica a como lo ves en consola
//        snprintf(mensajeCompleto, sizeof(mensajeCompleto), "[%s] %s", tag, mensaje);
//
//        // Pasamos el nuevo mensaje empaquetado al datalogger
//        datalogger.appendErrorLog(timestamp, mensajeCompleto);
//    }
//}
//
//void setup() {
//
//    Serial.begin(115200);
//    while(!Serial); 
//    
//   
//    if (!rtc.begin()) {
//        Serial.println("En esta aplicacion el RTC es vital..."); 
//        while(true);
//    }
//
//    rtc.adjust(DateTime(2026, 5, 26, 17, 11, 1));
//
//    esp_err_t err;
//
//    // 1. Hardware Base: Tarjeta SD
//    err = sd.begin();
//    check_critical_error(err, "Fallo en Hardware SD");
//
//    // 2. Servicios: Datalogger (CORREGIDO: Ahora propaga y evalúa correctamente el código err)
//    err = datalogger.begin();
//    check_critical_error(err, "Directorio /LOGS no accesible o Datalogger no inicializado");
//
//    // 3. Configuración: Carga de Mapas y Peticiones desde SD
//    err = regInterpreter.begin();
//    check_critical_error(err, "Fallo al leer Mapa de Registros (EM750map.csv)");
//
//    err = mb_csv.begin();
//    check_critical_error(err, "Fallo al acceder a configuración (MBReq.csv)");
//
//    err = mb_csv.loadFromSDParameters();
//    check_critical_error(err, "Fallo al obtener el modbus request"); 
//
//    // 4. Capa de Transporte: Conexión Modbus
//    Struct_MBRequest req;  
//    err = mb_csv.loadFromSDMbrequest(&req);
//    check_critical_error(err, "Petición Modbus inválida en CSV");
//
//    bool useTCP = true; 
//
//    
//    byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
//    IPAddress local_ip(192, 168, 0, 10);
//    IPAddress server_ip;
//
//    if(server_ip.fromString(mb_csv.getIpAdress())) {
//        ModbusTCPManager* tcp = new ModbusTCPManager(server_ip, SLAVE_ADDRESS, MODBUS_PORT);
//        tcp->begin(mac, local_ip);
//        modbus = tcp;
//    } else {
//        check_critical_error(ESP_ERR_CONFIG_INVALID_DATA, "IP del Servidor inválida");
//    }
//     
//    /*else {
//        ModbusRTUManager* rtu = new ModbusRTUManager(19200, 1, SERIAL_8E1);
//        rtu->begin();
//        modbus = rtu;
//    }*/
//
//    // 5. Driver de Dispositivo y Sesión de Log
//    err = energy_meter.begin(modbus);
//    check_critical_error(err, "Error al vincular Driver EM750");
//
//    Log_msg::registerCallback(onEnergyMeterError); 
//
//    err = regInterpreter.prepareAdvanceDatalogger(req, &datalogger, &rtc);
//    check_critical_error(err, "No se pudo iniciar la sesión de Datalogging");
//
//}
//
//void loop() {
//    // El intérprete gestiona internamente los tiempos de muestreo
//    esp_err_t err = regInterpreter.advancedDataloggerExec(&datalogger, &energy_meter, &rtc);
//    if(err != ESP_OK){
//      check_critical_error(err, "No se pudo iniciar la sesion de Datalogging");
//    }
//    // El delay es pequeño para mantener la responsividad
//    delay(1000); 
//}

