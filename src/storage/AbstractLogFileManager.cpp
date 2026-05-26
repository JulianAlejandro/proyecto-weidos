#include "AbstractLogFileManager.h"
//#include "esp_log.h"

static const char* TAG = "LOG_MGR";

AbstractLogFileManager::AbstractLogFileManager(SDManager* sdManager, uint16_t maxFiles)
    : _sd(sdManager), _userMaxFiles(maxFiles), _initialized(false){
    _currentLogFile[0] = '\0';
}

void AbstractLogFileManager::setMaxFiles(uint16_t maxFiles) {
    _userMaxFiles = maxFiles;
}

esp_err_t AbstractLogFileManager::begin() {
    if (!_sd || !_sd->isReady()) {
        ESP_LOGE(TAG, "begin: SD Manager no está listo o es nulo.");
        return ESP_ERR_SD_NOT_INIT; 
    }

    esp_err_t err = _sd->createDirectory(DIR_LOG_NAME);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "begin: Error al crear directorio raíz %s [0x%X]", DIR_LOG_NAME, err);
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
    if (_sd->exists(PATH_ERR_LOG)) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Creando archivo de logs de errores en: %s", PATH_ERR_LOG);
    esp_err_t err = _sd->createFile(PATH_ERR_LOG);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo inicializar la ruta de errores. Err: 0x%X", err);
        return err;
    }
    return ESP_OK;
}

/*
// inciar una sesion BASICA 
esp_err_t AbstractLogFileManager::setLastEnvironment(bool delete_rest) {
    //_currentLogFile[0] = 'algo?;
    return ESP_OK; // NO HACE NADA EN ESTA CLASE PADRE
}
*/

char* AbstractLogFileManager::getCurrentLogPath() {
    return _currentLogFile;
}

/*
esp_err_t AbstractLogFileManager::newFileLog(const char* timestamp) {
    return ESP_OK;
}

*/

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

    bool success = !_sd->withFileWrite(PATH_ERR_LOG, writeErrorCallback, tempLine);
    if (!success) {
        ESP_LOGE(TAG, "Fallo al escribir en el archivo de errores.");
        return ESP_ERR_SD_WRITE_FAIL; 
    }

    ESP_LOGW(TAG, "Error persistido en SD: %s", tempLine);
    return ESP_OK;
}



bool AbstractLogFileManager::requiresTimestamp() { 
    return false; // por defecto la abstracta no va con timestamp
}