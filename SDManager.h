#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <vector>

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
    //bool isReady() { return _initialized; }

    // Utilidades de Archivo Genéricas
    bool exists(const char* path);
    //bool remove(const char* path);
    void clearFile(const char* path);

    // Lectura Optimizada: Busca una línea que empiece por un ID (ej: "19000")
    // y copia el resto de la línea en destBuffer.
    bool getLineByID(const char* path, const char* id, char* destBuffer, size_t bufferSize);

    // Escritura Optimizada
    bool appendLine(const char* path, const char* data);
    
    void getLinesByRange(const char* path, long start, long end, std::vector<String>& result);

    void printFileToSerial(const char* path);
    
    // Acceso directo al objeto File si fuera necesario (usar con cuidado)
    //File openFile(const char* path, const char* mode = FILE_READ);
};

#endif

