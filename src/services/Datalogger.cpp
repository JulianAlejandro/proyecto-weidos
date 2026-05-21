#include "Datalogger.h"
#include <cstring>
#include <cstdio>
#include "../Debug.h"

static const char* TAG = "DATALOGGER";

Datalogger::Datalogger(SDManager* sdManager) 
    : _sd(sdManager), _fileManager(sdManager, MAX_LOGS), _initialized(false), _fileLimitReached(false) {
    _logPath[0] = '\0';
}

esp_err_t Datalogger::begin() {
    if (_sd == nullptr || !_sd->isReady()) {
        MY_LOGE(TAG, "begin: SD Manager no asignado o no está listo.");
        return ESP_ERR_SD_NOT_INIT;
    }

    // 1. Inicializamos el gestor de archivos interno
    esp_err_t err = _fileManager.begin();
    if (err != ESP_OK) {
        MY_LOGE(TAG, "begin: Falló inicialización de FileManager [0x%X]", err);
        return err; 
    }

    // 2. Escaneamos la SD para recuperar el último entorno de trabajo
    err = _fileManager.setCSVLastEnvironment(true); 
    if (err != ESP_OK) {
        MY_LOGE(TAG, "begin: Falló al establecer el entorno de logs [0x%X]", err);
        return err; 
    }

    char* currentLogPtr = _fileManager.getCurrentLogPath();
    
    // 3. Verificar si se detectó un archivo existente para continuar en él
    if (currentLogPtr != nullptr && currentLogPtr[0] != '\0') {
        strncpy(_logPath, currentLogPtr, sizeof(_logPath) - 1);
        _logPath[sizeof(_logPath) - 1] = '\0'; 

        _fileLimitReached = false;

        // Consultamos el tamaño real del archivo existente en la SD
        uint32_t sizeFile = 0; 
        err = _sd->getFileSize(_logPath, &sizeFile);
        
        if (err == ESP_OK) {
            _currentFileSizeBytes = sizeFile;
            if (_currentFileSizeBytes >= MAX_FILE_SIZE_BYTES) { 
                _fileLimitReached = true;
            }
            MY_LOGI(TAG, "Archivo previo recuperado: %s (%u bytes).", _logPath, _currentFileSizeBytes);
        } else {
            // Por seguridad, si falla la lectura física, asumimos 0 para evitar punteros corruptos
            _currentFileSizeBytes = 0; 
            MY_LOGW(TAG, "No se pudo leer tamaño de %s. Inicializado en 0 por seguridad. Err: 0x%X", _logPath, err);
        }

    } else {
        _logPath[0] = '\0';
        _currentFileSizeBytes = 0;
        _fileLimitReached = false;
        MY_LOGI(TAG, "Entorno inicializado limpio (sin logs previos en la carpeta).");
    }

    _initialized = true;
    return ESP_OK;
}

void Datalogger::setMaxFiles(uint16_t maxFiles) {
    _fileManager.setMaxFiles(maxFiles);
}

