#ifndef LOG_BUFFER_H
#define LOG_BUFFER_H

#include <stdint.h>
#include <stddef.h>

// Definimos el tamaño ideal alineado con los sectores de la SD (512 bytes)
#define LOG_BUFFER_SIZE 512

class LogBuffer {
private:
    uint8_t data[LOG_BUFFER_SIZE]; // El contenedor físico de los datos (bytes puros)
    size_t currentIndex;           // Rastrea cuántos bytes se han escrito actualmente
    uint32_t lastWriteTime;        // Timestamp (en ms) de la última vez que se añadió un dato

public:
    // Constructor: Inicializa el buffer vacío
    LogBuffer();

    // Destructor
    ~LogBuffer();

    /**
     * @brief Añade texto o datos binarios al buffer.
     * @param newData Puntero a los datos que se quieren guardar.
     * @param size Tamaño en bytes de los datos a guardar.
     * @return true si el buffer se llenó por completo tras esta operación.
     */
    bool append(const uint8_t* newData, size_t size);

    /**
     * @brief Sobrecarga conveniente para strings (c-strings).
     * @param message Cadena de texto a guardar.
     * @return true si el buffer se llenó.
     */
    bool appendString(const char* message);

    /**
     * @brief Vacía virtualmente el buffer (resetea el índice a 0).
     * Se llama DESPUÉS de haber guardado con éxito los datos en la SD.
     */
    void clear();

    /**
     * @brief Devuelve un puntero al inicio de los datos.
     * El DataLogger usará esto para pasarle los datos a la librería de la SD.
     */
    const uint8_t* getBufferPointer() const;

    /**
     * @brief Devuelve la cantidad de bytes que están actualmente almacenados.
     */
    size_t getCurrentSize() const;

    /**
     * @brief Devuelve el espacio libre restante en bytes.
     */
    size_t getAvailableSpace() const;

    /**
     * @brief Verifica si el buffer está completamente lleno.
     */
    bool isFull() const;

    /**
     * @brief Devuelve el timestamp (en milisegundos) de la última inserción de datos.
     * Útil para que el DataLogger calcule el "Timeout" y fuerce el guardado.
     */
    //uint32_t getLastActivityTime() const;

    //void dumpToSerial() const;
};

#endif // LOG_BUFFER_H