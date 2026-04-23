#include "Datalogger.h"

Datalogger::Datalogger(SDManager* sdManager) : _sd(sdManager), _fileCount(0) {
    memset(_filenames, 0, sizeof(_filenames));
    memset(_currentLogFile, 0, sizeof(_currentLogFile));
}

bool Datalogger::begin() {
    // 1. Verificar que el hardware SD esté listo
    if (!_sd->isReady()) {
        Serial.println(F("Datalogger: SDManager no detectado."));
        return false;
    }

    // 2. Asegurar que exista el directorio /LOGS
    // Usamos el manager para crearlo
    if (!_sd->createDirectory("/LOGS")) {
        Serial.println(F("Datalogger: Error al crear el directorio /LOGS"));
        return false;
    }

    // 3. Escanear archivos existentes (la función que hicimos antes)
    scanExistingLogs();

    // 4. Si el almacén está vacío, crear el primer log por defecto
    if (_fileCount == 0) {
        if (!addAndSetLogFile("/LOGS/log.txt")) {
            return false;
        }
    } else {
        // Si ya había archivos, seleccionamos el último
        selectLogByIndex(_fileCount - 1);
    }

    return true;
}

void Datalogger::scanExistingLogs() {
    File root = SD.open("/LOGS"); // Abrimos la carpeta específica
    if (!root || !root.isDirectory()) return;

    _fileCount = 0;

    while (true) {
        File entry = root.openNextFile();
        if (!entry) break;

        if (!entry.isDirectory()) {
            const char* name = entry.name();
            
            if (hasLogExtension(name) && _fileCount < MAX_LOG_FILES) {
                // IMPORTANTE: Construimos la ruta completa manualmente
                // para que _filenames guarde "/LOGS/nombre.log"
                snprintf(_filenames[_fileCount], FILE_NAME_SIZE, "/LOGS/%s", name);
                _fileCount++;
            }
        }
        entry.close();
    }
    root.close();
}

bool Datalogger::hasLogExtension(const char* filename) {
    size_t len = strlen(filename);
    if (len < 4) return false;
    
    // Buscamos ".log" o ".txt" al final (case insensitive o simple)
    const char* ext = filename + len - 4;
    return (strcasecmp(ext, ".log") == 0 || strcasecmp(ext, ".txt") == 0);
}

bool Datalogger::addAndSetLogFile(const char* filename) {
    if (_fileCount >= MAX_LOG_FILES) return false;
    if (strlen(filename) >= FILE_NAME_SIZE) return false; 

    // Guardar en nuestro almacén
    strncpy(_filenames[_fileCount], filename, FILE_NAME_SIZE - 1);
    _filenames[_fileCount][FILE_NAME_SIZE - 1] = '\0';
    
    // Establecer como archivo actual
    strncpy(_currentLogFile, _filenames[_fileCount], FILE_NAME_SIZE - 1);
    
    _fileCount++;
    return true;
}

void Datalogger::selectLogByIndex(uint16_t index) {
    if (index < _fileCount) {
        strncpy(_currentLogFile, _filenames[index], FILE_NAME_SIZE - 1);
    }
}

bool Datalogger::writeHeader(const char** titulos, uint16_t numTitulos) {
    if (numTitulos == 0 || strlen(_currentLogFile) == 0) return false;

    char buffer[256]; // Buffer temporal para la línea
    buffer[0] = '\0'; 

    for (uint16_t i = 0; i < numTitulos; i++) {
        strncat(buffer, titulos[i], sizeof(buffer) - strlen(buffer) - 1);
        if (i < numTitulos - 1) {
            strncat(buffer, ";", sizeof(buffer) - strlen(buffer) - 1);
        }
    }

    return _sd->appendLine(_currentLogFile, buffer);
}

bool Datalogger::writeRow(const char* timestamp, const float* values, uint16_t numValues) {
    if (numValues == 0 || strlen(_currentLogFile) == 0) return false;

    char buffer[256];
    // Iniciamos el buffer con el timestamp
    snprintf(buffer, sizeof(buffer), "%s", timestamp);

    for (uint16_t i = 0; i < numValues; i++) {
        char valBuffer[15];
        // Convertimos el float a char manualmente (2 decimales)
        dtostrf(values[i], 1, 2, valBuffer); 
        
        strncat(buffer, ";", sizeof(buffer) - strlen(buffer) - 1);
        strncat(buffer, valBuffer, sizeof(buffer) - strlen(buffer) - 1);
    }

    return _sd->appendLine(_currentLogFile, buffer);
}

void Datalogger::clearLogFile() {
    if (strlen(_currentLogFile) > 0) {
        _sd->clearFile(_currentLogFile);
    }
}

void Datalogger::printLogToSerial() {
    if (strlen(_currentLogFile) == 0) return;
    
    Serial.print(F("--- Contenido del Log: "));
    Serial.print(_currentLogFile);
    Serial.println(F(" ---"));

    _sd->printFileToSerial(_currentLogFile);
}
