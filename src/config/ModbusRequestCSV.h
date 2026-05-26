#ifndef MODBUS_REQUEST_CSV_H
#define MODBUS_REQUEST_CSV_H

#include "global_types.h"
#include "SDManager.h"


#define ESP_ERR_CONFIG_BASE         0x50000
#define ESP_ERR_CONFIG_NOT_INIT     (ESP_ERR_CONFIG_BASE + 1)
#define ESP_ERR_CONFIG_PARSE_FAIL   (ESP_ERR_CONFIG_BASE + 2)
#define ESP_ERR_CONFIG_INVALID_DATA (ESP_ERR_CONFIG_BASE + 3)


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
    static const char* TAG;

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
    esp_err_t begin();

    /**
     * @brief Parses the first lines of the CSV to load Device Name and IP Address.
     * @return true if parameters were successfully loaded.
     */
    esp_err_t loadFromSDParameters();

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
    esp_err_t loadFromSDMbrequest(Struct_MBRequest* out_request); 
};

#endif