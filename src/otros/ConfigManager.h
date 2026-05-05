/*
#include "SDManager.h"
//#include <SD.h>

// Estructura para agrupar los datos de forma limpia
struct DeviceConfig {
    int start_addr;
    int length;
    uint32_t log_interval_s;
    uint32_t meas_interval_ms;
};

class ConfigManager {
private:
    DeviceConfig _config;
    const char* _filename;// fichero donde buscar la informacion
    SDManager* _sd = nullptr; 

    static void staticCallback(Stream& data, void* context);

public:
    ConfigManager(SDManager* sdManager);

    // Método para entregar la configuración a otros objetos
    DeviceConfig getDeviceConfig();
};
*/