esp_err_t Datalogger::newCSVLogSesion(const char* current_timestamp, const char** titles, uint16_t numTitles) {
    if (!_initialized) {
        return ESP_ERR_DL_NOT_INIT;
    }
    if (titles == nullptr || numTitles == 0 || current_timestamp == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    // Si el buffer tiene datos rezagados del archivo anterior, los forzamos a guardarse
    if (_buffer.getCurrentSize() > 0 && _logPath[0] != '\0') {
        MY_LOGI(TAG, "Vaciando buffer remanente antes de abrir nueva sesión...");
        esp_err_t flushErr = flushBuffer();
        if (flushErr != ESP_OK) return flushErr;
    }

    // Solicitar nuevo archivo físico basado en el timestamp de control
    esp_err_t err = _fileManager.newFileLog(current_timestamp); 
    if (err != ESP_OK) {
        MY_LOGE(TAG, "No se pudo generar el nuevo archivo de log en FileManager [0x%X]", err);
        return err;
    }

    char* currentLogPtr = _fileManager.getCurrentLogPath();
    if (currentLogPtr != nullptr && currentLogPtr[0] != '\0') {
        strncpy(_logPath, currentLogPtr, sizeof(_logPath) - 1);
        _logPath[sizeof(_logPath) - 1] = '\0'; 

        _currentFileSizeBytes = 0;
        _fileLimitReached = false;
    } else {
        _logPath[0] = '\0';
        return ESP_ERR_NOT_FOUND; 
    }

    // Construcción segura de la cabecera CSV
    char tempLine[256] = "Timestamp;"; 

    for (uint16_t i = 0; i < numTitles; i++) {
        if (titles[i] != nullptr) {
            strncat(tempLine, titles[i], sizeof(tempLine) - strlen(tempLine) - 1);
        }
        if (i < numTitles - 1) {
            strncat(tempLine, ";", sizeof(tempLine) - strlen(tempLine) - 1);
        }
    }
    strncat(tempLine, "\n", sizeof(tempLine) - strlen(tempLine) - 1); 

    // Inserción en el buffer RAM
    return m_pushToBuffer(tempLine);
}


esp_err_t Datalogger::appendNewDataCSVToLog(const char* timestamp_msg, const char** values, uint16_t numValues) {
    if (!_initialized) return ESP_ERR_DL_NOT_INIT;
    if (_logPath[0] == '\0') return ESP_ERR_INVALID_STATE;
    if (timestamp_msg == nullptr || values == nullptr || numValues == 0) return ESP_ERR_INVALID_ARG;

    char tempLine[512]; 
    snprintf(tempLine, sizeof(tempLine), "%s", timestamp_msg);

    for (uint16_t i = 0; i < numValues; i++) {
        strncat(tempLine, ";", sizeof(tempLine) - strlen(tempLine) - 1);
        if (values[i] != nullptr) {
            strncat(tempLine, values[i], sizeof(tempLine) - strlen(tempLine) - 1);
        }
    }
    strncat(tempLine, "\n", sizeof(tempLine) - strlen(tempLine) - 1); 

    return m_pushToBuffer(tempLine);
}


// Callback privado auxiliar para interactuar con la interfaz Stream de la SD
void flushLogBufferCallback(Stream& stream, void* context) {
    if (context) {
        LogBuffer* buffer = (LogBuffer*)context;
        stream.write(buffer->getBufferPointer(), buffer->getCurrentSize());
    }
}

esp_err_t Datalogger::flushBuffer() {
    if (!_initialized) return ESP_ERR_DL_NOT_INIT;
    
    // Si no hay datos, se considera una operación exitosa sin coste de IO
    if (_buffer.getCurrentSize() == 0) {
        return ESP_OK;
    }
    if (_logPath[0] == '\0') {
        return ESP_ERR_INVALID_STATE;
    }

    size_t bytesToWrite = _buffer.getCurrentSize();
    MY_LOGD(TAG, "Volcando %d bytes de RAM a SD en: %s", bytesToWrite, _logPath);
    
    // Ejecución de la escritura por bloque síncrono
    bool failed = _sd->withFileWrite(_logPath, flushLogBufferCallback, &_buffer);
    
    if (!failed) {
        _currentFileSizeBytes += bytesToWrite;
        _buffer.clear(); 
        return ESP_OK;
    } else {
        MY_LOGE(TAG, "Error de escritura física en archivo: %s", _logPath);
        return ESP_ERR_SD_WRITE_FAIL;
    }
}

esp_err_t Datalogger::m_pushToBuffer(const char* csvLine) {
    if (_fileLimitReached) {
        return ESP_ERR_INVALID_STATE; // El archivo actual está bloqueado por tamaño máximo
    }

    size_t lineLength = strlen(csvLine);
    uint32_t projectedSize = _currentFileSizeBytes + _buffer.getCurrentSize() + lineLength;

    // Validación de límites de tamaño para prevenir desbordamientos en la partición SD
    if (projectedSize > MAX_FILE_SIZE_BYTES) {
        MY_LOGW(TAG, "Límite de tamaño de archivo alcanzado (%u bytes). Forzando cierre de bloque.", MAX_FILE_SIZE_BYTES);
        
        // Volcado de emergencia de lo que se pueda salvar en el buffer actual
        flushBuffer();
        _fileLimitReached = true;
        
        return ESP_ERR_INVALID_STATE; 
    }

    // Si la fila actual supera el espacio disponible del buffer dinámico, limpiamos primero la RAM
    if (lineLength > _buffer.getAvailableSpace()) {
        esp_err_t err = flushBuffer();
        if (err != ESP_OK) {
            MY_LOGE(TAG, "m_pushToBuffer: Imposible liberar espacio en RAM por fallo de SD.");
            return err;
        }
    }

    _buffer.appendString(csvLine);
    return ESP_OK;
}

esp_err_t Datalogger::appendErrorLog(const char* timestamp_msg, const char* err_message) {
    if (!_initialized) return ESP_ERR_DL_NOT_INIT;
    return _fileManager.appendErrorLog(timestamp_msg, err_message); 
}
