
#include "EMRegInterpreter.h"
#include <Arduino.h>
#include <esp_log.h>

static const char* TAG = "REG_INTERP";

/**
 * @struct StreamContext
 * @brief Helper to pass class context and parameters to static callback functions.
 */
struct StreamContext {
    EMRegInterpreter* instance;
    uint16_t start_addr;
    uint16_t size;
    //uint16_t section_idx; // <-- Añadimos esto
};
/*
struct StreamContext {
    EMRegInterpreter* instance;
    uint16_t start_addr;
    uint16_t size;
};*/

EMRegInterpreter::EMRegInterpreter(SDManager* sdManager)
  : _sd(sdManager) 
{
    for(int i = 0; i < MAX_MB_REQ_SECTIONS; i++){
        _MB_ReqSectionStruct[i].registrySize = 0;
    }
    _nModbusReqSections = 0; 
}

/**
 * @brief Checks if the SD manager is ready and initializes the interpreter.
 */
esp_err_t EMRegInterpreter::begin(){
    if (!_sd->isReady()) return ESP_ERR_SD_NOT_INIT;
    _initialized = true; 
    return ESP_OK;
}

/**
 * @brief Processes the parsed CSV data to filter registers within the requested range.
 */
//void EMRegInterpreter::processParserData(CSV_Parser& cp, uint16_t start, uint16_t size, uint16_t section_idx)
 void EMRegInterpreter::processParserData(CSV_Parser& cp, uint16_t start, uint16_t size) {
    // 1. Retrieve pointers and verify column headers
    int32_t *addrs = (int32_t*)cp["Address"];
    char **formats = (char**)cp["Format"];
    char **names = (char**)cp["Name"];
    char **logs = (char**)cp["Log"];

    if (addrs == nullptr || formats == nullptr || names == nullptr || logs == nullptr) {
        return;
    }

    int totalRows = cp.getRowsCount();
    // ¡Usamos section_idx aquí!
    uint16_t idx_MB_Sections = _nModbusReqSections-1; 
    _MB_ReqSectionStruct[idx_MB_Sections].registrySize = 0; 

    for (int i = 0; i < totalRows; i++) {
        uint16_t addr = (uint16_t)addrs[i];

        // Filter by the requested address range
        if (addr >= start && addr < (start + size)) {
            
            if (_MB_ReqSectionStruct[idx_MB_Sections].registrySize >= MAX_MODBUS_REGS) {
                break;
            }

            coded_format fmtEnum = stringToFormat(formats[i]);
            
            if (fmtEnum != FORMAT_UNKNOWN) {
                int idx = _MB_ReqSectionStruct[idx_MB_Sections].registrySize;
                
                _MB_ReqSectionStruct[idx_MB_Sections].registryBuffer[idx].format = fmtEnum;
                _MB_ReqSectionStruct[idx_MB_Sections].registryBuffer[idx].address = addr;

                // Determine if logging is enabled for this register
                _MB_ReqSectionStruct[idx_MB_Sections].registryBuffer[idx].logEnabled = (strcasecmp(logs[i], "Yes") == 0);
                
                // Store register name
                if (names[i] != nullptr) {
                    strncpy(_MB_ReqSectionStruct[idx_MB_Sections].registryBuffer[idx].name, names[i], MAX_TITLES_SIZE - 1);
                    _MB_ReqSectionStruct[idx_MB_Sections].registryBuffer[idx].name[MAX_TITLES_SIZE - 1] = '\0';
                } else {
                    strcpy(_MB_ReqSectionStruct[idx_MB_Sections].registryBuffer[idx].name, "Unknown");
                }

                // Update the cumulative Modbus request size
                _MB_ReqSectionStruct[idx_MB_Sections].current_request.size += getFormatSize(fmtEnum);
                _MB_ReqSectionStruct[idx_MB_Sections].registrySize++;
            }
        }
    }
}

