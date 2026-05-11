#ifndef MODBUS_RTU_MANAGER_H
#define MODBUS_RTU_MANAGER_H

#include <ArduinoRS485.h>
#include <ArduinoModbus.h>
#include "../core/ModbusTransport.h"

/**
 * @class ModbusRTUManager
 * @brief Handles Modbus RTU (Serial) communications by implementing the ModbusTransport interface.
 * * This class manages RS485 flow control pins, serial configuration, and Modbus RTU 
 * standard transactions. Designed specifically for ESP32/Weidos hardware architectures.
 */
class ModbusRTUManager : public ModbusTransport {
  private:
    uint32_t _baudrate;      ///< Serial communication speed (e.g., 9600, 19200)
    uint32_t _config;        ///< Serial frame configuration (e.g., SERIAL_8E1)
    uint8_t  _slaveID;       ///< Default Target Slave ID
    
    // Hardware-specific pins for RS485 Transceiver control
    int _txPin, _dePin, _rePin;

  public:
    /**
     * @brief Constructor for Modbus RTU Manager.
     * @param baudrate Communication speed.
     * @param slaveID Default Unit ID to target.
     * @param config Serial parity/stop bits configuration.
     */
    ModbusRTUManager(uint32_t baudrate = 19200, uint8_t slaveID = 1, uint32_t config = SERIAL_8E1);

    // --- INTERFACE IMPLEMENTATION (VIRTUAL METHODS) ---

    /**
     * @brief Initializes the RS485 hardware and the Modbus RTU Client.
     * @return true if the client started successfully.
     */
    bool begin() override;

    /**
     * @brief Checks the "connection" status.
     * @note In RTU, this typically returns true if the bus was successfully initialized.
     * @return true.
     */
    bool connected() override;
    
    /**
     * @brief Sends a Modbus request over the serial bus.
     * @param slaveAddress Target Unit ID.
     * @param type Register type (COILS, HOLDING_REGISTERS, etc.).
     * @param address Starting memory address.
     * @param nb Number of elements to read.
     * @return true if the request was sent without bus errors.
     */
    bool requestFrom(int slaveAddress, int type, uint16_t address, uint16_t nb) override;

    /**
     * @brief Reads the next available value from the serial buffer.
     * @return 16-bit register value or coil state.
     */
    uint16_t read() override;
    
    /**
     * @brief Specialized read for Holding Registers.
     * @param address Starting address.
     * @param quantity Number of registers.
     * @return true if request was successful.
     */
    bool readHoldingRegisters(uint16_t address, uint16_t quantity) override;

    /**
     * @brief Specialized read for Coils.
     * @param address Starting address.
     * @param quantity Number of coils.
     * @return true if request was successful.
     */
    bool readCoils(int address, int quantity) override;

    /**
     * @brief Writes a single 16-bit value to a Holding Register.
     * @param address Target address.
     * @param value Value to store.
     * @return true if write was successful.
     */
    bool writeHoldingRegister(uint16_t address, uint16_t value) override;

    /**
     * @brief Writes a single boolean value to a Coil.
     * @param address Target coil address.
     * @param value State to write.
     * @return true if write was successful.
     */
    bool writeCoil(uint16_t address, bool value) override;

    // --- RTU SPECIFIC METHODS ---

    /**
     * @brief Configures custom pins for the RS485 transceiver.
     * @param tx Transmit pin.
     * @param de Driver Enable pin.
     * @param re Receiver Enable pin.
     */
    void setPins(int tx, int de, int re);
};

#endif