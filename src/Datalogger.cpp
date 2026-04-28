#include "Datalogger.h"

Datalogger::Datalogger(SDManager* sdManager) : _sd(sdManager), _fileCount(0) {
    memset(_filenames, 0, sizeof(_filenames));
    memset(_currentLogFile, 0, sizeof(_currentLogFile));
}

bool Datalogger::begin() {
    if (!_sd->isReady()) {
        Serial.println(F("Datalogger: SDManager no detectado."));
        return false;
    }

    // 1. Usar el define para crear el directorio
    if (!_sd->createDirectory(DIR_LOG_NAME)) {
        Serial.print(F("Datalogger: Error al crear "));
        Serial.println(F(DIR_LOG_NAME));
        return false;
    }

    scanExistingLogs();

    if (_fileCount == 0) {
        // 2. Usar la nueva función para el log por defecto
        if (!newLog("log.txt")) {
            return false;
        }
    } else {
        selectLogByIndex(_fileCount - 1);
    }

    return true;
}

void Datalogger::scanExistingLogs() {
    // 3. Abrir el directorio usando el define
    File root = SD.open(DIR_LOG_NAME); 
    if (!root || !root.isDirectory()) return;

    _fileCount = 0;

    while (true) {
        File entry = root.openNextFile();
        if (!entry) break;

        if (!entry.isDirectory()) {
            const char* name = entry.name();
            
            if (hasLogExtension(name) && _fileCount < MAX_LOG_FILES) {
                // 4. Construir ruta completa usando el define
                snprintf(_filenames[_fileCount], FILE_NAME_SIZE, "%s/%s", DIR_LOG_NAME, name);
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

// Nueva función solicitada
bool Datalogger::newLog(const char* name) {
    char fullPath[FILE_NAME_SIZE];
    
    // Concatenamos el directorio y el nombre: "/LOGS/nombre"
    // El formato "%s/%s" asegura que haya una barra entre el dir y el archivo
    snprintf(fullPath, sizeof(fullPath), "%s/%s", DIR_LOG_NAME, name);
    
    return addAndSetLogFile(fullPath);
}

bool Datalogger::addAndSetLogFile(const char* filename) {
    if (_fileCount >= MAX_LOG_FILES) return false;

    char baseName[FILE_NAME_SIZE];
    char extension[10];
    char finalPath[FILE_NAME_SIZE];
    
    // 1. Separar el nombre base de la extensión (ej: "/LOGS/log" y ".txt")
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

    // 2. Buscar un nombre que no exista
    uint16_t counter = 0;
    strncpy(finalPath, filename, FILE_NAME_SIZE - 1);

    while (_sd->exists(finalPath)) {
        counter++;
        // Crea un nombre tipo "/LOGS/log_1.txt", "/LOGS/log_2.txt", etc.
        snprintf(finalPath, FILE_NAME_SIZE, "%s_%u%s", baseName, counter, extension);
        
        // Seguridad para no entrar en bucle infinito si algo falla
        if (counter > 999) break; 
    }

    // 3. Guardar el nombre final en nuestro almacén
    strncpy(_filenames[_fileCount], finalPath, FILE_NAME_SIZE - 1);
    _filenames[_fileCount][FILE_NAME_SIZE - 1] = '\0';
    
    // Establecer como archivo actual
    strncpy(_currentLogFile, _filenames[_fileCount], FILE_NAME_SIZE - 1);
    
    _fileCount++;
    
    // Opcional: Crear el archivo físicamente ahora para "reservarlo"
    return _sd->createFile(_currentLogFile);
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


bool Datalogger::writeRow(const char* timestamp, const char** values, uint16_t numValues) {
    // 1. Verificaciones de seguridad
    if (numValues == 0 || _currentLogFile[0] == '\0') return false;

    // 2. Buffer para la línea completa
    // ¡OJO! Si MAX_MODBUS_REGS es 125, 256 bytes es MUY poco. 
    // Recomendado aumentar a 512 o 1024 según tus necesidades.
    char buffer[512]; 
    
    // Iniciamos el buffer con el timestamp
    snprintf(buffer, sizeof(buffer), "%s", timestamp);

    // 3. Concatenamos cada valor de texto
    for (uint16_t i = 0; i < numValues; i++) {
        // Añadimos el separador
        strncat(buffer, ";", sizeof(buffer) - strlen(buffer) - 1);
        
        // Añadimos el valor (que ya es una cadena de texto)
        if (values[i] != nullptr) {
            strncat(buffer, values[i], sizeof(buffer) - strlen(buffer) - 1);
        }
    }

    // 4. Escribimos la línea completa en la SD
    return _sd->appendLine(_currentLogFile, buffer);
}
/*
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
*/

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

// todo poner en privado las funciones de apoyo
// todo analizar que los titulos concuerdan con el dato.....esto en un futuro
bool Datalogger::newSesion(const char * name, const char** titles, uint16_t numTitles){

    if(!newLog(name)){
        return false; 
    }
    clearLogFile();

          // 2. Creamos un nuevo buffer temporal con espacio para el Timestamp (+1)
    uint16_t totalTitulos = numTitles + 1;
    const char* cabeceraCompleta[totalTitulos];

    cabeceraCompleta[0] = "Timestamp";

    for (uint16_t i = 0; i < numTitles; i++) {
        cabeceraCompleta[i + 1] = titles[i];
    }

    if(! writeHeader(cabeceraCompleta, totalTitulos)){
        return false; 
    }
    return true; 
}
