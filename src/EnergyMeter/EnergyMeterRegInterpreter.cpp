#include "EnergyMeterRegInterpreter.h"
#include <Arduino.h>

struct InterpreterContext {
    EnergyMeterRegInterpreter* instance;
    uint16_t start_addr;
    uint16_t size;
};

// Constructor: Inicializamos el puntero al manager de la SD
EnergyMeterRegInterpreter::EnergyMeterRegInterpreter(SDManager* sdManager) {
    _sd = sdManager;
    _setupFile = SETUP_FILE;
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


void EnergyMeterRegInterpreter::staticCallback(const char* line, void* context) {
    InterpreterContext* ctx = (InterpreterContext*)context;

    if (line == nullptr || line[0] == '\0') return;

    // Creamos un buffer temporal para poder modificar la cadena
    // 128 bytes suelen ser suficientes para una línea de CSV de registros
    char buffer[128]; 
    strncpy(buffer, line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0'; // Asegurar cierre

    // Ahora pasamos el buffer (que es char*) a handleLine
    ctx->instance->handleLine(buffer, ctx->start_addr, ctx->size);
}



void EnergyMeterRegInterpreter::handleLine(char* line, uint16_t start, uint16_t size) {
    // 1. Verificación básica (ya no usamos isdigit aquí porque splitString lo hará)
    if (line == nullptr || line[0] == '\0') return;

    reg_EM_750 aux;
    if (splitString(line, ';', aux)) {
        // Validación: ¿Es el primer campo un número? (La dirección)
        if (!isdigit(aux.data[ADDR][0])) return;

        uint16_t addr = (uint16_t)atoi(aux.data[ADDR]);

        if (addr >= start && addr < (start + size)) {
            coded_format formatEnum = stringToFormat(aux.data[FORMAT]);

            if (formatEnum != FORMAT_UNKNOWN && _SDlastRowReadSize < MAX_MODBUS_REGS) {
                int i = _SDlastRowReadSize;

                _SDformatBuffer[i] = formatEnum;
                _SDaddrsBuffer[i] = addr;
                
                // Copiamos el título a la matriz fija
                strncpy(_titulos[i], aux.data[NOTE], MAX_TITLES_SIZE - 1);
                _titulos[i][MAX_TITLES_SIZE - 1] = '\0'; 

                _current_request.size += getFormatSize(formatEnum);
                _SDlastRowReadSize++;
            }
        }
    }
}


// TODO Modificar en donde esta el setup 
// TODO es importante mejorar esto teniendo en cuenta TODO el mapa de registros
// Esta funcion inicia una solicitud de interpretacion de un rango de direcciones. 
EM_request EnergyMeterRegInterpreter::startNewRequest(const uint16_t start_addr, const uint16_t size) {
    // 1. Reiniciar estado para la nueva petición
    _current_request.start_addr = start_addr; 
    _current_request.size = 0; 
    _SDlastRowReadSize = 0; 
    //_lastSizeReadRequestSended = 0;

    if (size > MAX_MODBUS_REGS) return _current_request; 

    // 2. Configurar el contexto para el callback
    // todo esto no debe ir aqui 
    InterpreterContext ctx; 
    ctx.instance = this; 
    ctx.start_addr = start_addr; 
    ctx.size = size; 
    
    // 3. Leer el archivo. El SDManager llamará a staticCallback -> handleLine por cada línea
    if (_sd->isReady()) {
        if(!_sd->getAllLines(_setupFile.c_str(), staticCallback, &ctx)){
                Serial.print("Error puede estar aqui"); 
        }
    }

    // 4. Una vez finalizada la lectura, actualizamos el registro de control
    //_lastSizeReadRequestSended = current_request.size;
    
    return _current_request; 
}

// Devuelve una estructura con la toda la informacion para generar los resultados. 
CompleteDataRegBuffer EnergyMeterRegInterpreter::getDataProcess(const uint16_t* datos, const uint16_t size) {
    CompleteDataRegBuffer res;
    res.buffer = _completeDataR; // Apuntamos al array persistente de la clase
    res.size = 0;

    uint16_t offsetOriginal = 0; // Para rastrear la posición en el array 'datos' raw

    // Recorremos las filas que encontramos previamente en la SD
    for (int i = 0; i < _SDlastRowReadSize; i++) {
        
        coded_format fmt = _SDformatBuffer[i];
        int regs_del_dato = getFormatSize(fmt);

        // Seguridad: No sobrepasar el tamaño del buffer raw recibido
        if (offsetOriginal + regs_del_dato > size) break;

        // Llenamos la estructura en la posición i del array de la clase
        _completeDataR[i].format = fmt;
        
        for (int j = 0; j < regs_del_dato; j++) {
            _completeDataR[i].data[j] = datos[offsetOriginal + j];
        }

        // Si el formato usa menos de MAX_DATA_SIZE, podemos limpiar el resto (opcional)
        for (int j = regs_del_dato; j < MAX_DATA_SIZE; j++) {
            _completeDataR[i].data[j] = 0;
        }

        offsetOriginal += regs_del_dato;
        res.size++;
    }

    return res; 
}

//TODO: para poder hacer este splitString se tiene que cumplir con el formato de registro ADDR;FORMAT;RD_WR;UNIT;NOTE; 
// como minimo NUM_COL_REG_EM750 valores , sino error
bool EnergyMeterRegInterpreter::splitString(char* linea, char div_char, reg_EM_750 &resultado) {
    if (linea == nullptr) return false;

    int col = 0;
    bool dentroDeComillas = false;
    char* ptr = linea;
    
    // El primer campo empieza al inicio de la línea
    resultado.data[col++] = ptr;

    while (*ptr != '\0') {
        if (*ptr == '"') {
            dentroDeComillas = !dentroDeComillas;
        } 
        else if (*ptr == div_char && !dentroDeComillas) {
            // Encontramos un divisor: lo convertimos en fin de cadena
            *ptr = '\0'; 
            
            // El siguiente campo empieza justo después
            if (col < NUM_COL_REG_EM750) {
                resultado.data[col++] = ptr + 1;
            }
        }
        ptr++;
    }

    // Opcional: Podrías añadir lógica aquí para limpiar comillas de cada resultado.data[i]
    return (col == NUM_COL_REG_EM750);
}


// funcion auxiliar para interpretar registros modbus de 16 bits a float
float EnergyMeterRegInterpreter::getFloatConversion(const uint16_t* data){
    if (data == nullptr) return 0.0f;

    float res; 

    uint32_t combinado = ((uint32_t)data[0] << 16) | data[1]; // TODO depende de si es little endian o Big endian

    memcpy(&res, &combinado, sizeof(res)); 

    return res; 
}

titlesBuffer EnergyMeterRegInterpreter::getTitles() {
    titlesBuffer res;
    
    // Solo devolvemos los títulos que pertenecen a la última request procesada
    // Si necesitas filtrar por start_addr aquí, puedes hacerlo, 
    // pero lo normal es que devuelvas lo que ya tienes en el buffer.
    
    for (int i = 0; i < _SDlastRowReadSize; i++) {
        res.buffer[i] = _titulos[i];
    }
    
    res.size = _SDlastRowReadSize;
    return res;
}

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

stringDataEM EnergyMeterRegInterpreter::getStringData() {
    stringDataEM res;
    res.size = 0;

    for (int i = 0; i < _SDlastRowReadSize; i++) {
        coded_format fmt = _completeDataR[i].format;
        uint16_t* d = _completeDataR[i].data;

        switch (fmt) {
            case FLOAT: {
                float f_val = getFloatConversion(d);
                snprintf(_netaData[i], MAX_TITLES_SIZE, "%.2f", f_val);
                break;
            }

            case SHORT: {
                // El cast a int16_t interpreta el bit de signo
                int16_t s_val = (int16_t)d[0];
                snprintf(_netaData[i], MAX_TITLES_SIZE, "%d", s_val);
                break;
            }

            case USHORT:
            case DFLOAT: { // DFLOAT suele ser un entero con decimales implícitos
                snprintf(_netaData[i], MAX_TITLES_SIZE, "%u", d[0]);
                break;
            }

            case INT: {
                // Combinar dos registros de 16 bits en un entero de 32 con signo
                int32_t i_val = (int32_t)(((uint32_t)d[0] << 16) | d[1]);
                snprintf(_netaData[i], MAX_TITLES_SIZE, "%ld", i_val);
                break;
            }

            case UINT: {
                uint32_t ui_val = ((uint32_t)d[0] << 16) | d[1];
                snprintf(_netaData[i], MAX_TITLES_SIZE, "%lu", ui_val);
                break;
            }

            case BYTE: {
                // Normalmente el byte está en la parte baja del registro
                snprintf(_netaData[i], MAX_TITLES_SIZE, "%u", d[0] & 0xFF);
                break;
            }

            case LONG64: {
                // Ojo: printf con uint64_t puede requerir %llu o PRIu64
                uint64_t l_val = ((uint64_t)d[0] << 48) | ((uint64_t)d[1] << 32) | 
                                 ((uint64_t)d[2] << 16) | (uint64_t)d[3];
                snprintf(_netaData[i], MAX_TITLES_SIZE, "%llu", l_val);
                break;
            }

            case STRING: {
                // Copia directamente los registros como caracteres (2 caracteres por registro)
                // Asegúrate de que no desborde MAX_TITLES_SIZE
                memcpy(_netaData[i], d, MAX_DATA_SIZE * 2);
                _netaData[i][MAX_DATA_SIZE * 2] = '\0'; 
                break;
            }

            default:
                snprintf(_netaData[i], MAX_TITLES_SIZE, "n/a");
                break;
        }

        res.buffer[i] = _netaData[i];
        res.size++;
    }

    return res;
}

/*
stringDataEM EnergyMeterRegInterpreter::getStringData() {
    stringDataEM res;
    res.size = 0;

    // Recorremos los registros procesados en la última petición
    for (int i = 0; i < _SDlastRowReadSize; i++) {
        
        // De momento solo gestionamos FLOAT como solicitaste
        if (_completeDataR[i].format == FLOAT) {
            // 1. Convertimos los 2 registros de 16 bits a un float real
            float f_data = EnergyMeterRegInterpreter::getFloatConversion(_completeDataR[i].data);
            
            // 2. Convertimos el float a cadena de texto (char array)
            // "%.2f" limita a 2 decimales. Puedes usar "%f" para más precisión.
            snprintf(_netaData[i], MAX_TITLES_SIZE, "%.2f", f_data);
       
        } else if(_completeDataR[i].format == SHORT) {
            // SHORT Tamaño 1 
            snprintf(_netaData[i], MAX_TITLES_SIZE, "%.2d", _completeDataR[i].data);
            
        } else if(_completeDataR[i].format == INT) {
                //uint32_t combinado = ((uint32_t)data[0] << 16) | data[1]; // TODO depende de si es little endian o Big endian

            uint32_t combinado = ((uint32_t)_completeDataR[i].buffer[0] << 16) | _completeDataR[i].buffer[1];
            snprintf(_netaData[i], MAX_TITLES_SIZE, "%.2d", combinado);

        } else if(_completeDataR[i].format == UINT) {

            uint32_t combinado = ((uint32_t)_completeDataR[i].buffer[0] << 16) | _completeDataR[i].buffer[1];
            snprintf(_netaData[i], MAX_TITLES_SIZE, "%.2d", combinado);

        } else if(_completeDataR[i].format == USHORT){

            snprintf(_netaData[i], MAX_TITLES_SIZE, "%.2d", _completeDataR[i].data);

        } else if(_completeDataR[i].format == BYTE){

            snprintf(_netaData[i], MAX_TITLES_SIZE, "%.2d", _completeDataR[i].data);

        } else if(_completeDataR[i].format == LONG64){

            uint64_t combinado = ((uint64_t)_completeDataR[i].buffer[0] << 48) | ((uint64_t)_completeDataR[i].buffer[1] << 32) | ((uint64_t)_completeDataR[i].buffer[2] << 16) | ((uint16_t)_completeDataR[i].buffer[3]);

        } else if(_completeDataR[i].format == DFLOAT){

            snprintf(_netaData[i], MAX_TITLES_SIZE, "%.2d", _completeDataR[i].data);

        } else if(_completeDataR[i].format == STRING){



        } else {
            // Para otros formatos (futuro), de momento ponemos un placeholder o vacío
            snprintf(_netaData[i], MAX_TITLES_SIZE, "n/a");
        }

        // 3. Asignamos el puntero de nuestra matriz persistente al buffer de salida
        res.buffer[i] = _netaData[i];
        res.size++;
    }

    return res;
}

*/