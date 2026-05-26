#include "BasicLogFileManager.h"
#include <cstdio>
#include <cstring>

static const char* TAG = "BASIC_FILE_MGR";


BasicLogFileManager::BasicLogFileManager(SDManager* sdManager, uint16_t maxFiles)
    : AbstractLogFileManager(sdManager, maxFiles, "/B_LOGS"), _fileCount(0) {
}


/**
 * @brief Callback que analiza archivo por archivo en el directorio.
 * Busca patrones "LOGXXXXX.txt" y guarda el número 'XXXXX' más alto encontrado.
 */
void BasicLogFileManager::buscarContadorMasAltoCallback(const char* fileName, bool isDir, void* context) {
    if (isDir || !fileName || !context) return;

    uint16_t* maxContador = (uint16_t*)context;
    int numeroExtraido = 0;

    const char* cleanName = (fileName[0] == '/') ? &fileName[1] : fileName;

    // 🚀 Cambiado: Concatenamos dinámicamente el patrón esperado con la extensión elegida
    if (sscanf(cleanName, "LOG%05d" LOG_FILE_EXT, &numeroExtraido) == 1) {
        if (numeroExtraido > *maxContador) {
            *maxContador = (uint16_t)numeroExtraido;
        }
    }
}

/**
 * @brief Busca en la SD el fichero con el índice más alto para restaurar la sesión.
 */
esp_err_t BasicLogFileManager::setLastSesion() {
    if (!_sd) return ESP_ERR_INVALID_STATE;

    uint16_t maxEncontrado = 0;
    
    esp_err_t err = _sd->listDirectory(_dirLogName, buscarContadorMasAltoCallback, &maxEncontrado);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "setLastSesion: Error listando directorio %s [0x%X]", _dirLogName, err);
        return err;
    }

    _fileCount = maxEncontrado;

    if (_fileCount == 0) {
        ESP_LOGI(TAG, "setLastSesion: No se encontraron logs previos. Próximo será el 1.");
        _currentLogFile[0] = '\0';
    } else {
        // 🚀 Cambiado: Se inyecta la macro de extensión en la ruta recuperada
        snprintf(_currentLogFile, sizeof(_currentLogFile), "%s/LOG%05u" LOG_FILE_EXT, _dirLogName, _fileCount);
        ESP_LOGI(TAG, "Última sesión recuperada con éxito: %s (Contador: %u)", _currentLogFile, _fileCount);
    }

    return ESP_OK;
}


esp_err_t BasicLogFileManager::setLastEnvironment(bool delete_rest) {
    return setLastSesion();
}



/**
 * @brief Genera un nuevo archivo incrementando el contador secuencial.
 * @param timestamp Parámetro ignorado por el diseño de este gestor básico (cumple interfaz).
 */
esp_err_t BasicLogFileManager::newFileLog(const char* timestamp) {
    if (!_initialized) return ESP_ERR_INVALID_STATE;

    // 1. Incrementamos el contador para el nuevo archivo
    _fileCount++;

    // 2. Control de rotación simple: si supera el máximo configurado por usuario, volvemos a empezar en 1
   // if (_fileCount > _userMaxFiles) {
     //   ESP_LOGW(TAG, "Límite de archivos alcanzado (%u). Rotando contador al archivo 1.", _userMaxFiles);
     //   _fileCount = 1;
   // }

    // 3. Construimos la nueva ruta
    snprintf(_currentLogFile, sizeof(_currentLogFile), "%s/LOG%05u" LOG_FILE_EXT, _dirLogName, _fileCount);
    // 4. Si el archivo ya existía del ciclo de rotación anterior, lo eliminamos primero
    //if (_sd->exists(_currentLogFile)) {
     //   _sd->deleteFile(_currentLogFile);
    //}

    // 5. Creamos el archivo físico limpio en la SD
    esp_err_t err = _sd->createFile(_currentLogFile);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "newFileLog: Error creando archivo %s [0x%X]", _currentLogFile, err);
        return err;
    }

    ESP_LOGI(TAG, "Nuevo log activo creado con éxito: %s", _currentLogFile);
    return ESP_OK;
}

