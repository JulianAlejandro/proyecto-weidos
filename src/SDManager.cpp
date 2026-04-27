//aqui copiar 

#include "SDManager.h"

#include <SPI.h>

SDManager::SDManager() {
    // Cuerpo vacío
}


//SDManager::SDManager(uint8_t csPin) : _csPin(csPin), _initialized(false) {}

// TODO: pensar que mas cosas tiene que hacer el administrador de SD
bool SDManager::begin() {
    // Si ya estaba inicializada, no repetimos el proceso
    if (_initialized) return true;

    // SD.begin() devuelve true si la comunicación SPI 
    // y el sistema de archivos FAT están listos.
    _initialized = SD.begin(); 
    
    return _initialized;
}


bool SDManager::createFile(const char* path) {
    // Si el archivo ya existe, no hacemos nada y retornamos true
    if (SD.exists(path)) {
        return true; 
    }

    // Intentamos abrirlo en modo escritura con creación (O_CREAT)
    // Al cerrarlo inmediatamente, queda creado como un archivo vacío de 0 bytes
    File f = SD.open(path, FILE_WRITE);
    if (f) {
        f.close();
        return true;
    }

    return false; // Error al crear (ej: tarjeta llena o nombre inválido)
}

bool SDManager::isReady() {
    return _initialized;
}

/*
bool SDManager::isReady() {
    // Si ni siquiera se ha llamado a begin una vez, es false
    if (!_initialized) return false;

    // Intentamos verificar si el volumen sigue siendo accesible
    // exists("/") es una operación rápida que confirma que el sistema FAT responde
    if (!SD.exists("/")) {
        _initialized = false; // Si falla, marcamos como no listo
        return false;
    }
    
    return true;
}
*/

bool SDManager::exists(const char* path) {
    return SD.exists(path);
}

//borrar el contenido dentro de un fichero
void SDManager::clearFile(const char* path) {

    File f = SD.open(path, O_WRITE | O_CREAT | O_TRUNC); 
    if (f) f.close(); 
}

bool SDManager::appendLine(const char* path, const char* data) {
    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;
    f.println(data);
    f.close();
    return true;

}


void SDManager::printFileToSerial(const char* path) {
    File f = SD.open(path, FILE_READ);
    if (!f) {
        Serial.println(F("Error: No se pudo abrir el archivo para lectura Serial."));
        return;
    }
    
    while (f.available()) {
        Serial.write(f.read());
    }
    f.close();
}


bool SDManager::createDirectory(const char* path) {
    if (!_initialized) return false;

    // SD.exists() funciona tanto para archivos como para directorios
    if (SD.exists(path)) {
        return true; 
    }

    // SD.mkdir devuelve true si tuvo éxito
    return SD.mkdir(path);
}

//devuelve cada linea del fichero en un callback
bool SDManager::getAllLines(const char* path, LineCallback callback, void* context) {
    if (!_initialized) return false;
    File file = SD.open(path, FILE_READ);
    if (!file) return false;

    char lineBuffer[128];
    while (file.available()) {
        int bytesRead = file.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
        lineBuffer[bytesRead] = '\0';
        
        if (bytesRead > 0) {
            // Le pasamos la línea y el contexto de vuelta al callback
            callback(lineBuffer, context);
        }
    }
    file.close();
    return true;
}


