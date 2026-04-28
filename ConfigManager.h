#include <ArduinoJson.h>
#include <SD.h>

// Estructura para agrupar los datos de forma limpia
struct DeviceConfig {
    int start_addr;
    int length;
    uint32_t log_interval;
    uint32_t meas_interval;
};

class ConfigManager {
private:
    DeviceConfig _config;
    const char* _filename;

public:
    ConfigManager(const char* filename) : _filename(filename) {}

    // Lee la SD y carga los datos en la estructura privada
    bool begin() {
        File file = SD.open(_filename);
        if (!file) return false;

        JsonDocument doc; // En v7 no necesitas el tamaño fijo si es pequeño
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) return false;

        // Mapeo de JSON a la estructura C++
        _config.start_addr   = doc["modbus"]["start_addr"] | 1;
        _config.length       = doc["modbus"]["length"] | 1;
        _config.log_interval = doc["intervals"]["log_interval_sec"] | 60;
        _config.meas_interval = doc["intervals"]["meas_interval_ms"] | 1000;

        return true;
    }

    // Método para entregar la configuración a otros objetos
    DeviceConfig getConfig() {
        return _config;
    }
};