//aqui copiar 

#include "SDManager.h"

#include <SPI.h>

//TODO añadir un _initialize para que sea mas robusta la aplicacion, de momento no esta para no meter mas cosas de las necesarias 

SDManager::SDManager() {
    // Cuerpo vacío
}

//SDManager::SDManager(uint8_t csPin) : _csPin(csPin), _initialized(false) {}

// TODO: pensar que mas cosas tiene que hacer el administrador de SD
bool SDManager::begin() {

    if (!SD.begin()) {
        return false;
    }
    return true; 

    /*if (_initialized) return true;
    _initialized = SD.begin(_csPin);
    return _initialized;*/
    
}

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


bool SDManager::getLineByID(const char* path, const char* id, char* destBuffer, size_t bufferSize) {
    File file = SD.open(path);
    if (!file) {
        Serial.println(F("Error: No se pudo abrir el archivo"));
        return false;
    }

    // Limpiamos el buffer de destino por seguridad
    memset(destBuffer, 0, bufferSize);

    while (file.available()) {
        // Leemos la línea directamente al buffer de destino para ahorrar RAM
        // Usamos bufferSize - 1 para dejar sitio al carácter nulo final '\0'
        int bytesRead = file.readBytesUntil('\n', destBuffer, bufferSize - 1);
        destBuffer[bytesRead] = '\0'; // Terminamos la cadena manualmente

        // Si la línea está vacía (solo un \r\n), saltamos
        if (bytesRead == 0) continue;

        // Buscamos el primer ';' para aislar el ID de la línea
        char* primerPuntoComa = strchr(destBuffer, ';');
        
        if (primerPuntoComa != nullptr) {
            // Calculamos la longitud del ID encontrado en la línea
            size_t idLength = primerPuntoComa - destBuffer;

            // Comparamos el ID buscado con el ID de la línea
            // strncmp compara solo los caracteres hasta el ';'
            if (strlen(id) == idLength && strncmp(destBuffer, id, idLength) == 0) {
                file.close();
                return true; // ¡Encontrado! El resto de la línea ya está en destBuffer
            }
        }
    }

    file.close();
    return false; // No se encontró el ID
}

void SDManager::getLinesByRange(const char* path, long start, long end, LineCallback callback) {
    File file = SD.open(path, FILE_READ);
    if (!file) {
        Serial.println(F("Error: No se pudo abrir archivo para rango"));
        return;
    }

    char lineBuffer[128]; // Buffer para leer cada línea. Ajusta el tamaño según tu archivo setup.

    while (file.available()) {
        // Leemos la línea directamente al buffer (ahorro máximo de RAM)
        int bytesRead = file.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
        lineBuffer[bytesRead] = '\0'; // Terminador nulo

        // Buscamos el ID (primer campo antes del ';')
        char* sepIdx = strchr(lineBuffer, ';');
        if (sepIdx != nullptr) {
            // Convertimos temporalmente el primer campo a número
            // (Hacemos que el ';' sea un '\0' momentáneamente para usar atol)
            *sepIdx = '\0';
            long idFound = atol(lineBuffer);
            *sepIdx = ';'; // Restauramos el ';'

            // Si está en el rango, disparamos el callback
            if (idFound >= start && idFound <= end) {
                callback(lineBuffer);
            }
        }
    }
    file.close();
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


bool SDManager::openFile(const char* path){
    _currentFile = SD.open(path, FILE_READ);
    return _currentFile;
}


bool SDManager::getNextLineInRange(uint16_t start_addr, uint16_t size, char* buffer, size_t buffer_size) {
    if (!_currentFile || !_currentFile.available()) return false;

    // Calculamos el límite superior (20,000 + 125 no desbordará un uint16_t)
    uint16_t end_addr = start_addr + size;

    while (_currentFile.available()) {
        // Leemos la línea hasta el salto de línea
        int bytesRead = _currentFile.readBytesUntil('\n', buffer, buffer_size - 1);
        buffer[bytesRead] = '\0';

        // Si la línea está vacía (solo un \n), saltamos a la siguiente
        if (bytesRead == 0) continue;

        // Buscamos el delimitador ';' para extraer la dirección (ID)
        char* sepIdx = strchr(buffer, ';');
        if (sepIdx != nullptr) {
            char originalChar = *sepIdx;
            *sepIdx = '\0'; // Truncamos temporalmente para la conversión
            
            // Convertimos a entero sin signo de 16 bits
            // strtoul es más seguro que atoi para uint16_t
            uint16_t idFound = (uint16_t)strtoul(buffer, NULL, 10);
            
            *sepIdx = originalChar; // Restauramos el carácter original

            // Comparamos el ID encontrado con el rango definido
            if (idFound >= start_addr && idFound < end_addr) {
                return true; 
            }
        }
    }
    return false; 
}
/*
bool SDManager::getNextLineInRange(long start_addr, long size, char* buffer, size_t buffer_size) {
    if (!_currentFile || !_currentFile.available()) return false;

    // Calculamos el límite superior basado en el tamaño solicitado
    long end_addr = start_addr + size;

    while (_currentFile.available()) {
        // Leemos la línea hasta el salto de línea
        int bytesRead = _currentFile.readBytesUntil('\n', buffer, buffer_size - 1);
        buffer[bytesRead] = '\0';

        // Si la línea está vacía (solo un \n), saltamos a la siguiente
        if (bytesRead == 0) continue;

        // Buscamos el delimitador ';' para extraer el ID
        char* sepIdx = strchr(buffer, ';');
        if (sepIdx != nullptr) {
            char originalChar = *sepIdx;
            *sepIdx = '\0'; // Truncamos temporalmente para usar atol
            long idFound = atol(buffer);
            *sepIdx = originalChar; // Restauramos el carácter original

            // Comparamos el ID encontrado con el nuevo rango definido
            if (idFound >= start_addr && idFound <= end_addr) {
                return true; 
            }
        }
    }
    return false; 
}
*/

void SDManager::closeFile(){
    if (_currentFile) _currentFile.close();
}

