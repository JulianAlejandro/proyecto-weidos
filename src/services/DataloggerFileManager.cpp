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
    if(err != ESP_OK) return err; 

    err = setErrorLog(); // inicializamos el path de errores 

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

/**
 * @brief Extrae el año y el timestamp numérico del archivo de log actual activo.
 * Si el archivo está vacío, inicializa los valores en 0.
 */
void DataloggerFileManager::setLastLogTime() {
    // 1. Si no hay archivo de log actual fijado, inicializamos a 0
    if (_currentLogFile[0] == '\0') {
        _intLastYearLog = 0;
        _intLastTimestampLog = 0;
        Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.println("] setLastLogTime: No hay log previo. Valores seteados a 0.");
        return;
    }

    // 2. Buscar el año en la ruta. Buscamos la primera barra desde el final
    // Ejemplo: /LOGS/2026/05201430.csv
    const char* lastSlash = strrchr(_currentLogFile, '/');
    if (lastSlash == nullptr) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.println("] setLastLogTime: Formato de ruta inválido.");
        return;
    }

    // El nombre del archivo empieza justo después de la última barra
    const char* fileName = lastSlash + 1;

    // Para encontrar el año, buscamos la penúltima barra
    // Copiamos la ruta temporalmente para poder manipularla de forma segura
    char tempPath[FILE_NAME_SIZE];
    snprintf(tempPath, sizeof(tempPath), "%s", _currentLogFile);
    
    // Reemplazamos la última barra por un terminador nulo para "recortar" el nombre del archivo
    size_t slashIndex = lastSlash - _currentLogFile;
    tempPath[slashIndex] = '\0'; // Ahora tempPath es "/LOGS/2026"

    // Buscamos la barra que precede al año
    const char* yearSlash = strrchr(tempPath, '/');
    if (yearSlash != nullptr) {
        _intLastYearLog = (uint16_t)atoi(yearSlash + 1); // Convierte "2026" -> 2026
    } else {
        _intLastYearLog = 0;
    }

    // 3. Extraer el timestamp del nombre del archivo (ej: "05201430.csv")
    // Copiamos los primeros 8 caracteres que corresponden a MMDDHHMM
    char tokenTimestamp[9];
    strncpy(tokenTimestamp, fileName, 8);
    tokenTimestamp[8] = '\0';

    // Convertimos la cadena "05201430" a uint32_t de manera segura
    _intLastTimestampLog = (uint32_t)strtoul(tokenTimestamp, nullptr, 10);

    // 4. Debug por consola para verificar la extracción correcta
    Serial.print("[INFO] ["); Serial.print(TAG_DFM); 
    Serial.print("] Ultimo log detectado -> Año: "); Serial.print(_intLastYearLog);
    Serial.print(" | Timestamp: "); Serial.println(_intLastTimestampLog);
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
    //actualizar el numero en base a basyear y timestamp...
    setLastLogTime(); 

    if(delete_rest){ 
        err = deleteInvalidFiles(); 
        if(err != ESP_OK) return err;
    }

    return ESP_OK; 
}


bool getDataTimestamp(const char* timestamp, uint16_t* outYear, uint32_t* outSession) {
    // 1. Validaciones de seguridad iniciales
    if (timestamp == nullptr || outYear == nullptr || outSession == nullptr) {
        return false;
    }

    size_t len = strlen(timestamp);
    if (len != 14) { 
        Serial.print("[ERROR] Longitud de timestamp incorrecta: "); Serial.println(len);
        return false;
    }

    // 2. Validar que todos los caracteres sean números
    for (size_t i = 0; i < 14; i++) { 
        if (timestamp[i] < '0' || timestamp[i] > '9') {
            Serial.println("[ERROR] El timestamp contiene caracteres no numéricos.");
            return false;
        }
    }

    // 3. Extracción del AÑO (Primeros 4 caracteres)
    char tempYear[5];
    strncpy(tempYear, timestamp, 4);
    tempYear[4] = '\0';
    *outYear = (uint16_t)atoi(tempYear); // Ej: "2026" -> 2026

    // 4. Extracción de la SESIÓN (Siguientes 8 caracteres: MMDDhhmm)
    // Saltamos los primeros 4 caracteres del año (timestamp + 4)
    char tempSession[9];
    strncpy(tempSession, timestamp + 4, 8);
    tempSession[8] = '\0';
    *outSession = (uint32_t)strtoul(tempSession, nullptr, 10); // Ej: "06271240" -> 6271240

    return true;
}



