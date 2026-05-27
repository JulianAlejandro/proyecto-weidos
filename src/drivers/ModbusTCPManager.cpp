#include "ModbusTCPManager.h"
#include "LogMsgGlobal.h"

static const char* TAG = "MB_TCP_MGR";


ModbusTCPManager::~ModbusTCPManager() {
    stop();
}

/**
 * @brief Gestión de reconexión con reporte de estado detallado.
 * Se ha cambiado el retorno a esp_err_t para diferenciar entre 
 * errores de socket y errores de configuración.
 */
esp_err_t ModbusTCPManager::ensureConnection() {
    if (_modbusClient.connected()) {
        return ESP_OK;
    }

    // Liberar recursos del socket antes de reintentar
    _modbusClient.stop(); 
    
    ESP_LOGW(TAG, "Reintentando conexión con servidor %s:%d", _serverIP.toString().c_str(), _port);
    
    if (!_modbusClient.begin(_serverIP, _port)) {
        ESP_LOGE(TAG, "Error de transporte: No se pudo abrir el socket TCP");
        Log_msg::println(TAG, "Error de transporte: No se pudo abrir el socket TCP");
        return ESP_ERR_MODBUS_TCP_SOCKET; 
    }
    
    delay(50); // Tiempo de cortesía para el handshake TCP
    ESP_LOGI(TAG, "Conexión establecida con éxito");
    return ESP_OK;
}

/**
 * @brief Inicialización de hardware Ethernet.
 */
esp_err_t ModbusTCPManager::begin(byte mac[], IPAddress localIP) {
    ESP_LOGI(TAG, "Inicializando chip Ethernet...");
    Ethernet.init(ETHERNET_CS); 
    Ethernet.begin(mac, localIP);
    
    // Verificación de hardware físico
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        ESP_LOGE(TAG, "Hardware Ethernet no detectado");
        Log_msg::println(TAG, "Hardware Ethernet no detectado");
        return ESP_ERR_NOT_FOUND;
    }
    
    delay(1000); 
    return ESP_OK;
}

/**
 * @brief Implementación de la interfaz: inicia la conexión Modbus.
 */
esp_err_t ModbusTCPManager::begin() {
    return ensureConnection();
}

/**
 * @brief Lectura de Holding Registers.
 * Propaga el error de conexión si falla, o devuelve TIMEOUT si el server no responde.
 */
esp_err_t ModbusTCPManager::readHoldingRegisters(uint16_t address, uint16_t quantity) {
    esp_err_t status = ensureConnection();
    if (status != ESP_OK) return status;

    if (!_modbusClient.requestFrom(_slaveID, HOLDING_REGISTERS, address, quantity)) {
        ESP_LOGE(TAG, "Fallo en lectura Regs @ 0x%04X. Cerrando socket.", address);
        Log_msg::println(TAG, "Fallo en lectura Regs @ 0x%04X. Cerrando socket.", address);
        _modbusClient.stop(); // Forzamos cierre para limpiar el buffer en caso de error
        return ESP_ERR_MODBUS_TIMEOUT;
    }
    return ESP_OK;
}

/**
 * @brief Lectura de Coils.
 */
esp_err_t ModbusTCPManager::readCoils(int address, int quantity) {
    esp_err_t status = ensureConnection();
    if (status != ESP_OK) return status;

    if (!_modbusClient.requestFrom(_slaveID, COILS, address, quantity)) {
        _modbusClient.stop();
        return ESP_ERR_MODBUS_TIMEOUT;
    }
    return ESP_OK;
}

/**
 * @brief Escritura de registro.
 */
esp_err_t ModbusTCPManager::writeHoldingRegister(uint16_t address, uint16_t value) {
    esp_err_t status = ensureConnection();
    if (status != ESP_OK) return status;

    if (!_modbusClient.holdingRegisterWrite(_slaveID, address, value)) {
        return ESP_ERR_MODBUS_TIMEOUT;
    }
    return ESP_OK;
}

/**
 * @brief Fetches data from the internal Modbus response buffer.
 */
uint16_t ModbusTCPManager::read() {
    return _modbusClient.read();
}

/**
 * @brief Escritura de Coil.
 */
esp_err_t ModbusTCPManager::writeCoil(uint16_t address, bool value) {
    esp_err_t status = ensureConnection();
    if (status != ESP_OK) return status;

    if (!_modbusClient.coilWrite(_slaveID, address, value)) {
        return ESP_ERR_MODBUS_TIMEOUT;
    }
    return ESP_OK;
}

/**
 * @brief Checks if the TCP socket is currently active.
 */
bool ModbusTCPManager::connected() {
    return _modbusClient.connected();
}

/**
 * @brief Implementación genérica requestFrom (Interface polimórfica).
 */
esp_err_t ModbusTCPManager::requestFrom(int slaveAddress, int type, uint16_t address, uint16_t nb){
    esp_err_t status = ensureConnection();
    if (status != ESP_OK) return status;

    if (!_modbusClient.requestFrom(slaveAddress, type, address, nb)) {
        return ESP_ERR_MODBUS_TIMEOUT;
    }
    return ESP_OK;
}

/**
 * @brief Cierra de forma ordenada la sesión Modbus y libera el socket TCP.
 */
void ModbusTCPManager::stop() {
    // Si el cliente está conectado, enviamos el cierre formal TCP (Handshake FIN/ACK)
    if (_modbusClient.connected()) {
        ESP_LOGI(TAG, "Cerrando conexión activa con servidor Modbus TCP de forma limpia...");
    }
    
    _modbusClient.stop(); // Detiene el wrapper de Modbus
    _ethClient.stop();    // Asegura la liberación del socket físico en el chip W5500/ENC28J60
}