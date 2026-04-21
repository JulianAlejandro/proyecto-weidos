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

public:
    // Constructor
    //SDManager(uint8_t csPin = 5);
    SDManager();

    // Inicialización
    bool begin();

    // Utilidades de Archivo Genéricas
    //dapertura de archivo para lectura secuencial
    bool openFile(const char* path); // No gusta que esta funcion sea publica
    bool getNextLineInRange (long start, long end, char* buffer, size_t size); 
    void closeFile(); // no gusta que esta funcion sea publica

    bool exists(const char* path);
    //bool remove(const char* path);
    void clearFile(const char* path);

    // Nueva versión con Callback: No devuelve nada, solo "notifica" cuando encuentra una línea
    void getLinesByRange(const char* path, long start, long end, LineCallback callback);
    
    // Escritura Optimizada
    bool getLineByID(const char* path, const char* id, char* destBuffer, size_t bufferSize);
    bool appendLine(const char* path, const char* data);
    void printFileToSerial(const char* path);
};

#endif

