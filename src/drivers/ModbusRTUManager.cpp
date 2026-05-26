#include "ModbusRTUManager.h"

static const char* TAG = "MB_RTU_MGR";

/**
 * @brief Initialize members with default serial and pin values.
 */
ModbusRTUManager::ModbusRTUManager(uint32_t baudrate, uint8_t slaveID, uint32_t config) 
    : _baudrate(baudrate), _config(config), _slaveID(slaveID) {
    
    // Default hardware pins for Weidos architecture
    _txPin = RS485_TX;
    _dePin = RS485_DE;
    _rePin = RS485_RE;
}

/**
 * @brief Updates the RS485 control pins. Must be called before begin().
 */
void ModbusRTUManager::setPins(int tx, int de, int re) {
    _txPin = tx;
    _dePin = de;
    _rePin = re;
}

/**
 * @brief Sets up the RS485 flow control and starts the serial client.
 */
esp_err_t ModbusRTUManager::begin() {
    ESP_LOGI(TAG, "Iniciando RTU (RS485) a %d bps...", _baudrate);
    
    RS485.setPins(_txPin, _dePin, _rePin);
    
    if (!ModbusRTUClient.begin(_baudrate, _config)) {
        ESP_LOGE(TAG, "Error crítico: No se pudo inicializar ModbusRTUClient (¿Puerto ocupado?)");
        return ESP_ERR_MODBUS_NOT_READY;
    }
    
    return ESP_OK;
}

/**
 * @brief Status check for Serial Modbus.
 * Since RTU is stateless, it returns true if the hardware is ready.
 */
bool ModbusRTUManager::connected() {
    return true; 
}

/**
 * @brief Ejecución de petición genérica con detección de errores de bus.
 */
esp_err_t ModbusRTUManager::requestFrom(int slaveAddress, int type, uint16_t address, uint16_t nb) {
    if (!ModbusRTUClient.requestFrom(slaveAddress, type, address, nb)) {
        // En RTU, un fallo suele ser por Timeout (el esclavo no responde)
        // o por CRC Error (ruido en el cable).
        ESP_LOGE(TAG, "Error RTU: Fallo en petición a esclavo %d (Addr: 0x%04X)", slaveAddress, address);
        
        // El cliente de ArduinoModbus no diferencia internamente entre CRC y Timeout,
        // pero por estadística en RS485 solemos reportar Timeout si no hay respuesta válida.
        return ESP_ERR_MODBUS_TIMEOUT;
    }
    return ESP_OK;
}

/**
 * @brief Fetches data from the Modbus serial buffer.
 */
uint16_t ModbusRTUManager::read() {
    return (uint16_t)ModbusRTUClient.read();
}

/**
 * @brief Lectura de registros Holding.
 */
esp_err_t ModbusRTUManager::readHoldingRegisters(uint16_t address, uint16_t quantity) {
    return requestFrom(_slaveID, HOLDING_REGISTERS, address, quantity);
}

/**
 * @brief Lectura de Coils.
 */
esp_err_t ModbusRTUManager::readCoils(int address, int quantity) {
    return requestFrom(_slaveID, COILS, (uint16_t)address, (uint16_t)quantity);
}

/**
 * @brief Escritura de registro con validación.
 */
esp_err_t ModbusRTUManager::writeHoldingRegister(uint16_t address, uint16_t value) {
    if (!ModbusRTUClient.holdingRegisterWrite(_slaveID, address, value)) {
        ESP_LOGE(TAG, "Error RTU: Fallo al escribir Holding Reg @ 0x%04X", address);
        return ESP_ERR_MODBUS_TIMEOUT;
    }
    return ESP_OK;
}

/**
 * @brief Escritura de Coil con validación.
 */
esp_err_t ModbusRTUManager::writeCoil(uint16_t address, bool value) {
    if (!ModbusRTUClient.coilWrite(_slaveID, address, value)) {
        ESP_LOGE(TAG, "Error RTU: Fallo al escribir Coil @ 0x%04X", address);
        return ESP_ERR_MODBUS_TIMEOUT;
    }
    return ESP_OK;
}