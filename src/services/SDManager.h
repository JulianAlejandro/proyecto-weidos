#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <SD.h>

/** * @brief Callback types for file processing.
 * LineCallback: Used for processing a file line by line.
 * StreamCallback: Used for direct access to the File stream.
 */
typedef void (*LineCallback)(const char* line, void* context);
typedef void (*StreamCallback)(Stream& stream, void* context);

class SDManager {
private:
    File _currentFile; 
    bool _initialized = false;

public:
    /**
     * @brief Constructor for SDManager.
     */
    SDManager();

    /**
     * @brief Initializes the SD card using the default SPI bus.
     * @return true if initialization was successful, false otherwise.
     */
    bool begin();

    /**
     * @brief Checks if the SD card is initialized and ready for operations.
     * @return Current initialization state.
     */
    bool isReady();

    /**
     * @brief Creates a new empty file if it doesn't exist.
     * @param path Full path to the file.
     * @return true if file exists or was created successfully.
     */
    bool createFile(const char* path);

    /**
     * @brief Creates a directory on the SD card.
     * @param path Full path of the directory.
     * @return true if directory exists or was created successfully.
     */
    bool createDirectory(const char* path);

    /**
     * @brief Checks if a file or directory exists at the given path.
     * @param path Path to check.
     * @return true if found.
     */
    bool exists(const char* path);

    /**
     * @brief Deletes all content from a file (truncates to zero size).
     * @param path Path to the file.
     */
    void clearFile(const char* path);

    /**
     * @brief Appends a line of text to a file.
     * @param path Path to the file.
     * @param data String data to append.
     * @return true if successfully written.
     */
    bool appendLine(const char* path, const char* data);

    /**
     * @brief Reads the entire content of a file and prints it to the Serial monitor.
     * @param path Path to the file.
     */
    void printFileToSerial(const char* path);

    /**
     * @brief Executes a callback function providing a Stream reference to the file.
     * Useful for parsing files without loading them entirely into RAM.
     * @param path Path to the file.
     * @param callback Function to execute.
     * @param context Pointer to user data for the callback.
     * @return true if file was opened successfully.
     */
    bool withFile(const char* path, StreamCallback callback, void* context);

    bool withFileWrite(const char* path, StreamCallback callback, void* context);
};

#endif