#include "Datalogger.h"
#include <esp_err.h>

struct RowData {
    const char* timestamp;
    const char** values;
    uint16_t numValues;
};
struct HeaderData {
    const char** titles;
    uint16_t numTitles;
};

void writeHeaderCallback(Stream& stream, void* context);
void writeRowCallback(Stream& stream, void* context);
/**
 * @brief Initialize members and clear internal buffers.
 */
Datalogger::Datalogger(SDManager* sdManager, uint16_t maxFiles) : _sd(sdManager), _fileCount(0) {
    setMaxFiles(maxFiles);
    memset(_filenames, 0, sizeof(_filenames));
    _currentLogFile[0] = '\0';
}

void Datalogger::setMaxFiles(uint16_t maxFiles) {
    if (maxFiles > MAX_LOG_CAPACITY) {
        _userMaxFiles = MAX_LOG_CAPACITY;
    } else if (maxFiles == 0) {
        _userMaxFiles = 1; // Evitamos división por cero o errores lógicos
    } else {
        _userMaxFiles = maxFiles;
    }
}

/**
 * @brief Sets up the directory structure and performs an initial scan of the SD card.
 */
bool Datalogger::begin() {
    if (!_sd->isReady()) return ESP_ERR_SD_NOT_INIT;

    // Intentar crear/verificar directorio. Propagamos el error si falla.
    esp_err_t err = _sd->createDirectory(DIR_LOG_NAME);
    if (err != ESP_OK) return err;

    scanExistingLogs();
    return ESP_OK;
}

/**
 * @brief Scans the directory to track existing logs. 
 * Enables the "Delete Oldest" feature by knowing what is already on the card.
 */
 //TODO
 
 void Datalogger::scanExistingLogs() {
    File root = SD.open(DIR_LOG_NAME); 
    if (!root || !root.isDirectory()) return;

    _fileCount = 0;
    int newestIndex = -1;

    while (true) {
        File entry = root.openNextFile();
        if (!entry) break;

        if (!entry.isDirectory()) {
            const char* name = entry.name();
            if (hasLogExtension(name) && _fileCount < _userMaxFiles) {
                snprintf(_filenames[_fileCount], FILE_NAME_SIZE, "%s/%s", DIR_LOG_NAME, name);
                
                // Si es el primer archivo o es alfabéticamente mayor al que teníamos
                if (newestIndex == -1 || strcmp(_filenames[_fileCount], _filenames[newestIndex]) > 0) {
                    newestIndex = _fileCount;
                }
                
                _fileCount++;
            }
        }
        entry.close();
    }
    root.close();

    // Ahora seleccionamos el que realmente tiene el nombre con fecha/hora más alta
    if (newestIndex != -1) {
        selectLogByIndex(newestIndex); 
    }
}
 /*
void Datalogger::scanExistingLogs() {
    File root = SD.open(DIR_LOG_NAME); 
    if (!root || !root.isDirectory()) return;

    _fileCount = 0;

    while (true) {
        File entry = root.openNextFile();
        if (!entry) break;

        if (!entry.isDirectory()) {
            const char* name = entry.name();
            // Solo guardamos hasta el límite definido por el usuario
            if (hasLogExtension(name) && _fileCount < _userMaxFiles) {
                snprintf(_filenames[_fileCount], FILE_NAME_SIZE, "%s/%s", DIR_LOG_NAME, name);
                _fileCount++;
            }
        }
        entry.close();
    }
    root.close();

    if (_fileCount > 0) {
        // Usamos la función que ya tienes para seleccionar por índice
        selectLogByIndex(_fileCount - 1); 
    }
}
*/

/**
 * @brief Checks for valid file extensions (.txt or .log) using case-insensitive comparison.
 */
bool Datalogger::hasLogExtension(const char* filename) {
    size_t len = strlen(filename);
    if (len < 4) return false;
    
    const char* ext = filename + len - 4;
    // Añadimos la comparación con .csv
    return (//strcasecmp(ext, ".log") == 0 || 
            //strcasecmp(ext, ".txt") == 0 || 
            strcasecmp(ext, ".csv") == 0); // <--- Nueva extensión
}

