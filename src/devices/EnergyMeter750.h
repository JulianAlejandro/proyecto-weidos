#ifndef ENERGY_METER_750_H
#define ENERGY_METER_750_H

#include <ArduinoModbus.h>
#include "../EnergyMeterRegInterpreter.h"
#include "../core/ModbusTransport.h"

// Constants for EM750 protocol and memory map
#define NUM_COL_REG_EM750 5
#define MAX_MODBUS_REGS_REQUEST 125
#define MAX_EM_ADDR 22000

/**
 * @struct rawDataBuffer
 * @brief Helper structure to return a pointer to the internal data and its size.
 */
struct rawDataBuffer {
    uint16_t* buffer;  ///< Pointer to the start of the data array
    uint16_t size;    ///< Number of valid registers currently stored
};

/**
 * @class EnergyMeter750
 * @brief High-level driver for the EM750 energy meter.
 * * Manages Modbus requests, input validation, and local buffering of raw 
 * register data obtained from the meter via a ModbusTransport interface.
 */
class EnergyMeter750 {
  private:
    ModbusTransport* _modbus;  ///< Pointer to the transport layer (TCP or RTU)
    bool _initialized = false; ///< Initialization state flag

    uint16_t _internalBuffer[MAX_MODBUS_REGS_REQUEST]; ///< Local storage for last Modbus read
    uint16_t _lastReadSize;                            ///< Actual size of data in the buffer

    /**
     * @brief Determines the register size required for a specific coded format.
     */
    int getFormatSize(coded_format f);
   
  public:
    /**
     * @brief Default constructor.
     */
    EnergyMeter750();

    /**
     * @brief Initializes the driver with a Modbus transport instance.
     * @param modbus Pointer to an implementation of ModbusTransport.
     * @return true if initialization was successful.
     */
    int begin(ModbusTransport* modbus);

    /**
     * @brief Checks if the driver has been properly initialized.
     */
    bool isReady() const { return _initialized; }

    /**
     * @brief Accesses the internal raw data buffer.
     * @return rawDataBuffer struct containing the pointer and current size.
     */
    rawDataBuffer readDataBuffer();

    /**
     * @brief Executes a multi-register Modbus request and stores results in the local buffer.
     * Includes range and size validation to prevent memory overflows.
     * @param req The request parameters (start address and size).
     * @return true if the Modbus transaction succeeded.
     */
    bool executeRequest(EM_request req);

    /**
     * @brief Reads a single register from a specific Modbus address.
     * @param addr The target register address.
     * @return The 16-bit value read, or 0 if uninitialized.
     */
    uint16_t readRegByAdress(uint16_t addr);
};

#endif
