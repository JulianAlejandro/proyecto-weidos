#include "EnergyMeter750.h"
//#include "EnergyMeterRegInterpreter.h"


EnergyMeter750::EnergyMeter750() {
    //_slaveAddress = slaveAddress;
}

/*
EnergyMeter750::EnergyMeter750(uint8_t slaveAddress) {
    //_slaveAddress = slaveAddress;
}
*/
int EnergyMeter750::begin(ModbusTCPManager* modbusTCP) {
    _modbus = modbusTCP;
   // modbus->begin(mac, ip); // aqui faltan estos elementos.....ya van inicializados de fuera?? o como
    return true;
}


/*
int EnergyMeter750::begin(ModbusTCPClient* modbusClient) {
    _modbus = modbusClient;
  
    return true;
}
*/

// En EnergyMeter750.cpp
bool EnergyMeter750::executeRequest(EM_request req) {
    _lastReadSize = 0; // Reset preventivo

    // 1. Validar que el tamaño sea coherente
    if (req.size == 0 || req.size > MAX_MODBUS_REGS) {
        return false;
    }

    // 2. Validar que la dirección de inicio sea válida
    if (req.start_addr > MAX_EM_ADDR) {
        return false;
    }

    // 3. Validar que el bloque completo (inicio + tamaño) no exceda el límite
    // Restamos 1 porque si pides la dirección 20000 con size 1, la dirección final es 20000.
    if ((req.start_addr + req.size - 1) > MAX_EM_ADDR) {
        return false;
    }
    if(_modbus->readHoldingRegisters(req.start_addr, req.size)){
        int i = 0;
        for (i = 0; i < req.size; i++) { // todo esto se puede mejorar tal y como esta abajo...
        //Serial.print("Registro ");
        //Serial.print(19000 + i);
        //Serial.print(": ");
        //Serial.println(modbus.getAvailableData());
        _internalBuffer[i] = _modbus->getAvailableData(); 
        }
        _lastReadSize = i; 
        return true; 
    } 

    /*
    if (_modbus->requestFrom(_slaveAddress, INPUT_REGISTERS, req.start_addr, req.size)) {
        uint16_t i = 0;
        while (_modbus->available() && i < req.size) {
            _internalBuffer[i] = _modbus->read();
            i++;
        }
        _lastReadSize = i; 
        return true;
    }
    */
    return false;
}

/*
// En EnergyMeter750.cpp
bool EnergyMeter750::executeRequest(EM_request req) {
    _lastReadSize = 0; // Reset preventivo

    // 1. Validar que el tamaño sea coherente
    if (req.size == 0 || req.size > MAX_MODBUS_REGS) {
        return false;
    }

    // 2. Validar que la dirección de inicio sea válida
    if (req.start_addr > MAX_EM_ADDR) {
        return false;
    }

    // 3. Validar que el bloque completo (inicio + tamaño) no exceda el límite
    // Restamos 1 porque si pides la dirección 20000 con size 1, la dirección final es 20000.
    if ((req.start_addr + req.size - 1) > MAX_EM_ADDR) {
        return false;
    }

    if (_modbus->requestFrom(_slaveAddress, INPUT_REGISTERS, req.start_addr, req.size)) {
        uint16_t i = 0;
        while (_modbus->available() && i < req.size) {
            _internalBuffer[i] = _modbus->read();
            i++;
        }
        _lastReadSize = i; 
        return true;
    }
    return false;
}
*/




rawDataBuffer EnergyMeter750::readDataBuffer(){ 
    return { _internalBuffer, _lastReadSize };
}


uint16_t EnergyMeter750::readRegByAdress(uint16_t addr){
    _modbus->readHoldingRegisters(addr, 1);
    //_modbus->requestFrom(_slaveAddress, INPUT_REGISTERS, addr, 1);
    return  _modbus->getAvailableData();
}

/*
uint16_t EnergyMeter750::readRegByAdress(uint16_t addr){
    _modbus->requestFrom(_slaveAddress, INPUT_REGISTERS, addr, 1);
    return  _modbus->read();
}

*/