/**
 * @brief Crea una nueva carpeta de año dentro de /LOGS y actualiza el entorno de trabajo.
 * @param year Entero con el año de 4 dígitos (ej: 2027).
 * @return ESP_OK si se creó con éxito, o el código de error correspondiente.
 */
esp_err_t DataloggerFileManager::newYearFile(uint16_t year) {
    if (!_initialized) return ESP_FAIL;
    if (year < 2026 || year > 2100) { // Validación de rango básico para control de errores
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.println("] Año fuera de rango para nueva sesión.");
        return ESP_ERR_INVALID_ARG;
    }

    // Construir la nueva ruta de la carpeta (ej: "/LOGS/2027")
    char newYearPath[FILE_NAME_SIZE];
    snprintf(newYearPath, sizeof(newYearPath), "%s/%u", DIR_LOG_NAME, year);

    Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] Detectado cambio de año. Creando directorio: "); Serial.println(newYearPath);

    esp_err_t err = _sd->createDirectory(newYearPath);
    if (err != ESP_OK) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.print("] Fallo crítico al crear el directorio del nuevo año: "); Serial.println(newYearPath);
        return err;
    }

    return ESP_OK;
}

/**
 * @brief Crea una nueva carpeta de año dentro de /LOGS y actualiza el entorno de trabajo.
 * @param year Cadena con el año de 4 dígitos (ej: "2027").
 * @return ESP_OK si se creó y configuró con éxito, o el código de error correspondiente.
 */
 /*
esp_err_t DataloggerFileManager::newYearFile(char* year) {
    // 1. Validaciones de seguridad iniciales
    if (!_initialized) return ESP_FAIL;
    if (year == nullptr || strlen(year) != 4) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.println("] Año inválido para nueva sesión.");
        return ESP_ERR_INVALID_ARG;
    }

    // 2. Construir la nueva ruta de la carpeta (ej: "/LOGS/2027")
    char newYearPath[FILE_NAME_SIZE];
    snprintf(newYearPath, sizeof(newYearPath), "%s/%s", DIR_LOG_NAME, year);

    Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] Detectado cambio de año. Creando directorio: "); Serial.println(newYearPath);

    // 3. Crear el directorio físicamente en la SD usando tu SDManager
    esp_err_t err = _sd->createDirectory(newYearPath);
    if (err != ESP_OK) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.print("] Fallo crítico al crear el directorio del nuevo año: "); Serial.println(newYearPath);
        return err;
    }




   // // 4. Actualizar el atributo de la clase para que apunte al nuevo año de trabajo
   // snprintf(_baseYearPath, sizeof(_baseYearPath), "%s", newYearPath);
//
   // // 5. Reinicializar el índice de archivos para esta nueva carpeta vacía
   // // Esto limpia los archivos indexados del año pasado en la RAM (_filenames)
   // err = setFilesLastSesion();
   // if (err != ESP_OK) {
   //     Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.println("] Error al indexar el nuevo entorno de año.");
   //     return err;
   // }
//
   // Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] Entorno actualizado con éxito al año: "); Serial.println(_baseYearPath);
    
    return ESP_OK;
}
*/


