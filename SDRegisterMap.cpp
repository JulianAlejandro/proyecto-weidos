

#include "SDRegisterMap.h"
#include <Arduino.h>

// Constructor: Inicializamos el puntero al manager de la SD
SDRegisterMap::SDRegisterMap(SDManager* sdManager) {
    _sd = sdManager;
}

int SDRegisterMap::begin(){
    if (!_sd->begin()) {
        Serial.println(F("Error: No se pudo iniciar la SD desde SDRegisterMap"));
        return false;
    }
}

#include "SDRegisterMap.h"

std::vector<coded_format> SDRegisterMap::devuelveRegData(long start_addr, long size) {
    std::vector<coded_format> result;
    
    // Reservar espacio ayuda a evitar múltiples reasignaciones de memoria en el heap
    result.reserve(size); 

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


bool SDRegisterMap::splitString(const char* linea, char div_char, reg_EM_750 &resultado) {
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
