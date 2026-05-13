#include "SDManager.h"
#include <SPI.h>

/**
 * @brief Default constructor.
 */
SDManager::SDManager() {}

/**
 * @brief Starts the SD card hardware communication.
 * Validates the SPI connection and FAT file system mounting.
 */
bool SDManager::begin() {
    // Avoid re-initialization if already active
    if (_initialized) return true;

    // Standard SD.begin() returns true if SPI hardware and FAT system respond
    _initialized = SD.begin(); 
    
    return _initialized;
}

/**
 * @brief Returns the initialization status.
 */
bool SDManager::isReady() {
    return _initialized;
}

/**
 * @brief Ensures a file exists. If it doesn't, it creates an empty one.
 */
bool SDManager::createFile(const char* path) {
    if (SD.exists(path)) {
        return true; 
    }
    // Open in WRITE mode to force file creation
    File f = SD.open(path, FILE_WRITE);
    if (f) {
        f.close();
        return true;
    }
    return false; // Creation failed (e.g., SD full or invalid name)
}

/**
 * @brief Checks existence of a resource at the given path.
 */
bool SDManager::exists(const char* path) {
    return SD.exists(path);
}

/**
 * @brief Overwrites an existing file with an empty state.
 * Uses O_TRUNC to reset file size to 0.
 */
void SDManager::clearFile(const char* path) {
    File f = SD.open(path, O_WRITE | O_CREAT | O_TRUNC); 
    if (f) f.close(); 
}

/**
 * @brief Opens file in append mode and adds a new line of data.
 */
/*
bool SDManager::appendLine(const char* path, const char* data) {
    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;
    
    f.println(data); // Appends data + line jump (\n)
    f.close();
    return true;
}
*/
/**
 * @brief Utility for debugging. Prints the full file content to Serial.
 */
void SDManager::printFileToSerial(const char* path) {
    File f = SD.open(path, FILE_READ);
    if (!f) {
        //Serial.println(F("SDManager Error: Could not open file for Serial output."));
        return;
    }
    
    while (f.available()) {
        Serial.write(f.read());
    }
    f.close();
}

/**
 * @brief Creates a directory structure.
 */
bool SDManager::createDirectory(const char* path) {
    if (!_initialized) return false;

    if (SD.exists(path)) {
        return true; 
    }

    return SD.mkdir(path);
}

/**
 * @brief Deletes a file from the SD card.
 * @param path Full path to the file.
 * @return true if the file was successfully deleted or didn't exist, false if deletion failed.
 */
bool SDManager::deleteFile(const char* path) {
    if (!_initialized) return false;

    // Si el archivo no existe, técnicamente la tarea de "eliminarlo" es un éxito
    if (!SD.exists(path)) return true;

    return SD.remove(path);
}

/**
 * @brief Higher-order function to handle file streaming via callbacks.
 * Ensures the file is safely closed after the callback execution.
 */
bool SDManager::withFile(const char* path, StreamCallback callback, void* context) {
    if (!_initialized) return false;

    File file = SD.open(path);
    if (!file) return false;

    // Pass the file stream to the external processing function
    callback(file, context);
    
    file.close();
    return true;
}


// Versión modificada de withFile para permitir escritura
bool SDManager::withFileWrite(const char* path, StreamCallback callback, void* context) {
    if (!_initialized) return false;

    File file = SD.open(path, FILE_WRITE); // Ahora usa el modo pasado por parámetro
    if (!file) return false;

    callback(file, context);
    
    file.close();
    return true;
}