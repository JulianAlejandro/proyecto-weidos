#include "EnergyMeterRegInterpreter.h"
#include <Arduino.h>
#include "devices/EnergyMeter750.h"
#include "services/Datalogger.h"
#include <RTClib.h>

/**
 * @struct StreamContext
 * @brief Helper to pass class context and parameters to static callback functions.
 */
struct StreamContext {
    EnergyMeterRegInterpreter* instance;
    uint16_t start_addr;
    uint16_t size;
};

// Internal Helper Prototypes
void lectura_modbus(Datalogger* datalogger, RTC_DS3231* rtc, EnergyMeter750* em, EM_request req);
bool crear_nueva_sesion_log(Datalogger* datalogger, RTC_DS3231* rtc, nameColValues* misTitulos); 
uint32_t getLogIntervalFromString(const char* log_interval_str); 

EnergyMeterRegInterpreter::EnergyMeterRegInterpreter(SDManager* sdManager) 
  : _sd(sdManager) 
{
    _registrySize = 0;
}

/**
 * @brief Checks if the SD manager is ready and initializes the interpreter.
 */
int EnergyMeterRegInterpreter::begin(){
    if (!_sd->isReady()) { 
        //Serial.println(F("Interpreter: SDManager not ready yet."));
        return false;
    }
    _initialized = true; 
    return true;
}

/**
 * @brief Processes the parsed CSV data to filter registers within the requested range.
 */
void EnergyMeterRegInterpreter::processParserData(CSV_Parser& cp, uint16_t start, uint16_t size) {
    // 1. Retrieve pointers and verify column headers
    int32_t *addrs = (int32_t*)cp["Address"];
    char **formats = (char**)cp["Format"];
    char **names = (char**)cp["Name"];
    char **logs = (char**)cp["Log"];

    if (addrs == nullptr || formats == nullptr || names == nullptr || logs == nullptr) {
        //Serial.println(F("Error: Required CSV columns not found. Check headers."));
        return;
    }

    int totalRows = cp.getRowsCount();
    _registrySize = 0; 

    for (int i = 0; i < totalRows; i++) {
        uint16_t addr = (uint16_t)addrs[i];

        // Filter by the requested address range
        if (addr >= start && addr < (start + size)) {
            
            if (_registrySize >= MAX_MODBUS_REGS) {
                //Serial.println(F("Warning: Reached MAX_MODBUS_REGS limit."));
                break;
            }

            coded_format fmtEnum = stringToFormat(formats[i]);
            
            if (fmtEnum != FORMAT_UNKNOWN) {
                int idx = _registrySize;
                
                _registryBuffer[idx].format = fmtEnum;
                _registryBuffer[idx].address = addr;

                // Determine if logging is enabled for this register
                _registryBuffer[idx].logEnabled = (strcasecmp(logs[i], "Yes") == 0);
                
                // Store register name
                if (names[i] != nullptr) {
                    strncpy(_registryBuffer[idx].name, names[i], MAX_TITLES_SIZE - 1);
                    _registryBuffer[idx].name[MAX_TITLES_SIZE - 1] = '\0';
                } else {
                    strcpy(_registryBuffer[idx].name, "Unknown");
                }

                // Update the cumulative Modbus request size (number of 16-bit words)
                _current_request.size += getFormatSize(fmtEnum);
                _registrySize++;
            }
        }
    }
    
    
    //Serial.print(F("Process finished. Rows in range: "));
    //Serial.println(_registrySize);
}

/**
 * @brief Initializes a new request by parsing the register map from SD.
 */
