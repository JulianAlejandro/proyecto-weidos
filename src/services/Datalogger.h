#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <Arduino.h>
#include "SDManager.h"

#define MAX_LOG_CAPACITY 150
#define FILE_NAME_SIZE 32 // Supports full path (e.g., /LOGS/YYMMDDHH.txt)
#define DIR_LOG_NAME "/LOGS"
//formato que recibe de nombre en el _currentLogFile

/**
 * @class Datalogger
 * @brief Manages circular logging, file creation, and structured data writing to SD.
 */
class Datalogger {
private:
    char _filenames[MAX_LOG_CAPACITY][FILE_NAME_SIZE]; // Array to store existing log paths
    uint16_t _fileCount;  
    uint16_t _userMaxFiles; //Client limit
                              // Current number of logged files
    char _currentLogFile[FILE_NAME_SIZE];           // Path of the currently active log
    SDManager* _sd;                                 // Pointer to the SD hardware manager

    /**
     * @brief Checks if a file has a valid logging extension (.log or .txt).
     */
    bool hasLogExtension(const char* filename);

    /**
     * @brief Internal logic to add a new file to the system. 
     * Handles circular buffer (deleting oldest) and duplicate naming.
     */
    bool addAndSetLogFile(const char* filename);

public:
    /**
     * @brief Constructor.
     * @param sdManager Pointer to an initialized SDManager instance.
     */
    Datalogger(SDManager* sdManager, uint16_t maxFiles);

    /**
     * @brief Initializes the logging directory and scans existing files.
     * @return true if the directory is ready.
     */
    bool begin();

    /**
     * @brief Starts a new session: creates file, clears it, and writes the header.
     * @param name Desired filename.
     * @param titles Array of column titles.
     * @param numTitles Number of titles provided.
     */
    bool newSesion(const char * name, const char** titles, uint16_t numTitles);

    /**
     * @brief Prepares a new log file path and registers it in the system.
     */
    bool newLog(const char* name);

    /**
     * @brief Scans the /LOGS directory to populate internal file tracking.
     */
    void scanExistingLogs();

    /**
     * @brief Sets the active log file by its index in the internal array.
     */
    void selectLogByIndex(uint16_t index);

    /**
     * @brief Writes a CSV-formatted header to the current log file.
     */
    bool writeHeader(const char** titulos, uint16_t numTitulos); 

    /**
     * @brief Writes a single row of data with a timestamp.
     * @param timestamp String representing the current time.
     * @param values Array of string values to be logged.
     * @param numValues Number of values in the array.
     */
    bool writeRow(const char* timestamp, const char** values, uint16_t numValues);

    /**
     * @brief Truncates the current log file to zero size.
     */
    void clearLogFile();

    /**
     * @brief Dumps the content of the current log file to the Serial monitor.
     */
    void printLogToSerial();

    /**
     * @brief Physically deletes all log files within the /LOGS directory.
     */
    void clearAllLogs();

    /**
     * @brief Returns the path of the currently active log file.
     * @return Constant pointer to the filename string.
    */
    const char* getCurrentLogFile() const { return _currentLogFile; }

    void setMaxFiles(uint16_t maxFiles);
};

#endif
