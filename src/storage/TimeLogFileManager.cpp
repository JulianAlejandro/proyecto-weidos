#include "TimeLogFileManager.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <string>

static const char* TAG = "DATA_MGR";

TimeLogFileManager::TimeLogFileManager(SDManager* sdManager, uint16_t maxFiles) 
    : AbstractLogFileManager(sdManager, maxFiles, "/LOGS"), 
      _fileCount(0), _intLastYearLog(0), _intLastTimestampLog(0) {
      
    // _currentLogFile y los paths de directorios ya los inicializó el padre.
    memset(_filenames, 0, sizeof(_filenames));
    _baseYearPath[0] = '\0';
    
    // Forzamos la validación de límites llamando al setMaxFiles sobreescrito
    //setMaxFiles(maxFiles); 
}

TimeLogFileManager::~TimeLogFileManager() {
    // Se deja vacío. Solo sirve para que el compilador tenga un punto de anclaje para la vtable.
}


void TimeLogFileManager::buscarAnioMasRecienteCallback(const char* fileName, bool isDir, void* context) {
    if (!isDir || !fileName || !context) return; 

    int* maxYear = (int*)context;
    const char* folderName = strrchr(fileName, '/');
    folderName = (folderName) ? folderName + 1 : fileName;

    int currentYear = atoi(folderName);
    if (currentYear > *maxYear) {
        *maxYear = currentYear;
    }
}

esp_err_t TimeLogFileManager::setLastSesion() {
    if (!_initialized) return ESP_ERR_DL_NOT_INIT;

    int maxYearFound = 0;
    esp_err_t err = _sd->listDirectory(_dirLogName, buscarAnioMasRecienteCallback, &maxYearFound);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "setLastSesion: Fallo al escanear directorio %s. Err: 0x%X", _dirLogName, err);
        return err;
    }

    if (maxYearFound == 0) {
        maxYearFound = DEFAULT_YEAR; 
        ESP_LOGW(TAG, "setLastSesion: No se hallaron sesiones. Forzando entorno inicial en %d", maxYearFound);
    }

    snprintf(_baseYearPath, sizeof(_baseYearPath), "%s/%d", _dirLogName, maxYearFound);

    err = _sd->createDirectory(_baseYearPath);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "setLastSesion: Fallo crítico creando %s. Err: 0x%X", _baseYearPath, err);
        return err;
    }

    ESP_LOGI(TAG, "Entorno de trabajo fijado en: %s", _baseYearPath);
    return ESP_OK;
}


void TimeLogFileManager::getSesionFilenamesCallback(const char* fileName, bool isDir, void* context) {
    if (isDir || !fileName || !context) return; 

    std::vector<std::string>* fileList = (std::vector<std::string>*)context;
    const char* shortName = strrchr(fileName, '/');
    shortName = (shortName) ? shortName + 1 : fileName;

    size_t len = strlen(shortName);
    ESP_LOGD(TAG, "Evaluando archivo detectado: %s", shortName);

    // 🚀 Cambiado: El nombre válido ahora mide 8 caracteres del timestamp + los caracteres de la extensión
    size_t expectedLen = 8 + strlen(LOG_FILE_EXT);

    if (len == expectedLen) { 
        std::string nameStr(shortName);
        std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);

        // 🚀 Cambiado: Busca dinámicamente la extensión configurada en el define
        if (nameStr.rfind(LOG_FILE_EXT) != std::string::npos) {
            fileList->push_back(shortName);
            ESP_LOGD(TAG, "-> ¡Archivo VALIDO añadido!: %s", shortName);
        }
    }
}


esp_err_t TimeLogFileManager::setFilesLastSesion() {
    if (!_initialized || strlen(_baseYearPath) == 0) return ESP_ERR_DL_NOT_INIT;

    _fileCount = 0;
    memset(_filenames, 0, sizeof(_filenames));
    std::vector<std::string> foundFiles;

    esp_err_t err = _sd->listDirectory(_baseYearPath, getSesionFilenamesCallback, &foundFiles);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "setFilesLastSesion: Error al listar archivos en %s. Err: 0x%X", _baseYearPath, err);
        return err;
    }

    if (foundFiles.empty()) {
        ESP_LOGI(TAG, "No se encontraron archivos de log (%s) en %s", LOG_FILE_EXT, _baseYearPath);
        return ESP_OK;
    }

    // Ordenar de forma descendente (más recientes primero)
    std::sort(foundFiles.begin(), foundFiles.end(), std::greater<std::string>());

    uint16_t filesToStore = std::min((uint16_t)foundFiles.size(), _userMaxFiles);
    for (uint16_t i = 0; i < filesToStore; i++) {
        snprintf(_filenames[i], FILE_NAME_SIZE, "%s/%s", _baseYearPath, foundFiles[i].c_str());
        _fileCount++;
    }

    // 🚀 Cambiado para mostrar la extensión en el log
    ESP_LOGI(TAG, "Indexados %d archivos %s más recientes.", _fileCount, LOG_FILE_EXT);
    for (uint16_t i = 0; i < _fileCount; i++) {
        ESP_LOGD(TAG, "Indexado [%d]: %s", i, _filenames[i]);
    }

    return ESP_OK;
}

