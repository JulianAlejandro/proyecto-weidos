#ifndef ILOG_FILE_MANAGER_H
#define ILOG_FILE_MANAGER_H

#include <esp_err.h>
#include <cstdint>

#define MAX_LOG_CAPACITY 150

#define ESP_ERR_DL_BASE           0x9000
#define ESP_ERR_SD_NOT_INIT       (ESP_ERR_DL_BASE + 1)
#define ESP_ERR_SD_WRITE_FAIL     (ESP_ERR_DL_BASE + 2)
#define ESP_ERR_DL_PAST_TIME      (ESP_ERR_DL_BASE + 3)
#define ESP_ERR_DL_NOT_INIT       (ESP_ERR_DL_BASE + 4)

class ILogFileManager {
public:
    virtual ~ILogFileManager() = default; // Destructor virtual obligatorio

    virtual esp_err_t begin() = 0;
    virtual void setMaxFiles(uint16_t maxFiles) = 0;

    virtual esp_err_t setLastEnvironment(bool delete_rest) = 0;
    virtual esp_err_t newFileLog(const char* timestamp = nullptr) = 0;

    virtual char* getCurrentLogPath() = 0;

    virtual esp_err_t appendErrorLog(const char* timestamp, const char* err_message) = 0;

    virtual bool requiresTimestamp() = 0;
};

#endif
