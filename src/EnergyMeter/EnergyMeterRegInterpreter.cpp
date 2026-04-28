#include "EnergyMeterRegInterpreter.h"
#include <Arduino.h>

struct InterpreterContext {
    EnergyMeterRegInterpreter* instance;
    uint16_t start_addr;
    uint16_t size;
};

/*
// Estructura interna para no mezclar con el flujo normal de datos
struct TitleContext {
    EnergyMeterRegInterpreter* instance;
    uint16_t start;
    uint16_t size;
    TitleHandler externalHandler;
    void* externalArg;
};
*/

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
/*
void EnergyMeterRegInterpreter::staticCallback(const char* line, void* context) {
    // Convertimos el puntero genérico de nuevo a nuestro objeto
    InterpreterContext* ctx = (InterpreterContext*)context;

    // 2. Usamos la instancia para llamar al manejador, pasando los datos extra
    ctx->instance->handleLine(line, ctx->start_addr, ctx->size);
}
*/

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

/*
// Cambiamos a char* line para permitir la tokenización in-situ
void EnergyMeterRegInterpreter::handleLine(char* line, uint16_t start, uint16_t size) {
    if (line == nullptr || line[0] == '\0' || !isdigit(line[0])) return;

    reg_EM_750 aux;
    if (splitString(line, ';', aux)) {
        // Convertimos el puntero de la columna ADDR a entero
        uint16_t addr = (uint16_t)atoi(aux.data[ADDR]);

        if (addr >= start && addr < (start + size)) {
            // Convertimos el puntero de FORMAT a nuestro Enum
            // (Tendrás que ajustar stringToFormat para que acepte const char*)
            coded_format formatEnum = stringToFormat(aux.data[FORMAT]);

            if (formatEnum != FORMAT_UNKNOWN && _SDlastRowReadSize < MAX_MODBUS_REGS) {
                int i = _SDlastRowReadSize;

                _SDformatBuffer[i] = formatEnum;
                _SDaddrsBuffer[i] = addr;
                
                // Copiamos el texto de la columna NOTE a nuestra matriz fija
                // aux.data[NOTE] es ahora un puntero a una parte de 'line'
                strncpy(_titulos[i], aux.data[NOTE], MAX_TITLES_SIZE - 1);
                _titulos[i][MAX_TITLES_SIZE - 1] = '\0'; 

                _current_request.size += getFormatSize(formatEnum);
                _SDlastRowReadSize++;
            }
        }
    }
}
*/
/*
// función que recibe lineas del mapa de registro y obtiene informacion
void EnergyMeterRegInterpreter::handleLine(const char* line, uint16_t start, uint16_t size) {
    // 1. Verificación básica: Si la línea está vacía o es un comentario (opcional)
    if (line == nullptr || line[0] == '\0' || !isdigit(line[0])) return;

    reg_EM_750 aux;
    // 2. Intentar dividir la cadena
    if (splitString(line, ';', aux)) {
        uint16_t addr = (uint16_t)aux.data[ADDR].toInt();

        // 3. Filtrar: ¿Está la dirección dentro del rango solicitado?
        if (addr >= start && addr < (start + size)) {
            coded_format formatEnum = stringToFormat(aux.data[FORMAT]);

            // 4. Validar formato y espacio en el buffer
            if (formatEnum != FORMAT_UNKNOWN && _SDlastRowReadSize < MAX_MODBUS_REGS) {
                int i = _SDlastRowReadSize; // Índice actual

                // Guardamos los metadatos del registro
                _SDformatBuffer[i] = formatEnum;
                _SDaddrsBuffer[i] = addr;
                
                // Manejo de títulos (String persistente para que el const char* no apunte a basura)
                //_titulosPersistentes[i] = aux.data[NOTE];
                //_titulosBuffer[i] = _titulosPersistentes[i].c_str();

                // Actualizamos el tamaño total de registros Modbus que se pedirán
                _current_request.size += getFormatSize(formatEnum);
                
                // Incrementamos el contador de filas válidas encontradas
                _SDlastRowReadSize++;
            }
        }
    }
}
*/
// TODO Modificar en donde esta el setup 
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

/*
bool EnergyMeterRegInterpreter::getTitles(const uint16_t start_addr, const uint16_t size, TitleHandler handler, void* arg) {
    // 1. Verificaciones de seguridad previas
    if (_sd == nullptr || !_sd->isReady()) {
        Serial.println(F("Error: SDManager no inicializado o no listo."));
        return false;
    }

    if (handler == nullptr) {
        Serial.println(F("Error: TitleHandler (callback) es nulo."));
        return false;
    }

    // 2. Preparar contexto
    TitleContext ctx = {this, start_addr, size, handler, arg};

    // 3. Ejecutar lectura y capturar el estado del SDManager
    // getAllLines debería devolver true si pudo abrir el archivo satisfactoriamente
    bool lecturaExitosa = _sd->getAllLines(_setupFile.c_str(), [](const char* line, void* context) {
        TitleContext* tCtx = (TitleContext*)context;
        
        if (line == nullptr || line[0] == '\0' || !isdigit(line[0])) return;

        reg_EM_750 aux;
        if (tCtx->instance->splitString(line, ';', aux)) {
            uint16_t addr = (uint16_t)aux.data[ADDR].toInt();
            
            if (addr >= tCtx->start && addr < (tCtx->start + tCtx->size)) {
                tCtx->externalHandler(aux.data[NOTE].c_str(), tCtx->externalArg);
            }
        }
    }, &ctx);

    if (!lecturaExitosa) {
        Serial.print(F("Error: No se pudo leer el archivo de configuracion: "));
        Serial.println(_setupFile);
        return false;
    }

    return true;
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


stringDataEM EnergyMeterRegInterpreter::getData() {
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
        } 
        else {
            // Para otros formatos (futuro), de momento ponemos un placeholder o vacío
            snprintf(_netaData[i], MAX_TITLES_SIZE, "n/a");
        }

        // 3. Asignamos el puntero de nuestra matriz persistente al buffer de salida
        res.buffer[i] = _netaData[i];
        res.size++;
    }

    return res;
}