EM_request EnergyMeterRegInterpreter::startNewRequest(const uint16_t start_addr, const uint16_t size) {
    _current_request.start_addr = 0;
    _current_request.size = 0;
    _registrySize = 0;

    if(!_initialized){
        //Serial.println(F("Interpreter not initialized."));
        return _current_request;
    }

    StreamContext ctx = {this, start_addr, size};
        
    _sd->withFile(MAP_FILE, [](Stream& file, void* arg) {
        StreamContext* sc = (StreamContext*)arg;
        
        // Skip metadata lines and jump to the register map table
        for (int i = 0; i < LINE_MAP_START; i++) {
            if (file.available()) file.readStringUntil('\n'); 
        }

        // Initialize CSV Parser: L=Long/Int32, s=String
        CSV_Parser cp("Lssss", true, ';');

        // Feed file stream into the parser
        while (file.available()) {
            String line = file.readStringUntil('\n');
            if (line.length() > 0) {
                line += "\n";
                cp << line.c_str();
            }
        }

        // Process parsed results
        sc->instance->processParserData(cp, sc->start_addr, sc->size);

    }, &ctx);
    _current_request.start_addr = _registryBuffer[0].address; 
    return _current_request;
}

/**
 * @brief Distributes a raw 16-bit response array into formatted data structures.
 */
bufRawDataReg EnergyMeterRegInterpreter::getBufferDataRaw(const uint16_t* data_readed, const uint16_t size) {
    bufRawDataReg res;
    res.buffer = _RawDataBuffer; 
    res.size = 0;

    if(!_initialized) return res;

    uint16_t offsetOriginal = 0; 

    for (int i = 0; i < _registrySize; i++) {
        coded_format fmt = _registryBuffer[i].format;
        int regs_del_dato = getFormatSize(fmt);

        if (offsetOriginal + regs_del_dato > size) break;

        _RawDataBuffer[i].format = fmt;
        
        // Populate the 16-bit internal buffer for each data point
        for (int j = 0; j < regs_del_dato; j++) {
            _RawDataBuffer[i].data[j] = data_readed[offsetOriginal + j];
        }

        // Clear remaining space in buffer
        for (int j = regs_del_dato; j < MAX_DATA_SIZE; j++) {
            _RawDataBuffer[i].data[j] = 0;
        }

        offsetOriginal += regs_del_dato;
        res.size++;
    }

    return res; 
}

/**
 * @brief Utility to convert two 16-bit registers into a IEEE 754 float.
 */
float EnergyMeterRegInterpreter::getFloatConversion(const uint16_t* data){
    if (data == nullptr) return 0.0f;
    float res; 
    // Assumes Big Endian: [0] = High Word, [1] = Low Word
    uint32_t combinado = ((uint32_t)data[0] << 16) | data[1]; 
    memcpy(&res, &combinado, sizeof(res)); 
    return res; 
}

/**
 * @brief Returns list of titles for registers that have logging enabled.
 */
nameColValues EnergyMeterRegInterpreter::getLastNameValues() {
    nameColValues res;
    int count = 0; 
    res.size = 0;

    for (int i = 0; i < _registrySize; i++) {
        if (_registryBuffer[i].logEnabled) {
            if (count < MAX_MODBUS_REGS) {
                res.buffer[count] = _registryBuffer[i].name;
                count++;
            }
        }
    }
    
    res.size = count;
    return res;
}

/**
 * @brief Returns the number of Modbus registers required for a given format.
 */
int EnergyMeterRegInterpreter::getFormatSize(coded_format f) {
    switch (f) {
        case FLOAT:   return SIZE_FLOAT; 
        case INT:     return SIZE_INT;
        case UINT:    return SIZE_UINT;
        case LONG64:  return SIZE_LONG64;
        case SHORT:   return SIZE_SHORT; 
        case USHORT:  return SIZE_USHORT;
        case BYTE:    return SIZE_BYTE;
        case DFLOAT:  return SIZE_DFLOAT;
        default:      return 0;
    }
}

/**
 * @brief String to Enum mapping for CSV formats.
 */
coded_format EnergyMeterRegInterpreter::stringToFormat(const char* str) {
    if (str == nullptr) return FORMAT_UNKNOWN;

    if (strcasecmp(str, "FLOAT") == 0)  return FLOAT;
    if (strcasecmp(str, "SHORT") == 0)  return SHORT;
    if (strcasecmp(str, "INT") == 0)    return INT;
    if (strcasecmp(str, "UINT") == 0)   return UINT;
    if (strcasecmp(str, "USHORT") == 0) return USHORT;
    if (strcasecmp(str, "BYTE") == 0)   return BYTE;
    if (strcasecmp(str, "LONG64") == 0) return LONG64;
    if (strcasecmp(str, "DFLOAT") == 0) return DFLOAT;
    if (strcasecmp(str, "STRING") == 0) return STRING;
    
    return FORMAT_UNKNOWN;
}

