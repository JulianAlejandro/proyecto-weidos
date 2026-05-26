#ifndef DATALOGGER_FILE_MANAGER_H
#define DATALOGGER_FILE_MANAGER_H

#include "ILogFileManager.h"
#include "SDManager.h"

//#include <esp_log.h> // <-- Cambiamos Serial por ESP_LOG

#define FILE_NAME_SIZE 32 
#define DIR_LOG_NAME "/LOGS"
#define PATH_ERR_LOG "/LOGS/ERROR"

#define DEFAULT_YEAR 2026

class TimeLogFileManager : public ILogFileManager {

private:
  SDManager* _sd; 
  bool _initialized = false;
  char _baseYearPath[FILE_NAME_SIZE]; 
  char _currentLogFile[FILE_NAME_SIZE];

  uint16_t _intLastYearLog; 
  uint32_t _intLastTimestampLog; 

  char _filenames[MAX_LOG_CAPACITY][FILE_NAME_SIZE]; 
  uint16_t _fileCount;  
  uint16_t _userMaxFiles; 

  esp_err_t setLastSesion(); 
  esp_err_t setFilesLastSesion();
  esp_err_t deleteInvalidFiles(); 

  static void buscarAnioMasRecienteCallback(const char* fileName, bool isDir, void* context);
  static void getSesionFilenamesCallback(const char* fileName, bool isDir, void* context);
  static void deleteInvalidFilesCallback(const char* fileName, bool isDir, void* context);

  esp_err_t setErrorLog();
  esp_err_t newYearFile(uint16_t year);
  void setLastLogTime();

public:
  TimeLogFileManager(SDManager* sdManager, uint16_t maxFiles);

  void setMaxFiles(uint16_t maxFiles) override;
  esp_err_t begin() override;
  esp_err_t setLastEnvironment(bool delete_rest) override;
  esp_err_t newFileLog(const char* timestamp) override;
  char* getCurrentLogPath() override; 
  esp_err_t appendErrorLog(const char* timestamp, const char* err_message);
  bool requiresTimestamp() override;
};

#endif

