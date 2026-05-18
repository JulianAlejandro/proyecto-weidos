//#ifndef ENERGY_METER_750_H
//#define ENERGY_METER_750_H
//
//#include <esp_err.h> 
//#include <esp_log.h>
//#include <ArduinoModbus.h>
//#include "../EnergyMeterRegInterpreter.h"
//#include "../core/ModbusTransport.h"
//
//// Definición de errores específicos (opcional pero profesional)
//#define ESP_ERR_EM750_BASE           0x20000
//#define ESP_ERR_EM750_NOT_INIT       (ESP_ERR_EM750_BASE + 1)
//#define ESP_ERR_EM750_ADDR_OUT_RANGE (ESP_ERR_EM750_BASE + 2)
//#define ESP_ERR_EM750_MODBUS_FAIL    (ESP_ERR_EM750_BASE + 3)
//
//#define MAX_MODBUS_REGS_REQUEST 125
//#define MAX_EM_ADDR 22000
//
//struct rawDataBuffer {
//    uint16_t* buffer;
//    uint16_t size;
//};
//
//class EnergyMeter750 {
//  private:
//    static constexpr const char* TAG = "EM750"; // Etiqueta para logs
//    ModbusTransport* _modbus = nullptr;
//    bool _initialized = false;
//    uint16_t _internalBuffer[MAX_MODBUS_REGS_REQUEST];
//    uint16_t _lastReadSize = 0;
//
//  public:
//    EnergyMeter750();
//    
//    // Cambiamos int/bool por esp_err_t
//    esp_err_t begin(ModbusTransport* modbus);
//    esp_err_t executeRequest(EM_request req);
//    
//    // Para lectura simple, a veces es mejor pasar un puntero para el resultado
//    // y devolver el error como estado de la función.
//    esp_err_t readRegByAdress(uint16_t addr, uint16_t *out_value);
//
//    bool isReady() const { return _initialized; }
//    rawDataBuffer readDataBuffer();
//};
//
//#endif
