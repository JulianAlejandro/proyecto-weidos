#include "ModbusRequestCSV.h"
#include <CSV_Parser.h>

const char* ModbusRequestCSV::TAG = "MB_CSV";

ModbusRequestCSV::ModbusRequestCSV(SDManager* sdManager) {
    _sd = sdManager;
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
    if (err != ESP_OK) return err;

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
esp_err_t ModbusRequestCSV::loadFromSDMbrequest(Struct_MBRequest *out_request) {
    if (!_initialized) return ESP_ERR_CONFIG_NOT_INIT; 
    if (out_request == nullptr) return ESP_ERR_INVALID_ARG;

    // Inicializar estructura de salida por seguridad
    *out_request = {0, 0, 0, 0, 0};

    CSV_Parser cp("sssss", false, ';');

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

    if (err != ESP_OK) return err;

    int rows = cp.getRowsCount();
    if (rows < 8) {
        ESP_LOGE(TAG, "Fila 8 de Modbus no encontrada. Filas totales: %d", rows);
        return ESP_ERR_CONFIG_PARSE_FAIL;
    }

    char **col0 = (char**)cp[0]; 
    char **col1 = (char**)cp[1]; 
    char **col2 = (char**)cp[2]; 
    char **col3 = (char**)cp[3]; 
    char **col4 = (char**)cp[4]; 

    if (col0[7] && col1[7] && col2[7] && col3[7] && col4[7]) {
        out_request->channel         = (uint16_t)atoi(col0[7]);
        out_request->start_addres    = (uint16_t)atoi(col1[7]);
        out_request->length          = (uint16_t)atoi(col2[7]);
        out_request->func_code       = (uint16_t)atoi(col3[7]);
        out_request->req_interval_ms = (uint16_t)atoi(col4[7]);
        return ESP_OK;
    }

    return ESP_ERR_CONFIG_INVALID_DATA;
}
