#include "EnergyMeterRegInterpreter.h" // cambiar nombre a csv functionalities
#include <Arduino.h>

/*
struct InterpreterContext {
    EnergyMeterRegInterpreter* instance;
    uint16_t start_addr;
    uint16_t size;
};
*/

// Estructura para pasar datos al callback de Stream
struct StreamContext {
    EnergyMeterRegInterpreter* instance;
    uint16_t start_addr;
    uint16_t size;
};

// Constructor: Inicializamos el puntero al manager de la SD
EnergyMeterRegInterpreter::EnergyMeterRegInterpreter(SDManager* sdManager) {
    _sd = sdManager;
    //_setupFile = SETUP_FILE;
}

int EnergyMeterRegInterpreter::begin(){
   // analizar si el SD ya esta iniciado 

    if (!_sd->isReady()) { 
        Serial.println("Datalogger: SDManager no está listo aún.");
        return false;
    }
    return true;

   /*
    if (!_sd->begin()) {
        Serial.println(F("Error: No se pudo iniciar la SD desde EnergyMeterRegInterpreter"));
        return false;
    }
    */
}

void EnergyMeterRegInterpreter::processParserData(CSV_Parser& cp, uint16_t start, uint16_t size) {
    // 1. Obtener punteros y VERIFICAR que no sean NULL
    int32_t *addrs = (int32_t*)cp["Address"];
    char **formats = (char**)cp["Format"];
    char **names = (char**)cp["Name"];
    char **logs = (char**)cp["Log"];

    // Si el CSV usa nombres distintos, cp[] devuelve NULL. 
    // Verifica que estos nombres coincidan EXACTAMENTE con la cabecera de tu CSV
    if (addrs == nullptr || formats == nullptr || names == nullptr || logs == nullptr) {
        Serial.println("Error: No se encontraron las columnas en el CSV. Revisa las cabeceras.");
        return;
    }

    int totalRows = cp.getRowsCount();
    _lastRowReadSize = 0; // Reiniciamos contador de filas encontradas en el rango

    for (int i = 0; i < totalRows; i++) {
        // Validación de seguridad para el índice
        uint16_t addr = (uint16_t)addrs[i];

        if (addr >= start && addr < (start + size)) {
            
            // Verificamos que no nos pasemos del buffer de la clase
            if (_lastRowReadSize >= MAX_MODBUS_REGS) {
                Serial.println("Advertencia: Se alcanzó el límite MAX_MODBUS_REGS");
                break;
            }

            coded_format fmtEnum = stringToFormat(formats[i]);
            
            if (fmtEnum != FORMAT_UNKNOWN) {
                int idx = _lastRowReadSize;
                
               // Almacenar formatos
                _bufFormatValues[idx] = fmtEnum;
                //almacenar direcciones
                _bufAddrsValues[idx] = addr;

                // almacenar LOG
                if (strcasecmp(logs[i], "Yes") == 0) {
                    _bufLogValues[idx] = true;  // Equivale a 1
                } else {
                    _bufLogValues[idx] = false; // Equivale a 0 (para "No" o cualquier otro valor)
                }
                
                // Calmacenar nombres 
                if (names[i] != nullptr) {
                    strncpy(_bufNamesValues[idx], names[i], MAX_TITLES_SIZE - 1);
                    _bufNamesValues[idx][MAX_TITLES_SIZE - 1] = '\0';
                } else {
                    strcpy(_bufNamesValues[idx], "Unknown");
                }

                // Actualizar el tamaño total de registros Modbus (16-bit units)
                _current_request.size += getFormatSize(fmtEnum);
                
                _lastRowReadSize++;
            }
        }
    }
    
    Serial.print("Proceso finalizado. Filas en rango: ");
    Serial.println(_lastRowReadSize);
}

