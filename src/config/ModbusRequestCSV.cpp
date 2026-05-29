#include "ModbusRequestCSV.h"
#include <CSV_Parser.h>

const char* ModbusRequestCSV::TAG = "MB_CSV";

ModbusRequestCSV::ModbusRequestCSV(SDManager* sdManager) {
    _sd = sdManager;
    _requests_count = 0; 

    for (int i = 0; i < MAX_MODBUS_REQUESTS_ROWS; i++) {
        _requests_table[i] = {0, 0, 0, 0, 0};
    }

}

esp_err_t ModbusRequestCSV::begin() {
    if (!_sd->isReady()) { 
        return ESP_ERR_SD_NOT_INIT;
    }
    _initialized = true;
    return ESP_OK;
}

/**
 * @brief Loads metadata (Device Name, IP) from the CSV.
 * @note Uses 'has_header = false' so the first line is treated as data.
 */
esp_err_t ModbusRequestCSV::loadFromSDParameters() {
    if (!_initialized) return ESP_ERR_CONFIG_NOT_INIT; 
    
    CSV_Parser cp("sssss", false, ';');
    
    // Capturamos el error de la SD
    esp_err_t err = _sd->withFile(MODBUS_REQ_FILE, [](Stream& file, void* arg) {
        CSV_Parser* parser = (CSV_Parser*)arg;
        for (int i = 0; i < FIRST_BLOCK; i++) {
            if (file.available()) {
                String line = file.readStringUntil('\n');
                line.trim();
                if (line.length() > 0) {
                    line += "\n"; 
                    *parser << line.c_str();
                }
            } else break;
        }
    }, &cp);

    // Si la SD falló (archivo no existe, etc.), propagamos ese error
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Fallo SD al leer parametros...");
        return err;
    }
    int rows = cp.getRowsCount();
    if (rows < 3) {
        ESP_LOGE(TAG, "CSV insuficiente: se esperaban 3 filas, hay %d", rows);
        return ESP_ERR_CONFIG_PARSE_FAIL;
    }

    char **values = (char**)cp[1];
    if (values == nullptr) return ESP_ERR_CONFIG_INVALID_DATA;

    if (values[1]) strncpy(_device_name, values[1], MAX_TITLES_SIZE - 1);
    if (values[2]) strncpy(_ip_address, values[2], MAX_TITLES_SIZE - 1);

    return ESP_OK;
}

/**
 * @brief Extracts the Modbus request configuration from Row 8 (Index 7) of the CSV.
 */
/**
 * @brief Extracts the FIRST Modbus request configuration row dynamically using headers.
 */
esp_err_t ModbusRequestCSV::loadFromSDMbrequests() {
    if (!_initialized) return ESP_ERR_CONFIG_NOT_INIT; 
    //if (out_request == nullptr) return ESP_ERR_INVALID_ARG;

    // Inicializar estructura de salida y contador interno por seguridad
    //*out_request = {0, 0, 0, 0, 0};
    _requests_count = 0;

    CSV_Parser cp("sssss", true, ';');

    esp_err_t err = _sd->withFile(MODBUS_REQ_FILE, [](Stream& file, void* arg) {
        CSV_Parser* parser = (CSV_Parser*)arg;
        int current_line = 0;

        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();

            // Saltamos las primeras 6 líneas de metadatos
            if (current_line < 6) {
                current_line++;
                continue; 
            }

            if (line.length() > 0) {
                line += "\n"; 
                *parser << line.c_str(); 
            }
            current_line++;
        }
    }, &cp);

    if (err != ESP_OK){
        ESP_LOGE(TAG, "Fallo SD al leer MB Request...");
        return err;
    }

    int data_rows = cp.getRowsCount();
    if (data_rows < 1) {
        ESP_LOGE(TAG, "No se encontraron filas de datos Modbus en la tabla.");
        return ESP_ERR_CONFIG_PARSE_FAIL;
    }

    // Vinculación por nombre de columna (independiente de la posición)
    char **col_channel   = (char**)cp["Channel"];
    char **col_start     = (char**)cp["Start Adress"];
    char **col_length    = (char**)cp["Length"];
    char **col_func      = (char**)cp["Function Code"];
    char **col_interval  = (char**)cp["Request Interval (ms)"];

    // Validar que existan todas las columnas requeridas en el archivo
    if (!col_channel || !col_start || !col_length || !col_func || !col_interval) {
        ESP_LOGE(TAG, "Faltan columnas requeridas en el archivo CSV");
        return ESP_ERR_CONFIG_INVALID_DATA;
    }

    // Determinamos cuántas filas vamos a almacenar (mínimo entre las que hay y el tope de 5)
    int rows_to_parse = (data_rows > MAX_MODBUS_REQUESTS_ROWS) ? MAX_MODBUS_REQUESTS_ROWS : data_rows;

    // --- EL BUCLE COLECTOR ---
    for (int i = 0; i < rows_to_parse; i++) {
        // Validamos que la celda concreta contenga texto y no sea un puntero nulo
        if (col_channel[i] && col_start[i] && col_length[i] && col_func[i] && col_interval[i]) {
            
            _requests_table[i].channel         = (uint16_t)atoi(col_channel[i]);
            _requests_table[i].start_addres    = (uint16_t)atoi(col_start[i]);
            _requests_table[i].length          = (uint16_t)atoi(col_length[i]);
            _requests_table[i].func_code       = (uint16_t)atoi(col_func[i]);
            _requests_table[i].req_interval_ms = (uint16_t)atoi(col_interval[i]);
            
            _requests_count++;
        } else {
            // Si hay una fila incompleta en medio del parseo, abortamos por corrupción de datos
            ESP_LOGE(TAG, "Datos corruptos o incompletos en la fila indice %d", i);
            return ESP_ERR_CONFIG_INVALID_DATA;
        }
    }

    return MBRequestValidation(); // analiza si es correcta la solicitud lectura modbus

    return ESP_ERR_CONFIG_INVALID_DATA;
}



esp_err_t ModbusRequestCSV::MBRequestValidation(){ // TODO

    if (_requests_count <= 0){
        ESP_LOGE(TAG, "Resquest count es 0");
        return ESP_ERR_CONFIG_INVALID_DATA;
    }

    return ESP_OK; 

}
