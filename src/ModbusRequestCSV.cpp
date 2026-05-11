#include "ModbusRequestCSV.h"
#include <CSV_Parser.h>

ModbusRequestCSV::ModbusRequestCSV(SDManager* sdManager) {
    _sd = sdManager;
}

bool ModbusRequestCSV::begin() {
    // Check if SD card communication is already established
    if (!_sd->isReady()) { 
        return false;
    }
    _initialized = true;
    return true;
}

/**
 * @brief Loads metadata (Device Name, IP) from the CSV.
 * @note Uses 'has_header = false' so the first line is treated as data.
 */
bool ModbusRequestCSV::loadFromSDParameters() {
    if (!_initialized) return false; 
    
    // CSV structure: 5 columns expected ("sssss")
    CSV_Parser cp("sssss", false, ';');
    
    bool flag = _sd->withFile(MODBUS_REQ_FILE, [](Stream& file, void* arg) {
        CSV_Parser* parser = (CSV_Parser*)arg;
        for (int i = 0; i < FIRST_BLOCK; i++) {
            if (file.available()) {
                String line = file.readStringUntil('\n');
                line.trim(); // Remove \r and extra spaces
                if (line.length() > 0) {
                    line += "\n"; 
                    *parser << line.c_str();
                }
            } else {
                break;
            }
        }
    }, &cp);

    if (!flag) return false;

    int rows = cp.getRowsCount();

    // Map based on the CSV structure:
    // Row 0: "Modbus Client Requests;;;;"
    // Row 1: "Device Name;EM 750;;;"
    // Row 2: "IP address;192.168.0.202;;;"

    if (rows < 3) return false; 

    // Access Column 1 where values are stored
    char **values = (char**)cp[1];

    if (values != nullptr) {
        // Load Device Name from Row 1
        if (values[1] != nullptr) {
            strncpy(_device_name, values[1], MAX_TITLES_SIZE - 1);
            _device_name[MAX_TITLES_SIZE - 1] = '\0';
        }

        // Load IP Address from Row 2
        if (values[2] != nullptr) {
            strncpy(_ip_address, values[2], MAX_TITLES_SIZE - 1);
            _ip_address[MAX_TITLES_SIZE - 1] = '\0';
        }
        
        return true;
    }

    return false;
}

/**
 * @brief Extracts the Modbus request configuration from Row 8 (Index 7) of the CSV.
 */
Struct_MBRequest ModbusRequestCSV::loadFromSDMbrequest() {
    Struct_MBRequest request = {0, 0, 0, 0, 0}; 
    
    if (!_initialized) return request; 

    CSV_Parser cp("sssss", false, ';');

    bool flag = _sd->withFile(MODBUS_REQ_FILE, [](Stream& file, void* arg) {
        CSV_Parser* parser = (CSV_Parser*)arg;
        for (int i = 0; i < FIRST_BLOCK; i++) {
            if (file.available()) {
                String line = file.readStringUntil('\n');
                line.trim();
                if (line.length() > 0) {
                    line += "\n"; 
                    *parser << line.c_str();
                }
            } else {
                break;
            }
        }
    }, &cp);

    int rows = cp.getRowsCount();
    
    // CSV Row 8 corresponds to Index 7 in the parser
    if (flag && rows >= 8) {
        char **col0 = (char**)cp[0]; // Channel
        char **col1 = (char**)cp[1]; // Start Address
        char **col2 = (char**)cp[2]; // Length
        char **col3 = (char**)cp[3]; // Function Code
        char **col4 = (char**)cp[4]; // Interval

        // Ensure Row 7 actually contains data
        if (col0[7] && col1[7] && col2[7] && col3[7] && col4[7]) {
            request.channel         = (uint16_t)atoi(col0[7]);
            request.start_addres    = (uint16_t)atoi(col1[7]);
            request.length          = (uint16_t)atoi(col2[7]);
            request.func_code       = (uint16_t)atoi(col3[7]);
            request.req_interval_ms = (uint16_t)atoi(col4[7]);
        }
    }

    return request;
}
