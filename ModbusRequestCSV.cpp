#include "ModbusRequestCSV.h"
#include <CSV_Parser.h>


ModbusRequestCSV::ModbusRequestCSV(SDManager* sdManager) {
    _sd = sdManager;
    //_setupFile = SETUP_FILE;
}

bool ModbusRequestCSV::begin(){
   // analizar si el SD ya esta iniciado 

    if (!_sd->isReady()) { 
       // Serial.println("Modbus Request: SDManager no está listo aún.");
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

bool ModbusRequestCSV::loadFromSDParameters() {
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

/*
bool ModbusRequestCSV::loadFromSDParameters() {
    // Definimos el formato: 5 columnas de strings
    CSV_Parser cp("sssss", true, ';');
    bool flag = false; 
    flag = _sd->withFile(MODBUS_REQ_FILE, [](Stream& file, void* arg) {
        CSV_Parser* parser = (CSV_Parser*)arg;
        for (int i = 0; i < FIRST_BLOCK; i++) {
            if (file.available()) {
                String line = file.readStringUntil('\n');
                if (line.length() > 0) {
                    line += "\n"; 
                    *parser << line.c_str();
                }
            }
        }
    }, &cp);

    if(!flag){
        Serial.println("algo paso"); 
    }

    // Verificamos que tenemos filas suficientes
    int rows = cp.getRowsCount();
    if (rows < 3){
        Serial.println("no filas suficientes");
        return false; // No hay suficientes líneas para Device e IP
    } 

    // En CSV_Parser, el acceso cp[columna][fila] suele ser más estable
    // Columna 0: Keys (Device Name, IP address)
    // Columna 1: Values (EM 750, 192.168.0.202)
    
    char **keys   = (char**)cp[0];
    char **values = (char**)cp[1];

    if (keys && values) {
        // Buscamos en la fila 1 (Device Name)
        if (rows > 1 && values[1]) {
            strncpy(_device_name, values[1], MAX_TITLES_SIZE - 1);
            _device_name[MAX_TITLES_SIZE - 1] = '\0';
        }

        // Buscamos en la fila 2 (IP address)
        if (rows > 2 && values[2]) {
            strncpy(_ip_address, values[2], MAX_TITLES_SIZE - 1);
            _ip_address[MAX_TITLES_SIZE - 1] = '\0';
        }
        
        return true;
    }

    return false;
}
*/
/*
bool ModbusRequestCSV::loadFromSDParameters(){
 
    CSV_Parser cp("sssss", true, ';');

    // Usamos withFile para obtener el stream de forma segura
    _sd->withFile(MODBUS_REQ_FILE, [](Stream& file, void* arg) {
        CSV_Parser* parser = (CSV_Parser*)arg;
        
        // Leemos solo hasta LINE_MAP_START (línea 4)
        for (int i = 0; i < FIRST_BLOCK; i++) {
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
    
    char **names  = (char**)cp[ (int)0 ]; 
    char **values = (char**)cp[ (int)1 ]; 

    if (values != nullptr && cp.getRowsCount() >= FIRST_BLOCK-1) {
     
        if (values[0]) {
            strncpy(_device_name, values[0], MAX_TITLES_SIZE - 1);
            _device_name[MAX_TITLES_SIZE - 1] = '\0';
        }
        
        if (values[1]) {
            strncpy(_ip_address, values[1], MAX_TITLES_SIZE - 1);
            _ip_address[MAX_TITLES_SIZE - 1] = '\0';
        }
        
       // if (values[2]) {
        //    strncpy(_max_files, values[2], MAX_TITLES_SIZE - 1);
       //     _max_files[MAX_TITLES_SIZE - 1] = '\0';
       // }
    }

}
*/
