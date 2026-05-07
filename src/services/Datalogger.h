
// TODO , si se supera el maximo de log, borrar el mas antiguo
#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <Arduino.h>
#include "SDManager.h"

#define MAX_LOG_FILES 50
#define FILE_NAME_SIZE 32 // Formato 8.3 (ej: LOG_0001.TXT)
#define DIR_LOG_NAME "/LOGS"

// TODO ANALIZAR LAS POSIBILIDADES DE SI DISPONE DE EL TIMESTAMP O NO
class Datalogger {
private:
char _filenames[MAX_LOG_FILES][FILE_NAME_SIZE]; // TODO mirar
    uint16_t _fileCount; 
    char _currentLogFile[FILE_NAME_SIZE]; // El archivo activo actualmente
    SDManager* _sd;

    bool hasLogExtension(const char* filename);
    bool addAndSetLogFile(const char* filename);

public:
    Datalogger(SDManager* sdManager);

    bool begin();

    bool newSesion(const char * name, const char** titles, uint16_t numTitles);

    bool newLog(const char* name); // Añadir esta línea

    void scanExistingLogs();
    //void setLogFile(const char* filename);

    void selectLogByIndex(uint16_t index);

    bool writeHeader(const char** titulos, uint16_t numTitulos); 
    bool writeRow(const char* timestamp, const char** values, uint16_t numValues);

    void clearLogFile();
    void printLogToSerial();

    void clearAllLogs();

};

#endif
