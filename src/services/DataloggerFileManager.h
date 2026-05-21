#ifndef DATALOGGER_FILE_MANAGER_H
#define DATALOGGER_FILE_MANAGER_H

#include "SDManager.h"
#include <esp_err.h>
#include <esp_log.h> // <-- Cambiamos Serial por ESP_LOG
#include "LogBuffer.h"

#define MAX_LOG_CAPACITY 150
#define FILE_NAME_SIZE 32 
#define DIR_LOG_NAME "/LOGS"
#define PATH_ERR_LOG "/LOGS/ERROR"

#define DEFAULT_YEAR 2026

// --- CÓDIGOS DE ERROR PERSONALIZADOS ---
#define ESP_ERR_DL_BASE           0x9000
#define ESP_ERR_SD_NOT_INIT       (ESP_ERR_DL_BASE + 1)
#define ESP_ERR_SD_WRITE_FAIL     (ESP_ERR_DL_BASE + 2)
#define ESP_ERR_DL_PAST_TIME      (ESP_ERR_DL_BASE + 3)
#define ESP_ERR_DL_NOT_INIT       (ESP_ERR_DL_BASE + 4)

class DataloggerFileManager {

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
  DataloggerFileManager(SDManager* sdManager, uint16_t maxFiles);
  void setMaxFiles(uint16_t maxFiles);
  esp_err_t begin();
  esp_err_t setCSVLastEnvironment(bool delete_rest);
  esp_err_t newFileLog(const char* timestamp);
  char* getCurrentLogPath(); 
  esp_err_t appendErrorLog(const char* timestamp, const char* err_message);
};

#endif

