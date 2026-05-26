
#ifndef BASIC_LOG_FILE_MANAGER_H
#define BASIC_LOG_FILE_MANAGER_H

#include "ILogFileManager.h"
#include "SDManager.h"

#define DIR_LOG_NAME "/B_LOGS"
#define FILE_NAME_SIZE 32

#define LOG_FILE_EXT   ".csv"

class BasicLogFileManager : public ILogFileManager {
private:
  SDManager* _sd; 
  bool _initialized = false;

  uint16_t _fileCount = 0;
  uint16_t _userMaxFiles; // TODO de moemntno no sirve para nada
  char _currentLogFile[FILE_NAME_SIZE]; 

  esp_err_t setLastSesion();
  //esp_err_t setFilesLastSesion();
  //esp_err_t deleteInvalidFiles();
  //esp_err_t setErrorLog();

  static void buscarContadorMasAltoCallback(const char* fileName, bool isDir, void* context);

public:
  BasicLogFileManager(SDManager* sdManager, uint16_t maxFiles);

  void setMaxFiles(uint16_t maxFiles) override;
  esp_err_t begin() override;
  esp_err_t setLastEnvironment(bool delete_rest) override;
  esp_err_t newFileLog(const char* timestamp) override;
  char* getCurrentLogPath() override; 
  esp_err_t appendErrorLog(const char* timestamp, const char* err_message) override;
  bool requiresTimestamp() override;
};

#endif

