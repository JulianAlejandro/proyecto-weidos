#include "ModbusTCPManager.h"

bool ModbusTCPManager::ensureConnection() {
  if (!_modbusClient.connected()) {
    Serial.println("Modbus: Intentando conectar al servidor...");
    if (!_modbusClient.begin(_serverIP, _port)) {
      Serial.println("Modbus: Fallo en la conexión.");
      return false;
    }
    Serial.println("Modbus: Conectado exitosamente.");
  }
  return true;
}


void ModbusTCPManager::begin(byte mac[], IPAddress localIP) {
  Ethernet.init(ETHERNET_CS); 
  Ethernet.begin(mac, localIP);
  delay(1000); // Dar tiempo al chip Ethernet
}


bool ModbusTCPManager::readCoils(int address, int quantity) {
  if (!ensureConnection()) return false;
  return _modbusClient.requestFrom(_slaveID, COILS, address, quantity);
}


bool ModbusTCPManager::readHoldingRegisters(int address, int quantity) {
  if (!ensureConnection()) return false;
  return _modbusClient.requestFrom(_slaveID, HOLDING_REGISTERS, address, quantity);
}

// Recupera el dato después de un request exitoso
long ModbusTCPManager::getAvailableData() {
  return _modbusClient.read();
}

// --- MÉTODOS DE ESCRITURA ---

bool ModbusTCPManager::writeHoldingRegister(int address, uint16_t value) {
  if (!ensureConnection()) return false;
  return _modbusClient.holdingRegisterWrite(_slaveID, address, value);
}

bool ModbusTCPManager::writeCoil(int address, bool value) {
  if (!ensureConnection()) return false;
  return _modbusClient.coilWrite(_slaveID, address, value);
}

// Check de estado
bool ModbusTCPManager::isConnected() {
  return _modbusClient.connected();
}