/**
 * @brief Formats raw data into strings for the logging buffer.
 */
netDataString EnergyMeterRegInterpreter::getBufNetDataString() {
    netDataString res;
    res.size = 0;
    int count = 0; 

    for (int i = 0; i < _registrySize; i++) {
        if(_registryBuffer[i].logEnabled){
            getNetDataString(_netDataStringBuffer[i], _RawDataBuffer[i]);

            if (count < MAX_MODBUS_REGS) {
                res.buffer[count] = _netDataStringBuffer[i];
                count++;
            }
        }    
    }

    res.size = count;
    return res;
}

/**
 * @brief Reads configuration parameters from the CSV header.
 */
void EnergyMeterRegInterpreter::loadParametersMapRegister() {
    CSV_Parser cp("sssss", true, ';');

    _sd->withFile(MAP_FILE, [](Stream& file, void* arg) {
        CSV_Parser* parser = (CSV_Parser*)arg;
        
        // Read header lines before the table starts
        for (int i = 0; i < LINE_MAP_START; i++) {
            if (file.available()) {
                String line = file.readStringUntil('\n');
                if (line.length() > 0) {
                    line += "\n";
                    *parser << line.c_str();
                }
            }
        }
    }, &cp);

    char **values = (char**)cp[ (int)1 ]; // Access second column

    if (values != nullptr && cp.getRowsCount() >= 3) {
        if (values[0]) {
            strncpy(_log_interval, values[0], MAX_TITLES_SIZE - 1);
            _log_interval[MAX_TITLES_SIZE - 1] = '\0';
        }
        if (values[1]) {
            strncpy(_new_file, values[1], MAX_TITLES_SIZE - 1);
            _new_file[MAX_TITLES_SIZE - 1] = '\0';
        }
        if (values[2]) {
            strncpy(_max_files, values[2], MAX_TITLES_SIZE - 1);
            _max_files[MAX_TITLES_SIZE - 1] = '\0';
        }
    }
}

Parameters EnergyMeterRegInterpreter::getParameters(){
    Parameters res; 
    res.log_interval = _log_interval;
    res.new_file = _new_file; 
    res.max_files = _max_files; 
    return res; 
}

/**
 * @brief Validates input requirements and prepares the SD log session.
 */
bool EnergyMeterRegInterpreter::prepareAdvanceDatalogger(Struct_MBRequest MB_req, Datalogger* datalogger, RTC_DS3231* rtc) {

    _advancedIsInitialized = false;

    // Validation filters
    if (MB_req.channel <= 0) return false;
    if (MB_req.start_addres >= MAX_EM_ADDR) return false;
    if (MB_req.length == 0 || MB_req.length > MAX_MODBUS_REGS_REQUEST) return false;
    if ((MB_req.start_addres + MB_req.length) > MAX_EM_ADDR) return false;
    if (MB_req.func_code < 1 || MB_req.func_code > 4) return false;
    if (MB_req.req_interval_ms < 1000) return false;
    
    startNewRequest(MB_req.start_addres, MB_req.length);


    if(_current_request.size == 0) return false;
    
    _misTitulos = getLastNameValues();
    if(_misTitulos.size == 0) return false;
    
    loadParametersMapRegister(); 
    Parameters param = getParameters();

    _int_log_interval = atoi(param.log_interval);
    _new_file_interval_s = getLogIntervalFromString(param.new_file);
    _int_max_files = atoi(param.max_files);

    
    // Business Logic Constraints
    //if (_int_log_interval != (int)MB_req.req_interval_ms) return false;
    //if (_new_file_interval_s != 3600) return false; // Hardcoded to 1 hour for now
    if (_int_max_files <= 0 || _int_max_files >= MAX_LOG_CAPACITY) return false;
   
   //if (_int_log_interval != (int)MB_req.req_interval_ms) return false;
   //if (_new_file_interval_s <= _int_log_interval) return false; // Hardcoded to 1 hour for now
   //if (_int_max_files <= 0 || _int_max_files >= 50) return false;


    datalogger->setMaxFiles(_int_max_files);

    //datalogger->clearAllLogs(); // Optional: clears folder on every reboot
    /*
    if(!crear_nueva_sesion_log(datalogger, rtc, &_misTitulos)) {
        //Serial.println(F("Error: Failed to create log session."));
        return false; 
    }
     */

    anteriorMillisModbus = 0;
    //anteriorMillisArchivo = 0;

    _advancedIsInitialized = true;
    return true; 
}


