
#ifndef ENERGY_METER_750_H
#define ENERGY_METER_750_H

#include <ArduinoModbus.h>
#include "SDManager.h"

#define NUM_COL_REG_EM750 5

struct reg_EM_750 {
    String data[NUM_COL_REG_EM750];
};

// Estructura para definir bloques de lectura
struct ModbusBlock {
    uint16_t startAddress;
    uint16_t quantity;
};

enum index_reg_EM750 { ADDR, FORMAT, RD_WR, UNIT, NOTE };

struct FormatWeight {
    static const int FLOAT  = 2;
    static const int SHORT  = 1;
    static const int INT = 2;
    static const int STRING  = 1;//¿?¿?¿? TODO
    static const int USHORT = 1;
    static const int UINT = 2;
    static const int BYTE = 1;
    static const int LONG64 = 4;
    static const int DFLOAT = 1; 
};

class EnergyMeter750 {
  private:
    uint8_t _slaveAddress;
    ModbusTCPClient* _modbus; // Usamos puntero para flexibilidad
    SDManager* _sd;
    bool splitString(const char* linea, char div_char, reg_EM_750 &resultado);

  public:

    // Constructor: le pasamos la dirección del esclavo
    EnergyMeter750(uint8_t slaveAddress);
    
    // Configura el cliente Modbus que usará
    int begin(SDManager* sdManager, ModbusTCPClient* modbusClient);

    // Definición de bloques comunes del EM750 (Ejemplos típicos)
    const ModbusBlock BLOCK_BASIC = { 19000, 2 }; // Voltajes, Corrientes
    const ModbusBlock BLOCK_ENERGY = { 19050, 10 }; // Energías acumuladas

    // Método para leer un bloque y procesarlo directamente a la SD
    // Evitamos devolver vectores, mejor pasamos una función callback o procesamos dentro
    //bool readAndProcess(ModbusBlock block, void (*callback)(uint16_t, uint32_t));
    bool readAndProcess_2(long start, long end, void (*callback)(float));
    
    // Función de ayuda para convertir dos registros de 16bits a Float/Long
    float registersToFloat(uint16_t high, uint16_t low);

        /**
     * @brief Devuelve cuántos registros Modbus ocupa un formato.
     * FLOAT/INT32 = 2 registros, INT16 = 1 registro.
     */
    static int getFormatSize(const String& format) {
        if (format == "LONG64") return FormatWeight::LONG64;
        
        if (format == "FLOAT" || format == "INT" || format == "UINT" || 
            format == "INT32" || format == "UINT32") 
            return FormatWeight::FLOAT;

        // SHORT, USHORT, BYTE, INT16, UINT16, STRING, DFLOAT
        return FormatWeight::SHORT; 
    }
};

#endif
