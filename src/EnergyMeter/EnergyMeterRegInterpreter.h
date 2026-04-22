
#ifndef ENERGY_METER_INTERPRETER_H
#define ENERGY_METER_INTERPRETER_H

#include "../SDManager.h"
#include <vector>

#define NUM_COL_REG_EM750 5
#define MAX_MODBUS_REGS 125 // TODO DE MOMENTO ESTE INTERPRETADOR DE REGISTROS FUNCIONA SOLO PARA MODBUS 

struct EM_request {
    uint16_t start_addr; 
    uint16_t size; 
};

struct reg_EM_750 {
    String data[NUM_COL_REG_EM750];
};


enum index_reg_EM750 { ADDR, FORMAT, RD_WR, UNIT, NOTE };

// Enviando este enum se sabe size y format
//enum coded_format {FLOAT, SHORT, INT, STRING, USHORT, UINT, BYTE, LONG64, DFLOAT };
enum coded_format { FORMAT_UNKNOWN, FLOAT, SHORT, INT, STRING, USHORT, UINT, BYTE, LONG64, DFLOAT };

class EnergyMeterRegInterpreter {

private:
    SDManager* _sd = nullptr; 

    uint16_t _SDaddrsBuffer[MAX_MODBUS_REGS];
    coded_format _SDformatBuffer[MAX_MODBUS_REGS];
    uint16_t _SDlastRowReadSize;

    uint16_t _lastSizeReadRequestSended; 

    float dataFloat[MAX_MODBUS_REGS];
    uint16_t _sizeData; 

    bool splitString(const char* linea, char div_char, reg_EM_750 &resultado);
    int getFormatSize(coded_format f);

public:
    // Constructor
    EnergyMeterRegInterpreter(SDManager* sdManager);

    int begin();

    EM_request startRequest (const uint16_t start_addr, const uint16_t size);
    //std::vector<coded_format> devuelveRegData(long start_addr, long size); // de momento lo hacemos asi
    float* getFloatValues(const uint16_t* datos, const uint16_t size);

    uint16_t getSizeData(){ return _sizeData; };

    // Función auxiliar para convertir String a Enum
    static coded_format stringToFormat(const String& str) {
        String s = str;
        s.toUpperCase(); // Seguridad ante minúsculas
        if (s == "FLOAT")  return FLOAT;
        if (s == "SHORT")  return SHORT;
        if (s == "INT")    return INT;
        if (s == "UINT")   return UINT;
        if (s == "USHORT") return USHORT;
        if (s == "BYTE")   return BYTE;
        if (s == "LONG64") return LONG64;
        if (s == "DFLOAT") return DFLOAT;
        if (s == "STRING") return STRING;
        return FORMAT_UNKNOWN;
    }

    // Es buena idea tener un método para verificar si el manager está listo
    bool isReady() { return _sd != nullptr; }

};

#endif
