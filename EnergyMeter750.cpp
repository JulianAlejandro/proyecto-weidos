
#include "EnergyMeter750.h"
#include "SDRegisterMap.h"

EnergyMeter750::EnergyMeter750(uint8_t slaveAddress) {
    _slaveAddress = slaveAddress;
}

int EnergyMeter750::begin(ModbusTCPClient* modbusClient) {
    _modbus = modbusClient;
     //_sd = sdManager;

/*
    if (!_sd->begin()) {
        Serial.println(F("Error: No se pudo iniciar la SD desde Datalogger"));
        return false;
    }
*/

/*
    // 2. Verificamos si el archivo de configuración existe usando el manager
    if (!_sd->exists(_setupFile.c_str())) {
        Serial.print(F("Warning: Setup file not found: "));
        Serial.println(_setupFile);
        // Aquí podrías llamar a _sd->appendLine para crear un archivo base si quisieras
    }*/
   
    return true;
}

bool EnergyMeter750::readAndProcess_2(long start_addr, long size, SDRegisterMap* mapa, void (*callback)(float)) {
    if (mapa == nullptr || _modbus == nullptr) return false;

    // PASO 1: Obtener el mapa de formatos desde la SD
    std::vector<coded_format> formatos = mapa->devuelveRegData(start_addr, size);
    if (formatos.empty()) return false;

    // PASO 2: Calcular cuántos registros Modbus (16-bit cada uno) necesitamos pedir
    int modbus_data_size = 0;
    for (coded_format f : formatos) {
        modbus_data_size += getFormatSize(f);
    }

    // PASO 3: Petición Modbus única para todo el bloque
    std::vector<uint16_t> rawValues;
    rawValues.reserve(modbus_data_size);

    if (_modbus->requestFrom(_slaveAddress, INPUT_REGISTERS, start_addr, modbus_data_size)) {
        while (_modbus->available()) {
            rawValues.push_back(_modbus->read());
        }
    } else {
        return false; // Error de comunicación Modbus
    }

    // PASO 4: Procesar y convertir los datos según el formato
    int idx = 0; // Índice para movernos por el array rawValues
    for (coded_format f : formatos) {
        
        if (f == FLOAT) {
            if (idx + 1 < rawValues.size()) {
                // Combinamos dos registros de 16 bits en uno de 32
                uint32_t combinado = ((uint32_t)rawValues[idx] << 16) | rawValues[idx + 1];
                float resultado;
                memcpy(&resultado, &combinado, sizeof(resultado));
                
                callback(resultado);
                idx += 2; // Avanzamos 2 registros
            }
        } 
        else if (f == INT || f == UINT) {
            // Ejemplo para otros formatos de 32 bits (2 registros)
            idx += 2;
        }
        else if (f == SHORT || f == USHORT || f == BYTE) {
            // Ejemplo para formatos de 16 bits (1 registro)
            // float val = (float)rawValues[idx];
            // callback(val);
            idx += 1;
        }
        // ... añadir más casos según necesites
    }

    return true;
}

/**
 * Función auxiliar para saber cuántos registros Modbus ocupa cada formato
 */
int EnergyMeter750::getFormatSize(coded_format f) {
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

/*
//TODO AQUI _setupFile.c_str() va hardcoded porque esta clase de momento no recibe el nombre del fichero de setup
bool EnergyMeter750::readAndProcess_2(long start, long end, void (*callback)(float)){ // este creo que es el que devuelve el callback, esta es la funcion que se llama desde el cliente y devuelve un stream
     Serial.print("entramos 1");
     String _setupFile = "/example2.txt";

    //int size = 0; pensar en si puede estar aqui en otro lado 
    //void getLinesByRange(const char* path, long start, long end, LineCallback callback);

    modbus_data_size = 0; // variable local del objeto
    registros.clear(); 
    idx_registros = 0; 

    _sd->getLinesByRange(_setupFile.c_str(), start, end, miProcesadorDeRegistros);

    //registros esta actualizado

    //miProcesador devuelve el valor de size
   // ModbusBlock aux_MB; 
    //aux_MB.startAddress = start; 
    //aux_MB.quantity = modbus_data_size; // este size es devuelto por miProcesadorDeRegistros
    
    //return readAndProcess(aux_MB, procesarRegistroIndividual)
    std::vector<uint32_t> val; 
    val.clear();
    if (_modbus->requestFrom(_slaveAddress, INPUT_REGISTERS, start, modbus_data_size)) {
        for (int i = 0; i < modbus_data_size; i++) {
            val.push_back(_modbus->read());
            // Enviamos la dirección actual y el valor al callback (que será el logger) 
        }

       // callback(block.startAddress + i, val);
       // return true;
    }

    //return false;

//TODO
    int idx = 0;
    //std::vector<String> filaParaSD;

    for(int i = 0; i < registros.size(); i++){
      idx = registros[i].data[ADDR].toInt() - start;
      uint32_t combinado = 0; 
      float resultado;

      if(registros[i].data[FORMAT] == "FLOAT"){
        combinado = ((uint32_t)val[idx] << 16) | val[idx+1];
        memcpy(&resultado, &combinado, sizeof(resultado));
      //  filaParaSD.push_back(String(resultado));
         callback(resultado);
      }
    }
    registros.clear(); 
   
}
*/
/*
bool EnergyMeter750::splitString(const char* linea, char div_char, reg_EM_750 &resultado) {
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
*/