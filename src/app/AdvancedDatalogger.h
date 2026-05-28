#pragma once
#include <RTClib.h>
#include "SDManager.h"
#include "Datalogger.h"
#include "EMRegInterpreter.h"
#include "EnergyMeter750.h"
#include "f_getParameters.h" // Contiene tu estructura Parameters

class AdvancedDatalogger {
private:
    SDManager* _sd;
    Datalogger* _datalogger;
    RTC_DS3231* _rtc;
    EMRegInterpreter* _regInterpreter;
    EnergyMeter750* _energyMeter;

    // Estado interno (Antes variables globales)
    bool _isInitialized;
    Parameters _param;
    int _logInterval;
    int _maxFiles;
    unsigned long _anteriorMillisModbus;
    int _ultimaUnidadTiempo;

    // Métodos de control internos (Antes funciones sueltas en main)
    esp_err_t crearNuevaSesionLog();
    esp_err_t lecturaModbus();

public:
    AdvancedDatalogger(SDManager* sd, Datalogger* dl, RTC_DS3231* rtc, 
                       EMRegInterpreter* ri, EnergyMeter750* em);

    esp_err_t begin(Struct_MBRequest mbReq);
    esp_err_t execute(); // Reemplaza a advancedDataloggerExec()
};


//#ifndef APP_DATALOGGER_H
//#define APP_DATALOGGER_H
//
//#include "global_types.h"
//
//// Forward declarations
//class EnergyMeter750;
//class Datalogger; 
//class RTC_DS3231;
//class EnergyMeter750; 
//class EMRegInterpreter; 
//
///**
// * @class EnergyMeterRegInterpreter
// * @brief Manages the parsing of CSV register maps and the conversion of raw Modbus data.
// */
//class AppDatalogger {
//
//private:
//    //SDManager* _sd = nullptr; 
//    //EM_request _current_request; 
//    bool _advancedIsInitialized = false; 
//
//    int ultimaUnidadTiempo;
//   
//    unsigned long anteriorMillisModbus = 0; 
//    //unsigned long anteriorMillisArchivo = 0; 
//
//    esp_err_t lectura_modbus(Datalogger* datalogger, RTC_DS3231* rtc, EnergyMeter750* em, EM_request req);
//    
//public:
//    AppDatalogger();
//
//    esp_err_t prepareAdvanceDatalogger(Struct_MBRequest MB_req,  EnergyMeter750* energy_meter,  Datalogger* datalogger, RTC_DS3231* rtc);
//    esp_err_t advancedDataloggerExec(Datalogger* datalogger, EnergyMeter750* em, RTC_DS3231* rtc);
//};
//
//#endif



