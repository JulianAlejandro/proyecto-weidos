#ifndef MODBUS_REQUEST_CSV_H
#define MODBUS_REQUEST_CSV_H

#include "services/SDManager.h"

/**
 * @struct Struct_MBRequest
 * @brief Container for Modbus transaction parameters extracted from CSV.
 */
struct Struct_MBRequest {
    uint16_t channel;           ///< Slave ID / Unit ID
    uint16_t start_addres;      ///< Register starting address
    uint16_t length;            ///< Number of registers to read
    uint16_t func_code;         ///< Modbus Function Code (e.g., 3 or 4)
    uint16_t req_interval_ms;   ///< Polling interval in milliseconds
};

#define MAX_TITLES_SIZE 32
#define MODBUS_REQ_FILE "MBReq.csv"
#define FIRST_BLOCK 10          ///< Number of lines to parse from the beginning of the file

/**
 * @class ModbusRequestCSV
 * @brief Helper class to extract device parameters and Modbus requests from a CSV file on SD.
 * @note This class serves as a temporary manager for SD-based configuration.
 */
class ModbusRequestCSV {

private:
    SDManager* _sd = nullptr;
    bool _initialized = false;

    char _device_name[MAX_TITLES_SIZE];
    char _ip_address[MAX_TITLES_SIZE];

public:
    /**
     * @brief Constructor requiring a pointer to an initialized SDManager.
     */
    ModbusRequestCSV(SDManager* sdManager); 

    /**
     * @brief Verifies if the SD manager is ready for operations.
     * @return true if initialized successfully.
     */
    bool begin();

    /**
     * @brief Parses the first lines of the CSV to load Device Name and IP Address.
     * @return true if parameters were successfully loaded.
     */
    bool loadFromSDParameters();

    /**
     * @brief Gets the loaded Device Name.
     */
    char* getDeviceName() { return _device_name; }

    /**
     * @brief Gets the loaded IP Address as a string.
     */
    char* getIpAdress() { return _ip_address; } 

    /**
     * @brief Parses the CSV to extract a specific Modbus request structure.
     * @return A populated Struct_MBRequest (all zeros if parsing fails).
     */
    Struct_MBRequest loadFromSDMbrequest(); 
};

#endif