/*
void EnergyMeterRegInterpreter::processParserData(CSV_Parser& cp, uint16_t start, uint16_t size) {
    int32_t *addrs = (int32_t*)cp["Address"];
    char **formats = (char**)cp["Format"];
    char **names = (char**)cp["Name"];
    int totalRows = cp.getRowsCount();

    for (int i = 0; i < totalRows; i++) {
        uint16_t addr = (uint16_t)addrs[i];

        if (addr >= start && addr < (start + size)) {
            coded_format fmtEnum = stringToFormat(formats[i]);
            
            if (fmtEnum != FORMAT_UNKNOWN && _lastRowReadSize < MAX_MODBUS_REGS) {
                int idx = _lastRowReadSize;
                
                _bufFormatValues[idx] = fmtEnum;
                _bufAddrsValues[idx] = addr;
                
                strncpy(_bufNamesValues[idx], names[i], MAX_TITLES_SIZE - 1);
                _bufNamesValues[idx][MAX_TITLES_SIZE - 1] = '\0';

                _current_request.size += getFormatSize(fmtEnum);
                
                _lastRowReadSize++;
                
            }
            
        }
    }
}
*/

EM_request EnergyMeterRegInterpreter::startNewRequest(const uint16_t start_addr, const uint16_t size) {
    _current_request.start_addr = start_addr;
    _current_request.size = 0;
    _lastRowReadSize = 0;

    StreamContext ctx = {this, start_addr, size};
        
    // Usamos withFile para obtener el Stream del archivo
    _sd->withFile(MAP_FILE, [](Stream& file, void* arg) {
        StreamContext* sc = (StreamContext*)arg;
        
        // 1. Extraer parámetros iniciales (las primeras líneas que no son la tabla)
        // Leemos hasta encontrar la cabecera "Address"
        /*
        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.startsWith("Address")) break; 
            
            // Aquí puedes llamar a una función que guarde Log Interval, etc.
            //sc->instance->handleConfigLine(line); 
        }*/

        // FUNCION SIMPLE PARA SALTAR A LA UBICACION DEL MAPA DE REGISTRO
        for (int i = 0; i < LINE_MAP_START; i++) {
            if (file.available()) {
            file.readStringUntil('\n'); 
             }
        }

    
        // 2. Usar CSV_Parser para el resto del archivo (la tabla)
        // "Lssss" -> Address(L), Format(s), Unit(s), Name(s), Log(s)
        CSV_Parser cp("Lssss", true, ';');

        // Alimentamos el parser con el resto del stream
        while (file.available()) {
            String line = file.readStringUntil('\n');
            if (line.length() > 0) {
                line += "\n";
                cp << line.c_str();
            }
        }

        // 3. Procesar los resultados del Parser y filtrarlos por rango
        sc->instance->processParserData(cp, sc->start_addr, sc->size);

    }, &ctx);
        
    return _current_request;
}


// Devuelve una estructura con la toda la informacion para generar los resultados. 
bufRawDataReg EnergyMeterRegInterpreter::getBufferDataRaw(const uint16_t* datos, const uint16_t size) {
    bufRawDataReg res;
    res.buffer = _bufRawData; // Apuntamos al array persistente de la clase
    res.size = 0;

    uint16_t offsetOriginal = 0; // Para rastrear la posición en el array 'datos' raw

    // Recorremos las filas que encontramos previamente en la SD
    for (int i = 0; i < _lastRowReadSize; i++) {
        
        coded_format fmt = _bufFormatValues[i];
        int regs_del_dato = getFormatSize(fmt);

        // Seguridad: No sobrepasar el tamaño del buffer raw recibido
        if (offsetOriginal + regs_del_dato > size) break;

        // Llenamos la estructura en la posición i del array de la clase
        _bufRawData[i].format = fmt;
        
        for (int j = 0; j < regs_del_dato; j++) {
            _bufRawData[i].data[j] = datos[offsetOriginal + j];
        }

        // Si el formato usa menos de MAX_DATA_SIZE, podemos limpiar el resto (opcional)
        for (int j = regs_del_dato; j < MAX_DATA_SIZE; j++) {
            _bufRawData[i].data[j] = 0;
        }

        offsetOriginal += regs_del_dato;
        res.size++;
    }

    return res; 
}


// funcion auxiliar para interpretar registros modbus de 16 bits a float
float EnergyMeterRegInterpreter::getFloatConversion(const uint16_t* data){
    if (data == nullptr) return 0.0f;

    float res; 

    uint32_t combinado = ((uint32_t)data[0] << 16) | data[1]; // TODO depende de si es little endian o Big endian

    memcpy(&res, &combinado, sizeof(res)); 

    return res; 
}

