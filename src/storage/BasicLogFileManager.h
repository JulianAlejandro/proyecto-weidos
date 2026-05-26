
#ifndef BASIC_LOG_FILE_MANAGER_H
#define BASIC_LOG_FILE_MANAGER_H

#include "AbstractLogFileManager.h"

class BasicLogFileManager : public AbstractLogFileManager {
private:
  uint16_t _fileCount = 0;

  esp_err_t setLastSesion();
  static void buscarContadorMasAltoCallback(const char* fileName, bool isDir, void* context);

public:
  BasicLogFileManager(SDManager* sdManager, uint16_t maxFiles);

  esp_err_t setLastEnvironment(bool delete_rest) override;
  esp_err_t newFileLog(const char* timestamp) override; 
};

#endif

