#include "ModbusRTUManager.h"

/**
 * @brief Initialize members with default serial and pin values.
 */
ModbusRTUManager::ModbusRTUManager(uint32_t baudrate, uint8_t slaveID, uint32_t config) 
    : _baudrate(baudrate), _config(config), _slaveID(slaveID) {
    
    // Default hardware pins for Weidos architecture
    _txPin = RS485_TX;
    _dePin = RS485_DE;
    _rePin = RS485_RE;
}

/**
 * @brief Updates the RS485 control pins. Must be called before begin().
 */
void ModbusRTUManager::setPins(int tx, int de, int re) {
    _txPin = tx;
    _dePin = de;
    _rePin = re;
}

/**
 * @brief Sets up the RS485 flow control and starts the serial client.
 */
bool ModbusRTUManager::begin() {
    // Apply flow control pins to the underlying RS485 driver
    RS485.setPins(_txPin, _dePin, _rePin);
    
    if (!ModbusRTUClient.begin(_baudrate, _config)) {
        //Serial.println(F("RTU Error: Failed to start Modbus client."));
        return false;
    }
    //Serial.println(F("RTU Status: Client initialized successfully."));
    return true;
}

/**
 * @brief Status check for Serial Modbus.
 * Since RTU is stateless, it returns true if the hardware is ready.
 */
bool ModbusRTUManager::connected() {
    return true; 
}

/**
 * @brief Executes a generic Modbus request.
 * Logs slave-specific errors to the Serial port for debugging.
 */
bool ModbusRTUManager::requestFrom(int slaveAddress, int type, uint16_t address, uint16_t nb) {
    if (!ModbusRTUClient.requestFrom(slaveAddress, type, address, nb)) {
        //Serial.print(F("RTU Error: Request failed for slave "));
        //Serial.println(slaveAddress);
        return false;
    }
    return true;
}

/**
 * @brief Fetches data from the Modbus serial buffer.
 */
uint16_t ModbusRTUManager::read() {
    return (uint16_t)ModbusRTUClient.read();
}

/**
 * @brief Implementation of Holding Register reads via the abstraction layer.
 */
bool ModbusRTUManager::readHoldingRegisters(uint16_t address, uint16_t quantity) {
    return requestFrom(_slaveID, HOLDING_REGISTERS, address, quantity);
}

/**
 * @brief Implementation of Coil reads via the abstraction layer.
 */
bool ModbusRTUManager::readCoils(int address, int quantity) {
    return requestFrom(_slaveID, COILS, (uint16_t)address, (uint16_t)quantity);
}

/**
 * @brief Direct write to a Holding Register.
 */
bool ModbusRTUManager::writeHoldingRegister(uint16_t address, uint16_t value) {
    return ModbusRTUClient.holdingRegisterWrite(_slaveID, address, value);
}

/**
 * @brief Direct write to a specific Coil.
 */
bool ModbusRTUManager::writeCoil(uint16_t address, bool value) {
    return ModbusRTUClient.coilWrite(_slaveID, address, value);
}