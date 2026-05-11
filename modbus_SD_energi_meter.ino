#include <RTClib.h>

// Transport Layer implementations
#include "src/transport/ModbusTCPManager.h" 
#include "src/transport/ModbusRTUManager.h"

// Core Services
#include "src/services/Datalogger.h"
#include "src/devices/EnergyMeter750.h"
#include "src/services/SDManager.h"

// Logic and Configuration Managers
#include "src/EnergyMeterRegInterpreter.h"
#include "src/ModbusRequestCSV.h"

// Default Modbus Settings
#define SLAVE_ADDRESS 1 
#define MODBUS_PORT 502

// Global Instances
RTC_DS3231 rtc;
SDManager sd;  
Datalogger datalogger(&sd); 
EnergyMeterRegInterpreter regInterpreter(&sd); 
EnergyMeter750 energy_meter; 
ModbusRequestCSV mb_csv(&sd);
ModbusTransport* modbus = nullptr; // Polymorphic pointer for TCP or RTU

void setup() {
  Serial.begin(115200);

  // 1. Initialize SD Card (System Critical)
  if (!sd.begin()) {
    Serial.println(F("Error: SD Card could not be initialized. Blocking system..."));
    while(1); // System cannot operate without SD for configuration/logging
  }

  // 2. Initialize Datalogger (Ensures log directory structure exists)
  if(!datalogger.begin()){ 
    Serial.println(F("Error: Datalogger initialization failed."));
    while(1);
  }
 
  // 3. Initialize Register Interpreter (Loads parameter mapping)
  if(!regInterpreter.begin()){
    Serial.println(F("Error: Register Interpreter failed."));
    while(1);
  }
  
  // 4. Initialize Real Time Clock (RTC)
  if (!rtc.begin()) {
     Serial.print(F("Error: RTC not found."));
     while(1); // Consider implementing a non-blocking fallback if hardware allows
  }

  // NOTE: Manual RTC sync. In production, this should be synced with an NTP server 
  // or only set if the RTC lost power.
  rtc.adjust(DateTime(2026, 5, 11, 15, 26, 0)); 

  // 5. Load Modbus Requests and Device Parameters from CSV on SD
  if(!mb_csv.begin()){
    Serial.println(F("Error: Modbus CSV Manager failed."));
    while(1);
  }

  if(!mb_csv.loadFromSDParameters()){
    Serial.println(F("Warning: Failed to load device parameters from SD."));    
  }

  // Display identified device info
  Serial.print(F("Device Identified: "));
  Serial.println(mb_csv.getDeviceName());
  Serial.print(F("Target IP: "));
  Serial.println(mb_csv.getIpAdress());
  
  // Load specific Modbus request structure (start address, length, etc.)
  Struct_MBRequest req = mb_csv.loadFromSDMbrequest(); 

  // 6. Transport Layer Selection (TCP vs RTU)
  // Logic currently defaults to TCP. Update condition to switch based on config.
  if(true) { 
    // Ethernet Settings
    byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xE9 };
    IPAddress ip(192, 168, 0, 10); // Local device IP
    IPAddress server;

    if(server.fromString(mb_csv.getIpAdress())){
      // Instantiate TCP Manager
      ModbusTCPManager* tcpModbus = new ModbusTCPManager(server, SLAVE_ADDRESS, MODBUS_PORT);
      tcpModbus->begin(mac, ip); 
      modbus = tcpModbus; // Assign to generic interface pointer
    }
      
  } else {
    // Instantiate RTU Manager (RS485)
    ModbusRTUManager* rtuModbus = new ModbusRTUManager(19200, 1, SERIAL_8E1);
    rtuModbus->begin();
    modbus = rtuModbus; // Assign to generic interface pointer
  }

  // 7. Attach Transport to Energy Meter Driver
  if(!energy_meter.begin(modbus)){
    Serial.println(F("Error: Could not link Modbus to Energy Meter."));
    while(1);
  }

  // 8. Prepare Advanced Datalogging Session
  // This validates the CSV request against the meter's memory map and starts the SD log file
  if(!regInterpreter.prepareAdvanceDatalogger(req, &datalogger, &rtc)){
    Serial.println(F("Error: Advanced Datalogger initialization failed. Check request ranges."));
  } 
}

void loop() {
  /**
   * Main execution: 
   * Reads data from Energy Meter, interprets registers, and logs to SD 
   * based on the intervals defined in the configuration.
   */
  regInterpreter.advancedDataloggerExec(&datalogger, &energy_meter, &rtc); 

  // Minor delay to yield to other tasks or background processes
  delay(100); 
}
