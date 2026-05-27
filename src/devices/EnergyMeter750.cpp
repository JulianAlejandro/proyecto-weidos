#include "EnergyMeter750.h"
#include "LogMsgGlobal.h"

EnergyMeter750::EnergyMeter750(){}

esp_err_t EnergyMeter750::begin(IModbusTransport* modbus) {
    if (modbus == nullptr) {
        ESP_LOGE(TAG, "Fallo al iniciar: Puntero Modbus nulo");
        return ESP_ERR_INVALID_ARG;
    }
    _modbus = modbus;
    _initialized = true;
    ESP_LOGI(TAG, "Controlador EM750 inicializado correctamente");
    return ESP_OK;
}

esp_err_t EnergyMeter750::executeRequest(EM_request req) {
    if (!_initialized) {
        ESP_LOGW(TAG, "Intento de ejecución sin inicializar");
        return ESP_ERR_EM750_NOT_INIT;
    }

    // 1. Validaciones previas
    if (req.size == 0 || req.size > MAX_MODBUS_REGS_REQUEST) {
        ESP_LOGE(TAG, "Tamaño de request inválido: %d", req.size);
        //reportError("Request invalido: Size %d erroneo", req.size);
        Log_msg::println(TAG, "Request invalido: Size %d erroneo", req.size);
        return ESP_ERR_INVALID_SIZE;
    }

    if (req.start_addr > MAX_EM_ADDR || (req.start_addr + req.size - 1) > MAX_EM_ADDR) {
        ESP_LOGE(TAG, "Dirección fuera de rango: 0x%04X", req.start_addr);
        //reportError("Direccion fuera de rango: 0x%04X", req.start_addr);
        Log_msg::println(TAG, "Direccion fuera de rango: 0x%04X", req.start_addr);
        return ESP_ERR_EM750_ADDR_OUT_RANGE;
    }

    _lastReadSize = 0;

    // 2. Ejecución del transporte
    // Capturamos el esp_err_t que devuelve el transport (TCP o RTU)
    esp_err_t err = _modbus->readHoldingRegisters(req.start_addr, req.size);

    if (err == ESP_OK) {
        // Solo si la comunicación fue exitosa (ESP_OK = 0), leemos el buffer
        for (int i = 0; i < req.size; i++) {
            _internalBuffer[i] = _modbus->read();
        }
        _lastReadSize = req.size;
        return ESP_OK;
    }

    // 3. Si hubo error, lo propagamos tal cual vino del transporte
    ESP_LOGE(TAG, "Error Modbus (0x%X) en dirección 0x%04X", err, req.start_addr);
    //reportError("Error Modbus 0x%X leyendo %d registros desde 0x%04X", err, req.size, req.start_addr);
    Log_msg::println(TAG, "Error Modbus 0x%X leyendo %d registros desde 0x%04X", err, req.size, req.start_addr);

    return err; 
}



esp_err_t EnergyMeter750::readRegByAdress(uint16_t addr, uint16_t *out_value) {
    if (!_initialized) return ESP_ERR_EM750_NOT_INIT;
    if (out_value == nullptr) return ESP_ERR_INVALID_ARG;

    // Capturamos el error del transporte
    esp_err_t err = _modbus->readHoldingRegisters(addr, 1);

    if (err == ESP_OK) {
        *out_value = _modbus->read();
        return ESP_OK;
    }

    // Propagamos el error (Timeout, Connection Fail, etc.)
    //reportError("Error 0x%X en lectura simple de direccion 0x%04X", err, addr);
    Log_msg::println(TAG, "Error 0x%X en lectura simple de direccion 0x%04X", err, addr);
    return err;
}

rawDataBuffer EnergyMeter750::readDataBuffer() {
    return { _internalBuffer, _lastReadSize };
}


/*
#include "EnergyMeter750.h"

EnergyMeter750::EnergyMeter750() : ObservableError(TAG) {}

esp_err_t EnergyMeter750::begin(IModbusTransport* modbus) {
    if (modbus == nullptr) {
        ESP_LOGE(TAG, "Fallo al iniciar: Puntero Modbus nulo");
        return ESP_ERR_INVALID_ARG;
    }
    _modbus = modbus;
    _initialized = true;
    ESP_LOGI(TAG, "Controlador EM750 inicializado correctamente");
    return ESP_OK;
}

esp_err_t EnergyMeter750::executeRequest(EM_request req) {
    if (!_initialized) {
        ESP_LOGW(TAG, "Intento de ejecución sin inicializar");
        return ESP_ERR_EM750_NOT_INIT;
    }

    // 1. Validaciones previas
    if (req.size == 0 || req.size > MAX_MODBUS_REGS_REQUEST) {
        ESP_LOGE(TAG, "Tamaño de request inválido: %d", req.size);
        reportError("Request invalido: Size %d erroneo", req.size);
        return ESP_ERR_INVALID_SIZE;
    }

    if (req.start_addr > MAX_EM_ADDR || (req.start_addr + req.size - 1) > MAX_EM_ADDR) {
        ESP_LOGE(TAG, "Dirección fuera de rango: 0x%04X", req.start_addr);
        reportError("Direccion fuera de rango: 0x%04X", req.start_addr);
        return ESP_ERR_EM750_ADDR_OUT_RANGE;
    }

    _lastReadSize = 0;

    // 2. Ejecución del transporte
    // Capturamos el esp_err_t que devuelve el transport (TCP o RTU)
    esp_err_t err = _modbus->readHoldingRegisters(req.start_addr, req.size);

    if (err == ESP_OK) {
        // Solo si la comunicación fue exitosa (ESP_OK = 0), leemos el buffer
        for (int i = 0; i < req.size; i++) {
            _internalBuffer[i] = _modbus->read();
        }
        _lastReadSize = req.size;
        return ESP_OK;
    }

    // 3. Si hubo error, lo propagamos tal cual vino del transporte
    ESP_LOGE(TAG, "Error Modbus (0x%X) en dirección 0x%04X", err, req.start_addr);
    reportError("Error Modbus 0x%X leyendo %d registros desde 0x%04X", err, req.size, req.start_addr);

    return err; 
}



esp_err_t EnergyMeter750::readRegByAdress(uint16_t addr, uint16_t *out_value) {
    if (!_initialized) return ESP_ERR_EM750_NOT_INIT;
    if (out_value == nullptr) return ESP_ERR_INVALID_ARG;

    // Capturamos el error del transporte
    esp_err_t err = _modbus->readHoldingRegisters(addr, 1);

    if (err == ESP_OK) {
        *out_value = _modbus->read();
        return ESP_OK;
    }

    // Propagamos el error (Timeout, Connection Fail, etc.)
    reportError("Error 0x%X en lectura simple de direccion 0x%04X", err, addr);
    return err;
}

rawDataBuffer EnergyMeter750::readDataBuffer() {
    return { _internalBuffer, _lastReadSize };
}

*/