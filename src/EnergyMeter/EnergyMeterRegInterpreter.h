
#ifndef ENERGY_METER_INTERPRETER_H
#define ENERGY_METER_INTERPRETER_H

#include "../SDManager.h"
//#include <vector>

#define NUM_COL_REG_EM750 5
#define MAX_MODBUS_REGS 125 // TODO DE MOMENTO ESTE INTERPRETADOR DE REGISTROS FUNCIONA SOLO PARA MODBUS 
#define MAX_TITLES_SIZE 32

#define MAX_DATA_SIZE 4 // numero maximo de tamaño que puede tener un dato en el mapa de registros 

#define SETUP_FILE "/EM750map.csv"

// size number reg 16 bits
#define SIZE_FLOAT   2 // 2 reg 16 bits
#define SIZE_INT     2
#define SIZE_UINT    2
#define SIZE_LONG64  4
#define SIZE_SHORT   1
#define SIZE_USHORT  1
#define SIZE_BYTE    1
#define SIZE_DFLOAT  1

enum coded_format { FORMAT_UNKNOWN, FLOAT, SHORT, INT, STRING, USHORT, UINT, BYTE, LONG64, DFLOAT };


//Estructura para realziar una solicitud de valores de registros. 
struct EM_request {
    uint16_t start_addr; 
    uint16_t size; 
};

//Estructura de registro 
struct completeDataReg {
    uint16_t data[MAX_DATA_SIZE];
    coded_format format; 
};

struct CompleteDataRegBuffer{
    completeDataReg* buffer;
    uint16_t size; 
};

// TODO pensar en si es aconsejable esto
enum index_reg_EM750 { ADDR, FORMAT, RD_WR, UNIT, NOTE };
//enum index_parameters { NAME, VALUE };

//todo ya
struct reg_EM_750 { 
    const char* data[NUM_COL_REG_EM750];
};


struct titlesBuffer {
    const char* buffer[MAX_MODBUS_REGS]; // Array de punteros a las cadenas
    uint16_t size;
};

struct stringDataEM{
    const char* buffer[MAX_MODBUS_REGS];
    uint16_t size; 
};

typedef void (*TitleHandler)(const char* title, void* arg);

//TODO String _setup cambiar
class EnergyMeterRegInterpreter { // TODO Esta clase tendra que cambiar de nombre a algo que gestione CSVs

private:
    SDManager* _sd = nullptr; 
    EM_request _current_request; // TODO podemos pasarlo por copia y no por referencial....
    String _setupFile;

//variables de apoyo a la lectura de la SD
    uint16_t _SDaddrsBuffer[MAX_MODBUS_REGS]; // almacena los Addr que se quieren leer en una request. 250 bytes reservados stack
    coded_format _SDformatBuffer[MAX_MODBUS_REGS]; // almacena los formatos que hay en una request

    // TODO, peligrosos titulos muy grandes pueden dar problemas
    // Modificar estos titulos persistentes 
     char _titulos[MAX_MODBUS_REGS][MAX_TITLES_SIZE]; 
    //const char* _titulosBuffer[MAX_MODBUS_REGS]; // buffer con los titulos de los registros que hay en la request

    //completeDataReg data_readed[MAX_MODBUS_REG]; 
    completeDataReg _completeDataR[MAX_MODBUS_REGS]; // 125 x (4x2 + 4) = 1500 bytes aprox
    char _netaData[MAX_MODBUS_REGS][MAX_TITLES_SIZE];

    uint16_t _SDlastRowReadSize; // size del numero de registros que se quieren leer en una request. 

    //TODO valorar donde poner esto
    // estos son almacenes para mantener la persistencia de datos obtenidos en startNewRequest para estos parametros 
    char _log_interval[MAX_TITLES_SIZE]; 
    char _new_file[MAX_TITLES_SIZE]; 
    char _max_files[MAX_TITLES_SIZE];
   
    bool splitString(char* linea, char div_char, reg_EM_750 &resultado); // funcion de apoyo para obtener valores de registro en Strings
    int getFormatSize(coded_format f); // Convierte un enum en valores de int para obtener el tamaño de cada registro

    void handleLine(char* line, uint16_t start, uint16_t size); // funciones para callback
    static void staticCallback(const char* line, void* context);

public:
    // Constructor
    EnergyMeterRegInterpreter(SDManager* sdManager);

    int begin();

    EM_request startNewRequest (const uint16_t start_addr, const uint16_t size);

    CompleteDataRegBuffer getDataProcess(const uint16_t* datos, const uint16_t size);

    titlesBuffer getTitles();

    stringDataEM getStringData();
    //void getTitles(const uint16_t start_addr, const uint16_t size, TitleHandler handler, void* arg);

    // Función auxiliar para convertir String a Enum
    coded_format stringToFormat(const char* str);

    // Es buena idea tener un método para verificar si el manager está listo
    bool isReady() { return _sd != nullptr; }

    // TODO analizar si es necesario interpretar que es little endian o no
    static float getFloatConversion(const uint16_t* data);

    //OBTENCION DE PARAMETROS EXISTENTES DENTRO DEL FICHERO EM750map.csv
    const char* getLogInterval() { return _log_interval; }
    const char* getNewFile() { return _new_file; }
    const char* getMaxFiles() { return _max_files; }

    //TODO funciones relacionadas con la MODBUS REQUEST, EN UN FUTURO REFACTORIZAR 
    //bool modbusRequestByFile(char* filename);

};

#endif