esp_err_t EMRegInterpreter::appendRequest(const uint16_t start_addr, const uint16_t size) {
    if(!_initialized) return ESP_ERR_INTERPRETER_NOT_INIT;
    _nModbusReqSections++; 

    // 1. Validar si ya alcanzamos el límite máximo de sectiones permitidas
    if (_nModbusReqSections >= MAX_MB_REQ_SECTIONS) {
        return ESP_ERR_INVALID_SIZE; // O un código personalizado como ESP_ERR_INTERPRETER_BAD_CONF
    }

    // El índice donde guardaremos será el valor actual del contador
    uint16_t idx_mb_section = _nModbusReqSections-1;

    // 2. Limpiar la estructura en esa posición específica
    _MB_ReqSectionStruct[idx_mb_section].current_request.start_addr = 0;
    _MB_ReqSectionStruct[idx_mb_section].current_request.size = 0;
    _MB_ReqSectionStruct[idx_mb_section].registrySize = 0;

    // 3. Crear contexto apuntando a 'current_idx'
    StreamContext ctx = {this, start_addr, size};
        
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
        // Se procesa de forma dinámica en el índice que le corresponde
        sc->instance->processParserData(cp, sc->start_addr, sc->size);
    }, &ctx);

    if (err != ESP_OK) return err;
    
    // Si el rango pedido no encontró ningún registro coincidente en el mapa CSV
    if (_MB_ReqSectionStruct[idx_mb_section].registrySize == 0) {
        return ESP_ERR_INTERPRETER_MAP_MISS;
    }
   
    // Ajustamos la dirección de inicio real en base al primer registro válido mapeado
    _MB_ReqSectionStruct[idx_mb_section].current_request.start_addr = _MB_ReqSectionStruct[idx_mb_section].registryBuffer[0].address; 
    
    // 4. ¡ÉXITO! Incrementamos de forma segura el contador de sectiones cargadas // esto daba error a la hora de leer 
    //n_ModbusReqSections++;
    
    return ESP_OK;
}


/**
 * @brief Distributes a raw 16-bit response array into formatted data structures.
 */
bufRawDataReg EMRegInterpreter::getBufferDataRaw(const uint16_t* data_readed, const uint16_t size, uint16_t idx_mb_section) {
    bufRawDataReg res;
    res.buffer = nullptr; 
    res.size = 0; 

    if(!_initialized) return res;
    if(idx_mb_section > _nModbusReqSections) return res; 
    
    res.buffer = _MB_ReqSectionStruct[idx_mb_section].RawDataBuffer; 
    
    uint16_t offsetOriginal = 0; 

    for (int i = 0; i < _MB_ReqSectionStruct[idx_mb_section].registrySize; i++) {
        coded_format fmt = _MB_ReqSectionStruct[idx_mb_section].registryBuffer[i].format;
        int regs_del_dato = getFormatSize(fmt);

        if (offsetOriginal + regs_del_dato > size) break;

        _MB_ReqSectionStruct[idx_mb_section].RawDataBuffer[i].format = fmt;
        
        // Populate the 16-bit internal buffer for each data point
        for (int j = 0; j < regs_del_dato; j++) {
            _MB_ReqSectionStruct[idx_mb_section].RawDataBuffer[i].data[j] = data_readed[offsetOriginal + j];
        }

        // Clear remaining space in buffer
        for (int j = regs_del_dato; j < MAX_DATA_SIZE; j++) {
            _MB_ReqSectionStruct[idx_mb_section].RawDataBuffer[i].data[j] = 0;
        }

        offsetOriginal += regs_del_dato;
        res.size++;
    }

    return res; 
}

/**
 * @brief Utility to convert two 16-bit registers into a IEEE 754 float.
 */
