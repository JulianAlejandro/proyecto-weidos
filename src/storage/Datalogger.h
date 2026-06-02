#ifndef DATALOGGER_H
#define DATALOGGER_H

#include "SDManager.h"
#include "ILogFileManager.h"
#include "LogBuffer.h"
#include <esp_err.h>

#define MAX_LOGS 4
#define FILE_TEXT_SIZE 32 

// Umbral máximo por archivo de log (Ej: 20MB o 2KB para pruebas)
//#define MAX_FILE_SIZE_BYTES (20 * 1024 * 1024UL)
#define MAX_FILE_SIZE_BYTES (2 * 1024UL)

class Datalogger {
private:
    SDManager* _sd;
    ILogFileManager* _fileManager;
    LogBuffer _buffer; 

    char _logPath[FILE_TEXT_SIZE];
    bool _initialized = false;

    uint32_t _currentFileSizeBytes = 0; 
    bool _fileLimitReached = false; 
    uint16_t _lastNumberColumns; 

    // Modificada para retornar esp_err_t ante problemas de desbordamiento o SD
    esp_err_t m_pushToBuffer(const char* csvLine); 

public:
    Datalogger(SDManager* sdManager, ILogFileManager* fileManager);

    esp_err_t begin();
    void setMaxFiles(uint16_t maxFiles);

    // Métodos para estructura CSV
    esp_err_t newCSVLogSesion(const char* current_timestamp, const char** titles, uint16_t numTitles);
    esp_err_t appendNewDataCSVToLog(const char* timestamp_msg, const char** values, uint16_t numValues); 
    
    esp_err_t appendErrorLog(const char* timestamp_msg, const char* err_message);

    // Cambiado de bool a esp_err_t para propagar errores físicos de la SD
    esp_err_t flushBuffer();

    bool isFileLimitReached() const { return _fileLimitReached; }

    uint16_t º(){ return _lastNumberColumns; }
};

#endif