/**
 * @brief Main execution loop for timed logging and file rotation.
 */
 void EnergyMeterRegInterpreter::advancedDataloggerExec(Datalogger* datalogger, EnergyMeter750* em, RTC_DS3231* rtc) {
    if (!_advancedIsInitialized) return;

    unsigned long actualMillis = millis();
    DateTime ahora = rtc->now();

    // --- CAMBIO DE SESIÓN (Basado en RTC) ---
    bool debeCambiarSesion = false;

    // Comparamos el tiempo actual con la última vez que se cambió de archivo
    if (ahora.minute() != ultimaUnidadTiempo) { 
        // Ejemplo para cambio cada MINUTO
        if (strcasecmp(_new_file, "minute") == 0) debeCambiarSesion = true;
        
        // Ejemplo para cambio cada HORA (si el minuto es 0 y cambió la hora)
        if (strcasecmp(_new_file, "hour") == 0 && ahora.minute() == 0) debeCambiarSesion = true;
        
        // Ejemplo para cambio cada DÍA (si es medianoche)
        if (strcasecmp(_new_file, "day") == 0 && ahora.hour() == 0 && ahora.minute() == 0) debeCambiarSesion = true;

        if (debeCambiarSesion) {
            ultimaUnidadTiempo = ahora.minute(); // Actualizamos bandera
            crear_nueva_sesion_log(datalogger, rtc, &_misTitulos);
        }
    }

    // --- MUESTREO (Basado en millis) ---
    if (actualMillis - anteriorMillisModbus >= (unsigned long)_int_log_interval) {
        anteriorMillisModbus += _int_log_interval;
        lectura_modbus(datalogger, rtc, em, _current_request);
    }
}
 /*
void EnergyMeterRegInterpreter::advancedDataloggerExec(Datalogger* datalogger, EnergyMeter750* em, RTC_DS3231* rtc){
   
    if (_advancedIsInitialized){
        unsigned long actualMillis = millis();

        // Timer for file rotation (new log session)
        if (actualMillis - anteriorMillisArchivo >= (_new_file_interval_s * 1000UL)) { 
            anteriorMillisArchivo = actualMillis;
            crear_nueva_sesion_log(datalogger, rtc, &_misTitulos);
        }  
        
        // Timer for Modbus sampling
        if (actualMillis - anteriorMillisModbus >= (unsigned long)_int_log_interval) {  
            anteriorMillisModbus = actualMillis;
            lectura_modbus(datalogger, rtc, em, _current_request);       
        }      
        
    } else {
        //Serial.println(F("System not initialized. Waiting..."));
        //delay(5000); 
    }
}
*/
/**
 * @brief Converts binary raw registers to human-readable strings based on format.
 */
