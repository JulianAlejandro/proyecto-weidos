
#ifndef MODBUS_TCP_MANAGER_H
#define MODBUS_TCP_MANAGER_H

#include <Ethernet.h>
#include <ArduinoModbus.h>
#include "../core/ModbusTransport.h" // 1. Incluimos la interfaz

// 2. Heredamos de ModbusTransport
class ModbusTCPManager : public ModbusTransport {
  private:
    IPAddress _serverIP;
    uint16_t _port;
    uint8_t _slaveID;
    EthernetClient _ethClient;
    ModbusTCPClient _modbusClient;

    bool ensureConnection();

  public:
    ModbusTCPManager(IPAddress server, uint8_t slaveID = 1, uint16_t port = 502) 
      : _modbusClient(_ethClient), _serverIP(server), _port(port), _slaveID(slaveID) {}

    // --- IMPLEMENTACIÓN DE LA INTERFAZ (MÉTODOS VIRTUALES) ---

    // Cambiamos void por bool para cumplir con la interfaz
    bool begin() override; 
    
    // Sobrecarga para mantener tu lógica de inicialización de red
    void begin(byte mac[], IPAddress localIP);

    bool connected() override; 
    //bool connected() override { return isConnected(); }

    // Implementación del request genérico
    bool requestFrom(int slaveAddress, int type, uint16_t address, uint16_t nb) override;

    // Implementación de lectura
    //long read() override { return _modbusClient.read(); }
    
    uint16_t read() override;

    // Métodos de escritura (ajustados los nombres a la interfaz)
    bool writeHoldingRegister(uint16_t address, uint16_t value) override;

    bool writeCoil(uint16_t address, bool value) override;

    // Implementación del configurador genérico
    /*
    void setConfig(void* configData) override {
        // Ejemplo: si pasas un puntero a IPAddress
        if (configData != nullptr) {
            _serverIP = *(IPAddress*)configData;
        }
    }
*/
    // --- TUS MÉTODOS ORIGINALES (OPCIONALES) ---
    // Puedes mantenerlos por compatibilidad o eliminarlos si usas requestFrom
    bool readHoldingRegisters(uint16_t address, uint16_t quantity) override;
    bool readCoils(int address, int quantity) override; 
    
    //void setIpServer(IPAddress server); // todo pensar en su poner o no
};

#endif

/*
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

    void setIpServer(IPAddress server); 

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

*/