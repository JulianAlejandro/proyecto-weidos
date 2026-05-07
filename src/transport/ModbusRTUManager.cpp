#include "ModbusRTUManager.h"

ModbusRTUManager::ModbusRTUManager(uint32_t baudrate, uint8_t slaveID, uint32_t config) 
    : _baudrate(baudrate), _config(config), _slaveID(slaveID) {
    // Valores por defecto para Weidos si no se cambian con setPins
    _txPin = RS485_TX;
    _dePin = RS485_DE;
    _rePin = RS485_RE;
}

void ModbusRTUManager::setPins(int tx, int de, int re) {
    _txPin = tx;
    _dePin = de;
    _rePin = re;
}

bool ModbusRTUManager::begin() {
    // Configurar pines de control de flujo RS485
    RS485.setPins(_txPin, _dePin, _rePin);
    
    if (!ModbusRTUClient.begin(_baudrate, _config)) {
        Serial.println(F("RTU: Fallo al iniciar el cliente."));
        return false;
    }
    Serial.println(F("RTU: Cliente iniciado correctamente."));
    return true;
}

bool ModbusRTUManager::connected() {
    // En RTU no hay una "sesión" activa como en TCP. 
    // Si el hardware inició, se considera conectado.
    return true; 
}

bool ModbusRTUManager::requestFrom(int slaveAddress, int type, uint16_t address, uint16_t nb) {
    if (!ModbusRTUClient.requestFrom(slaveAddress, type, address, nb)) {
        Serial.print(F("RTU Error: Fallo request en esclavo "));
        Serial.println(slaveAddress);
        return false;
    }
    return true;
}

uint16_t ModbusRTUManager::read() {
    return (uint16_t)ModbusRTUClient.read();
}

bool ModbusRTUManager::readHoldingRegisters(uint16_t address, uint16_t quantity) {
    return requestFrom(_slaveID, HOLDING_REGISTERS, address, quantity);
}

bool ModbusRTUManager::readCoils(int address, int quantity) {
    return requestFrom(_slaveID, COILS, (uint16_t)address, (uint16_t)quantity);
}

bool ModbusRTUManager::writeHoldingRegister(uint16_t address, uint16_t value) {
    return ModbusRTUClient.holdingRegisterWrite(_slaveID, address, value);
}

bool ModbusRTUManager::writeCoil(uint16_t address, bool value) {
    return ModbusRTUClient.coilWrite(_slaveID, address, value);
}