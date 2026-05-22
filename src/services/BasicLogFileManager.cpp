#include "BasicLogFileManager.h"
#include <cstdio>
#include <cstring>
#include "../Debug.h"

static const char* TAG = "BASIC_FILE_MGR";


BasicLogFileManager::BasicLogFileManager(SDManager* sdManager, uint16_t maxFiles)
    : _sd(sdManager), _userMaxFiles(maxFiles), _initialized(false), _fileCount(0) {
    _currentLogFile[0] = '\0';
}

void BasicLogFileManager::setMaxFiles(uint16_t maxFiles) {
    _userMaxFiles = maxFiles;
}

esp_err_t BasicLogFileManager::begin() {
    if (!_sd || !_sd->isReady()) {
        MY_LOGE(TAG, "begin: SD Manager no está listo o es nulo.");
        return ESP_ERR_SD_NOT_INIT; 
    }

    esp_err_t err = _sd->createDirectory(DIR_LOG_NAME);
    if (err != ESP_OK) {
        MY_LOGE(TAG, "begin: Error al crear directorio raíz %s [0x%X]", DIR_LOG_NAME, err);
        return err; 
    }
/*
    err = setErrorLog(); 
    if (err != ESP_OK) {
        MY_LOGE(TAG, "begin: Error al asegurar el log de errores [0x%X]", err);
        return err;
    }
*/
    _initialized = true; 
    MY_LOGI(TAG, "Datalogger inicializado correctamente.");
    return ESP_OK; 
}



/**
 * @brief Callback que analiza archivo por archivo en el directorio.
 * Busca patrones "LOGXXXXX.txt" y guarda el número 'XXXXX' más alto encontrado.
 */
