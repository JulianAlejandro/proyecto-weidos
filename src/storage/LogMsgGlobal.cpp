#include "LogMsgGlobal.h"
#include <cstdarg>
#include <cstdio>

// Incluimos FreeRTOS para asegurar que el buffer sea Thread-Safe (Fiable)
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace Log_msg {
    
    // Puntero interno que guardará el callback que le pases desde el main
    static LogCallback internalCallback = nullptr;
    
    // Mutex para evitar que dos tareas pisen el buffer al mismo tiempo
    static SemaphoreHandle_t loggerMutex = nullptr;

    void registerCallback(LogCallback cb) {
        internalCallback = cb;
        
        // Creamos el mutex de forma segura la primera vez
        if (loggerMutex == nullptr) {
            loggerMutex = xSemaphoreCreateMutex();
        }
    }

    void println(const char* tag, const char* formato, ...) {
        // Defensa: Si en el main no se ha registrado ningún callback, ignoramos el mensaje
        if (internalCallback == nullptr) return;

        // Intentamos tomar el Mutex. Si otra tarea está escribiendo, esperamos pacientemente.
        if (loggerMutex != nullptr && xSemaphoreTake(loggerMutex, portMAX_DELAY) == pdTRUE) {
            
            // Creamos el buffer en la pila (stack) de la tarea actual. No consume RAM estática permanente.
            char bufferMensaje[128]; 

            // Procesamos los argumentos variables (... o va_list) estilo printf
            va_list args;
            va_start(args, formato);
            
            // vsnprintf limita la escritura a 128 bytes, evitando desbordamientos de memoria (Buffer Overflow)
            vsnprintf(bufferMensaje, sizeof(bufferMensaje), formato, args);
            
            va_end(args);

            // ¡Disparamos TU callback del main! Le pasamos el TAG y el mensaje formateado
            internalCallback(tag, bufferMensaje);

            // Liberamos el mutex para que otras clases puedan reportar cosas
            xSemaphoreGive(loggerMutex);
        }
    }
}