// Estructura auxiliar para pasar datos al callback de borrado de forma segura
struct DeleteContext {
    TimeLogFileManager* instance;
    uint32_t deletedCount;
};

void TimeLogFileManager::deleteInvalidFilesCallback(const char* fileName, bool isDir, void* context) {
    if (isDir || !fileName || !context) return;

    DeleteContext* ctx = (DeleteContext*)context;
    TimeLogFileManager* self = ctx->instance;

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
        ESP_LOGI(TAG, "Eliminando archivo obsoleto o no indexado: %s", fullPath);
        esp_err_t err = self->_sd->deleteFile(fullPath);
        if (err == ESP_OK) {
            ctx->deletedCount++;
        } else {
            ESP_LOGE(TAG, "No se pudo borrar físicamente: %s. Err: 0x%X", fullPath, err);
        }
    }
}

esp_err_t TimeLogFileManager::deleteInvalidFiles() {
    if (!_initialized || strlen(_baseYearPath) == 0) return ESP_ERR_DL_NOT_INIT;

    ESP_LOGI(TAG, "Iniciando purga de archivos no indexados en: %s", _baseYearPath);

    DeleteContext ctx = { .instance = this, .deletedCount = 0 };
    esp_err_t err = _sd->listDirectory(_baseYearPath, deleteInvalidFilesCallback, &ctx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error en directorio durante la purga. Err: 0x%X", err);
        return err;
    }

    ESP_LOGI(TAG, "Purga completada. Archivos eliminados: %u", ctx.deletedCount);
    return ESP_OK;
}

/**
 * @brief Extrae el año y el timestamp numérico del archivo de log actual activo.
 * Si el archivo está vacío, inicializa los valores en 0.
 */
void TimeLogFileManager::setLastLogTime() {
    if (_currentLogFile[0] == '\0') {
        _intLastYearLog = 0;
        _intLastTimestampLog = 0;
        ESP_LOGI(TAG, "setLastLogTime: No hay log activo previo. Línea de tiempo en 0.");
        return;
    }

    const char* lastSlash = strrchr(_currentLogFile, '/');
    if (lastSlash == nullptr) {
        ESP_LOGE(TAG, "setLastLogTime: Formato de ruta inválido (%s)", _currentLogFile);
        return;
    }

    const char* fileName = lastSlash + 1;
    char tempPath[FILE_NAME_SIZE];
    snprintf(tempPath, sizeof(tempPath), "%s", _currentLogFile);
    
    size_t slashIndex = lastSlash - _currentLogFile;
    tempPath[slashIndex] = '\0'; 

    const char* yearSlash = strrchr(tempPath, '/');
    _intLastYearLog = (yearSlash != nullptr) ? (uint16_t)atoi(yearSlash + 1) : 0;

    char tokenTimestamp[9];
    strncpy(tokenTimestamp, fileName, 8);
    tokenTimestamp[8] = '\0';
    _intLastTimestampLog = (uint32_t)strtoul(tokenTimestamp, nullptr, 10);

    ESP_LOGI(TAG, "Último histórico detectado en SD -> Año: %u | Timestamp: %u", _intLastYearLog, _intLastTimestampLog);
}

esp_err_t TimeLogFileManager::setLastEnvironment(bool delete_rest) {
    esp_err_t err = setLastSesion();
    if (err != ESP_OK) return err;
    
    err = setFilesLastSesion();
    if (err != ESP_OK) return err;

    if (_fileCount > 0) {
        snprintf(_currentLogFile, sizeof(_currentLogFile), "%s", _filenames[0]);
        ESP_LOGI(TAG, "Log actual fijado en el más reciente: %s", _currentLogFile);
    } else {
        _currentLogFile[0] = '\0';
        ESP_LOGI(TAG, "Carpeta vacía. _currentLogFile inicializado vacío.");
    }
    
    setLastLogTime(); 

    if (delete_rest) { 
        err = deleteInvalidFiles(); 
        if (err != ESP_OK) return err;
    }

    return ESP_OK; 
}


bool getDataTimestamp(const char* timestamp, uint16_t* outYear, uint32_t* outSession) {
    if (!timestamp || !outYear || !outSession) return false;

    size_t len = strlen(timestamp);
    if (len != 14) { 
        ESP_LOGE("TIMESTAMP_PARSE", "Longitud incorrecta: %d (Se esperan 14)", len);
        return false;
    }

    for (size_t i = 0; i < 14; i++) { 
        if (timestamp[i] < '0' || timestamp[i] > '9') {
            ESP_LOGE("TIMESTAMP_PARSE", "Caracteres no numéricos detectados.");
            return false;
        }
    }

    char tempYear[5];
    strncpy(tempYear, timestamp, 4);
    tempYear[4] = '\0';
    *outYear = (uint16_t)atoi(tempYear);

    char tempSession[9];
    strncpy(tempSession, timestamp + 4, 8);
    tempSession[8] = '\0';
    *outSession = (uint32_t)strtoul(tempSession, nullptr, 10);

    return true;
}

