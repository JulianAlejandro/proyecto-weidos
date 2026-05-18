#include "DataloggerFileManager.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <string>

const char* TAG_DFM = "DATA_MGR";

DataloggerFileManager::DataloggerFileManager(SDManager* sdManager, uint16_t maxFiles) : _sd(sdManager), _fileCount(0) {
    setMaxFiles(maxFiles);
    memset(_filenames, 0, sizeof(_filenames));
    _currentLogFile[0] = '\0';
}

void DataloggerFileManager::setMaxFiles(uint16_t maxFiles) {
    if (maxFiles > MAX_LOG_CAPACITY) {
        _userMaxFiles = MAX_LOG_CAPACITY;
    } else if (maxFiles == 0) {
        _userMaxFiles = 1; 
    } else {
        _userMaxFiles = maxFiles;
    }
}

esp_err_t DataloggerFileManager::begin() {
    if(!_sd->isReady()){
        return ESP_ERR_SD_NOT_INIT; 
    }
    esp_err_t err = _sd->createDirectory(DIR_LOG_NAME);

    if(err == ESP_OK){
        _initialized = true; 
    }
    return err; 
}

void DataloggerFileManager::buscarAnioMasRecienteCallback(const char* fileName, bool isDir, void* context) {
    if (!isDir) return; 

    int* maxYear = (int*)context;

    const char* folderName = strrchr(fileName, '/');
    if (folderName) {
        folderName++; 
    } else {
        folderName = fileName;
    }

    int currentYear = atoi(folderName);

    if (currentYear > *maxYear) {
        *maxYear = currentYear;
    }
}

esp_err_t DataloggerFileManager::setLastSesion() {
    if (!_initialized) return ESP_FAIL;

    int maxYearFound = 0;

    esp_err_t err = _sd->listDirectory(DIR_LOG_NAME, buscarAnioMasRecienteCallback, &maxYearFound);

    if (err != ESP_OK) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.print("] Error al escanear el directorio "); Serial.println(DIR_LOG_NAME);
        return err;
    }

    if (maxYearFound == 0) {
        maxYearFound = 2026; 
        Serial.print("[WARN] ["); Serial.print(TAG_DFM); Serial.print("] No se encontraron sesiones previas. Iniciando entorno en "); Serial.println(maxYearFound);
    }

    snprintf(_baseYearPath, sizeof(_baseYearPath), "%s/%d", DIR_LOG_NAME, maxYearFound);

    err = _sd->createDirectory(_baseYearPath);
    if (err != ESP_OK) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.print("] Fallo critico al asegurar el directorio base: "); Serial.println(_baseYearPath);
        return err;
    }

    Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] Entorno de trabajo fijado con exito en: "); Serial.println(_baseYearPath);
    
    return ESP_OK;
}

// funcion que obtiene los nombres de los x ficheros 
void DataloggerFileManager::getSesionFilenamesCallback(const char* fileName, bool isDir, void* context) {
    if (isDir) return; 

    std::vector<std::string>* fileList = (std::vector<std::string>*)context;

    // 1. Extraer el nombre corto buscando la última barra "/"
    const char* shortName = strrchr(fileName, '/');
    if (shortName) {
        shortName++; 
    } else {
        shortName = fileName;
    }

    size_t len = strlen(shortName);
    
    // Imprime esto de forma temporal para ver exactamente qué está leyendo de la SD
    Serial.print("[DEBUG] Evaluando archivo detectado: "); 
    Serial.println(shortName);

    // 2. Comprobar longitud básica (MMDDHHMM.csv -> 12 caracteres)
    if (len == 12) {
        // Creamos un string temporal en minúsculas para comparar la extensión de forma segura
        std::string nameStr(shortName);
        std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);

        // Comprobamos si termina en ".csv"
        if (nameStr.rfind(".csv") != std::string::npos) {
            // Guardamos el nombre original (por si respetas mayúsculas en el sistema de archivos)
            fileList->push_back(shortName);
            Serial.print("[DEBUG] -> ¡Archivo VALIDO añadido!: "); 
            Serial.println(shortName);
        }
    }
}


