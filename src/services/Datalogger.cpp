#include "Datalogger.h"

/**
 * @brief Initialize members and clear internal buffers.
 */
Datalogger::Datalogger(SDManager* sdManager) : _sd(sdManager), _fileCount(0) {
    memset(_filenames, 0, sizeof(_filenames));
    _currentLogFile[0] = '\0';
}

/**
 * @brief Sets up the directory structure and performs an initial scan of the SD card.
 */
bool Datalogger::begin() {
    if (!_sd->isReady()) return false;

    // Create the base log directory if it doesn't exist
    if (!_sd->createDirectory(DIR_LOG_NAME)) return false;

    scanExistingLogs();
    
    // Ensure the current log pointer is empty until a session starts
    memset(_currentLogFile, 0, sizeof(_currentLogFile));
    return true;
}

/**
 * @brief Scans the directory to track existing logs. 
 * Enables the "Delete Oldest" feature by knowing what is already on the card.
 */
void Datalogger::scanExistingLogs() {
    File root = SD.open(DIR_LOG_NAME); 
    if (!root || !root.isDirectory()) return;

    _fileCount = 0;

    while (true) {
        File entry = root.openNextFile();
        if (!entry) break;

        if (!entry.isDirectory()) {
            const char* name = entry.name();
            
            if (hasLogExtension(name) && _fileCount < MAX_LOG_FILES) {
                // Store full path for direct SD access
                snprintf(_filenames[_fileCount], FILE_NAME_SIZE, "%s/%s", DIR_LOG_NAME, name);
                _fileCount++;
            }
        }
        entry.close();
    }
    root.close();
}

/**
 * @brief Checks for valid file extensions (.txt or .log) using case-insensitive comparison.
 */
bool Datalogger::hasLogExtension(const char* filename) {
    size_t len = strlen(filename);
    if (len < 4) return false;
    
    const char* ext = filename + len - 4;
    return (strcasecmp(ext, ".log") == 0 || strcasecmp(ext, ".txt") == 0);
}

/**
 * @brief Formats a filename with the system directory and triggers creation logic.
 */
bool Datalogger::newLog(const char* name) {
    char fullPath[FILE_NAME_SIZE];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", DIR_LOG_NAME, name);
    return addAndSetLogFile(fullPath);
}

/**
 * @brief Core logic for file rotation and creation.
 * 1. If the limit is reached, deletes the alphabetically "smallest" file (oldest date).
 * 2. If the filename already exists, appends a counter (e.g., _1, _2).
 */
bool Datalogger::addAndSetLogFile(const char* filename) {
    // 1. Manage Circular Buffer: Delete oldest if full
    if (_fileCount >= MAX_LOG_FILES) {
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
            return false; // SD Error or file locked
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
    return _sd->createFile(_currentLogFile);
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
bool Datalogger::writeHeader(const char** titulos, uint16_t numTitulos) {
    if (numTitulos == 0 || strlen(_currentLogFile) == 0) return false;

    char buffer[256]; 
    buffer[0] = '\0'; 

    for (uint16_t i = 0; i < numTitulos; i++) {
        strncat(buffer, titulos[i], sizeof(buffer) - strlen(buffer) - 1);
        if (i < numTitulos - 1) {
            strncat(buffer, ";", sizeof(buffer) - strlen(buffer) - 1);
        }
    }

    return _sd->appendLine(_currentLogFile, buffer);
}

/**
 * @brief Appends a data row. Converts timestamp and value array into a semicolon-separated line.
 */
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