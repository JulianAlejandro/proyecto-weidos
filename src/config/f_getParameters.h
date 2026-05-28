#pragma once

#include "SDManager.h" // Necesario porque usamos SDManager* en el parámetro

#define MAX_TEXT_SIZE 32

/**
 * @struct Parameters
 * @brief High-level datalogger configuration parameters extracted from CSV.
 */
struct Parameters {
    char log_interval[MAX_TEXT_SIZE];
    char new_file[MAX_TEXT_SIZE];
    char max_files[MAX_TEXT_SIZE]; 
};

/**
 * @brief Lee el archivo CSV de la SD y extrae los parámetros de configuración.
 * @param _sd Puntero al manejador de la tarjeta SD.
 * @return Estructura Parameters con los datos leídos.
 */
Parameters SDgetParameters(SDManager* _sd);