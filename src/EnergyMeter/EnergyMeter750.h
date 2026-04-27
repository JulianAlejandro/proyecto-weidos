
#ifndef ENERGY_METER_750_H
#define ENERGY_METER_750_H

#include <ArduinoModbus.h>
//#include "SDManager.h"
#include "EnergyMeterRegInterpreter.h"

#define NUM_COL_REG_EM750 5
#define MAX_MODBUS_REGS_REQUEST 125
#define MAX_EM_ADDR 22000

struct rawDataBuffer {
    uint16_t* buffer;  // Puntero a los datos
    uint16_t size;    // Cantidad de registros leídos
};

class EnergyMeter750 {
  private:
    uint8_t _slaveAddress;
    ModbusTCPClient* _modbus; // Usamos puntero para flexibilidad

    uint16_t _internalBuffer[MAX_MODBUS_REGS];
    uint16_t _lastReadSize; 

    int getFormatSize(coded_format f);
   
  public:

    // Constructor: le pasamos la dirección del esclavo
    EnergyMeter750(uint8_t slaveAddress);
    
    // Configura el cliente Modbus que usará
    int begin(ModbusTCPClient* modbusClient);

    uint16_t readData(uint16_t addr); 

    rawDataBuffer readDataBuffer(){ return { _internalBuffer, _lastReadSize };}

    bool executeRequest(EM_request req);

};

#endif
