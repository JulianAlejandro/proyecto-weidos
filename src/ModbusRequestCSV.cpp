#include "ModbusRequestCSV.h"
#include <CSV_Parser.h>


ModbusRequestCSV::ModbusRequestCSV(SDManager* sdManager) {
    _sd = sdManager;
}

bool ModbusRequestCSV::begin(){
   // analizar si el SD ya esta iniciado 

    if (!_sd->isReady()) { 
       // Serial.println("Modbus Request: SDManager no está listo aún.");
        return false;
    }
    _initialized = true;
    return true;

   /*
    if (!_sd->begin()) {
        Serial.println(F("Error: No se pudo iniciar la SD desde EnergyMeterRegInterpreter"));
        return false;
    }
    */
}

bool ModbusRequestCSV::loadFromSDParameters() {

    if(!_initialized) return false; 
    // 1. Usamos false en has_header para que la primera línea TAMBIÉN cuente como fila.
    // Usamos "sssss" porque tu CSV tiene 5 columnas (4 separadores ';')
    CSV_Parser cp("sssss", false, ';');
    
    bool flag = _sd->withFile(MODBUS_REQ_FILE, [](Stream& file, void* arg) {
        CSV_Parser* parser = (CSV_Parser*)arg;
        for (int i = 0; i < FIRST_BLOCK; i++) {
            if (file.available()) {
                String line = file.readStringUntil('\n');
                line.trim(); // Limpia \r y espacios
                if (line.length() > 0) {
                    line += "\n"; 
                    *parser << line.c_str();
                }
            } else {
                break;
            }
        }
    }, &cp);

    if (!flag) {
        //Serial.println("Error: No se pudo abrir el archivo.");
        return false;
    }

    int rows = cp.getRowsCount();
    //Serial.print("Filas totales detectadas: "); Serial.println(rows);

    // Según tu CSV con has_header = false:
    // Fila 0: "Modbus Client Requests;;;;"
    // Fila 1: "Device Name;EM 750;;;"
    // Fila 2: "IP address;192.168.0.202;;;"

    if (rows < 3) {
        //Serial.println("Error: No hay filas suficientes en el CSV.");
        return false; 
    } 

    // Acceso directo a la columna 1 (donde están los valores)
    char **values = (char**)cp[1];

    if (values != nullptr) {
        // El Device Name está en la FILA 1
        if (values[1] != nullptr) {
            strncpy(_device_name, values[1], MAX_TITLES_SIZE - 1);
            _device_name[MAX_TITLES_SIZE - 1] = '\0';
            //Serial.print("Device Name cargado: "); Serial.println(_device_name);
        }

        // El IP Address está en la FILA 2
        if (values[2] != nullptr) {
            strncpy(_ip_address, values[2], MAX_TITLES_SIZE - 1);
            _ip_address[MAX_TITLES_SIZE - 1] = '\0';
            //Serial.print("IP Address cargada: "); Serial.println(_ip_address);
        }
        
        return true;
    }

    return false;
}


Struct_MBRequest ModbusRequestCSV::loadFromSDMbrequest() {
    

    Struct_MBRequest request = {0, 0, 0, 0, 0}; // Inicializamos a cero
    
    if(!_initialized) return request; 

    // Usamos "sssss" para leer strings y luego convertirlos
    CSV_Parser cp("sssss", false, ';');

    bool flag = _sd->withFile(MODBUS_REQ_FILE, [](Stream& file, void* arg) {
        CSV_Parser* parser = (CSV_Parser*)arg;
        // Leemos hasta FIRST_BLOCK (que debe ser >= 8)
        for (int i = 0; i < FIRST_BLOCK; i++) {
            if (file.available()) {
                String line = file.readStringUntil('\n');
                line.trim();
                if (line.length() > 0) {
                    line += "\n"; 
                    *parser << line.c_str();
                }
            } else {
                break;
            }
        }
    }, &cp);

    int rows = cp.getRowsCount();
    
    // La fila 8 del CSV es el índice 7 para el parser
    if (flag && rows >= 8) {
        // Obtenemos los punteros de cada columna
        char **col0 = (char**)cp[0]; // Channel
        char **col1 = (char**)cp[1]; // Start Address
        char **col2 = (char**)cp[2]; // Length
        char **col3 = (char**)cp[3]; // Function Code
        char **col4 = (char**)cp[4]; // Interval

        // Verificamos que la fila 7 (Fila 8 del CSV) tenga datos
        if (col0[7] && col1[7] && col2[7] && col3[7] && col4[7]) {
            request.channel         = (uint16_t)atoi(col0[7]);
            request.start_addres    = (uint16_t)atoi(col1[7]);
            request.length          = (uint16_t)atoi(col2[7]);
            request.func_code       = (uint16_t)atoi(col3[7]);
            request.req_interval_ms = (uint16_t)atoi(col4[7]);
        }
    }

    return request;
}
