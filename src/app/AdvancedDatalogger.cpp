

#include "AdvancedDatalogger.h"

AdvancedDatalogger::AdvancedDatalogger(SDManager* sd, Datalogger* dl, RTC_DS3231* rtc, 
                                       EMRegInterpreter* ri, EnergyMeter750* em)
    : _sd(sd), _datalogger(dl), _rtc(rtc), _regInterpreter(ri), _energyMeter(em),
      _isInitialized(false), _anteriorMillisModbus(0), _ultimaUnidadTiempo(-1) {}

esp_err_t AdvancedDatalogger::begin(Struct_MBRequest mbReq) {
    if (mbReq.channel <= 0) return ESP_ERR_INVALID_ARG;
    if (mbReq.length == 0 || mbReq.length > MAX_MODBUS_REGS_REQUEST) return ESP_ERR_INVALID_SIZE;

    esp_err_t err = _regInterpreter->startNewRequest(mbReq.start_addres, mbReq.length);
    if (err != ESP_OK) return err;

    nameColValues misTitulos = _regInterpreter->getLastNameValues();
    if(misTitulos.size == 0) return ESP_ERR_INTERPRETER_MAP_MISS;

    // Recuperamos los parámetros de la SD usando tu función auxiliar
    _param = SDgetParameters(_sd);
    _logInterval = atoi(_param.log_interval);
    _maxFiles = atoi(_param.max_files);

    if (_maxFiles <= 0 || _maxFiles >= MAX_LOG_CAPACITY) return ESP_ERR_INTERPRETER_BAD_CONF;

    _datalogger->setMaxFiles(_maxFiles);
    _isInitialized = true;

    return ESP_OK;
}

esp_err_t AdvancedDatalogger::execute() {
    if (!_isInitialized) return ESP_ERR_INTERPRETER_NOT_INIT;

    unsigned long actualMillis = millis();
    DateTime ahora = _rtc->now();
    bool debeCambiarSesion = false;
    esp_err_t err;

    if (ahora.minute() != _ultimaUnidadTiempo) {
        if (strcasecmp(_param.new_file, "minute") == 0) debeCambiarSesion = true;
        if (strcasecmp(_param.new_file, "hour") == 0 && ahora.minute() == 0) debeCambiarSesion = true;
        if (strcasecmp(_param.new_file, "day") == 0 && ahora.hour() == 0 && ahora.minute() == 0) debeCambiarSesion = true;

        if (debeCambiarSesion) {
            _ultimaUnidadTiempo = ahora.minute();
            err = crearNuevaSesionLog();
            if (err != ESP_OK) return err;
        }
    }

    if (actualMillis - _anteriorMillisModbus >= (unsigned long)_logInterval) {
        _anteriorMillisModbus += _logInterval;
        err = lecturaModbus();
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

esp_err_t AdvancedDatalogger::crearNuevaSesionLog() {
    DateTime ahora = _rtc->now();
    char nombreFichero[16];
    snprintf(nombreFichero, sizeof(nombreFichero), "%02d%02d%02d%02d%02d%02d", 
             ahora.year(), ahora.month(), ahora.day(), ahora.hour(), ahora.minute(), ahora.second());

    nameColValues misTitulos = _regInterpreter->getLastNameValues();
    return _datalogger->newCSVLogSesion(nombreFichero, misTitulos.buffer, misTitulos.size);
}

esp_err_t AdvancedDatalogger::lecturaModbus() {
    EM_request req = _regInterpreter->getLastEMRequest();
    esp_err_t err = _energyMeter->executeRequest(req);
    if (err != ESP_OK) return err;

    rawDataBuffer raw = _energyMeter->readDataBuffer();
    _regInterpreter->getBufferDataRaw(raw.buffer, raw.size);
    netDataString res = _regInterpreter->getBufNetDataString();

    DateTime now = _rtc->now();
    char bufferTime[20];
    snprintf(bufferTime, sizeof(bufferTime), "%04d-%02d-%02d %02d:%02d:%02d", 
             now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

    err = _datalogger->appendNewDataCSVToLog(bufferTime, res.buffer, res.size);
    if (err != ESP_OK) {
        if (err == ESP_ERR_INVALID_STATE && _datalogger->isFileLimitReached()) {
            return ESP_OK; 
        }
        return err;
    }
    return ESP_OK;
}