/**
 * @brief Formats a filename with the system directory and triggers creation logic.
 */
bool Datalogger::newLog(const char* name) {
    char fullPath[FILE_NAME_SIZE];
    
    // Verificamos si el nombre ya trae .csv, si no, se lo ponemos
    if (strstr(name, ".csv") == nullptr) {
        snprintf(fullPath, sizeof(fullPath), "%s/%s.csv", DIR_LOG_NAME, name);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", DIR_LOG_NAME, name);
    }
    
    return addAndSetLogFile(fullPath);
}

/**
 * @brief Core logic for file rotation and creation.
 * 1. If the limit is reached, deletes the alphabetically "smallest" file (oldest date).
 * 2. If the filename already exists, appends a counter (e.g., _1, _2).
 */
bool Datalogger::addAndSetLogFile(const char* filename) {
    // 1. Manage Circular Buffer: Delete oldest if full
    if (_fileCount >= _userMaxFiles) {
        int oldestIndex = 0;
        
        // Find the "smallest" string (Format YYMMDDHH works perfectly here)
        for (int i = 1; i < _fileCount; i++) {
            if (strcmp(_filenames[i], _filenames[oldestIndex]) < 0) {
                oldestIndex = i;
            }
        }

        // Delete from SD and shift array to maintain order
        if (SD.remove(_filenames[oldestIndex])) {
            for (int i = oldestIndex; i < _fileCount - 1; i++) {
                strncpy(_filenames[i], _filenames[i + 1], FILE_NAME_SIZE);
            }
            _fileCount--; 
        } else {
            return false; 
        }
    }

    // 2. Prevent Duplicates (Logic to handle filename collisions)
    char baseName[FILE_NAME_SIZE];
    char extension[10];
    char finalPath[FILE_NAME_SIZE];
    
    const char *dot = strrchr(filename, '.');
    if (dot) {
        size_t baseLen = dot - filename;
        strncpy(baseName, filename, baseLen);
        baseName[baseLen] = '\0';
        strncpy(extension, dot, sizeof(extension) - 1);
        extension[sizeof(extension) - 1] = '\0';
    } else {
        strncpy(baseName, filename, sizeof(baseName) - 1);
        baseName[sizeof(baseName) - 1] = '\0';
        extension[0] = '\0';
    }

    uint16_t counter = 0;
    strncpy(finalPath, filename, FILE_NAME_SIZE - 1);

    // Increment suffix if file already exists
    while (_sd->exists(finalPath)) {
        counter++;
        snprintf(finalPath, FILE_NAME_SIZE, "%s_%u%s", baseName, counter, extension);
        if (counter > 999) break; 
    }

    // 3. Register the new file
    strncpy(_filenames[_fileCount], finalPath, FILE_NAME_SIZE - 1);
    _filenames[_fileCount][FILE_NAME_SIZE - 1] = '\0';
    strncpy(_currentLogFile, _filenames[_fileCount], FILE_NAME_SIZE - 1);
    
    _fileCount++;
    return !_sd->createFile(_currentLogFile);
}

/**
 * @brief Selects an active log based on internal array index.
 */
void Datalogger::selectLogByIndex(uint16_t index) {
    if (index < _fileCount) {
        strncpy(_currentLogFile, _filenames[index], FILE_NAME_SIZE - 1);
    }
}

/**
 * @brief Writes CSV header columns separated by semicolons.
 */
/**
 * @brief Writes CSV header columns using the file stream callback.
 */
bool Datalogger::writeHeader(const char** titles, uint16_t numTitles) {
    if (numTitles == 0 || _currentLogFile[0] == '\0') {
        return false;
    }

    HeaderData data = { titles, numTitles };
    
    // Ejecutamos la escritura a través del SDManager usando el callback
    return !_sd->withFileWrite(_currentLogFile, writeHeaderCallback, &data);
}

