#ifndef SD_REGISTER_MAP_H
#define SD_REGISTER_MAP_H

#include "SDManager.h"
#include <vector>


#define NUM_COL_REG_EM750 5

struct reg_EM_750 {
    String data[NUM_COL_REG_EM750];
};

enum index_reg_EM750 { ADDR, FORMAT, RD_WR, UNIT, NOTE };

// Enviando este enum se sabe size y format
//enum coded_format {FLOAT, SHORT, INT, STRING, USHORT, UINT, BYTE, LONG64, DFLOAT };
enum coded_format { FORMAT_UNKNOWN, FLOAT, SHORT, INT, STRING, USHORT, UINT, BYTE, LONG64, DFLOAT };

/*
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
*/
class SDRegisterMap {
public:
    // Constructor
    SDRegisterMap(SDManager* sdManager);

    int begin();

    std::vector<coded_format> devuelveRegData(long start_addr, long size); // de momento lo hacemos asi

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

  /*
    static int getFormatSize(const String& format) {
        if (format == "LONG64") return FormatWeight::LONG64;
        
        if (format == "FLOAT" || format == "INT" || format == "UINT" || 
            format == "INT32" || format == "UINT32") 
            return FormatWeight::FLOAT;

        // SHORT, USHORT, BYTE, INT16, UINT16, STRING, DFLOAT
        return FormatWeight::SHORT; 
    }
    */

private:
    SDManager* _sd = nullptr; 

    bool splitString(const char* linea, char div_char, reg_EM_750 &resultado);

};

#endif
