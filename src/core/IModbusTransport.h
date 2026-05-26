#ifndef IMODBUS_TRANSPORT_H
#define IMODBUS_TRANSPORT_H

#include <Arduino.h>
#include <esp_err.h>

#define ESP_ERR_MODBUS_BASE          0x30000
#define ESP_ERR_MODBUS_NOT_READY     (ESP_ERR_MODBUS_BASE + 1) // El bus está ocupado o no iniciado
#define ESP_ERR_MODBUS_TIMEOUT       (ESP_ERR_MODBUS_BASE + 2) // El esclavo no respondió a tiempo
#define ESP_ERR_MODBUS_INVALID_RESP  (ESP_ERR_MODBUS_BASE + 3) // Respuesta corrupta o CRC erróneo
#define ESP_ERR_MODBUS_SERVER_REJECT (ESP_ERR_MODBUS_BASE + 4) // El esclavo devolvió una excepción Modbus

/**
 * @class ModbusTransport
 * @brief Abstract Interface for Modbus Communication.
 * * This class defines the standard API for any Modbus transport layer (TCP, RTU, or ASCII).
 * By using this interface, the EnergyMeter drivers remain decoupled from the 
 * specific hardware implementation (Ethernet or RS485).
 */
class IModbusTransport {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~IModbusTransport() {}

    // --- Control Methods ---
    
    /**
     * @brief Initializes the underlying hardware, client, or bus.
     * @return true if initialization was successful.
     */
    virtual esp_err_t begin() = 0;

    /**
     * @brief Checks if the communication channel is active.
     * @note In TCP, this verifies the socket state. In RTU, it confirms the bus is ready.
     * @return true if the transport is ready for transactions.
     */
    virtual bool connected() = 0;

    // --- Read Methods ---

    /**
     * @brief Sends a generic Modbus request to a slave device.
     * @param slaveAddress The Unit ID / Slave ID (typically 1 for TCP or specific ID for RTU).
     * @param type Register type (e.g., HOLDING_REGISTERS, COILS).
     * @param address The starting memory address for the request.
     * @param nb The number of registers or points to read.
     * @return true if the request was successfully accepted or acknowledged by the bus.
     */
    virtual esp_err_t requestFrom(int slaveAddress, int type, uint16_t address, uint16_t nb) = 0;

    /**
     * @brief Fetches the next available value from the internal response buffer.
     * @note This should be called after a successful requestFrom() or specialized read call.
     * @return The 16-bit register value.
     */
    virtual uint16_t read() = 0;

    /**
     * @brief Specialized request for Holding Registers (Function Code 03/04).
     * @param address Starting address.
     * @param quantity Number of registers to read.
     * @return true if the transaction succeeded.
     */
    virtual esp_err_t readHoldingRegisters(uint16_t address, uint16_t quantity) = 0; 
    
    /**
     * @brief Specialized request for Coils (Function Code 01).
     * @param address Starting address.
     * @param quantity Number of coils to read.
     * @return true if the transaction succeeded.
     */
    virtual esp_err_t readCoils(int address, int quantity) = 0;

    // --- Write Methods ---

    /**
     * @brief Writes a single 16-bit value to a Holding Register.
     * @param address Target memory address.
     * @param value The value to write.
     * @return true if the write operation was confirmed.
     */
    virtual esp_err_t writeHoldingRegister(uint16_t address, uint16_t value) = 0;

    /**
     * @brief Writes a single boolean state to a Coil.
     * @param address Target coil address.
     * @param value State to write (true/false).
     * @return true if the write operation was confirmed.
     */
    virtual esp_err_t writeCoil(uint16_t address, bool value) = 0;

    /* * FUTURE ENHANCEMENTS (Currently Commented):
     * * virtual void setConfig(void* configData) = 0; 
     * This allows runtime configuration updates (IP/Baudrate) via JSON or structs.
     */

     
};

#endif
