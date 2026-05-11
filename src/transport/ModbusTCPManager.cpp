#include "ModbusTCPManager.h"

/**
 * @brief Automatic reconnection management.
 * Ensures the socket is properly cleared before attempting a retry to prevent 
 * resource leaks in the Wiznet/Ethernet hardware.
 */
bool ModbusTCPManager::ensureConnection() {
    if (!_modbusClient.connected()) {
        // Stop the client to free up socket resources on the Ethernet chip
        _modbusClient.stop(); 
        
        //Serial.println(F("ModbusTCP: Attempting to connect to server..."));
        
        if (!_modbusClient.begin(_serverIP, _port)) {
            //Serial.println(F("ModbusTCP: TCP connection failed."));
            return false;
        }
        
        // Brief delay for handshake stabilization
        delay(50); 
        //Serial.println(F("ModbusTCP: Successfully connected."));
    }
    return true;
}

/**
 * @brief Specific hardware initialization for the Ethernet peripheral.
 */
void ModbusTCPManager::begin(byte mac[], IPAddress localIP) {
    Ethernet.init(ETHERNET_CS); 
    Ethernet.begin(mac, localIP);
    delay(1000); // Allow network hardware to stabilize
}

/**
 * @brief Implements the begin() method required by the ModbusTransport interface.
 * Triggers the initial TCP connection.
 */
bool ModbusTCPManager::begin() {
    return ensureConnection();
}

/**
 * @brief Reads Holding Registers using the Modbus TCP protocol.
 * Forces a client stop if the request fails to reset the state machine.
 */
bool ModbusTCPManager::readHoldingRegisters(uint16_t address, uint16_t quantity) {
    if (!ensureConnection()) return false;

    if (!_modbusClient.requestFrom(_slaveID, HOLDING_REGISTERS, address, quantity)) {
        //Serial.println(F("ModbusTCP: Request failed. Forcing stop..."));
        _modbusClient.stop(); 
        return false;
    }
    return true;
}

/**
 * @brief Reads Coils from the target slave address.
 */
bool ModbusTCPManager::readCoils(int address, int quantity) {
    if (!ensureConnection()) return false;
    return _modbusClient.requestFrom(_slaveID, COILS, address, quantity);
}

/**
 * @brief Writes a single value to a Holding Register.
 */
bool ModbusTCPManager::writeHoldingRegister(uint16_t address, uint16_t value) {
  if (!ensureConnection()) return false;
  return _modbusClient.holdingRegisterWrite(_slaveID, address, value);
}

/**
 * @brief Fetches data from the internal Modbus response buffer.
 */
uint16_t ModbusTCPManager::read() {
  return _modbusClient.read();
}

/**
 * @brief Writes a single state to a specific Coil.
 */
bool ModbusTCPManager::writeCoil(uint16_t address, bool value) {
    if (!ensureConnection()) return false;
    return _modbusClient.coilWrite(_slaveID, address, value);
}

/**
 * @brief Checks if the TCP socket is currently active.
 */
bool ModbusTCPManager::connected() {
  return _modbusClient.connected();
}

/**
 * @brief Implements the generic requestFrom logic for use within the abstraction layer.
 */
bool ModbusTCPManager::requestFrom(int slaveAddress, int type, uint16_t address, uint16_t nb){
    if (!ensureConnection()) return false;
    return _modbusClient.requestFrom(slaveAddress, type, address, nb);
}