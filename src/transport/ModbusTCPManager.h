#ifndef MODBUS_TCP_MANAGER_H
#define MODBUS_TCP_MANAGER_H

#include <Ethernet.h>
#include <ArduinoModbus.h>
#include "../core/ModbusTransport.h" 

#define ESP_ERR_MODBUS_TCP_IP_NOT_FOUND (ESP_ERR_MODBUS_BASE + 101)
#define ESP_ERR_MODBUS_TCP_SOCKET       (ESP_ERR_MODBUS_BASE + 102)

/**
 * @class ModbusTCPManager
 * @brief Handles Modbus TCP communications by implementing the ModbusTransport interface.
 * * This class manages Ethernet client connections, automatic reconnections, 
 * and standard Modbus TCP transactions.
 */
class ModbusTCPManager : public ModbusTransport {
  private:
    IPAddress _serverIP;         ///< IP Address of the Modbus TCP Server/Slave
    uint16_t _port;              ///< TCP Port (Default is 502)
    uint8_t _slaveID;            ///< Unit Identifier (Slave Address)
    EthernetClient _ethClient;   ///< Underlying Ethernet client
    ModbusTCPClient _modbusClient; ///< High-level Modbus TCP client

    /**
     * @brief Internal helper to verify and maintain the TCP connection.
     * @return true if connection is active or successfully restored.
     */
    esp_err_t ensureConnection();

  public:
    /**
     * @brief Constructor for the TCP Manager.
     * @param server IPAddress of the target server.
     * @param slaveID Unit ID (Default is 1).
     * @param port TCP Port (Default is 502).
     */
    ModbusTCPManager(IPAddress server, uint8_t slaveID = 1, uint16_t port = 502) 
      : _modbusClient(_ethClient), _serverIP(server), _port(port), _slaveID(slaveID) {}

    // --- INTERFACE IMPLEMENTATION (VIRTUAL METHODS) ---

    /**
     * @brief Standard initialization from interface.
     * @return true if communication with the server is established.
     */
    esp_err_t begin() override; 
    
    /**
     * @brief Hardware-specific initialization for the Ethernet shield.
     * @param mac Hardware MAC address array.
     * @param localIP Static IP assigned to the device.
     */
    esp_err_t begin(byte mac[], IPAddress localIP);

    /**
     * @brief Checks the current connection status.
     * @return true if connected to the Modbus server.
     */
    bool connected() override; 

    /**
     * @brief Generic request for Modbus data.
     * @param slaveAddress Unit ID (overwrites default if needed).
     * @param type Register type (COILS, HOLDING_REGISTERS, etc.).
     * @param address Starting memory address.
     * @param nb Number of registers/coils to request.
     * @return true if the request was successfully sent and acknowledged.
     */
    esp_err_t requestFrom(int slaveAddress, int type, uint16_t address, uint16_t nb) override;

    /**
     * @brief Reads the next available value from the response buffer.
     * @return 16-bit register value or coil state.
     */
    uint16_t read() override;

    /**
     * @brief Writes a single 16-bit value to a Holding Register.
     * @param address Target register address.
     * @param value Value to write.
     * @return true if write was successful.
     */
    esp_err_t writeHoldingRegister(uint16_t address, uint16_t value) override;

    /**
     * @brief Writes a single boolean value to a Coil.
     * @param address Target coil address.
     * @param value Boolean state to write.
     * @return true if write was successful.
     */
    esp_err_t writeCoil(uint16_t address, bool value) override;

    /**
     * @brief Interface specific implementation for reading Holding Registers.
     * @param address Starting address.
     * @param quantity Number of registers.
     * @return true if successful.
     */
    esp_err_t readHoldingRegisters(uint16_t address, uint16_t quantity) override;

    /**
     * @brief Interface specific implementation for reading Coils.
     * @param address Starting address.
     * @param quantity Number of coils.
     * @return true if successful.
     */
    esp_err_t readCoils(int address, int quantity) override; 
};

#endif