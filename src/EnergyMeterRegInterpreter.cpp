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
esp_err_t lectura_modbus(Datalogger* datalogger, RTC_DS3231* rtc, EnergyMeter750* em, EM_request req);
esp_err_t crear_nueva_sesion_log(Datalogger* datalogger, RTC_DS3231* rtc, nameColValues* misTitulos); 

EnergyMeterRegInterpreter::EnergyMeterRegInterpreter(SDManager* sdManager) 
  : _sd(sdManager) 
{
    _registrySize = 0;
}

/**
 * @brief Checks if the SD manager is ready and initializes the interpreter.
 */
esp_err_t EnergyMeterRegInterpreter::begin(){
    if (!_sd->isReady()) return ESP_ERR_SD_NOT_INIT;
    _initialized = true; 
    return ESP_OK;
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
esp_err_t EnergyMeterRegInterpreter::startNewRequest(const uint16_t start_addr, const uint16_t size, EM_request *out_req) {
    if(!_initialized) return ESP_ERR_INTERPRETER_NOT_INIT;

    _current_request.start_addr = 0;
    _current_request.size = 0;
    _registrySize = 0;

    StreamContext ctx = {this, start_addr, size};
        
    // Propagamos el error que devuelva withFile (ej: 0x40003 si no existe el archivo)
    esp_err_t err = _sd->withFile(MAP_FILE, [](Stream& file, void* arg) {
        StreamContext* sc = (StreamContext*)arg;
        for (int i = 0; i < LINE_MAP_START; i++) {
            if (file.available()) file.readStringUntil('\n'); 
        }

        CSV_Parser cp("Lssss", true, ';');
        while (file.available()) {
            String line = file.readStringUntil('\n');
            if (line.length() > 0) {
                line += "\n";
                cp << line.c_str();
            }
        }
        sc->instance->processParserData(cp, sc->start_addr, sc->size);
    }, &ctx);

    if (err != ESP_OK) return err;
    if (_registrySize == 0) return ESP_ERR_INTERPRETER_MAP_MISS;

    _current_request.start_addr = _registryBuffer[0].address; 
    if (out_req) *out_req = _current_request;
    
    return ESP_OK;
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
esp_err_t EnergyMeterRegInterpreter::prepareAdvanceDatalogger(Struct_MBRequest MB_req, Datalogger* datalogger, RTC_DS3231* rtc) {
    _advancedIsInitialized = false;

    // Validaciones de negocio
    if (MB_req.channel <= 0) return ESP_ERR_INVALID_ARG;
    if (MB_req.length == 0 || MB_req.length > MAX_MODBUS_REGS_REQUEST) return ESP_ERR_INVALID_SIZE;
    
    // Intentar cargar mapa
    esp_err_t err = startNewRequest(MB_req.start_addres, MB_req.length, nullptr);
    if (err != ESP_OK) return err;

    _misTitulos = getLastNameValues();
    if(_misTitulos.size == 0) return ESP_ERR_INTERPRETER_MAP_MISS;
    
    // Cargar parámetros adicionales
    loadParametersMapRegister(); 
    Parameters param = getParameters();
    
    _int_log_interval = atoi(param.log_interval);
    _int_max_files = atoi(param.max_files);

    Serial.print("log interval: ");
    Serial.println(param.log_interval);

    Serial.print("max files: ");
    Serial.println(param.max_files);

    Serial.print("tiempo file"); 
    Serial.println(param.new_file);

    if (_int_max_files <= 0 || _int_max_files >= MAX_LOG_CAPACITY) {
        return ESP_ERR_INTERPRETER_BAD_CONF;
    }

    datalogger->setMaxFiles(_int_max_files);
    //datalogger->clearAllLogs(); // Optional: clears folder on every reboot
    
    _advancedIsInitialized = true;

    //err = crear_nueva_sesion_log(datalogger, rtc, &_misTitulos);
 
    return err; 
}


/**
 * @brief Main execution loop for timed logging and file rotation.
 */
 esp_err_t EnergyMeterRegInterpreter::advancedDataloggerExec(Datalogger* datalogger, EnergyMeter750* em, RTC_DS3231* rtc) {
    if (!_advancedIsInitialized){ 
        Serial.println("error de que no se inicializo el advanced"); 
        return ESP_ERR_INTERPRETER_NOT_INIT;
    };

    unsigned long actualMillis = millis();
    DateTime ahora = rtc->now();

    // --- CAMBIO DE SESIÓN (Basado en RTC) ---
    bool debeCambiarSesion = false;
    esp_err_t err; 

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
            err = crear_nueva_sesion_log(datalogger, rtc, &_misTitulos);

            if (err != ESP_OK){Serial.println("error 2");  return err;}
        }
    }

    // --- MUESTREO (Basado en millis) ---
    if (actualMillis - anteriorMillisModbus >= (unsigned long)_int_log_interval) {
        anteriorMillisModbus += _int_log_interval;
        
        err = lectura_modbus(datalogger, rtc, em, _current_request);
       
        if (err != ESP_OK){
            Serial.println("error 3");  
            return err;
        };
    }
    return ESP_OK;
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
esp_err_t crear_nueva_sesion_log(Datalogger* datalogger, RTC_DS3231* rtc, nameColValues* misTitulos) {
    DateTime ahora = rtc->now();
    char nombreFichero[16]; 

    // Formato: MMDDHHmm
    snprintf(nombreFichero, sizeof(nombreFichero), "%02d%02d%02d%02d%02d%02d", 
        ahora.year(), // Extrae los últimos dos dígitos del año (ej: 2026 -> 26)
        ahora.month(), 
        ahora.day(), 
        ahora.hour(),
        ahora.minute(),
        ahora.second());
    
    //const char* actual = datalogger->getCurrentLogFile();

    // --- DEBUG LOGS ---
   //Serial.println(F("--- Comparación de Sesión ---"));
   //Serial.print(F("Nueva sugerencia (nombreFichero): ")); 
   //Serial.println(nombreFichero);
   //Serial.print(F("Sesión activa (actual): ")); 
   //Serial.println(actual[0] == '\0' ? "[VACÍO]" : actual);
    // ------------------

    // Verificamos si nombreFichero está contenido en la ruta actual
    /*
    if (actual[0] != '\0' && strstr(actual, nombreFichero) != nullptr) {
        //Serial.println(F(">> COINCIDENCIA DETECTADA: Manteniendo sesión actual.")); 
        return ESP_OK; 
    }
*/
    esp_err_t err; 
    //Serial.println(F(">> NO COINCIDE: Creando nueva sesión...")); 

    Serial.print("nueva sesion: "); 
    Serial.println(nombreFichero);

    err = datalogger->newCSVLogSesion(nombreFichero, misTitulos->buffer, misTitulos->size);
    if(err != ESP_OK){
        return err; 
    }

/*
     if(datalogger->newCSVLogSesion(nombreFichero, misTitulos->buffer, misTitulos->size)){
            Serial.println("nueva session correcta");
     }else{
            Serial.println("nueva sesion fracaso");
     }
*/
     return ESP_OK;
}

/**
 * @brief Internal Modbus task: Executes request, converts data, and writes to SD.
 */
esp_err_t EnergyMeterRegInterpreter::lectura_modbus(Datalogger* datalogger, RTC_DS3231* rtc, EnergyMeter750* em, EM_request req){
    
    // 1. Ejecutar Modbus y capturar error
    esp_err_t err = em->executeRequest(req);
    
    if (err != ESP_OK) {
        ESP_LOGE("INTERP", "Fallo Modbus: 0x%X", err);
        return err; // No intentamos procesar datos basura
    }

    // 2. Procesar datos (esto es interno, confiamos en el buffer)
    rawDataBuffer raw = em->readDataBuffer();
    getBufferDataRaw(raw.buffer, raw.size);
    netDataString res = getBufNetDataString(); 

    // 3. Obtener tiempo
    DateTime now = rtc->now();
    char bufferTime[20];
    snprintf(bufferTime, sizeof(bufferTime), "%04d-%02d-%02d %02d:%02d:%02d", 
            now.year(), now.month(), now.day(), 
            now.hour(), now.minute(), now.second());

    // 4. Intentar escribir en SD y capturar error
    // (Asumiendo que writeRow de Datalogger también se actualiza a esp_err_t)

    err = datalogger->appendNewDataCSVToLog(bufferTime, res.buffer, res.size);
    if(err != ESP_OK){
        Serial.println("el error tiene pinta de que es aqui"); 
        return err; 
    }
/*
    if(datalogger->appendNewDataCSVToLog(bufferTime, res.buffer, res.size)){
         Serial.println("se escribe un nuevo dato en el buffer"); 
    }else{
        Serial.println("algo va mal mal");
    }
*/
    return ESP_OK; 
}

