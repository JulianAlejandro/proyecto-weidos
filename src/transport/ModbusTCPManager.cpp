

#include "ModbusTCPManager.h"

/**
 * @brief Gestión de la reconexión automática. 
 * Se asegura de que el socket esté limpio antes de reintentar.
 */
bool ModbusTCPManager::ensureConnection() {
    if (!_modbusClient.connected()) {
        // Cerramos el cliente Ethernet subyacente para liberar sockets en el chip Wiznet
        _modbusClient.stop(); 
        
        Serial.println(F("ModbusTCP: Intentando conectar al servidor..."));
        
        if (!_modbusClient.begin(_serverIP, _port)) {
            Serial.println(F("ModbusTCP: Fallo en la conexión TCP."));
            return false;
        }
        
        delay(50); // Estabilización del handshake
        Serial.println(F("ModbusTCP: Conectado exitosamente."));
    }
    return true;
}

/**
 * @brief Inicialización de hardware específica para Ethernet.
 */
void ModbusTCPManager::begin(byte mac[], IPAddress localIP) {
    Ethernet.init(ETHERNET_CS); 
    Ethernet.begin(mac, localIP);
    delay(1000); 
}

/**
 * @brief Implementación del begin() de la interfaz ModbusTransport.
 */
bool ModbusTCPManager::begin() {
    return ensureConnection();
}

/**
 * @brief Implementación de lectura de registros Holding (Interfaz).
 */
bool ModbusTCPManager::readHoldingRegisters(uint16_t address, uint16_t quantity) {
    if (!ensureConnection()) return false;

    if (!_modbusClient.requestFrom(_slaveID, HOLDING_REGISTERS, address, quantity)) {
        Serial.println(F("ModbusTCP: Error en Request. Forzando stop..."));
        _modbusClient.stop(); 
        return false;
    }
    return true;
}

/**
 * @brief Lectura de Coils (Interfaz).
 */
bool ModbusTCPManager::readCoils(int address, int quantity) {
    if (!ensureConnection()) return false;
    return _modbusClient.requestFrom(_slaveID, COILS, address, quantity);
}

/**
 * @brief Recupera el dato del buffer (Interfaz).
 */
 /*
long ModbusTCPManager::read() {
    return _modbusClient.read();
}
*/

/**
 * @brief Implementación de escritura de registros Holding (Interfaz).
 */
bool ModbusTCPManager::writeHoldingRegister(uint16_t address, uint16_t value) {
  if (!ensureConnection()) return false;
  return _modbusClient.holdingRegisterWrite(_slaveID, address, value);
}


uint16_t ModbusTCPManager::read() {
  return _modbusClient.read();
}

/**
 * @brief Implementación de escritura de Coils (Interfaz).
 */
bool ModbusTCPManager::writeCoil(uint16_t address, bool value) {
    if (!ensureConnection()) return false;
    return _modbusClient.coilWrite(_slaveID, address, value);
}

/**
 * @brief Mantenemos tu método original por si el EnergyMeter lo usa directamente.
 */
void ModbusTCPManager::setIpServer(IPAddress server){
    _serverIP = server;
}

bool ModbusTCPManager::connected() {
  return _modbusClient.connected();
}

// Implementación del request genérico
bool ModbusTCPManager::requestFrom(int slaveAddress, int type, uint16_t address, uint16_t nb){
    if (!ensureConnection()) return false;
    return _modbusClient.requestFrom(slaveAddress, type, address, nb);
}

/*
#include "ModbusTCPManager.h"


bool ModbusTCPManager::ensureConnection() {
    // Si no está conectado según la librería
    if (!_modbusClient.connected()) {
        // Por seguridad, forzamos el cierre del cliente Ethernet subyacente
        // Esto libera el socket en el chip Wiznet
        _modbusClient.stop(); 
        
        Serial.println(F("Modbus: Intentando conectar al servidor..."));
        
        if (!_modbusClient.begin(_serverIP, _port)) {
            Serial.println(F("Modbus: Fallo en la conexión TCP."));
            return false;
        }
        
        // Opcional: Un pequeño delay tras conectar ayuda a estabilizar el handshake
        delay(50); 
        Serial.println(F("Modbus: Conectado exitosamente."));
    }
    return true;
}

//bool ModbusTCPManager::ensureConnection() {
//  if (!_modbusClient.connected()) {
//    Serial.println("Modbus: Intentando conectar al servidor...");
//    if (!_modbusClient.begin(_serverIP, _port)) {
//      Serial.println("Modbus: Fallo en la conexión.");
//      return false;
//    }
//    Serial.println("Modbus: Conectado exitosamente.");
//  }
//  return true;
//}


void ModbusTCPManager::begin(byte mac[], IPAddress localIP) {
  Ethernet.init(ETHERNET_CS); 
  Ethernet.begin(mac, localIP);
  delay(1000); // Dar tiempo al chip Ethernet
}

void ModbusTCPManager::setIpServer(IPAddress server){
  _serverIP = server;
}


bool ModbusTCPManager::readCoils(int address, int quantity) {
  if (!ensureConnection()) return false;
  return _modbusClient.requestFrom(_slaveID, COILS, address, quantity);
}

bool ModbusTCPManager::readHoldingRegisters(int address, int quantity) {
    if (!ensureConnection()) return false;

    if (!_modbusClient.requestFrom(_slaveID, HOLDING_REGISTERS, address, quantity)) {
        Serial.println(F("Modbus: Error en Request. Forzando desconexión..."));
        _modbusClient.stop(); 
        return false;
    }
    return true;
}

//bool ModbusTCPManager::readHoldingRegisters(int address, int quantity) {
//  if (!ensureConnection()) return false;
//  return _modbusClient.requestFrom(_slaveID, HOLDING_REGISTERS, address, quantity);
//}

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
*/