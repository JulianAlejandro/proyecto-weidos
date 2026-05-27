#ifndef LOG_MSG_GLOBAL_H
#define LOG_MSG_GLOBAL_H

#include <functional>

namespace Log_msg {
    // 1. Definimos el tipo de callback. 
    // Tu función del main debe coincidir exactamente con esta firma: recibir un TAG y un Mensaje.
    using LogCallback = std::function<void(const char* tag, const char* mensaje)>;

    /**
     * @brief Registra la función del main que se encargará de procesar los mensajes.
     * @param cb Función callback (puede apuntar a tu DataLogger, Serial, etc.)
     */
    void registerCallback(LogCallback cb);

    /**
     * @brief Envía un mensaje con formato estilo printf al callback registrado.
     * @param tag Identificador de la clase que envía el mensaje (ej: "EM750", "WIFI")
     * @param formato Cadena de texto con formato (ej: "Error código: %d")
     */
    void println(const char* tag, const char* formato, ...) __attribute__((format(printf, 2, 3)));
}

#endif // LOG_MSG_GLOBAL_H