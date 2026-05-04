
#ifndef MODBUS_REQUEST_CSV_H
#define MODBUS_REQUEST_CSV_H

#include "src/SDManager.h"

//clase momentanea que accde a SD para devolver valores de Modbus request. 
#define MAX_TITLES_SIZE 32 // TODO cambiatar esto de nombre
#define MODBUS_REQ_FILE "MBReq.csv"

//#define FIRST_BLOCK 3
#define FIRST_BLOCK 10

struct Struct_MBRequest {
    uint16_t channel; 
    uint16_t start_addres; 
    uint16_t length; 
    uint16_t func_code;
    uint16_t req_interval_ms; 
};

class ModbusRequestCSV { // TODO Esta clase tendra que cambiar de nombre a algo que gestione CSVs

private:
  SDManager* _sd = nullptr;

  char _device_name[MAX_TITLES_SIZE];
  char _ip_address[MAX_TITLES_SIZE];

public:
  ModbusRequestCSV(SDManager* sdManager); 
  bool begin();

  bool loadFromSDParameters();

  char* getDeviceName(){return _device_name;}
  char* getIpAdress(){return _ip_address;} 

  Struct_MBRequest loadFromSDMbrequest(); 

};

#endif