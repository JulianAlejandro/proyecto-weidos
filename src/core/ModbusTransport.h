
//TODO: descomentar las funciones que estan comentadas para valorar su utilidad en mejorar el modbus
#ifndef MODBUS_TRANSPORT_H
#define MODBUS_TRANSPORT_H

#include <Arduino.h>

/**
 * @brief Tipos de registros Modbus (basados en ArduinoModbus)
 */
 
 /*
enum ModbusRegisterType {
    COILS,
    DISCRETE_INPUTS,
    HOLDING_REGISTERS,
    INPUT_REGISTERS
};
*/
/**
 * @brief Clase Interfaz para abstraer el transporte Modbus (TCP o RTU)
 */
class ModbusTransport {
public:
    virtual ~ModbusTransport() {}

    // --- Métodos de Control ---
    
    /**
     * @brief Inicializa el hardware/cliente. 
     * @return true si la inicialización fue exitosa.
     */
    virtual bool begin() = 0;

//todo cambiar el isConnected por connected
    /**
     * @brief Verifica si el cliente está conectado o el bus está listo.
     */
    virtual bool isConnected() = 0;
    //virtual bool connected() = 0;

    // --- Métodos de Lectura ---

    /**
     * @brief Envía una petición de lectura al esclavo.
     * @param slaveAddress ID del esclavo (en TCP suele ser 1 o 255).
     * @param type Tipo de registro (COILS, HOLDING_REGISTERS, etc.)
     * @param address Dirección de inicio.
     * @param nb Cantidad de registros/puntos.
     * @return true si la petición fue aceptada/enviada con éxito.
     */
    //virtual bool requestFrom(int slaveAddress, int type, uint16_t address, uint16_t nb) = 0;

    /**
     * @brief Lee el siguiente valor del buffer de la última petición requestFrom.
     * @return El valor leído (convertido a long para soportar diferentes tipos).
     */
    //virtual long read() = 0;

    /**
     * @brief Método de conveniencia que suele usar tu EnergyMeter para holding registers.
     */
     /*
    virtual bool readHoldingRegisters(uint16_t address, uint16_t quantity) {
        // Implementación por defecto usando el requestFrom genérico
        return requestFrom(1, HOLDING_REGISTERS, address, quantity);
    }
    */
    virtual bool readHoldingRegisters(uint16_t address, uint16_t quantity) = 0; 
    virtual bool readCoils(int address, int quantity) = 0;

    /**
     * @brief Devuelve un dato del buffer (equivalente a read() en tus ejemplos).
     */
    virtual uint16_t getAvailableData() = 0;

    // --- Métodos de Escritura ---

    virtual bool writeHoldingRegister(uint16_t address, uint16_t value) = 0;
    virtual bool writeCoil(uint16_t address, bool value) = 0;

    // --- Configuración (Lo que hablamos del JSON) ---
    
    /**
     * @brief Permite al Orquestador configurar IP/Puerto o Baudrate/ID 
     * pasando un puntero a los datos (puede ser un JSON o estructura).
     */
    //virtual void setConfig(void* configData) = 0;
};

#endif


//#ifndef MODBUS_TRANSPORT_H
//#define MODBUS_TRANSPORT_H
//
//#include <Arduino.h>
//
///**
// * @brief Tipos de registros Modbus (basados en ArduinoModbus)
// */
//enum ModbusRegisterType {
//    COILS,
//    DISCRETE_INPUTS,
//    HOLDING_REGISTERS,
//    INPUT_REGISTERS
//};
//
///**
// * @brief Clase Interfaz para abstraer el transporte Modbus (TCP o RTU)
// */
//class ModbusTransport {
//public:
//    virtual ~ModbusTransport() {}
//
//    // --- Métodos de Control ---
//    
//    /**
//     * @brief Inicializa el hardware/cliente. 
//     * @return true si la inicialización fue exitosa.
//     */
//    virtual bool begin() = 0;
//
//    /**
//     * @brief Verifica si el cliente está conectado o el bus está listo.
//     */
//    virtual bool connected() = 0;
//
//    // --- Métodos de Lectura ---
//
//    /**
//     * @brief Envía una petición de lectura al esclavo.
//     * @param slaveAddress ID del esclavo (en TCP suele ser 1 o 255).
//     * @param type Tipo de registro (COILS, HOLDING_REGISTERS, etc.)
//     * @param address Dirección de inicio.
//     * @param nb Cantidad de registros/puntos.
//     * @return true si la petición fue aceptada/enviada con éxito.
//     */
//    virtual bool requestFrom(int slaveAddress, int type, uint16_t address, uint16_t nb) = 0;
//
//    /**
//     * @brief Lee el siguiente valor del buffer de la última petición requestFrom.
//     * @return El valor leído (convertido a long para soportar diferentes tipos).
//     */
//    virtual long read() = 0;
//
//    /**
//     * @brief Método de conveniencia que suele usar tu EnergyMeter para holding registers.
//     */
//    virtual bool readHoldingRegisters(uint16_t address, uint16_t nb) {
//        // Implementación por defecto usando el requestFrom genérico
//        return requestFrom(1, HOLDING_REGISTERS, address, nb);
//    }
//
//    /**
//     * @brief Devuelve un dato del buffer (equivalente a read() en tus ejemplos).
//     */
//    virtual uint16_t getAvailableData() = 0;
//
//    // --- Métodos de Escritura ---
//
//    virtual bool holdingRegisterWrite(uint16_t address, uint16_t value) = 0;
//    virtual bool coilWrite(uint16_t address, bool value) = 0;
//
//    // --- Configuración (Lo que hablamos del JSON) ---
//    
//    /**
//     * @brief Permite al Orquestador configurar IP/Puerto o Baudrate/ID 
//     * pasando un puntero a los datos (puede ser un JSON o estructura).
//     */
//    virtual void setConfig(void* configData) = 0;
//};
//
//#endif

