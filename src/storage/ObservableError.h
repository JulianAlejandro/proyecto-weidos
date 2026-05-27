#ifndef OBSERVABLE_ERROR_H
#define OBSERVABLE_ERROR_H

#include <functional>
#include <cstdarg>
#include <cstdio>

class ObservableError {
public:
    // El tipo de callback común para cualquier componente del sistema
    using OnErrorCallback = std::function<void(const char* tag, const char* mensaje)>;

protected:
    OnErrorCallback _errorCallback = nullptr;
    const char* _subclassTag;

    // El constructor obliga a la clase hija a identificarse pasándole su TAG
    ObservableError(const char* subclassTag) : _subclassTag(subclassTag) {}

    // Herramienta protegida: las clases hijas la usan exactamente igual que antes
    void reportError(const char* formato, ...) {
        if (_errorCallback == nullptr) return;

        char bufferMensaje[128];
        va_list args;
        va_start(args, formato);
        vsnprintf(bufferMensaje, sizeof(bufferMensaje), formato, args);
        va_end(args);

        // Envía el TAG automático de la clase hija y el mensaje
        _errorCallback(_subclassTag, bufferMensaje);
    }

public:
    virtual ~ObservableError() = default;

    // Este método lo heredarán públicamente todas las clases hijas
    void registerOnError(OnErrorCallback cb) {
        _errorCallback = cb;
    }
};

#endif