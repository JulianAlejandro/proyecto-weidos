#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <SD.h>

// Definimos el tipo de callback: una función que recibe un puntero a char (la línea)
typedef void (*LineCallback)(const char* line, void* context);

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


    bool exists(const char* path);
    //bool remove(const char* path);
    void clearFile(const char* path);

    bool appendLine(const char* path, const char* data);

    //TODO: DEBUG envia por serial todo lo que hay dentro del fichero seleccionado
    void printFileToSerial(const char* path);

    bool getAllLines(const char* path, LineCallback callback, void* context);
};

#endif

