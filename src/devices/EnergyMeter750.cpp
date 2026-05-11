#include "EnergyMeter750.h"

/**
 * @brief Default constructor. Initialization happens in begin().
 */
EnergyMeter750::EnergyMeter750() {
}

/**
 * @brief Sets the transport layer and marks the driver as ready.
 * @param modbus Reference to the Modbus communication object.
 */
int EnergyMeter750::begin(ModbusTransport* modbus) {
    if(modbus == nullptr){
        return false;
    }
    _modbus = modbus;
    _initialized = true;
    return true;
}

/**
 * @brief Validates and executes a Modbus read operation.
 * * Performs three safety checks:
 * 1. Validates that size is within MAX_MODBUS_REGS.
 * 2. Validates that the start address exists in the EM750 map.
 * 3. Validates that the full block (start + size) does not exceed memory limits.
 */
bool EnergyMeter750::executeRequest(EM_request req) {
    if(!_initialized) return false; 

    _lastReadSize = 0; // Preemptive reset

    // 1. Validate request size
    if (req.size == 0 || req.size > MAX_MODBUS_REGS_REQUEST) {
        return false;
    }

    // 2. Validate start address range
    if (req.start_addr > MAX_EM_ADDR) {
        return false;
    }

    // 3. Validate that the full block (start + size) is within the device map
    // We subtract 1 because addr 20000 with size 1 ends at addr 20000.
    if ((req.start_addr + req.size - 1) > MAX_EM_ADDR) {
        return false;
    }

    // Perform the Modbus read
    if(_modbus->readHoldingRegisters(req.start_addr, req.size)){
        int i = 0;
        // Transfer data from transport buffer to local internal buffer
        for (i = 0; i < req.size; i++) {
            _internalBuffer[i] = _modbus->read(); 
        }
        _lastReadSize = i; 
        return true; 
    } 

    return false;
}

/**
 * @brief Returns the structure used to access the internal raw data.
 */
rawDataBuffer EnergyMeter750::readDataBuffer(){ 
    return { _internalBuffer, _lastReadSize };
}

/**
 * @brief Reads a single register. Used for isolated parameters 
 * that do not require full buffer updates.
 */
uint16_t EnergyMeter750::readRegByAdress(uint16_t addr){
    if(!_initialized) return 0; 

    // Request exactly 1 register
    if(_modbus->readHoldingRegisters(addr, 1)) {
        return _modbus->read();
    }
    
    return 0;
}