esp_err_t DataloggerFileManager::setFilesLastSesion() {
    if (!_initialized || strlen(_baseYearPath) == 0) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.println("] Entorno no inicializado. Llama primero a setLastSesion()");
        return ESP_FAIL;
    }

    _fileCount = 0;
    memset(_filenames, 0, sizeof(_filenames));

    std::vector<std::string> foundFiles;

    esp_err_t err = _sd->listDirectory(_baseYearPath, getSesionFilenamesCallback, &foundFiles);
    if (err != ESP_OK) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.print("] Error al listar archivos en: "); Serial.println(_baseYearPath);
        return err;
    }

    if (foundFiles.empty()) {
        Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] No se encontraron archivos de log (.csv) en "); Serial.println(_baseYearPath);
        return ESP_OK;
    }

    std::sort(foundFiles.begin(), foundFiles.end(), std::greater<std::string>());

    uint16_t filesToStore = std::min((uint16_t)foundFiles.size(), _userMaxFiles);

    for (uint16_t i = 0; i < filesToStore; i++) {
        snprintf(_filenames[i], FILE_NAME_SIZE, "%s/%s", _baseYearPath, foundFiles[i].c_str());
        _fileCount++;
    }

    Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] Se han indexado los "); Serial.print(_fileCount); Serial.println(" archivos .csv mas recientes de la sesion.");
    
    for (uint16_t i = 0; i < _fileCount; i++) {
        Serial.print("resultados: ");
        Serial.println(_filenames[i]);
    }

    return ESP_OK;
}

// Estructura auxiliar para pasar datos al callback de borrado de forma segura
struct DeleteContext {
    DataloggerFileManager* instance;
    uint32_t deletedCount;
};

void DataloggerFileManager::deleteInvalidFilesCallback(const char* fileName, bool isDir, void* context) {
    if (isDir) return; // Ignoramos directorios, solo queremos borrar archivos

    // Recuperamos el contexto
    DeleteContext* ctx = (DeleteContext*)context;
    DataloggerFileManager* self = ctx->instance;

    // 1. Construir la ruta completa del archivo que estamos evaluando en la SD
    // Dependiendo de la librería de Arduino, file.name() puede devolver la ruta completa o solo el nombre.
    char fullPath[FILE_NAME_SIZE];
    if (fileName[0] == '/') {
        snprintf(fullPath, sizeof(fullPath), "%s", fileName);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", self->_baseYearPath, fileName);
    }

    // 2. Comprobar si esta ruta existe dentro del array de archivos válidos (_filenames)
    bool isFileValid = false;
    for (uint16_t i = 0; i < self->_fileCount; i++) {
        if (strcmp(self->_filenames[i], fullPath) == 0) {
            isFileValid = true;
            break; // Es un archivo válido indexado, no se toca
        }
    }

    // 3. Si el archivo no es válido, proceder a su eliminación física
    if (!isFileValid) {
        Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] Eliminando archivo obsoleto: "); Serial.println(fullPath);
        
        esp_err_t err = self->_sd->deleteFile(fullPath);
        if (err == ESP_OK) {
            ctx->deletedCount++;
        } else {
            Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.print("] No se pudo borrar: "); Serial.println(fullPath);
        }
    }
}

esp_err_t DataloggerFileManager::deleteInvalidFiles() {
    if (!_initialized || strlen(_baseYearPath) == 0) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.println("] Entorno no inicializado para borrado.");
        return ESP_FAIL;
    }

    Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] Iniciando purga de archivos no indexados en: "); Serial.println(_baseYearPath);

    // Inicializamos el contexto de borrado pasándole esta misma instancia y el contador a 0
    DeleteContext ctx;
    ctx.instance = this;
    ctx.deletedCount = 0;

    // Escaneamos el directorio del año. El callback se encargará de discriminar y borrar
    esp_err_t err = _sd->listDirectory(_baseYearPath, deleteInvalidFilesCallback, &ctx);
    if (err != ESP_OK) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.println("] Error al escanear directorio durante la purga.");
        return err;
    }

    Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] Purga completada. Archivos eliminados: "); Serial.println(ctx.deletedCount);
    return ESP_OK;
}


