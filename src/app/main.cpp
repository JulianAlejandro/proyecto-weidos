

#include <RTClib.h>
#include "SDManager.h"
#include "TimeLogFileManager.h"
#include "Datalogger.h"
#include "EMRegInterpreter.h"
#include "EnergyMeter750.h"
#include "ModbusTCPManager.h"
#include "ModbusRequestCSV.h"
#include "AdvancedDatalogger.h" 
#include "LogMsgGlobal.h"

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
void onEnergyMeterError(const char* tag, const char* mensaje);

void setup() {
    Serial.begin(115200);
    while(!Serial);

    if (!rtc.begin()) while(true);
    rtc.adjust(DateTime(2026, 5, 29, 13, 10, 1));

    // Inicializaciones obligatorias de HW y archivos
    check_critical_error(sd.begin(), "Fallo en Hardware SD");
    check_critical_error(datalogger.begin(), "Datalogger no inicializado");
    check_critical_error(regInterpreter.begin(), "Fallo al leer Mapa");
    check_critical_error(mb_csv.begin(), "Fallo al acceder a config");
    check_critical_error(mb_csv.loadFromSDParameters(), "Fallo al obtener modbus req");

    //Struct_MBRequest req;  
    check_critical_error(mb_csv.loadFromSDMbrequests(), "Petición Modbus inválida");

    uint16_t n_requests = mb_csv.getLastRequestsCount(); 
    const Struct_MBRequest* table_reqs = mb_csv.getLastRequestsTable(); 

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

    Log_msg::registerCallback(onEnergyMeterError); 

    // ¡Arrancamos el motor! El orquestador toma el control
    check_critical_error(loggerEngine.begin(table_reqs, n_requests), "No se pudo iniciar la sesión");
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

        Serial.println("BLOQUEAMOS EL MICRO");
        delay(1000);
        sd.end(); 
        while(true); 

        Serial.println("Reiniciando sistema en 5 segundos...");
        delay(5000); 
        ESP.restart(); 
    }
}


void onEnergyMeterError(const char* tag, const char* mensaje) {
    // 1. Mostrar el error en el monitor serial (se queda igual)
    Serial.printf("[ERROR CONTROLADO] [%s] -> %s\n", tag, mensaje);

    // 2. Si la SD está lista, guardar la información completa
    if (sd.isReady()) {
        char timestamp[64];
        DateTime now = rtc.now();
        
        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d", 
                 now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

        // 🛠️ SOLUCIÓN: Creamos un nuevo buffer para empaquetar "[TAG] Mensaje"
        char mensajeCompleto[192]; // 64 (para el TAG) + 128 (del mensaje original) = 192 bytes seguro
        
        // Formateamos el mensaje incluyendo el TAG de forma idéntica a como lo ves en consola
        snprintf(mensajeCompleto, sizeof(mensajeCompleto), "[%s] %s", tag, mensaje);

        // Pasamos el nuevo mensaje empaquetado al datalogger
        datalogger.appendErrorLog(timestamp, mensajeCompleto);
    }
}