esp_err_t DataloggerFileManager::newFileLog(const char* timestamp) {

    // 1. Validaciones de estado y argumentos iniciales
    if (!_initialized || strlen(_baseYearPath) == 0) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.println("] Entorno no inicializado. Imposible crear log.");
        return ESP_FAIL;
    }

    if (timestamp == nullptr || strlen(timestamp) == 0) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.println("] Timestamp inválido (puntero nulo o vacío).");
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t incomingYear = 0;
    uint32_t incomingTimestamp = 0; 

    // Extraemos de forma segura los valores numéricos del timestamp entrante
    if (!getDataTimestamp(timestamp, &incomingYear, &incomingTimestamp)) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.println("] Error al parsear el timestamp entrante.");
        return ESP_ERR_INVALID_ARG;
    }

    // 2. Control de tiempo estricto (Evitar escribir en el pasado de la SD)
    
    // CASO A: El año entrante es menor que el último registrado
    if (incomingYear < _intLastYearLog) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); 
        Serial.print("] Rechazado: Año entrante ("); Serial.print(incomingYear);
        Serial.print(") es más antiguo que el último log ("); Serial.print(_intLastYearLog); Serial.println(").");
        return ESP_ERR_INVALID_ARG; // Retorna error porque viene del pasado
    } 
    
    // CASO B: El año entrante es significativamente mayor (Cambio de año)
    else if (incomingYear > _intLastYearLog) {
        Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.println("] Cambio de año detectado de forma síncrona.");
        
        esp_err_t err = newYearFile(incomingYear); 
        if (err != ESP_OK) return err;

        // Establecemos el entorno completo en la nueva carpeta borrando archivos inválidos si es necesario
        err = setCSVLastEnvironment(true); 
        if (err != ESP_OK) return err; 
    } 
    
    // CASO C: Estamos en el mismo año, validamos el timestamp numérico (MMDDhhmm)
    else {
        if (incomingTimestamp <= _intLastTimestampLog) {
            Serial.print("[ERROR] ["); Serial.print(TAG_DFM); 
            Serial.print("] Rechazado: El timestamp de la sesión ("); Serial.print(incomingTimestamp);
            Serial.print(") no es mayor que el último registrado ("); Serial.print(_intLastTimestampLog); Serial.println(").");
            return ESP_ERR_INVALID_ARG; // Retorna error: no se permiten duplicados ni marcas de tiempo pasadas
        }
    }

    // 3. Construir la ruta completa del nuevo archivo (ej: /LOGS/2026/06271240.csv)
    char newFilePath[FILE_NAME_SIZE];
    snprintf(newFilePath, sizeof(newFilePath), "%s/%08u.csv", _baseYearPath, incomingTimestamp);

    Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] Solicitud de nuevo log aprobada: "); Serial.println(newFilePath);

    // 4. Control de capacidad máxima de archivos mediante rotación dinámica (_userMaxFiles)
    if (_fileCount >= _userMaxFiles) {
        uint16_t oldestIndex = _fileCount - 1; // El más antiguo está en el último índice del histórico indexado
        Serial.print("[WARN] ["); Serial.print(TAG_DFM); Serial.print("] Máximo de archivos alcanzado. Purgando archivo físico antiguo: ");
        Serial.println(_filenames[oldestIndex]);

        // Borrado físico en la memoria SD
        esp_err_t delErr = _sd->deleteFile(_filenames[oldestIndex]);
        if (delErr != ESP_OK) {
            Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.println("] No se pudo borrar el archivo antiguo físicamente. Abortando.");
            return delErr;
        }

        // Limpiamos la RAM de ese slot
        memset(_filenames[oldestIndex], 0, FILE_NAME_SIZE);
        _fileCount--;
    }

    // 5. Crear el archivo vacío en el almacenamiento físico
    esp_err_t createErr = _sd->createFile(newFilePath);
    if (createErr != ESP_OK) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.print("] Fallo al crear el archivo físico en la SD: "); Serial.println(newFilePath);
        return createErr;
    }

    // 6. Desplazar los punteros del array a la derecha para indexar el archivo nuevo en la posición [0]
    for (int i = _fileCount; i > 0; i--) {
        strncpy(_filenames[i], _filenames[i - 1], FILE_NAME_SIZE);
    }

    // Insertar la nueva ruta al inicio del índice de sesión activo
    strncpy(_filenames[0], newFilePath, FILE_NAME_SIZE);
    _fileCount++;

    // Actualizar el puntero global de escritura de logs del sistema
    strncpy(_currentLogFile, _filenames[0], sizeof(_currentLogFile));

    // 7. ACTUALIZACIÓN CRÍTICA: Fijar la línea de tiempo del Datalogger a los nuevos valores máximos
    _intLastYearLog = incomingYear;
    _intLastTimestampLog = incomingTimestamp;

    Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] Nuevo archivo fijado y activo: "); Serial.println(_currentLogFile);
    Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] Línea de tiempo actualizada -> Año: "); Serial.print(_intLastYearLog);
    Serial.print(" | Timestamp: "); Serial.println(_intLastTimestampLog);

    return ESP_OK;
}


/*
esp_err_t DataloggerFileManager::newCSVLogSesion(const char * timestamp, const char** titles, uint16_t numTitles){
    
    if (numTitles == 0 || _currentLogFile[0] == '\0') return ESP_FAIL;

    esp_err_t err = newFileLog(timestamp); 
    if(err != ESP_OK) return err; // Si no se crea el fichero no tiene sentido seguir 

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
    //TODO AHORA ESTO SE HACE EXTERNAMENTE A LA CLASE
    //m_pushToBuffer(tempLine);
    //borrar solo es prueba: 
    //_buffer.dumpToSerial();
    
    return ESP_OK;
}
*/

