
#ifndef ENERGY_METER_750_H
#define ENERGY_METER_750_H

#include <ArduinoModbus.h>
//#include "SDManager.h"
#include "EnergyMeterRegInterpreter.h"

#define NUM_COL_REG_EM750 5
#define MAX_MODBUS_REGS 125

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

    //uint16_t readAdress(long addr); 

    uint16_t* getData(){ return _internalBuffer;}
    uint16_t getLastSize() { return _lastReadSize; }

    bool readRegisters(EM_request req);
    //bool readAndProcess_2(long start_addr, long size, EnergyMeterRegInterpreter* mapa, void (*callback)(float));

};

#endif
