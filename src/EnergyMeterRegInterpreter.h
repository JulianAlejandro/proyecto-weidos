#ifndef ENERGY_METER_INTERPRETER_H
#define ENERGY_METER_INTERPRETER_H

#include "services/SDManager.h"
#include <CSV_Parser.h>
#include "ModbusRequestCSV.h"

// codigos
#define ESP_ERR_INTERPRETER_BASE      0x60000
#define ESP_ERR_INTERPRETER_NOT_INIT  (ESP_ERR_INTERPRETER_BASE + 1)
#define ESP_ERR_INTERPRETER_MAP_MISS  (ESP_ERR_INTERPRETER_BASE + 2) // No hay registros en ese rango
#define ESP_ERR_INTERPRETER_BAD_CONF  (ESP_ERR_INTERPRETER_BASE + 3) // Configuración de CSV inválida

// Constants for Register Mapping
#define NUM_COL_REG_EM750 5
#define MAX_MODBUS_REGS 125    ///< Limit for the number of processable Modbus registers
#define MAX_TITLES_SIZE 32     ///< Buffer size for column names/titles
#define MAX_TEXT_SIZE 32       ///< Buffer size for general strings

#define MAX_DATA_SIZE 4        ///< Maximum 16-bit registers per data point (e.g., 64-bit long)
#define LINE_MAP_START 4       ///< CSV line where the register table begins

#define MAP_FILE "/EM750map.csv"

// Register size definitions (Number of 16-bit Modbus registers)
#define SIZE_FLOAT   2 
#define SIZE_INT     2
#define SIZE_UINT    2
#define SIZE_LONG64  4
#define SIZE_SHORT   1
#define SIZE_USHORT  1
#define SIZE_BYTE    1
#define SIZE_DFLOAT  1

/**
 * @enum coded_format
 * @brief Supported data formats within the register map.
 */
enum coded_format { FORMAT_UNKNOWN, FLOAT, SHORT, INT, STRING, USHORT, UINT, BYTE, LONG64, DFLOAT };

/**
 * @struct RegisterEntry
 * @brief Representation of a single row in the CSV register map.
 */
struct RegisterEntry {
    uint16_t address;
    coded_format format;
    char name[MAX_TITLES_SIZE];
    bool logEnabled;
};

/**
 * @struct EM_request
 * @brief Structure to define a Modbus range request.
 */
struct EM_request {
    uint16_t start_addr; 
    uint16_t size; 
};

/**
 * @struct rawDataReg
 * @brief Container for raw 16-bit Modbus data and its format metadata.
 */
struct rawDataReg {
    uint16_t data[MAX_DATA_SIZE];
    coded_format format; 
};

/**
 * @struct bufRawDataReg
 * @brief Helper to return a buffer of processed raw data.
 */
struct bufRawDataReg {
    rawDataReg* buffer;
    uint16_t size; 
};

/**
 * @struct nameColValues
 * @brief Buffer of strings containing the names of columns intended for logging.
 */
struct nameColValues {
    const char* buffer[MAX_MODBUS_REGS]; 
    uint16_t size;
};

/**
 * @struct netDataString
 * @brief Buffer of strings representing formatted data ready for the log file.
 */
struct netDataString {
    const char* buffer[MAX_MODBUS_REGS];
    uint16_t size; 
};

/**
 * @struct Parameters
 * @brief High-level datalogger configuration parameters extracted from CSV.
 */
struct Parameters {
    const char* log_interval;
    const char* new_file;
    const char* max_files; 
};

// Forward declarations
class EnergyMeter750;
class Datalogger; 
class RTC_DS3231; 

/**
 * @class EnergyMeterRegInterpreter
 * @brief Manages the parsing of CSV register maps and the conversion of raw Modbus data.
 */
class EnergyMeterRegInterpreter {

private:
    SDManager* _sd = nullptr; 
    EM_request _current_request; 
    bool _initialized = false; 

    int ultimaUnidadTiempo;
   
    unsigned long anteriorMillisModbus = 0; 
    //unsigned long anteriorMillisArchivo = 0; 
    nameColValues _misTitulos;

    int _int_log_interval;
    uint32_t _new_file_interval_s; 
    int _int_max_files; 

    bool _advancedIsInitialized = false; 
    
    RegisterEntry _registryBuffer[MAX_MODBUS_REGS]; 
    uint16_t _registrySize; 

    rawDataReg _RawDataBuffer[MAX_MODBUS_REGS]; 
    char _netDataStringBuffer[MAX_MODBUS_REGS][MAX_TEXT_SIZE];
  
    char _log_interval[MAX_TEXT_SIZE]; 
    char _new_file[MAX_TEXT_SIZE]; 
    char _max_files[MAX_TEXT_SIZE];

    bool _creadaUnaSesion = false;
   
    int getFormatSize(coded_format f); 
    static void getNetDataString(char* dest, rawDataReg rawRegister);
    esp_err_t lectura_modbus(Datalogger* datalogger, RTC_DS3231* rtc, EnergyMeter750* em, EM_request req);
    void processParserData(CSV_Parser& cp, uint16_t start, uint16_t size);

public:
    EnergyMeterRegInterpreter(SDManager* sdManager);
    esp_err_t begin();

    /**
     * @brief Reads the SD map and filters registers based on a requested range.
     * @return An EM_request object with the calculated total size.
     */
    esp_err_t startNewRequest(const uint16_t start_addr, const uint16_t size, EM_request *out_request);

    /**
     * @brief Maps raw 16-bit arrays into the formatted rawDataReg buffer.
     */
    bufRawDataReg getBufferDataRaw(const uint16_t* data_readed, const uint16_t size);

    /**
     * @brief Gets titles for columns where 'Log' is enabled.
     */
    nameColValues getLastNameValues();

    /**
     * @brief Returns a buffer of formatted strings (e.g., "12.34") for the logger.
     */
    netDataString getBufNetDataString();

    coded_format stringToFormat(const char* str);
    bool isReady() { return _sd != nullptr && _initialized; }

    static float getFloatConversion(const uint16_t* data);

    /**
     * @brief Loads global configuration (interval, max files) from the first lines of the CSV.
     */
    void loadParametersMapRegister(); 

    Parameters getParameters();

    /**
     * @brief Automates the entire datalogging setup and execution.
     */
    esp_err_t prepareAdvanceDatalogger(Struct_MBRequest MB_req, Datalogger* datalogger, RTC_DS3231* rtc);
    esp_err_t advancedDataloggerExec(Datalogger* datalogger, EnergyMeter750* em, RTC_DS3231* rtc);
};

#endif