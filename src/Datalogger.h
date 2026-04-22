
#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <Arduino.h>
#include "SDManager.h"

class Datalogger {
private:
    String _logFile; 
    SDManager* _sd;

public:
    Datalogger(SDManager* sdManager);

    bool begin();
    void setLogFile(const char* filename);
    
    // Escribe los títulos (ej: {"Timestamp", "Voltaje", "Corriente"})
    // numTitulos: cantidad de elementos en el array
    bool writeHeader(const char** titulos, uint16_t numTitulos); 

    // Escribe una fila de datos numéricos
    // values: el array de floats que viene del Interpreter
    // numValues: cantidad de datos
    bool writeRow(const uint32_t timestamp, const float* values, uint16_t numValues); 
    
    void clearLogFile();
    void printLogToSerial();
};

#endif
/*
que hace un datalogger 
- 1. begin. se inicializa lo que necesita que principalmente es el HW de la SD. 
- 2. Se le puede decir que se desea hacer un nuevo log 
    - fichero en el que va a ir almacenado todo 
    - Solicita solo la primera entrada de tamaño N con los titulos 
    - habilita una funcion de carga de valores pra recibir lineas de N tamaño de puros Strings 

- 3. permite la limpieza del fichero 
*/
/*

#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <Arduino.h>
#include "SDManager.h"

class Datalogger {
private:
    
    String _logFile; 
    SDManager* _sd;
    //uint8_t _csPin;

public:
    
    Datalogger(SDManager* sdManager);

    // Gestión del Sistema
    bool begin();
    void clearLogFile();
    
    // Escritura de Datos (Datalogging)
    //void startLog();
    //void writeHeader(const std::vector<String>& titulos); // MODIFICAR SIN VECTORES
    //void writeRow(const std::vector<String>& datos); // MODIFICAR SIN VECTORES 
    
    // Debug
    void printLogToSerial();
    
};

#endif
*/
/*
//TODO: de esta clase hay que quitar todos los std::vector que no son necesarioas 
//hay que hacer todas las modificaciones para limitar los registros en RAM
#ifndef EM750_DATALOGGER_H
#define EM750_DATALOGGER_H

#include <Arduino.h>
//#include <SD.h>
#include "SDManager.h"
#include <vector>


#define NUM_COL_REG_EM750 5

struct reg_EM_750 {
    String data[NUM_COL_REG_EM750];
};

struct RegRequest {
    uint16_t baseAddress;
    uint16_t totalSize;
};

//enum para generar un indice para saber en que posicion esta cada tipo de dato
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

class EM750_Datalogger {
private:
    String _setupFile;
    String _logFile;
    std::vector<reg_EM_750> registros; 
    SDManager* _sd;
    //uint8_t _csPin;

    // Métodos privados auxiliares (los que no necesita ver el usuario)
    bool splitString(const String& linea, char div_char, reg_EM_750 &resultado);
    String getRowStringByAddress(long addr);
    std::vector<String> getRowsStringByAddressRange(long addrStart, long addrFin);

public:
    // Constructor
    //EM750_Datalogger(String setupFile = "/setup.txt", String logFile = "/tabla.txt", uint8_t csPin = 5);
    //EM750_Datalogger(const String& setupFile = "/setup.txt",const String& logFile = "/log.txt");
    EM750_Datalogger(SDManager* sdManager, const String& setupFile, const String& logFile);

    // Gestión del Sistema
    bool begin();
    void clearLogFile();
    
    // Lectura de Configuración (Setup)
    bool getRegDataByAddr(long addr, reg_EM_750 &resultado);
    std::vector<reg_EM_750> getRegsByRange(long addrStart, long addrEnd);

    // funcion orientada a la lectura por parte del modbus. 
    RegRequest RegRequestParamsFromRangeAddr(long addrStart, long addrEnd);
    std::vector<String> obtener_fila (std::vector<long> data, int base_addr); 

    // Escritura de Datos (Datalogging)
    void writeHeader(const std::vector<String>& titulos);
    void writeRow(const std::vector<String>& datos);
    
    // Debug
    void printLogToSerial();



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

*/