void BasicLogFileManager::buscarContadorMasAltoCallback(const char* fileName, bool isDir, void* context) {
    if (isDir || !fileName || !context) return;

    uint16_t* maxContador = (uint16_t*)context;
    int numeroExtraido = 0;

    // Saltamos la barra inicial si el framework la incluye en el callback (ej: "/LOG00001.txt" -> "LOG00001.txt")
    const char* cleanName = (fileName[0] == '/') ? &fileName[1] : fileName;

    // Intentamos parsear el formato estricto LOG%05d.txt
    if (sscanf(cleanName, "LOG%05d.txt", &numeroExtraido) == 1) {
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
    
    // Escaneamos la carpeta usando nuestro callback
    esp_err_t err = _sd->listDirectory(DIR_LOG_NAME, buscarContadorMasAltoCallback, &maxEncontrado);
    if (err != ESP_OK) {
        MY_LOGE(TAG, "setLastSesion: Error listando directorio %s [0x%X]", DIR_LOG_NAME, err);
        return err;
    }

    // Actualizamos el contador de la clase con el máximo histórico encontrado en la SD
    _fileCount = maxEncontrado;

    if (_fileCount == 0) {
        MY_LOGI(TAG, "setLastSesion: No se encontraron logs previos. Próximo será el 1.");
        _currentLogFile[0] = '\0';
    } else {
        // Reconstruimos la ruta del archivo más reciente encontrado
        snprintf(_currentLogFile, sizeof(_currentLogFile), "%s/LOG%05u.txt", DIR_LOG_NAME, _fileCount);
        MY_LOGI(TAG, "Última sesión recuperada con éxito: %s (Contador: %u)", _currentLogFile, _fileCount);
    }

    return ESP_OK;
}

//esp_err_t BasicLogFileManager::setFilesLastSesion() {

    // aqui lo que se hacia era entrar en el año revisar lo que habia 
    // establecer en la clase los N nombres de los ficheros internos

    // en nuestro caso es actualizar el contador y ya. 

    // TODO ESTO ES MUCHO MAS SENCILLO EN ESTA CLASE

    /*
    if (!_initialized || strlen(_baseYearPath) == 0) return ESP_ERR_DL_NOT_INIT;

    _fileCount = 0;
    memset(_filenames, 0, sizeof(_filenames));
    std::vector<std::string> foundFiles;

    esp_err_t err = _sd->listDirectory(_baseYearPath, getSesionFilenamesCallback, &foundFiles);
    if (err != ESP_OK) {
        MY_LOGE(TAG, "setFilesLastSesion: Error al listar archivos en %s. Err: 0x%X", _baseYearPath, err);
        return err;
    }

    if (foundFiles.empty()) {
        MY_LOGI(TAG, "No se encontraron archivos de log (.txt) en %s", _baseYearPath);
        return ESP_OK;
    }

    // Ordenar de forma descendente (más recientes primero)
    std::sort(foundFiles.begin(), foundFiles.end(), std::greater<std::string>());

    uint16_t filesToStore = std::min((uint16_t)foundFiles.size(), _userMaxFiles);
    for (uint16_t i = 0; i < filesToStore; i++) {
        snprintf(_filenames[i], FILE_NAME_SIZE, "%s/%s", _baseYearPath, foundFiles[i].c_str());
        _fileCount++;
    }

    MY_LOGI(TAG, "Indexados %d archivos .txt más recientes.", _fileCount);
    for (uint16_t i = 0; i < _fileCount; i++) {
        MY_LOGD(TAG, "Indexado [%d]: %s", i, _filenames[i]);
    }

    return ESP_OK;

    */
//}


//void BasicLogFileManager::deleteInvalidFilesCallback(const char* fileName, bool isDir, void* context) {

    // borrar todo lo que no cumpla con el nombre. 
    /*
    if (isDir || !fileName || !context) return;

    DeleteContext* ctx = (DeleteContext*)context;
    DataloggerFileManager* self = ctx->instance;

    char fullPath[FILE_NAME_SIZE];
    if (fileName[0] == '/') {
        snprintf(fullPath, sizeof(fullPath), "%s", fileName);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", self->_baseYearPath, fileName);
    }

    bool isFileValid = false;
    for (uint16_t i = 0; i < self->_fileCount; i++) {
        if (strcmp(self->_filenames[i], fullPath) == 0) {
            isFileValid = true;
            break; 
        }
    }

    if (!isFileValid) {
        MY_LOGI(TAG, "Eliminando archivo obsoleto o no indexado: %s", fullPath);
        esp_err_t err = self->_sd->deleteFile(fullPath);
        if (err == ESP_OK) {
            ctx->deletedCount++;
        } else {
            MY_LOGE(TAG, "No se pudo borrar físicamente: %s. Err: 0x%X", fullPath, err);
        }
    }
    */
//}



esp_err_t BasicLogFileManager::setLastEnvironment(bool delete_rest) {
    // Buscamos el último archivo existente para actualizar el estado del contador interno
    esp_err_t err = setLastSesion();
    if (err != ESP_OK) return err;

    // Nota: Como es un gestor básico secuencial, 'delete_rest' o rotaciones pesadas se pueden omitir
    // o implementar de manera simple basándote en la capacidad máxima de archivos (_userMaxFiles).
    return ESP_OK;
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
     //   MY_LOGW(TAG, "Límite de archivos alcanzado (%u). Rotando contador al archivo 1.", _userMaxFiles);
     //   _fileCount = 1;
   // }

    // 3. Construimos la nueva ruta
    snprintf(_currentLogFile, sizeof(_currentLogFile), "%s/LOG%05u.txt", DIR_LOG_NAME, _fileCount);

    // 4. Si el archivo ya existía del ciclo de rotación anterior, lo eliminamos primero
    //if (_sd->exists(_currentLogFile)) {
     //   _sd->deleteFile(_currentLogFile);
    //}

    // 5. Creamos el archivo físico limpio en la SD
    esp_err_t err = _sd->createFile(_currentLogFile);
    if (err != ESP_OK) {
        MY_LOGE(TAG, "newFileLog: Error creando archivo %s [0x%X]", _currentLogFile, err);
        return err;
    }

    MY_LOGI(TAG, "Nuevo log activo creado con éxito: %s", _currentLogFile);
    return ESP_OK;
}


char* BasicLogFileManager::getCurrentLogPath() {
    return _currentLogFile;
}

esp_err_t BasicLogFileManager::appendErrorLog(const char* timestamp, const char* err_message) {
    /*
    if (!_initialized) return ESP_ERR_INVALID_STATE;
    
    char errorPath[FILE_NAME_SIZE];
    snprintf(errorPath, sizeof(errorPath), "%s/ERRORS.txt", DIR_LOG_NAME);

    // Contexto temporal para escribir una línea simple usando el Stream de withFileWrite
    struct ErrorCtx { const char* t; const char* m; };
    ErrorCtx ctx = { timestamp ? timestamp : "N/A", err_message };

    return _sd->withFileWrite(errorPath, [](Stream& stream, void* context) {
        ErrorCtx* c = (ErrorCtx*)context;
        stream.printf("[%s] ERROR: %s\n", c->t, c->m);
    }, &ctx);
    */
}

/*
esp_err_t BasicLogFileManager::setErrorLog() {
    char errorPath[FILE_NAME_SIZE];
    snprintf(errorPath, sizeof(errorPath), "%s/ERRORS.txt", DIR_LOG_NAME);
    return _sd->createFile(errorPath);
}
*/

bool BasicLogFileManager::requiresTimestamp() { 
    return false; 
}