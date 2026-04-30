#include "ModbusRequestCSV.h"


ModbusRequestCSV::ModbusRequestCSV(SDManager* sdManager) {
    _sd = sdManager;
    //_setupFile = SETUP_FILE;
}

bool ModbusRequestCSV::begin(){
   // analizar si el SD ya esta iniciado 

    if (!_sd->isReady()) { 
       // Serial.println("Modbus Request: SDManager no está listo aún.");
        return false;
    }
    return true;

   /*
    if (!_sd->begin()) {
        Serial.println(F("Error: No se pudo iniciar la SD desde EnergyMeterRegInterpreter"));
        return false;
    }
    */
}