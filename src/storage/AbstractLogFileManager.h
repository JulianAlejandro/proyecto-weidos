#ifndef ABSTRACT_LOG_FILE_MANAGER_H
#define ABSTRACT_LOG_FILE_MANAGER_H

#include "ILogFileManager.h"
#include "SDManager.h"

#define FILE_NAME_SIZE 32 

#define LOG_FILE_EXT   ".csv"
#define PATH_ERROR_LOG "/ERR_LOG.txt"

class AbstractLogFileManager : public ILogFileManager {
protected:
  SDManager* _sd;
  bool _initialized = false;
  uint16_t _userMaxFiles;
  char _currentLogFile[FILE_NAME_SIZE]; 

  char _dirLogName[32];
  //char _pathErrLog[48];

  esp_err_t setErrorLog();
public:
  AbstractLogFileManager(SDManager* sdManager, uint16_t maxFiles, const char* dirRoot);
  virtual ~AbstractLogFileManager() {}

  esp_err_t begin() override;
  char* getCurrentLogPath() override;
  void setMaxFiles(uint16_t maxFiles) override;

  esp_err_t appendErrorLog(const char* timestamp, const char* err_message) override; 
  bool requiresTimestamp() override;
      
};

#endif

