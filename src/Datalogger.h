
#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <Arduino.h>
#include "SDManager.h"

#define MAX_LOG_FILES 50
#define FILE_NAME_SIZE 16 // Formato 8.3 (ej: LOG_0001.TXT)

// TODO ANALIZAR LAS POSIBILIDADES DE SI DISPONE DE EL TIMESTAMP O NO
class Datalogger {
private:
char _filenames[MAX_LOG_FILES][FILE_NAME_SIZE];
    uint16_t _fileCount; 
    char _currentLogFile[FILE_NAME_SIZE]; // El archivo activo actualmente
    SDManager* _sd;

    bool hasLogExtension(const char* filename);

public:
    Datalogger(SDManager* sdManager);

    bool begin();
    void scanExistingLogs();
    //void setLogFile(const char* filename);

    bool addAndSetLogFile(const char* filename);

    void selectLogByIndex(uint16_t index);

    bool writeHeader(const char** titulos, uint16_t numTitulos); 
    bool writeRow(const char* timestamp, const float* values, uint16_t numValues);

    void clearLogFile();
    void printLogToSerial();
};

#endif
