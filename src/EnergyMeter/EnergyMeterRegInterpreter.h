
#ifndef ENERGY_METER_INTERPRETER_H
#define ENERGY_METER_INTERPRETER_H

#include "../SDManager.h"
#include <CSV_Parser.h>
//#include <vector>
//#include "EnergyMeter750.h"

#define NUM_COL_REG_EM750 5
#define MAX_MODBUS_REGS 125 // TODO DE MOMENTO ESTE INTERPRETADOR DE REGISTROS FUNCIONA SOLO PARA MODBUS 
#define MAX_TITLES_SIZE 32 // TODO cambiatar esto de nombre
#define MAX_TEXT_SIZE 32

#define MAX_DATA_SIZE 4 // numero maximo de tamaño que puede tener un dato en el mapa de registros 
#define LINE_MAP_START 4

#define MAP_FILE "/EM750map.csv"

// size number reg 16 bits
#define SIZE_FLOAT   2 // 2 reg 16 bits
#define SIZE_INT     2
#define SIZE_UINT    2
#define SIZE_LONG64  4
#define SIZE_SHORT   1
#define SIZE_USHORT  1
#define SIZE_BYTE    1
#define SIZE_DFLOAT  1

// todo añadir algunos defines para que las columnas no se busquen hardcoded
//#define ADDRESS 

enum coded_format { FORMAT_UNKNOWN, FLOAT, SHORT, INT, STRING, USHORT, UINT, BYTE, LONG64, DFLOAT };

//Estructura para realziar una solicitud de valores de registros. 
struct EM_request {
    uint16_t start_addr; 
    uint16_t size; 
};

//regustro que almacena los registros que forman un dato e informacion importante como el formato, log etc 
// TODO esto hay qye cambiarlo 
struct rawDataReg {
    uint16_t data[MAX_DATA_SIZE];
    coded_format format; 

};

// buffer de datos , estructura usada para recuperar datos procesados 
struct bufRawDataReg{
    rawDataReg* buffer;
    uint16_t size; 
};

// TODO pensar en si es aconsejable esto
//enum index_col_regs {ADDRESS, FORMAT, UNIT, NAME, LOG};
//enum index_parameters { NAME, VALUE };

//estructura que apunta a los ultimos valores de la columna nombres 
struct nameColValues {
    const char* buffer[MAX_MODBUS_REGS]; // Array de punteros a las cadenas
    uint16_t size;
};

struct netDataString{
    const char* buffer[MAX_MODBUS_REGS];
    uint16_t size; 
};

struct Parameters{
    const char* log_interval;
    const char* new_file;
    const char* max_files; 
};

typedef void (*TitleHandler)(const char* title, void* arg);

/*
class EnergyMeter750;
class Datalogger; 
class RTC_DS3231; 
*/
//TODO String _setup cambiar
class EnergyMeterRegInterpreter { // TODO Esta clase tendra que cambiar de nombre a algo que gestione CSVs

private:
    SDManager* _sd = nullptr; 
    EM_request _current_request; // TODO podemos pasarlo por copia y no por referencial....
    String _setupFile;

//TODO aqui podemos hacer un destrozo
    uint16_t _bufAddrsValues[MAX_MODBUS_REGS]; // almacena los Addr que se quieren leer en una request. 250 bytes reservados stack
    coded_format _bufFormatValues[MAX_MODBUS_REGS]; // almacena los formatos que hay en una request
    char _bufNamesValues[MAX_MODBUS_REGS][MAX_TITLES_SIZE]; 
    bool _bufLogValues[MAX_MODBUS_REGS];
    //char _bufUnitValues[MAX_MODBUS_REGS][MAX_TITLES_SIZE];
    
    // Este buffer solo se actualiza tras recuperar los datos de EM
    rawDataReg _bufRawData[MAX_MODBUS_REGS]; // 125 x (4x2 + 4) = 1500 bytes aprox

    char _bufNetDataString[MAX_MODBUS_REGS][MAX_TEXT_SIZE];

    uint16_t _lastRowReadSize; // size del numero de registros que se quieren leer en una request. 

    //TODO valorar donde poner esto
    // estos son almacenes para mantener la persistencia de datos obtenidos en startNewRequest para estos parametros 
    char _log_interval[MAX_TEXT_SIZE]; 
    char _new_file[MAX_TEXT_SIZE]; 
    char _max_files[MAX_TEXT_SIZE];
   
    //bool splitString(char* linea, char div_char, reg_EM_750 &resultado); // funcion de apoyo para obtener valores de registro en Strings
    int getFormatSize(coded_format f); // Convierte un enum en valores de int para obtener el tamaño de cada registro

    //void handleLine(char* line, uint16_t start, uint16_t size); // funciones para callback
    //static void staticCallback(const char* line, void* context);

    void processParserData(CSV_Parser& cp, uint16_t start, uint16_t size);

public:
    // Constructor
    EnergyMeterRegInterpreter(SDManager* sdManager);

    int begin();

    EM_request startNewRequest (const uint16_t start_addr, const uint16_t size);

    bufRawDataReg getBufferDataRaw(const uint16_t* datos, const uint16_t size);

    nameColValues getLastNameValues();

    netDataString getBufNetDataString();
    //void getTitles(const uint16_t start_addr, const uint16_t size, TitleHandler handler, void* arg);

    // Función auxiliar para convertir String a Enum
    coded_format stringToFormat(const char* str);

    // Es buena idea tener un método para verificar si el manager está listo
    bool isReady() { return _sd != nullptr; }

    // TODO analizar si es necesario interpretar que es little endian o no
    static float getFloatConversion(const uint16_t* data);

    //OBTENCION DE PARAMETROS EXISTENTES DENTRO DEL FICHERO EM750map.csv
    void loadParametersMapRegister(); 

    Parameters getParameters();
    //const char* getLogInterval() { return _log_interval; }
    //const char* getNewFile() { return _new_file; }
    //const char* getMaxFiles() { return _max_files; }

    //TODO funciones relacionadas con la MODBUS REQUEST, EN UN FUTURO REFACTORIZAR 
    //bool modbusRequestByFile(char* filename);

    //void functionDatalogger(Datalogger* datalogger, EnergyMeter750* em, RTC_DS3231* rtc);

};

#endif