/*
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
    //todo ahora esto se hace externamente a la clase 
   // m_pushToBuffer(tempLine);
    //_buffer.dumpToSerial();
    return ESP_OK;

}
*/

/*
// Callback exclusivo para vaciar el búfer completo a la SD
void flushLogBufferCallback(Stream& stream, void* context) {
    LogBuffer* buffer = (LogBuffer*)context;
    // Escribe todos los bytes acumulados de golpe
    stream.write(buffer->getBufferPointer(), buffer->getCurrentSize());
}
*/
/*
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

*/

/*
void DataloggerFileManager::m_pushToBuffer(const char* csvLine) {
    size_t lineLength = strlen(csvLine);

    // Si la línea actual no cabe en lo que queda de búfer, primero vaciamos la RAM a la SD
    if (lineLength > _buffer.getAvailableSpace()) {
        flushBuffer();
    }

    // Guardamos la línea en el búfer de forma segura
    _buffer.appendString(csvLine);
}
*/


/**
 * @brief Asegura la existencia del archivo de errores. Si no existe, lo crea vacío.
 * @return ESP_OK si ya existe o se creó con éxito. ESP_FAIL si hay fallos.
 */
esp_err_t DataloggerFileManager::setErrorLog() {
    
    if (_sd->exists(PATH_ERR_LOG)) {
        Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.println("] Archivo de errores detectado y listo.");
        return ESP_OK;
    }

    // Si no existe, lo creamos
    Serial.print("[INFO] ["); Serial.print(TAG_DFM); Serial.print("] Creando archivo de errores en: "); Serial.println(PATH_ERR_LOG);
    esp_err_t err = _sd->createFile(PATH_ERR_LOG);
    
    if (err != ESP_OK) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.println("] No se pudo crear el archivo de errores.");
        return err;
    }

    return ESP_OK;
}

// Callback auxiliar estático y privado (puedes ponerlo justo arriba de appendErrorLog)
// Se encarga de escribir la línea de texto directamente en el archivo abierto por el SDManager
void writeErrorCallback(Stream& stream, void* context) {
    const char* errorLine = (const char*)context;
    stream.print(errorLine);
}
 
/**
 * @brief Escribe una línea de error en formato "TIMESTAMP;MENSAJE\n" directamente en la SD.
 * @param timestamp Sello de tiempo del evento.
 * @param err_message Descripción del error.
 * @return ESP_OK si la escritura fue exitosa.
 */
esp_err_t DataloggerFileManager::appendErrorLog(const char* timestamp, const char* err_message) {
    if (!_initialized) return ESP_FAIL;

    // Validaciones de seguridad de los argumentos
    if (timestamp == nullptr || err_message == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    // 1. Asegurar que el archivo de errores existe antes de intentar abrirlo para escritura
    esp_err_t err = setErrorLog();
    if (err != ESP_OK) return err;

    // 2. Construir la línea del log de errores de forma segura (ej: "06271240;Fallo de lectura de sensor DHT\n")
    char tempLine[512];
    snprintf(tempLine, sizeof(tempLine), "%s;%s\n", timestamp, err_message);

    // 3. Escribir directamente en la SD de forma síncrona
    // Se utiliza withFileWrite que abre el archivo en modo FILE_WRITE (el cual en Arduino por defecto es Append / Añadir)
    bool success = !_sd->withFileWrite(PATH_ERR_LOG, writeErrorCallback, tempLine);

    if (!success) {
        Serial.print("[ERROR] ["); Serial.print(TAG_DFM); Serial.println("] Fallo al escribir el error en la SD.");
        return ESP_ERR_SD_WRITE_FAIL; // O el código de error de escritura que utilices
    }

    Serial.print("[WARN] ["); Serial.print(TAG_DFM); Serial.print("] Error registrado en SD: "); Serial.print(tempLine);
    return ESP_OK;
}

char* DataloggerFileManager::getCurrentLogPath(){
    return _currentLogFile; 
}


/*
char* DataloggerFileManager::getCurrentBaseYearPath(){
    return _baseYearPath;
}
*/