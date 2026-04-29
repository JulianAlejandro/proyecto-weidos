#ifndef MODBUS_TCP_MANAGER_H
#define MODBUS_TCP_MANAGER_H

#include <Ethernet.h>
#include <ArduinoModbus.h>

class ModbusTCPManager {
  private:
    IPAddress _serverIP;
    uint16_t _port;
    uint8_t _slaveID;
    EthernetClient _ethClient;
    ModbusTCPClient _modbusClient;

    // Método privado para asegurar que estamos conectados antes de operar
    bool ensureConnection() {
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

  public:
    // Constructor
    ModbusTCPManager(IPAddress server, uint8_t slaveID = 1, uint16_t port = 502) 
      : _modbusClient(_ethClient) {
      _serverIP = server;
      _port = port;
      _slaveID = slaveID;
    }

    // Inicialización (se llama en el setup)
    void begin(byte mac[], IPAddress localIP) {
      Ethernet.init(ETHERNET_CS); 
      Ethernet.begin(mac, localIP);
      delay(1000); // Dar tiempo al chip Ethernet
    }

    // --- MÉTODOS DE LECTURA ---

    bool readCoils(int address, int quantity) {
      if (!ensureConnection()) return false;
      return _modbusClient.requestFrom(_slaveID, COILS, address, quantity);
    }

    bool readHoldingRegisters(int address, int quantity) {
      if (!ensureConnection()) return false;
      return _modbusClient.requestFrom(_slaveID, HOLDING_REGISTERS, address, quantity);
    }

    // Recupera el dato después de un request exitoso
    long getAvailableData() {
      return _modbusClient.read();
    }

    // --- MÉTODOS DE ESCRITURA ---

    bool writeHoldingRegister(int address, uint16_t value) {
      if (!ensureConnection()) return false;
      return _modbusClient.holdingRegisterWrite(_slaveID, address, value);
    }

    bool writeCoil(int address, bool value) {
      if (!ensureConnection()) return false;
      return _modbusClient.coilWrite(_slaveID, address, value);
    }

    // Check de estado
    bool isConnected() {
      return _modbusClient.connected();
    }
};

#endif