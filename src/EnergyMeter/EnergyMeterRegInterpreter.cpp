
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
    // Convertimos el puntero genérico de nuevo a nuestro objeto
    InterpreterContext* ctx = (InterpreterContext*)context;

    // 2. Usamos la instancia para llamar al manejador, pasando los datos extra
    ctx->instance->handleLine(line, ctx->start_addr, ctx->size);
}

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
                _titulosPersistentes[i] = aux.data[NOTE];
                _titulosBuffer[i] = _titulosPersistentes[i].c_str();

                // Actualizamos el tamaño total de registros Modbus que se pedirán
                current_request.size += getFormatSize(formatEnum);
                
                // Incrementamos el contador de filas válidas encontradas
                _SDlastRowReadSize++;
            }
        }
    }
}

EM_request EnergyMeterRegInterpreter::startNewRequest(const uint16_t start_addr, const uint16_t size) {
    // 1. Reiniciar estado para la nueva petición
    current_request.start_addr = start_addr; 
    current_request.size = 0; 
    _SDlastRowReadSize = 0; 
    _lastSizeReadRequestSended = 0;

    if (size > MAX_MODBUS_REGS) return current_request; 

    // 2. Configurar el contexto para el callback
    String _setupFile = "/example2.txt"; 
    InterpreterContext ctx; 
    ctx.instance = this; 
    ctx.start_addr = start_addr; 
    ctx.size = size; 
    
    // 3. Leer el archivo. El SDManager llamará a staticCallback -> handleLine por cada línea
    if (_sd->isReady()) {
        _sd->getAllLines(_setupFile.c_str(), staticCallback, &ctx);
    }

    // 4. Una vez finalizada la lectura, actualizamos el registro de control
    _lastSizeReadRequestSended = current_request.size;
    
    return current_request; 
}
/*
//TODO mejorar esta funcion, hay long y mas cosas hardcodeadas y sin sentidos 
EM_request EnergyMeterRegInterpreter::startNewRequest(const uint16_t start_addr, const uint16_t size) {
    EM_request result; 
    result.start_addr = start_addr; 
    result.size = 0; 
    _lastSizeReadRequestSended = 0; 
    
    // 1. IMPORTANTE: Inicializar variables locales 
    int i = 0; 
    long total_req_size = 0;

    if (size > MAX_MODBUS_REGS) return result; 

    String _setupFile = "/example2.txt";  // TODO , modificar donde esta esto
    char lineBuffer[128]; 

    if (_sd->openFile(_setupFile.c_str())) {
        // 2. Añadimos i < MAX_MODBUS_REGS por seguridad de memoria
        while (i < MAX_MODBUS_REGS && _sd->getNextLineInRange(start_addr, size, lineBuffer, sizeof(lineBuffer))) {
            reg_EM_750 aux;
                
            if (splitString(lineBuffer, ';', aux)) {
                coded_format formatEnum = stringToFormat(aux.data[FORMAT]);
                
                if (formatEnum != FORMAT_UNKNOWN) {
                    total_req_size += getFormatSize(formatEnum);
                    _SDformatBuffer[i] = formatEnum; 
                    _SDaddrsBuffer[i] = aux.data[ADDR].toInt();
                    //aux.data[NOTE]// Actualizar el buffer de titulos tambien (en NOTE estan los titulos)

                    //Actualizacion de titulos 
                    _titulosPersistentes[i] = aux.data[NOTE];
                    _titulosBuffer[i] = _titulosPersistentes[i].c_str();
                    i++; 
                } else {
                    // Si un formato es desconocido, abortamos por seguridad
                    _sd->closeFile(); // No olvides cerrar el archivo antes de salir
                    _SDlastRowReadSize = 0;  // TODO. pensar
                    result.size = 0; // TODO. pensar
                    return result; // Faltaba ;
                }
            } else {
                _sd->closeFile();
                _SDlastRowReadSize = 0; 
                result.size = 0;
                return result; // Faltaba ;
            }
        }
        _sd->closeFile();
    }

    // 3. Guardamos cuántos tipos de datos (formatos) procesamos
    _SDlastRowReadSize = i; 
    result.size = total_req_size; 

    _lastSizeReadRequestSended = total_req_size; 
    return result; 
}

*/