// solo devuelve dato si log es true
nameColValues EnergyMeterRegInterpreter::getLastNameValues() {
    nameColValues res;
    int count = 0; // Índice para el buffer de destino
    
    // Inicializamos el tamaño a 0 por seguridad
    res.size = 0;

    for (int i = 0; i < _lastRowReadSize; i++) {
        // Filtramos: solo si el booleano es true
        if (_bufLogValues[i] == true) {
            // Verificamos que no superemos el límite del buffer de salida
            if (count < MAX_MODBUS_REGS) {
                res.buffer[count] = _bufNamesValues[i];
                count++;
            }
        }
    }
    
    // Guardamos cuántos elementos se han filtrado realmente
    res.size = count;
    return res;
}
/*
nameColValues EnergyMeterRegInterpreter::getLastNameValues() {
    nameColValues res;
    
    // Solo devolvemos los títulos que pertenecen a la última request procesada
    // Si necesitas filtrar por start_addr aquí, puedes hacerlo, 
    // pero lo normal es que devuelvas lo que ya tienes en el buffer.
    
    for (int i = 0; i < _lastRowReadSize; i++) {
        res.buffer[i] = _bufNamesValues[i];
    }
    
    res.size = _lastRowReadSize;
    return res;
}

*/

/**
 * Función auxiliar para saber cuántos registros Modbus ocupa cada formato
 */
int EnergyMeterRegInterpreter::getFormatSize(coded_format f) {

    switch (f) {
        case FLOAT:
            return SIZE_FLOAT; 
        case INT:
            return SIZE_INT;
        case UINT:
            return SIZE_UINT; // 32 bits = 2 registros Modbus
        case LONG64:
            return SIZE_LONG64; // 64 bits = 4 registros Modbus
        case SHORT:
            return SIZE_SHORT; 
        case USHORT:
            return SIZE_USHORT;
        case BYTE:
            return SIZE_BYTE;
        case DFLOAT:
            return SIZE_DFLOAT; // 16 bits = 1 registro Modbus
        default:
            return 0;
    }
}

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

