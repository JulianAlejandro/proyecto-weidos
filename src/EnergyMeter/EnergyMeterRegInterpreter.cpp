
#include "EnergyMeterRegInterpreter.h"
#include <Arduino.h>


// Constructor: Inicializamos el puntero al manager de la SD
EnergyMeterRegInterpreter::EnergyMeterRegInterpreter(SDManager* sdManager) {
    _sd = sdManager;
}

int EnergyMeterRegInterpreter::begin(){
    if (!_sd->begin()) {
        Serial.println(F("Error: No se pudo iniciar la SD desde EnergyMeterRegInterpreter"));
        return false;
    }
}

EM_request EnergyMeterRegInterpreter::startRequest(const uint16_t start_addr, const uint16_t size) {
    EM_request result; 
    result.start_addr = start_addr; 
    result.size = 0; 

    _lastSizeReadRequestSended = 0; 
    
    // 1. IMPORTANTE: Inicializar variables locales
    long total_req_size = 0; 
    int i = 0; 

    if (size > MAX_MODBUS_REGS) return result; 

    String _setupFile = "/example2.txt"; 
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
                    i++; 
                } else {
                    // Si un formato es desconocido, abortamos por seguridad
                    _sd->closeFile(); // No olvides cerrar el archivo antes de salir
                    _SDlastRowReadSize = 0; 
                    result.size = 0;
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
/*
EM_request EnergyMeterRegInterpreter::startRequest (const long start_addr, const long size){

    EM_request result; 
    result.start_addr = start_addr; 
    result.size = 0; 

    if (size > MAX_MODBUS_REGS) return result; // TODO: De momento esta clase solo funciona para modbus

    String _setupFile = "/example2.txt"; // TODO: El nombre de este fichero tiene que entrar en algun sitio externo 
    char lineBuffer[128];
    long total_req_size; 

    if (_sd->openFile(_setupFile.c_str())) { // de cada linea almacenamos alguna informacion adicional
    // Leemos líneas dentro del rango de IDs solicitado
    int i = 0; 
    while (_sd->getNextLineInRange(start_addr, size, lineBuffer, sizeof(lineBuffer))) {
        reg_EM_750 aux;
            
        if (splitString(lineBuffer, ';', aux)) { // devuelve true si ha extraido exactamente NUM_COL_REG_EM750 Columnas
            // Convertimos el String de la columna FORMAT a nuestro Enum
            coded_format formatEnum = stringToFormat(aux.data[FORMAT]);
            
            // Solo lo añadimos si es un formato válidoc:\Users\wm04082\Documents\Arduino\modbus_SD_energi_meter\src\SDManager.h
            if (formatEnum != FORMAT_UNKNOWN) {

                total_req_size = total_req_size + getFormatSize(formatEnum);
                _formatBuffer[i] = formatEnum; 
                i++; 

            }else{
                // error en la solicitud.... se solicita 0
                result.start_addr = start_addr; 
                result.size = 0; 
                //limpiamos el buffer por si acaso para que se note que algo ha ido mal
                _lastReadSize = 0; 
                return result
            }
        }else{
            result.start_addr = start_addr; 
            result.size = 0; 
            //limpiamos el buffer por si acaso para que se note que algo ha ido mal
            _lastReadSize = 0; 
            return result
        }
            
            // Seguridad: Si ya tenemos suficientes, paramos
           // if (result.size() >= size) break;
        }
        _sd->closeFile();
    }

    _lastReadSize = i; 
    result.size = total_req_size; 

    return result; 
}
*/
/*
std::vector<coded_format> EnergyMeterRegInterpreter::devuelveRegData(long start_addr, long size) {

    //std::vector<coded_format> result;
    
    // Reservar espacio ayuda a evitar múltiples reasignaciones de memoria en el heap
    result.reserve(size); 

    //TODO
    String _setupFile = "/example2.txt"; 
    char lineBuffer[128];

    if (_sd->openFile(_setupFile.c_str())) {
        // Leemos líneas dentro del rango de IDs solicitado
        while (_sd->getNextLineInRange(start_addr, size, lineBuffer, sizeof(lineBuffer))) {
            reg_EM_750 aux;
            
            if (splitString(lineBuffer, ';', aux)) {
                // Convertimos el String de la columna FORMAT a nuestro Enum
                coded_format formatEnum = stringToFormat(aux.data[FORMAT]);
                
                // Solo lo añadimos si es un formato válido
                if (formatEnum != FORMAT_UNKNOWN) {
                    result.push_back(formatEnum);
                }
            }
            
            // Seguridad: Si ya tenemos suficientes, paramos
            if (result.size() >= size) break;
        }
        _sd->closeFile();
    }
    
    return result; 
}
*/

float* EnergyMeterRegInterpreter::getFloatValues(const uint16_t* rawValues, const uint16_t size_rawValues){
    
    // este dato COMO MUCHO puede 
    int idx_datos = 0; 
    if (size_rawValues == _lastSizeReadRequestSended){ // lo que ha leido modbus es lo mismo que este objeto le dijo que leyera
        
        for( int i = 0; i < _SDlastRowReadSize; i++){
            if(_SDformatBuffer[i] == FLOAT){
                uint32_t combinado = ((uint32_t)rawValues[(_SDaddrsBuffer[i] - _SDaddrsBuffer[0])] << 16) | rawValues[(_SDaddrsBuffer[i] - _SDaddrsBuffer[0]) + 1];
                
                memcpy(&dataFloat[idx_datos], &combinado, sizeof(dataFloat[idx_datos]));
                idx_datos = idx_datos + 1; 
            }
        }

    }
    _sizeData = idx_datos; 
    return dataFloat; 
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