netFloatDataBuffer EnergyMeterRegInterpreter::getFloatValues(const uint16_t* rawValues, const uint16_t size_rawValues){
    
    // este dato COMO MUCHO puede 
    int idx_datos = 0; 
    if (size_rawValues == _lastSizeReadRequestSended){ // lo que ha leido modbus es lo mismo que este objeto le dijo que leyera
        
        for( int i = 0; i < _SDlastRowReadSize; i++){
            if(_SDformatBuffer[i] == FLOAT){
                uint32_t combinado = ((uint32_t)rawValues[(_SDaddrsBuffer[i] - _SDaddrsBuffer[0])] << 16) | rawValues[(_SDaddrsBuffer[i] - _SDaddrsBuffer[0]) + 1];
                
                memcpy(&_dataFloat[idx_datos], &combinado, sizeof(_dataFloat[idx_datos]));
                idx_datos = idx_datos + 1; 
            }
        }

    }
    _sizeData = idx_datos; 
    return {_dataFloat, _sizeData}; 
}


//TODO: para poder hacer este splitString se tiene que cumplir con el formato de registro ADDR;FORMAT;RD_WR;UNIT;NOTE; 
// como minimo NUM_COL_REG_EM750 valores , sino error
bool EnergyMeterRegInterpreter::splitString(const char* linea, char div_char, reg_EM_750 &resultado) {
    // 1. Limpieza preventiva del struct
    for (int i = 0; i < NUM_COL_REG_EM750; i++) {
        resultado.data[i] = "";
    }

    if (linea == nullptr) return false;

    int posInicio = 0;
    int col = 0;
    bool dentroDeComillas = false;
    int n = strlen(linea);

    // 2. Recorremos la línea carácter por carácter
    for (int i = 0; i <= n; i++) {
        char c = linea[i]; // El carácter actual (incluyendo el '\0' al final)

        // Detectar si entramos o salimos de una zona de comillas
        if (c == '"') {
            dentroDeComillas = !dentroDeComillas;
        }

        // Si encontramos el divisor (y no estamos en comillas) O llegamos al final de la cadena (\0)
        if ((c == div_char && !dentroDeComillas) || c == '\0') {
            if (col < NUM_COL_REG_EM750) {
                // Calculamos la longitud del segmento
                int longitudSegmento = i - posInicio;
                
                // Creamos un String temporal a partir del buffer de caracteres
                // Esto es eficiente porque solo extraemos el trozo necesario
                String segmento = "";
                segmento.reserve(longitudSegmento); // Opcional: optimiza memoria
                for (int j = 0; j < longitudSegmento; j++) {
                    segmento += linea[posInicio + j];
                }
                
                segmento.trim();
                
                // Eliminar las comillas exteriores si existen
                if (segmento.startsWith("\"") && segmento.endsWith("\"")) {
                    segmento = segmento.substring(1, segmento.length() - 1);
                }
                
                resultado.data[col] = segmento;
                col++;
            }
            posInicio = i + 1;
        }
    }

    return (col == NUM_COL_REG_EM750);
}

/**
 * Función auxiliar para saber cuántos registros Modbus ocupa cada formato
 */
int EnergyMeterRegInterpreter::getFormatSize(coded_format f) {
    switch (f) {
        case FLOAT:
        case INT:
        case UINT:
            return 2; // 32 bits = 2 registros Modbus
        case LONG64:
            return 4; // 64 bits = 4 registros Modbus
        case SHORT:
        case USHORT:
        case BYTE:
        case DFLOAT:
            return 1; // 16 bits = 1 registro Modbus
        default:
            return 0;
    }
}