esp_err_t DataloggerFileManager::setCSVLastEnvironment(bool delete_rest){

    esp_err_t err; 
    
    err = setLastSesion(); // Establece _baseYearPath
    if(err != ESP_OK) return err;
    
    err = setFilesLastSesion(); // Carga los nombres en _filenames y actualiza _fileCount
    if(err != ESP_OK) return err;

    // Establecemos el archivo actual de logs
    if (_fileCount > 0) {
        // Copiamos de forma segura el archivo MÁS RECIENTE (posición 0)
        snprintf(_currentLogFile, sizeof(_currentLogFile), "%s", _filenames[0]);
        Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] Log actual fijado en el mas reciente: "); Serial.println(_currentLogFile);
    } else {
        // Si la carpeta estaba vacía, el archivo actual se queda vacío por ahora hasta que newSesion() cree uno
        _currentLogFile[0] = '\0';
        Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.println("] No hay archivos previos. _currentLogFile inicializado vacio.");
    }
    
    if(delete_rest){ 
        err = deleteInvalidFiles(); 
        if(err != ESP_OK) return err;
    }

    return ESP_OK; 
}

esp_err_t DataloggerFileManager::newCSVLogSesion(const char * timestamp, const char** titles, uint16_t numTitles){
    
    if (numTitles == 0 || _currentLogFile[0] == '\0') return ESP_FAIL;

    char tempLine[256] = ""; // Búfer temporal para armar la línea del encabezado

    for (uint16_t i = 0; i < numTitles; i++) {
        if (titles[i] != nullptr) {
            strncat(tempLine, titles[i], sizeof(tempLine) - strlen(tempLine) - 1);
        }
        if (i < numTitles - 1) {
            strncat(tempLine, ";", sizeof(tempLine) - strlen(tempLine) - 1);
        }
    }
    strncat(tempLine, "\n", sizeof(tempLine) - strlen(tempLine) - 1); // Salto de línea CSV

    // Enviamos la línea armada al gestor del búfer
    m_pushToBuffer(tempLine);
    //borrar solo es prueba: 
    //_buffer.dumpToSerial();
    
    return ESP_OK;

} 

esp_err_t DataloggerFileManager::appendNewDataCSVToLog(const char* timestamp, const char** values, uint16_t numValues){

    if (_currentLogFile[0] == '\0') return ESP_FAIL;

    char tempLine[512]; // Ajusta el tamaño según la longitud máxima estimada de tus filas
    snprintf(tempLine, sizeof(tempLine), "%s", timestamp);

    for (uint16_t i = 0; i < numValues; i++) {
        strncat(tempLine, ";", sizeof(tempLine) - strlen(tempLine) - 1);
        if (values[i] != nullptr) {
            strncat(tempLine, values[i], sizeof(tempLine) - strlen(tempLine) - 1);
        }
    }
    strncat(tempLine, "\n", sizeof(tempLine) - strlen(tempLine) - 1); // Salto de línea CSV

    // Enviamos la fila armada al gestor del búfer
    m_pushToBuffer(tempLine);
    //_buffer.dumpToSerial();
    return ESP_OK;

}


// Callback exclusivo para vaciar el búfer completo a la SD
void flushLogBufferCallback(Stream& stream, void* context) {
    LogBuffer* buffer = (LogBuffer*)context;
    // Escribe todos los bytes acumulados de golpe
    stream.write(buffer->getBufferPointer(), buffer->getCurrentSize());
}

// Método público para forzar el volcado (por timeout o cierre)
bool DataloggerFileManager::flushBuffer() {
    // Si el búfer está vacío, no perdemos tiempo abriendo la SD
    if (_buffer.getCurrentSize() == 0 || _currentLogFile[0] == '\0') {
        return true;
    }

    ESP_LOGI("DATALOGGER", "Vaciando %d bytes del búfer a la SD...", _buffer.getCurrentSize());
    
    // Abrimos el archivo una única vez para volcar el bloque entero
    bool success = !_sd->withFileWrite(_currentLogFile, flushLogBufferCallback, &_buffer);
    
    if (success) {
        _buffer.clear(); // Reseteamos el búfer si se escribió correctamente
    }
    
    return success;
}

void DataloggerFileManager::m_pushToBuffer(const char* csvLine) {
    size_t lineLength = strlen(csvLine);

    // Si la línea actual no cabe en lo que queda de búfer, primero vaciamos la RAM a la SD
    if (lineLength > _buffer.getAvailableSpace()) {
        flushBuffer();
    }

    // Guardamos la línea en el búfer de forma segura
    _buffer.appendString(csvLine);
}
