#ifndef TIME_LOG_FILE_MANAGER_H
#define TIME_LOG_FILE_MANAGER_H

#include "AbstractLogFileManager.h"

//#define DIR_LOG_NAME "/LOGS"
#define DEFAULT_YEAR 2026

class TimeLogFileManager : public AbstractLogFileManager {

private:
  
  char _baseYearPath[FILE_NAME_SIZE]; 
  
  uint16_t _intLastYearLog; 
  uint32_t _intLastTimestampLog; 

  char _filenames[MAX_LOG_CAPACITY][FILE_NAME_SIZE]; 
  uint16_t _fileCount;  

  esp_err_t setLastSesion(); 
  esp_err_t setFilesLastSesion();
  esp_err_t deleteInvalidFiles(); 

  static void buscarAnioMasRecienteCallback(const char* fileName, bool isDir, void* context);
  static void getSesionFilenamesCallback(const char* fileName, bool isDir, void* context);
  static void deleteInvalidFilesCallback(const char* fileName, bool isDir, void* context);

  esp_err_t newYearFile(uint16_t year);
  void setLastLogTime();

public:
  TimeLogFileManager(SDManager* sdManager, uint16_t maxFiles);
  virtual ~TimeLogFileManager();

  esp_err_t setLastEnvironment(bool delete_rest) override;
  esp_err_t newFileLog(const char* timestamp) override;

  bool requiresTimestamp() override;
};

#endif

