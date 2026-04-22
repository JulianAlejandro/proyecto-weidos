
#include "EnergyMeter750.h"
#include "EnergyMeterRegInterpreter.h"

EnergyMeter750::EnergyMeter750(uint8_t slaveAddress) {
    _slaveAddress = slaveAddress;
}

int EnergyMeter750::begin(ModbusTCPClient* modbusClient) {
    _modbus = modbusClient;
  
    return true;
}

// En EnergyMeter750.cpp
bool EnergyMeter750::readRegisters(EM_request req) {
    // 1. Limpieza/Reset por seguridad (opcional)
    _lastReadSize = 0; 

    // 2. Validación de límites
    if (req.size > MAX_MODBUS_REGS) return false;

    // 3. Intento de lectura Modbus
    if (_modbus->requestFrom(_slaveAddress, INPUT_REGISTERS, req.start_addr, req.size)) {
        uint16_t i = 0;
        while (_modbus->available() && i < req.size) {
            _internalBuffer[i] = _modbus->read();
            i++;
        }
        
        // 4. ACTUALIZACIÓN CRÍTICA: Guardamos cuánto se leyó realmente
        _lastReadSize = i; 
        return true;
    }
    
    return false; // Si falla, _lastReadSize se queda en 0
}

/*
bool EnergyMeter750::readAndProcess_2(long start_addr, long size, EnergyMeterRegInterpreter* mapa, void (*callback)(float)) {
    if (mapa == nullptr || _modbus == nullptr) return false;

//TODO: no quiero que devuelva un vector 
    // PASO 1: Obtener el mapa de formatos desde la SD
    std::vector<coded_format> formatos = mapa->devuelveRegData(start_addr, size);
    if (formatos.empty()) return false;

    // PASO 2: Calcular cuántos registros Modbus (16-bit cada uno) necesitamos pedir
    int modbus_data_size = 0;
    for (coded_format f : formatos) {
        modbus_data_size += getFormatSize(f);
    }

    // PASO 3: Petición Modbus única para todo el bloque
    std::vector<uint16_t> rawValues;
    rawValues.reserve(modbus_data_size);

//TODO A la hora de hacer una solicitud MODBUS hay que definir estos parametros
    if (_modbus->requestFrom(_slaveAddress, INPUT_REGISTERS, start_addr, modbus_data_size)) {
        while (_modbus->available()) {
            rawValues.push_back(_modbus->read());
        }
    } else {
        return false; // Error de comunicación Modbus
    }

    // PASO 4: Procesar y convertir los datos según el formato
    int idx = 0; // Índice para movernos por el array rawValues
    for (coded_format f : formatos) {
        
        if (f == FLOAT) {
            if (idx + 1 < rawValues.size()) {
                // Combinamos dos registros de 16 bits en uno de 32
                uint32_t combinado = ((uint32_t)rawValues[idx] << 16) | rawValues[idx + 1];
                float resultado;
                memcpy(&resultado, &combinado, sizeof(resultado));
                
                callback(resultado);
                idx += 2; // Avanzamos 2 registros
            }
        } 
        else if (f == INT || f == UINT) {
            // Ejemplo para otros formatos de 32 bits (2 registros)
            idx += 2;
        }
        else if (f == SHORT || f == USHORT || f == BYTE) {
            // Ejemplo para formatos de 16 bits (1 registro)
            // float val = (float)rawValues[idx];
            // callback(val);
            idx += 1;
        }
        // ... añadir más casos según necesites
    }

    return true;
}
*/


/**
 * Función auxiliar para saber cuántos registros Modbus ocupa cada formato
 */
 /*
int EnergyMeter750::getFormatSize(coded_format f) {
    switch (f) {
        case FLOAT:
        case INT:
        case UINT:
            return 2; // 32 bits = 2 registros Modbus
        case LONG64:
            return 4; // 64 bits = 4 registros Modbus
        case SHORT:
        case USHORT:
        case BYTE:
        case DFLOAT:
            return 1; // 16 bits = 1 registro Modbus
        default:
            return 0;
    }
}

 uint16_t readAdress(long addr){

 }
*/