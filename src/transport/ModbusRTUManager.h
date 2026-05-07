#ifndef MODBUS_RTU_MANAGER_H
#define MODBUS_RTU_MANAGER_H

#include <ArduinoRS485.h>
#include <ArduinoModbus.h>
#include "../core/ModbusTransport.h"

class ModbusRTUManager : public ModbusTransport {
  private:
    uint32_t _baudrate;
    uint32_t _config;
    uint8_t  _slaveID;
    
    // Pines específicos para el hardware Weidos/ESP32
    int _txPin, _dePin, _rePin;

  public:
    /**
     * @brief Constructor para Modbus RTU
     * @param baudrate Velocidad (ej. 19200)
     * @param slaveID ID del esclavo por defecto
     * @param config Configuración serial (ej. SERIAL_8E1)
     */
    ModbusRTUManager(uint32_t baudrate = 19200, uint8_t slaveID = 1, uint32_t config = SERIAL_8E1);

    // Métodos de la Interfaz ModbusTransport
    bool begin() override;
    bool connected() override; // En RTU suele devolver siempre true si el bus inició
    
    bool requestFrom(int slaveAddress, int type, uint16_t address, uint16_t nb) override;
    uint16_t read() override;
    
    bool readHoldingRegisters(uint16_t address, uint16_t quantity) override;
    bool readCoils(int address, int quantity) override;

    bool writeHoldingRegister(uint16_t address, uint16_t value) override;
    bool writeCoil(uint16_t address, bool value) override;

    // Métodos específicos de RTU
    void setPins(int tx, int de, int re);
};

#endif