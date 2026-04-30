
#ifndef MODBUS_REQUEST_CSV_H
#define MODBUS_REQUEST_CSV_H

#include "src/SDManager.h"

//clase momentanea que accde a SD para devolver valores de Modbus request. 
#define MAX_TITLES_SIZE 32 // TODO cambiatar esto de nombre


struct ModbusReqParameters{
  char device_name[MAX_TITLES_SIZE];
  char ip_address[MAX_TITLES_SIZE];
};

class ModbusRequestCSV { // TODO Esta clase tendra que cambiar de nombre a algo que gestione CSVs

private:
  SDManager* _sd = nullptr;
public:
  ModbusRequestCSV(SDManager* sdManager); 
  bool begin();

};

#endif