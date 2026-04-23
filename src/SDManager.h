#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <SD.h>

// Definimos el tipo de callback: una función que recibe un puntero a char (la línea)
typedef void (*LineCallback)(const char* line);

class SDManager {
private:
    //uint8_t _csPin;
    //bool _initialized;
    File _currentFile; 
    bool _initialized = false;

public:
    // Constructor
    //SDManager(uint8_t csPin = 5);
    SDManager();

    // Inicialización
    bool begin();

    bool isReady();

    // Utilidades de Archivo Genéricas
    //dapertura de archivo para lectura secuencial
    bool createFile(const char* path);
    bool createDirectory(const char* path);

    bool openFile(const char* path); // No gusta que esta funcion sea publica

    //tTODO: esta funcion se puede basar en una mas generica
    bool getNextLineInRange (uint16_t start, uint16_t size, char* buffer, size_t buffer_size); 
    void closeFile(); // no gusta que esta funcion sea publica

    bool exists(const char* path);
    //bool remove(const char* path);
    void clearFile(const char* path);

    // Nueva versión con Callback: No devuelve nada, solo "notifica" cuando encuentra una línea
    //TODO ESTO NO VA A QUI
    void getLinesByRange(const char* path, uint16_t start, uint16_t end, LineCallback callback);
    
    // Escritura Optimizada
    //TODO esto no va aqui
    bool getLineByID(const char* path, const char* id, char* destBuffer, size_t bufferSize);
    bool appendLine(const char* path, const char* data);
    void printFileToSerial(const char* path);
};

#endif

