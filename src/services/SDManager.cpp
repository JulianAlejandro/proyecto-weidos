#include "SDManager.h"
#include <SPI.h>

const char* SDManager::TAG = "SD_MGR";
/**
 * @brief Default constructor.
 */
SDManager::SDManager() {}

/**
 * @brief Starts the SD card hardware communication.
 * Validates the SPI connection and FAT file system mounting.
 */
esp_err_t SDManager::begin() {
    if (_initialized) return ESP_OK;

    // Intentamos montar la tarjeta
    if (!SD.begin()) {
        ESP_LOGE(TAG, "Fallo crítico: No se pudo montar la tarjeta SD");
        return ESP_ERR_SD_MOUNT;
    }

    _initialized = true;
    ESP_LOGI(TAG, "Tarjeta SD montada correctamente");
    return ESP_OK;
}

/**
 * @brief Returns the initialization status.
 */
bool SDManager::isReady() {
    return _initialized;
}

/**
 * @brief Ensures a file exists. If it doesn't, it creates an empty one.
 */
esp_err_t SDManager::createFile(const char* path) {
    if (!_initialized) return ESP_ERR_SD_NOT_INIT;

    if (SD.exists(path)) return ESP_OK;

    File f = SD.open(path, FILE_WRITE);
    if (f) {
        f.close();
        return ESP_OK;
    }
    
    ESP_LOGE(TAG, "Error al crear archivo: %s", path);
    return ESP_ERR_SD_WRITE_FAIL;
}

/**
 * @brief Checks existence of a resource at the given path.
 */
bool SDManager::exists(const char* path) {
    return SD.exists(path);
}

/**
 * @brief Overwrites an existing file with an empty state.
 * Uses O_TRUNC to reset file size to 0.
 */
void SDManager::clearFile(const char* path) {
    if (!_initialized) return;
    // O_TRUNC borra el contenido
    File f = SD.open(path, O_WRITE | O_CREAT | O_TRUNC); 
    if (f) f.close(); 
}

/**
 * @brief Opens file in append mode and adds a new line of data.
 */
/*
bool SDManager::appendLine(const char* path, const char* data) {
    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;
    
    f.println(data); // Appends data + line jump (\n)
    f.close();
    return true;
}
*/
/**
 * @brief Utility for debugging. Prints the full file content to Serial.
 */
void SDManager::printFileToSerial(const char* path) {
    if (!_initialized) return;
    File f = SD.open(path, FILE_READ);
    if (!f) return;
    
    while (f.available()) {
        Serial.write(f.read());
    }
    f.close();
}

/**
 * @brief Creates a directory structure.
 */
esp_err_t SDManager::createDirectory(const char* path) {
    if (!_initialized) return ESP_ERR_SD_NOT_INIT;

    if (SD.exists(path)) return ESP_OK;

    if (SD.mkdir(path)) {
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Fallo al crear directorio: %s", path);
    return ESP_ERR_SD_DIR_FAIL;
}

/**
 * @brief Deletes a file from the SD card.
 * @param path Full path to the file.
 * @return true if the file was successfully deleted or didn't exist, false if deletion failed.
 */
esp_err_t SDManager::deleteFile(const char* path) {
    if (!_initialized) return ESP_ERR_SD_NOT_INIT;

    if (!SD.exists(path)) return ESP_OK;

    if (SD.remove(path)) {
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Fallo al eliminar archivo: %s", path);
    return ESP_ERR_SD_WRITE_FAIL;
}

/**
 * @brief Higher-order function to handle file streaming via callbacks.
 * Ensures the file is safely closed after the callback execution.
 */
esp_err_t SDManager::withFile(const char* path, StreamCallback callback, void* context) {
    if (!_initialized) return ESP_ERR_SD_NOT_INIT;

    File file = SD.open(path, FILE_READ);
    if (!file) {
        ESP_LOGW(TAG, "Archivo no encontrado para lectura: %s", path);
        return ESP_ERR_SD_FILE_NOT_FOUND;
    }

    callback(file, context);
    file.close();
    return ESP_OK;
}


// Versión modificada de withFile para permitir escritura
esp_err_t SDManager::withFileWrite(const char* path, StreamCallback callback, void* context) {
    if (!_initialized) return ESP_ERR_SD_NOT_INIT;

    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        ESP_LOGE(TAG, "No se pudo abrir para escritura: %s", path);
        return ESP_ERR_SD_WRITE_FAIL;
    }

    callback(file, context);
    file.close();
    return ESP_OK;
}