void writeHeaderCallback(Stream& stream, void* context) {
    HeaderData* data = (HeaderData*)context;
    File* file = (File*)&stream;

    for (uint16_t i = 0; i < data->numTitles; i++) {
        if (data->titles[i] != nullptr) {
            file->print(data->titles[i]);
        }
        
        // Imprimir punto y coma excepto en el último elemento
        if (i < data->numTitles - 1) {
            file->print(";");
        }
    }
    file->println(); // Salto de línea al final del encabezado
}

/**
 * @brief Appends a data row. Converts timestamp and value array into a semicolon-separated line.
 */
 bool Datalogger::writeRow(const char* timestamp, const char** values, uint16_t numValues) {
    RowData data = { timestamp, values, numValues };
    
    // Llamamos a withFile pasando la estructura de datos
    return !_sd->withFileWrite(_currentLogFile, writeRowCallback, &data);
}

/**
 * @brief Appends a data row. Converts timestamp and value array into a semicolon-separated line.
 */
void writeRowCallback(Stream& stream, void* context) {
    // El 'context' es un puntero a una estructura con nuestros datos
    RowData* data = (RowData*)context;
    File* file = (File*)&stream; // Cast a File para usar funciones de archivo

    file->print(data->timestamp);
    for (uint16_t i = 0; i < data->numValues; i++) {
        file->print(";");
        if (data->values[i] != nullptr) {
            file->print(data->values[i]);
        }
    }
    file->println(); // Final de línea
}

 /*
bool Datalogger::writeRow(const char* timestamp, const char** values, uint16_t numValues) {
    if (numValues == 0 || _currentLogFile[0] == '\0') return false;

    char buffer[512]; 
    snprintf(buffer, sizeof(buffer), "%s", timestamp);

    for (uint16_t i = 0; i < numValues; i++) {
        strncat(buffer, ";", sizeof(buffer) - strlen(buffer) - 1);
        if (values[i] != nullptr) {
            strncat(buffer, values[i], sizeof(buffer) - strlen(buffer) - 1);
        }
    }

    return _sd->appendLine(_currentLogFile, buffer);
}
*/

/**
 * @brief Resets the current file to empty state.
 */
void Datalogger::clearLogFile() {
    if (strlen(_currentLogFile) > 0) {
        _sd->clearFile(_currentLogFile);
    }
}

/**
 * @brief Prints current log path and its contents to Serial.
 */
void Datalogger::printLogToSerial() {
    if (strlen(_currentLogFile) == 0) return;
    
    Serial.print(F("--- Log Content: "));
    Serial.print(_currentLogFile);
    Serial.println(F(" ---"));

    _sd->printFileToSerial(_currentLogFile);
}

/**
 * @brief Orchestrates the start of a logging session: New file -> Clear -> Header.
 */
bool Datalogger::newSesion(const char * name, const char** titles, uint16_t numTitles){
    if(!newLog(name)) return false; 
    
    clearLogFile();

    // Prepare header including an automatic Timestamp column
    uint16_t totalTitulos = numTitles + 1;
    const char* cabeceraCompleta[totalTitulos];
    cabeceraCompleta[0] = "Timestamp";

    for (uint16_t i = 0; i < numTitles; i++) {
        cabeceraCompleta[i + 1] = titles[i];
    }

    return writeHeader(cabeceraCompleta, totalTitulos); 
}

/**
 * @brief Wipes the /LOGS directory and resets internal tracking.
 */
 //TODO
void Datalogger::clearAllLogs() {
    File root = SD.open(DIR_LOG_NAME);
    if (!root || !root.isDirectory()) return;

    while (true) {
        File entry = root.openNextFile();
        if (!entry) break; 

        if (!entry.isDirectory()) {
            char fullPath[FILE_NAME_SIZE + 16]; 
            snprintf(fullPath, sizeof(fullPath), "%s/%s", DIR_LOG_NAME, entry.name());
            
            entry.close(); 
            SD.remove(fullPath); 
        } else {
            entry.close();
        }
    }
    root.close();

    _fileCount = 0;
    memset(_filenames, 0, sizeof(_filenames));
    memset(_currentLogFile, 0, sizeof(_currentLogFile));
}