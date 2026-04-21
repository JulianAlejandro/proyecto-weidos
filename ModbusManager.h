#ifndef MODBUS_MANAGER_H
#define MODBUS_MANAGER_H

#include <Ethernet.h>
#include <ArduinoModbus.h>

class ModbusManager {
private:
    IPAddress _serverIp;
    uint16_t _port;
    EthernetClient _ethClient;
    ModbusTCPClient _modbusClient;

public:
    ModbusManager(IPAddress ip, uint16_t port = 502) 
        : _serverIp(ip), _port(port), _modbusClient(_ethClient) {}

    bool begin() {
        // Aquí podrías incluso meter el Ethernet.begin si quisieras encapsular todo
        return _modbusClient.begin(_serverIp, _port);
    }

    bool ensureConnection() {
        if (!_modbusClient.connected()) {
            Serial.println("ModbusManager: Reconectando...");
            return _modbusClient.begin(_serverIp, _port);
        }
        return true;
    }

    // Este método permite que EnergyMeter use el cliente interno
    ModbusTCPClient* getClient() {
        return &_modbusClient;
    }
};

#endif