void EnergyMeterRegInterpreter::getNetDataString(char* dest, rawDataReg rawRegister){
    coded_format fmt = rawRegister.format;
    uint16_t* d = rawRegister.data;

    switch (fmt) {
    case FLOAT: {
        float f_val = getFloatConversion(d);
        snprintf(dest, MAX_TITLES_SIZE, "%.2f", f_val);
        break;
    }
    case SHORT: {
        int16_t s_val = (int16_t)d[0];
        snprintf(dest, MAX_TITLES_SIZE, "%d", s_val);
        break;
    }
    case USHORT:
    case DFLOAT: {
        snprintf(dest, MAX_TITLES_SIZE, "%u", d[0]);
        break;
    }
    case INT: {
        int32_t i_val = (int32_t)(((uint32_t)d[0] << 16) | d[1]);
        snprintf(dest, MAX_TITLES_SIZE, "%ld", i_val);
        break;
    }
    case UINT: {
        uint32_t ui_val = ((uint32_t)d[0] << 16) | d[1];
        snprintf(dest, MAX_TITLES_SIZE, "%lu", ui_val);
        break;
    }
    case BYTE: {
        snprintf(dest, MAX_TITLES_SIZE, "%u", d[0] & 0xFF);
        break;
    }
    case LONG64: {
        uint64_t l_val = ((uint64_t)d[0] << 48) | ((uint64_t)d[1] << 32) | ((uint64_t)d[2] << 16) | (uint64_t)d[3];
        snprintf(dest, MAX_TITLES_SIZE, "%llu", l_val);
        break;
    }
    case STRING: {
        memcpy(dest, d, MAX_DATA_SIZE * 2);
        dest[MAX_DATA_SIZE * 2] = '\0'; 
        break;
    }
    default:
        snprintf(dest, MAX_TITLES_SIZE, "n/a");
        break;
    }
} 

//--------- Helper Functions ------------------

/**
 * @brief Genera un nombre de archivo basado en el RTC y crea sesión solo si el nombre cambia.
 */
bool crear_nueva_sesion_log(Datalogger* datalogger, RTC_DS3231* rtc, nameColValues* misTitulos) {
    DateTime ahora = rtc->now();
    char nombreFichero[16]; 

    // Formato: MMDDHHmm
    snprintf(nombreFichero, sizeof(nombreFichero), "%02d%02d%02d%02d", 
            ahora.month(), 
            ahora.day(), 
            ahora.hour(),
            ahora.minute());
    
    const char* actual = datalogger->getCurrentLogFile();

    // --- DEBUG LOGS ---
    Serial.println(F("--- Comparación de Sesión ---"));
    Serial.print(F("Nueva sugerencia (nombreFichero): ")); 
    Serial.println(nombreFichero);
    Serial.print(F("Sesión activa (actual): ")); 
    Serial.println(actual[0] == '\0' ? "[VACÍO]" : actual);
    // ------------------

    // Verificamos si nombreFichero está contenido en la ruta actual
    if (actual[0] != '\0' && strstr(actual, nombreFichero) != nullptr) {
        Serial.println(F(">> COINCIDENCIA DETECTADA: Manteniendo sesión actual.")); 
        return true; 
    }

    Serial.println(F(">> NO COINCIDE: Creando nueva sesión...")); 
    return datalogger->newSesion(nombreFichero, misTitulos->buffer, misTitulos->size);
}

/**
 * @brief Internal Modbus task: Executes request, converts data, and writes to SD.
 */
void EnergyMeterRegInterpreter::lectura_modbus(Datalogger* datalogger, RTC_DS3231* rtc, EnergyMeter750* em, EM_request req){
    
  //Serial.println(F("Writing data row to log..."));

  if (!em->executeRequest(req)) { 
      //Serial.println(F("Error: Modbus request execution failed."));
  } else {
    rawDataBuffer raw = em->readDataBuffer();
    getBufferDataRaw(raw.buffer, raw.size);
    netDataString res = getBufNetDataString(); 

    DateTime now = rtc->now();
    char bufferTime[20];
    sprintf(bufferTime, "%04d-%02d-%02d %02d:%02d:%02d", 
            now.year(), now.month(), now.day(), 
            now.hour(), now.minute(), now.second());

    if(!datalogger->writeRow(bufferTime, res.buffer, res.size)){
        //Serial.println(F("Error: Writing to SD failed.")); 
    } 
    datalogger->printLogToSerial();
  }
}

/**
 * @brief Converts textual interval descriptions into seconds.
 */
uint32_t getLogIntervalFromString(const char* log_interval_str) {
    if (log_interval_str == nullptr) return 0;
    if (strcasecmp(log_interval_str, "minute") == 0) return 60UL;
    if (strcasecmp(log_interval_str, "hour") == 0)   return 3600UL;
    if (strcasecmp(log_interval_str, "day") == 0)    return 86400UL;
    if (strcasecmp(log_interval_str, "month") == 0)  return 2592000UL; // Avg 30 days
    return 0;
}