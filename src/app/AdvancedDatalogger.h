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

    esp_err_t begin(const Struct_MBRequest* mbReqs, uint16_t n_reqs);
    esp_err_t execute(); // Reemplaza a advancedDataloggerExec()
};
