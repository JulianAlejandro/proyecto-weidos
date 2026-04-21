
#include "EnergyMeter750.h"


EnergyMeter750::EnergyMeter750(uint8_t slaveAddress) {
    _slaveAddress = slaveAddress;
}

int EnergyMeter750::begin(SDManager* sdManager, ModbusTCPClient* modbusClient) {
    _modbus = modbusClient;
     _sd = sdManager;

    if (!_sd->begin()) {
        Serial.println(F("Error: No se pudo iniciar la SD desde Datalogger"));
        return false;
    }

/*
    // 2. Verificamos si el archivo de configuración existe usando el manager
    if (!_sd->exists(_setupFile.c_str())) {
        Serial.print(F("Warning: Setup file not found: "));
        Serial.println(_setupFile);
        // Aquí podrías llamar a _sd->appendLine para crear un archivo base si quisieras
    }*/
   
    return true;
}

//TODO: MODIFICAR  
bool EnergyMeter750::readAndProcess_2(long start, long end, void (*callback)(float)) {
    String _setupFile = "/example2.txt"; // 
    
    int modbus_data_size = 0;
    std::vector<reg_EM_750> registros;
    char lineBuffer[128];

    // PASO 1: Escanear SD para saber cuántos registros pedir y guardarlos
    if (_sd->openFile(_setupFile.c_str())) {
        while (_sd->getNextLineInRange(start, end, lineBuffer, sizeof(lineBuffer))) {
            reg_EM_750 aux;
            if (splitString(lineBuffer, ';', aux)) {
                modbus_data_size += getFormatSize(aux.data[FORMAT]);
                registros.push_back(aux);
            }
        }
        _sd->closeFile();
    }

    // PASO 2: Petición Modbus única
    std::vector<uint16_t> val; 
    if (_modbus->requestFrom(_slaveAddress, INPUT_REGISTERS, start, modbus_data_size)) {
        while (_modbus->available()) {
            val.push_back(_modbus->read());
        }
    } else {
        return false; // Error de comunicación
    }

    // PASO 3: Procesar los datos obtenidos con la lista de registros guardada
    for (const auto& reg : registros) {
        int idx = reg.data[ADDR].toInt() - start;
        
        if (reg.data[FORMAT] == "FLOAT" && (idx + 1) < val.size()) {
            uint32_t combinado = ((uint32_t)val[idx] << 16) | val[idx+1];
            float resultado;
            memcpy(&resultado, &combinado, sizeof(resultado));
            callback(resultado);
        }
        // Aquí puedes añadir más formatos (INT, LONG...) fácilmente
    }

    return true;
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
