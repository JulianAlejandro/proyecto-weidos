
/*
#include <Arduino.h>

// Definimos una etiqueta (TAG) para identificar de qué parte del código viene el mensaje.
// Es una buena práctica usar una etiqueta por cada archivo .cpp
static const char* TAG = "MAIN_APP";

void setup() {
    // Inicializamos el puerto serie a la misma velocidad del platformio.ini
    Serial.begin(115200);
    delay(2000); // Un pequeño delay para que te dé tiempo de abrir el monitor serie

    Serial.println("--- Empezando prueba de ESP_LOG ---");

    // 1. ESP_LOGE - Error (Rojo): Algo crítico falló.
    ESP_LOGE(TAG, "Esto es un ERROR crítico (Ej: No se pudo conectar al Wi-Fi)");

    // 2. ESP_LOGW - Warning/Advertencia (Amarillo): Algo no va perfecto, pero el programa sigue.
    ESP_LOGW(TAG, "Esto es una ADVERTENCIA (Ej: Voltaje de batería bajo: %.2fV)", 3.42);

    // 3. ESP_LOGI - Info (Verde): Mensajes generales de éxito o estado.
    ESP_LOGI(TAG, "Esto es un mensaje de INFORMACION (Ej: Conectado a la IP: %s)", "192.168.1.50");

    // 4. ESP_LOGD - Debug (Blanco/Normal): Datos técnicos para ti mientras desarrollas.
    int variables_leidas = 4;
    ESP_LOGD(TAG, "Esto es un mensaje de DEBUG (Ej: Se leyeron %d registros del CSV)", variables_leidas);

    // 5. ESP_LOGV - Verbose (Gris/Normal): Logs extremadamente detallados (volcado de datos brutos).
    ESP_LOGV(TAG, "Esto es un mensaje VERBOSE (Detalle al milímetro de lo que pasa por dentro)");
}

void loop() {
    // Un log repetitivo cada 5 segundos para ver que sigue vivo
    static uint32_t contador = 0;
    
    ESP_LOGI(TAG, "Vuelta del bucle número: %Ld", contador);
    contador++;
    
    delay(5000);
}
*/


#include <RTClib.h>

// Transport Layer implementations
#include "transport/ModbusTCPManager.h" 
#include "transport/ModbusRTUManager.h"

// Core Services
//#include "services/BasicLogFileManager.h"
#include "services/TimeLogFileManager.h"
#include "services/Datalogger.h"
#include "devices/EnergyMeter750.h"
#include "services/SDManager.h"

// Logic and Configuration Managers
#include "EnergyMeterRegInterpreter.h"
#include "ModbusRequestCSV.h"

//#include "Debug.h"

// Default Modbus Settings
#define SLAVE_ADDRESS 1 
#define MODBUS_PORT 502

// Global Instances
RTC_DS3231 rtc;
SDManager sd; 

//BasicLogFileManager fileManager(&sd, 4);
TimeLogFileManager fileManager(&sd, 4);
Datalogger datalogger(&sd, &fileManager);

EnergyMeterRegInterpreter regInterpreter(&sd); 
EnergyMeter750 energy_meter; 
ModbusRequestCSV mb_csv(&sd);
IModbusTransport* modbus = nullptr; // Polymorphic pointer for TCP or RTU

//uint8_t g_my_log_current_level = MY_LOG_LEVEL_VERBOSE; 


void check_critical_error(esp_err_t err, const char* msg) {
    if (err != ESP_OK) {
        Serial.printf("\n[CRITICO] %s | Error: 0x%X\n", msg, err);

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

        sd.end(); 
        while(true); 
        Serial.println("Reiniciando sistema en 5 segundos...");
        delay(5000); 
        ESP.restart(); 
    }
}


void setup() {

    Serial.begin(115200);
    while(!Serial); 
    
    //my_log_level_set(MY_LOG_LEVEL_VERBOSE); // imponemos un nivel de log de error 

   
    if (!rtc.begin()) {
        Serial.println("En esta aplicacion el RTC es vital..."); 
        while(true);
    }

    rtc.adjust(DateTime(2026, 5, 26, 12, 52, 1));

    esp_err_t err;

    // 1. Hardware Base: Tarjeta SD
    err = sd.begin();
    check_critical_error(err, "Fallo en Hardware SD");

    // 2. Servicios: Datalogger (CORREGIDO: Ahora propaga y evalúa correctamente el código err)
    err = datalogger.begin();
    check_critical_error(err, "Directorio /LOGS no accesible o Datalogger no inicializado");

    // 3. Configuración: Carga de Mapas y Peticiones desde SD
    err = regInterpreter.begin();
    check_critical_error(err, "Fallo al leer Mapa de Registros (EM750map.csv)");

    err = mb_csv.begin();
    check_critical_error(err, "Fallo al acceder a configuración (MBReq.csv)");

    err = mb_csv.loadFromSDParameters();
    check_critical_error(err, "Fallo al obtener el modbus request"); 

    // 4. Capa de Transporte: Conexión Modbus
    Struct_MBRequest req;  
    err = mb_csv.loadFromSDMbrequest(&req);
    check_critical_error(err, "Petición Modbus inválida en CSV");

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
            check_critical_error(ESP_ERR_CONFIG_INVALID_DATA, "IP del Servidor inválida");
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
    check_critical_error(err, "No se pudo iniciar la sesión de Datalogging");

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


