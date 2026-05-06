#ifndef ENERGY_METER_750_H
#define ENERGY_METER_750_H

#include <ArduinoModbus.h>
#include "EnergyMeterRegInterpreter.h"
#include "../ModbusTCPManager.h" 

#define NUM_COL_REG_EM750 5
#define MAX_MODBUS_REGS_REQUEST 125
#define MAX_EM_ADDR 22000

struct rawDataBuffer {
    uint16_t* buffer;  // Puntero a los datos
    uint16_t size;    // Cantidad de registros leídos
};

class EnergyMeter750 {
  private:
    ModbusTCPManager* _modbus; // Usamos puntero para flexibilidad

    uint16_t _internalBuffer[MAX_MODBUS_REGS_REQUEST];
    uint16_t _lastReadSize; 

    int getFormatSize(coded_format f);
   
  public:

    EnergyMeter750();

    int begin(ModbusTCPManager* modbusTCP);

    uint16_t readData(uint16_t addr); 

    rawDataBuffer readDataBuffer();

    bool executeRequest(EM_request req);

    uint16_t readRegByAdress(uint16_t addr);

};

#endif