esp_err_t TimeLogFileManager::newYearFile(uint16_t year) {
    if (!_initialized) return ESP_ERR_DL_NOT_INIT;
    if (year < 2026 || year > 2100) { 
        ESP_LOGE(TAG, "newYearFile: Año fuera de rango (%u)", year);
        return ESP_ERR_INVALID_ARG;
    }

    char newYearPath[FILE_NAME_SIZE];
    snprintf(newYearPath, sizeof(newYearPath), "%s/%u", _dirLogName, year);

    ESP_LOGI(TAG, "Creando directorio para nuevo año: %s", newYearPath);
    esp_err_t err = _sd->createDirectory(newYearPath);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo crítico al crear directorio: %s. Err: 0x%X", newYearPath, err);
        return err;
    }

    return ESP_OK;
}


/**
 * @brief Crea una nueva carpeta de año dentro de /LOGS y actualiza el entorno de trabajo.
 * @param year Entero con el año de 4 dígitos (ej: 2027).
 * @return ESP_OK si se creó con éxito, o el código de error correspondiente.
 */
esp_err_t TimeLogFileManager::newFileLog(const char* timestamp) {
    if (!_initialized || strlen(_baseYearPath) == 0) return ESP_ERR_DL_NOT_INIT;
    if (!timestamp || strlen(timestamp) == 0) return ESP_ERR_INVALID_ARG;

    uint16_t incomingYear = 0;
    uint32_t incomingTimestamp = 0; 

    if (!getDataTimestamp(timestamp, &incomingYear, &incomingTimestamp)) {
        ESP_LOGE(TAG, "newFileLog: Error de parseo en timestamp: %s", timestamp);
        return ESP_ERR_INVALID_ARG;
    }

    if (incomingYear < _intLastYearLog) {
        ESP_LOGE(TAG, "Rechazado: Año entrante (%u) es más antiguo que el último log (%u).", incomingYear, _intLastYearLog);
        return ESP_ERR_DL_PAST_TIME; 
    } 
    else if (incomingYear > _intLastYearLog) {
        ESP_LOGI(TAG, "Cambio de año detectado de forma síncrona (%u -> %u).", _intLastYearLog, incomingYear);
        esp_err_t err = newYearFile(incomingYear); 
        if (err != ESP_OK) return err;

        err = setLastEnvironment(true); 
        if (err != ESP_OK) return err; 
    } 
    else { 
        if (incomingTimestamp <= _intLastTimestampLog) {
            ESP_LOGE(TAG, "Rechazado: Timestamp entrante (%u) no es mayor que el último registrado (%u).", incomingTimestamp, _intLastTimestampLog);
            return ESP_ERR_DL_PAST_TIME; 
        }
    }

    char newFilePath[FILE_NAME_SIZE];
    // 🚀 Cambiado: Concatenamos automáticamente el literal "%s/%08u" con la macro LOG_FILE_EXT
    snprintf(newFilePath, sizeof(newFilePath), "%s/%08u" LOG_FILE_EXT, _baseYearPath, incomingTimestamp);
    ESP_LOGI(TAG, "Aprobada la creación de nuevo log: %s", newFilePath);

    if (_fileCount >= _userMaxFiles) {
        uint16_t oldestIndex = _fileCount - 1; 
        ESP_LOGW(TAG, "Capacidad máxima alcanzada. Rotando archivo antiguo: %s", _filenames[oldestIndex]);

        esp_err_t delErr = _sd->deleteFile(_filenames[oldestIndex]);
        if (delErr != ESP_OK) {
            ESP_LOGE(TAG, "Fallo al eliminar archivo antiguo en rotación. Err: 0x%X", delErr);
            return delErr;
        }
        memset(_filenames[oldestIndex], 0, FILE_NAME_SIZE);
        _fileCount--;
    }

    esp_err_t createErr = _sd->createFile(newFilePath);
    if (createErr != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al crear archivo en SD: %s. Err: 0x%X", newFilePath, createErr);
        return createErr;
    }

    for (int i = _fileCount; i > 0; i--) {
        strncpy(_filenames[i], _filenames[i - 1], FILE_NAME_SIZE);
    }

    strncpy(_filenames[0], newFilePath, FILE_NAME_SIZE);
    _fileCount++;
    strncpy(_currentLogFile, _filenames[0], sizeof(_currentLogFile));

    _intLastYearLog = incomingYear;
    _intLastTimestampLog = incomingTimestamp;

    ESP_LOGI(TAG, "Nuevo archivo activo: %s. Línea de tiempo actualizada.", _currentLogFile);
    return ESP_OK;
}

bool TimeLogFileManager::requiresTimestamp() { 
    return true; 
}