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
    bool ensureConnection();

  public:
    // Constructor
    ModbusTCPManager(IPAddress server, uint8_t slaveID = 1, uint16_t port = 502) 
      : _modbusClient(_ethClient) {
      _serverIP = server;
      _port = port;
      _slaveID = slaveID;
    }

    // Inicialización (se llama en el setup)
    void begin(byte mac[], IPAddress localIP);

    // --- MÉTODOS DE LECTURA ---

    bool readCoils(int address, int quantity);

    bool readHoldingRegisters(int address, int quantity);

    // Recupera el dato después de un request exitoso
    long getAvailableData();

    // --- MÉTODOS DE ESCRITURA ---

    bool writeHoldingRegister(int address, uint16_t value);

    bool writeCoil(int address, bool value);

    // Check de estado
    bool isConnected();
};

#endif