// modificada
netDataString EnergyMeterRegInterpreter::getBufNetDataString() {
    netDataString res;
    res.size = 0;
    int count = 0; // Índice para el buffer de destino filtrado

    for (int i = 0; i < _lastRowReadSize; i++) {
        // FILTRO: Solo procesamos y añadimos si log es true
        if (_bufLogValues[i] == true) {
            
            coded_format fmt = _bufRawData[i].format;
            uint16_t* d = _bufRawData[i].data;

            switch (fmt) {
                case FLOAT: {
                    float f_val = getFloatConversion(d);
                    snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%.2f", f_val);
                    break;
                }

                case SHORT: {
                    int16_t s_val = (int16_t)d[0];
                    snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%d", s_val);
                    break;
                }

                case USHORT:
                case DFLOAT: {
                    snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%u", d[0]);
                    break;
                }

                case INT: {
                    int32_t i_val = (int32_t)(((uint32_t)d[0] << 16) | d[1]);
                    snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%ld", i_val);
                    break;
                }

                case UINT: {
                    uint32_t ui_val = ((uint32_t)d[0] << 16) | d[1];
                    snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%lu", ui_val);
                    break;
                }

                case BYTE: {
                    snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%u", d[0] & 0xFF);
                    break;
                }

                case LONG64: {
                    uint64_t l_val = ((uint64_t)d[0] << 48) | ((uint64_t)d[1] << 32) | 
                                     ((uint64_t)d[2] << 16) | (uint64_t)d[3];
                    snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%llu", l_val);
                    break;
                }

                case STRING: {
                    memcpy(_bufNetDataString[i], d, MAX_DATA_SIZE * 2);
                    _bufNetDataString[i][MAX_DATA_SIZE * 2] = '\0'; 
                    break;
                }

                default:
                    snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "n/a");
                    break;
            }

            // Asignamos al buffer de salida usando el índice compacto 'count'
            if (count < MAX_MODBUS_REGS) {
                res.buffer[count] = _bufNetDataString[i];
                count++;
            }
        }
    }

    res.size = count; // El tamaño final es el número de elementos que pasaron el filtro
    return res;
}
/*
netDataString EnergyMeterRegInterpreter::getBufNetDataString() {
    netDataString res;
    res.size = 0;

    for (int i = 0; i < _lastRowReadSize; i++) {
        // if (_bufLogValues[i] == true)
        coded_format fmt = _bufRawData[i].format;
        uint16_t* d = _bufRawData[i].data;

        switch (fmt) {
            case FLOAT: {
                float f_val = getFloatConversion(d);
                snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%.2f", f_val);
                break;
            }

            case SHORT: {
                // El cast a int16_t interpreta el bit de signo
                int16_t s_val = (int16_t)d[0];
                snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%d", s_val);
                break;
            }

            case USHORT:
            case DFLOAT: { // DFLOAT suele ser un entero con decimales implícitos
                snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%u", d[0]);
                break;
            }

            case INT: {
                // Combinar dos registros de 16 bits en un entero de 32 con signo
                int32_t i_val = (int32_t)(((uint32_t)d[0] << 16) | d[1]);
                snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%ld", i_val);
                break;
            }

            case UINT: {
                uint32_t ui_val = ((uint32_t)d[0] << 16) | d[1];
                snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%lu", ui_val);
                break;
            }

            case BYTE: {
                // Normalmente el byte está en la parte baja del registro
                snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%u", d[0] & 0xFF);
                break;
            }

            case LONG64: {
                // Ojo: printf con uint64_t puede requerir %llu o PRIu64
                uint64_t l_val = ((uint64_t)d[0] << 48) | ((uint64_t)d[1] << 32) | 
                                 ((uint64_t)d[2] << 16) | (uint64_t)d[3];
                snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "%llu", l_val);
                break;
            }

            case STRING: {
                // Copia directamente los registros como caracteres (2 caracteres por registro)
                // Asegúrate de que no desborde MAX_TITLES_SIZE
                memcpy(_bufNetDataString[i], d, MAX_DATA_SIZE * 2);
                _bufNetDataString[i][MAX_DATA_SIZE * 2] = '\0'; 
                break;
            }

            default:
                snprintf(_bufNetDataString[i], MAX_TITLES_SIZE, "n/a");
                break;
        }

        res.buffer[i] = _bufNetDataString[i];
        res.size++;
    }

    return res;
}
*/

void EnergyMeterRegInterpreter::loadParametersMapRegister() {
    // Definimos el parser para 2 columnas de tipo string ("ss")
    // Columna 0: Nombre del parámetro, Columna 1: Valor
    CSV_Parser cp("sssss", true, ';');

    // Usamos withFile para obtener el stream de forma segura
    _sd->withFile(MAP_FILE, [](Stream& file, void* arg) {
        CSV_Parser* parser = (CSV_Parser*)arg;
        
        // Leemos solo hasta LINE_MAP_START (línea 4)
        for (int i = 0; i < LINE_MAP_START; i++) {
            if (file.available()) {
                String line = file.readStringUntil('\n');
                if (line.length() > 0) {
                    line += "\n";
                    // Alimentamos el parser con las líneas de configuración
                    *parser << line.c_str();
                }
            }
        }
    }, &cp);

    // Una vez cerrado el archivo, extraemos los valores del objeto cp
    // CSV_Parser asume que la primera línea leída son las cabeceras.
    // Si tu archivo empieza con "Log Interval (ms);5000", esa será la cabecera.
    // Para evitar problemas, usaremos los índices de columna [0] y [1].
    
    char **names  = (char**)cp[ (int)0 ]; // Primera columna
    char **values = (char**)cp[ (int)1 ]; // Segunda columna

    if (values != nullptr && cp.getRowsCount() >= 3) {
        // cp.getRowsCount() devuelve las filas SIN contar la cabecera.
        // Fila 0 (Header): Log Interval (ms); 5000
        // Fila 1: New File... ; minute
        // Fila 2: Max files ; 30

        // 1. Log Interval (está en la cabecera si es la primera línea, o fila 0 si hay header)
        // Como el parser es un poco especial con las cabeceras, 
        // lo más seguro es acceder a las filas disponibles:
        
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


/*
        while (file.available()) {
            String line = file.readStringUntil('\n');
            if (line.length() > 0) {
                line += "\n";
                cp << line.c_str();
            }
        }
*/


