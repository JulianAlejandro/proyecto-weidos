#include "LogBuffer.h"
#include <string.h> // Para memcpy y strlen
#include <Arduino.h>

// Dependiendo de tu framework (ej. ESP32 con Arduino), puedes usar millis().
// Si usas ESP-IDF puro, podrías cambiarlo por: esp_timer_get_time() / 1000;
extern unsigned long millis(); 

// ==========================================
// CONSTRUCTOR Y DESTRUCTOR
// ==========================================

LogBuffer::LogBuffer() : currentIndex(0), lastWriteTime(0) {
    // Opcional: Inicializar el array a cero por seguridad
    memset(data, 0, LOG_BUFFER_SIZE);
}

LogBuffer::~LogBuffer() {
    // No requiere liberación dinámica ya que 'data' es un array estático
}

// ==========================================
// MÉTODOS DE ESCRITURA (APPEND)
// ==========================================

bool LogBuffer::append(const uint8_t* newData, size_t size) {
    // Protección: Si los datos no existen o el tamaño es 0, no hacemos nada
    if (newData == nullptr || size == 0) {
        return isFull();
    }

    // Protección crucial: Si no cabe el bloque completo, no escribimos nada parcialmente.
    // Esto respeta la lógica de "Prevenir en lugar de reaccionar" del DataLogger.
    if (size > getAvailableSpace()) {
        return false; 
    }

    // Copiamos de forma segura los bytes a la posición actual del buffer
    memcpy(&data[currentIndex], newData, size);
    
    // Desplazamos el índice
    currentIndex += size;

    // Actualizamos el timestamp de actividad
    //lastWriteTime = millis();

    // Devolvemos si se ha llenado por completo tras esta operación
    return isFull();
}

bool LogBuffer::appendString(const char* message) {
    if (message == nullptr) {
        return isFull();
    }
    
    // Convertimos el string a bytes puros y usamos el append general
    size_t length = strlen(message);
    return append(reinterpret_cast<const uint8_t*>(message), length);
}

// ==========================================
// MÉTODOS DE CONTROL Y LIMPIEZA
// ==========================================

void LogBuffer::clear() {
    currentIndex = 0;
    // Nota: No es necesario borrar físicamente el array con memset aquí. 
    // Al poner currentIndex a 0, los nuevos appends sobrescribirán lo viejo, ahorrarás ciclos de CPU.
}

// ==========================================
// MÉTODOS GETTERS (READ-ONLY)
// ==========================================

const uint8_t* LogBuffer::getBufferPointer() const {
    return data;
}

size_t LogBuffer::getCurrentSize() const {
    return currentIndex;
}

size_t LogBuffer::getAvailableSpace() const {
    return LOG_BUFFER_SIZE - currentIndex;
}

bool LogBuffer::isFull() const {
    return currentIndex >= LOG_BUFFER_SIZE;
}
/*
uint32_t LogBuffer::getLastActivityTime() const {
    return lastWriteTime;
}*/

/*
void LogBuffer::dumpToSerial() const {
    if (currentIndex == 0) {
        Serial.println(F("Búfer Vacío"));
        return;
    }
    
    // Usamos Serial.write porque trabaja con bytes puros (uint8_t)
    // e imprimirá exactamente la longitud almacenada actual
    Serial.write(data, currentIndex);
    Serial.println(); // Un salto de línea final decorativo
}
*/