float EMRegInterpreter::getFloatConversion(const uint16_t* data){
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
nameColValues EMRegInterpreter::getLastNameValues(uint16_t idx_mb_section) {
    nameColValues res;
    int count = 0; 
    res.size = 0;
    if(!_initialized) return res;
    if(idx_mb_section > _nModbusReqSections) return res; 

    for (int i = 0; i < _MB_ReqSectionStruct[idx_mb_section].registrySize; i++) {
        if (_MB_ReqSectionStruct[idx_mb_section].registryBuffer[i].logEnabled) {
            if (count < MAX_MODBUS_REGS) {
                res.buffer[count] = _MB_ReqSectionStruct[idx_mb_section].registryBuffer[i].name;
                count++;
            }
        }
    }
    
    res.size = count;
    return res;
}

EM_request EMRegInterpreter::getLastEMRequest(uint16_t idx_mb_section){

    if(idx_mb_section > _nModbusReqSections){
        return {0,0}; 
    } 

    return _MB_ReqSectionStruct[idx_mb_section].current_request;; 
}

/**
 * @brief Returns the number of Modbus registers required for a given format.
 */
int EMRegInterpreter::getFormatSize(coded_format f) {
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
coded_format EMRegInterpreter::stringToFormat(const char* str) {
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
netDataString EMRegInterpreter::getBufNetDataString(uint16_t idx_mb_section) {
    netDataString res;
    res.size = 0;
    int count = 0; 
    if(!_initialized) return res;
    if(idx_mb_section > _nModbusReqSections) return res; 

    for (int i = 0; i < _MB_ReqSectionStruct[idx_mb_section].registrySize; i++) {
        if(_MB_ReqSectionStruct[idx_mb_section].registryBuffer[i].logEnabled){
            getNetDataString(_MB_ReqSectionStruct[idx_mb_section].netDataStringBuffer[i], _MB_ReqSectionStruct[idx_mb_section].RawDataBuffer[i]);

            if (count < MAX_MODBUS_REGS) {
                res.buffer[count] = _MB_ReqSectionStruct[idx_mb_section].netDataStringBuffer[i];
                count++;
            }
        }    
    }

    res.size = count;
    return res;
}


/**
 * @brief Converts binary raw registers to human-readable strings based on format.
 */
void EMRegInterpreter::getNetDataString(char* dest, rawDataReg rawRegister){
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

void EMRegInterpreter::debugMostrarTodo(){
  /*  
    // quiero una funcion que me muestre por serial lo que hay dentro de la estuctrutura
    Serial.print("mostramos a ver:::"); 
    Serial.print("idx:");
    Serial.println(_nModbusReqSections); 
    Serial.println(_MB_ReqSectionStruct[_nModbusReqSections].registryBuffer[0].name);
    Serial.println(_MB_ReqSectionStruct[_nModbusReqSections].registryBuffer[1].name);
    Serial.println(_MB_ReqSectionStruct[_nModbusReqSections].registryBuffer[2].name);
    Serial.println(_MB_ReqSectionStruct[_nModbusReqSections].registryBuffer[3].name);
    Serial.println(_MB_ReqSectionStruct[_nModbusReqSections].registryBuffer[4].name);
    Serial.println(_MB_ReqSectionStruct[_nModbusReqSections].registryBuffer[5].name);
    Serial.println(_MB_ReqSectionStruct[_nModbusReqSections].registryBuffer[6].name);
*/

}


uint16_t EMRegInterpreter::getCountRegsInSection(uint16_t idx_mb_section){

    if(!_initialized) return 0;
    if(idx_mb_section > _nModbusReqSections) return 0; 

    return _MB_ReqSectionStruct[idx_mb_section].registrySize; 
    
}

uint16_t EMRegInterpreter::getCountAllRegsLoaded(){
    if(!_initialized) return 0;
    
    uint16_t count_res = 0;
    
    for (int i = 0; i < _nModbusReqSections; i++){
        count_res = count_res + _MB_ReqSectionStruct[i].registrySize; 
    }
    return count_res; 
}


bool EMRegInterpreter::getLogsRegs(uint16_t idx_mb_section, uint16_t idx_reg){
    if(idx_mb_section > MAX_MB_REQ_SECTIONS || idx_reg > MAX_MODBUS_REGS) return false; // Valor por defecto
    return _MB_ReqSectionStruct[idx_mb_section].registryBuffer[idx_reg].logEnabled; 
}

char* EMRegInterpreter::getNameRegs(uint16_t idx_mb_section, uint16_t idx_reg){
    if(idx_mb_section > MAX_MB_REQ_SECTIONS || idx_reg > MAX_MODBUS_REGS) return nullptr; 
    return _MB_ReqSectionStruct[idx_mb_section].registryBuffer[idx_reg].name; 
}

char* EMRegInterpreter::getValueRegs(uint16_t idx_mb_section, uint16_t idx_reg){
    if(idx_mb_section > MAX_MB_REQ_SECTIONS || idx_reg > MAX_MODBUS_REGS) return nullptr; 
    return _MB_ReqSectionStruct[idx_mb_section].netDataStringBuffer[idx_reg];
}