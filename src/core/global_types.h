// src/global_types.h
#ifndef GLOBAL_TYPES_H
#define GLOBAL_TYPES_H

#include <cstdint>   // <--- ¡ESTA ES LA LÍNEA CRUCIAL PARA ARREGLAR 'uint16_t'!
#include <esp_err.h>  // Opcional por si usas esp_err_t aquí dentro

struct Struct_MBRequest {
    uint16_t channel;           ///< Slave ID / Unit ID
    uint16_t start_addres;      ///< Register starting address
    uint16_t length;            ///< Number of registers to read
    uint16_t func_code;         ///< Modbus Function Code (e.g., 3 or 4)
    uint16_t req_interval_ms;   ///< Polling interval in milliseconds
};



struct EM_request {
    uint16_t start_addr; 
    uint16_t size; 
};

#endif