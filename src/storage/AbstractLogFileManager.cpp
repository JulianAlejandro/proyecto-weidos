#include "AbstractLogFileManager.h"
#include <cstdio>
#include <cstring>

static const char* TAG = "LOG_MGR";

AbstractLogFileManager::AbstractLogFileManager(SDManager* sdManager, uint16_t maxFiles, const char* dirRoot)
    : _sd(sdManager), _userMaxFiles(maxFiles), _initialized(false) {
    _currentLogFile[0] = '\0';
    
    // Almacenamos dinámicamente las rutas que nos pasa la clase hija
    snprintf(_dirLogName, sizeof(_dirLogName), "%s", dirRoot);
    //snprintf(_pathErrLog, sizeof(_pathErrLog), "%s/ERROR", dirRoot);
}

/*
void AbstractLogFileManager::setMaxFiles(uint16_t maxFiles) {
    _userMaxFiles = maxFiles;
}
*/

void AbstractLogFileManager::setMaxFiles(uint16_t maxFiles) {
    if (maxFiles > MAX_LOG_CAPACITY) {
        _userMaxFiles = MAX_LOG_CAPACITY;
    } else if (maxFiles == 0) {
        _userMaxFiles = 1; 
    } else {
        _userMaxFiles = maxFiles;
    }
}

esp_err_t AbstractLogFileManager::begin() {
    if (!_sd || !_sd->isReady()) {
        ESP_LOGE(TAG, "begin: SD Manager no está listo o es nulo.");
        return ESP_ERR_SD_NOT_INIT; 
    }

    esp_err_t err = _sd->createDirectory(_dirLogName);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "begin: Error al crear directorio raíz %s [0x%X]", _dirLogName, err);
        return err; 
    }

    err = setErrorLog(); 
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "begin: Error al asegurar el log de errores [0x%X]", err);
        return err;
    }

    _initialized = true; 
    ESP_LOGI(TAG, "Datalogger inicializado correctamente.");
    return ESP_OK; 
}

esp_err_t AbstractLogFileManager::setErrorLog() {
    if (_sd->exists(PATH_ERROR_LOG)) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Creando archivo de logs de errores en: %s", PATH_ERROR_LOG);
    esp_err_t err = _sd->createFile(PATH_ERROR_LOG);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo inicializar la ruta de errores. Err: 0x%X", err);
        return err;
    }
    return ESP_OK;
}



static void writeErrorCallback(Stream& stream, void* context) {
    if (context) {
        const char* errorLine = (const char*)context;
        stream.print(errorLine);
    }
}

esp_err_t AbstractLogFileManager::appendErrorLog(const char* timestamp, const char* err_message) {
    if (!_initialized) return ESP_ERR_DL_NOT_INIT;
    if (!timestamp || !err_message) return ESP_ERR_INVALID_ARG;

    esp_err_t err = setErrorLog();
    if (err != ESP_OK) return err;

    char tempLine[512];
    snprintf(tempLine, sizeof(tempLine), "%s;%s\n", timestamp, err_message);

    bool success = !_sd->withFileWrite(PATH_ERROR_LOG, writeErrorCallback, tempLine);
    if (!success) {
        ESP_LOGE(TAG, "Fallo al escribir en el archivo de errores.");
        return ESP_ERR_SD_WRITE_FAIL; 
    }

    ESP_LOGW(TAG, "Error persistido en SD: %s", tempLine);
    return ESP_OK;
}


char* AbstractLogFileManager::getCurrentLogPath() {
    return _currentLogFile;
}


bool AbstractLogFileManager::requiresTimestamp() { 
    return false; // por defecto la abstracta no va con timestamp
}