#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <vector>

// Definimos el tipo de callback: una función que recibe un puntero a char (la línea)
typedef void (*LineCallback)(const char* line);

class SDManager {
private:
    //uint8_t _csPin;
    //bool _initialized;

public:
    // Constructor
    //SDManager(uint8_t csPin = 5);
    SDManager();

    // Inicialización
    bool begin();

    // Utilidades de Archivo Genéricas
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

