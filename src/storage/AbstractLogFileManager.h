#ifndef ABSTRACT_LOG_FILE_MANAGER_H
#define ABSTRACT_LOG_FILE_MANAGER_H

#include "ILogFileManager.h"
#include "SDManager.h"
//#include <esp_err.h>

#define FILE_NAME_SIZE 32 
#define DIR_LOG_NAME "/LOGS"
#define PATH_ERR_LOG "/LOGS/ERROR"

#define LOG_FILE_EXT   ".csv"

class AbstractLogFileManager : public ILogFileManager {
private:
  SDManager* _sd;
  bool _initialized = false;

  uint16_t _userMaxFiles;
  char _currentLogFile[FILE_NAME_SIZE]; 

  esp_err_t setErrorLog();
public:
  AbstractLogFileManager(SDManager* sdManager, uint16_t maxFiles);
  

  esp_err_t begin() override;
  esp_err_t setLastEnvironment(bool delete_rest) override;
  char* getCurrentLogPath() override;
  esp_err_t newFileLog(const char* timestamp) override;
  void setMaxFiles(uint16_t maxFiles) override;
  esp_err_t appendErrorLog(const char* timestamp, const char* err_message) override; 
  bool requiresTimestamp() override;
      
};

#endif

