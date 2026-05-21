
#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

// Definición de Niveles de Log
#define MY_LOG_LEVEL_NONE    0
#define MY_LOG_LEVEL_ERROR   1
#define MY_LOG_LEVEL_WARN    2
#define MY_LOG_LEVEL_INFO    3
#define MY_LOG_LEVEL_DEBUG   4
#define MY_LOG_LEVEL_VERBOSE 5

// Variable externa para controlar el nivel en tiempo de ejecución
extern uint8_t g_my_log_current_level;

// Función rápida para cambiar el nivel desde cualquier parte del código
inline void my_log_level_set(uint8_t level) {
    g_my_log_current_level = level;
}

// Macros inteligentes: Solo imprimen si el nivel del mensaje es menor o igual al configurado
#define MY_LOGE(tag, format, ...) do { if (g_my_log_current_level >= MY_LOG_LEVEL_ERROR)   Serial.printf("[E][%s] " format "\n", tag, ##__VA_ARGS__); } while(0)
#define MY_LOGW(tag, format, ...) do { if (g_my_log_current_level >= MY_LOG_LEVEL_WARN)    Serial.printf("[W][%s] " format "\n", tag, ##__VA_ARGS__); } while(0)
#define MY_LOGI(tag, format, ...) do { if (g_my_log_current_level >= MY_LOG_LEVEL_INFO)    Serial.printf("[I][%s] " format "\n", tag, ##__VA_ARGS__); } while(0)
#define MY_LOGD(tag, format, ...) do { if (g_my_log_current_level >= MY_LOG_LEVEL_DEBUG)   Serial.printf("[D][%s] " format "\n", tag, ##__VA_ARGS__); } while(0)
#define MY_LOGV(tag, format, ...) do { if (g_my_log_current_level >= MY_LOG_LEVEL_VERBOSE) Serial.printf("[V][%s] " format "\n", tag, ##__VA_ARGS__); } while(0)

#endif


//#ifndef DEBUG_H
//#define DEBUG_H
//
//#include <Arduino.h>
//
//// Códigos de color ANSI para el Monitor Serie
//#define LOG_CLR_RESET   "\033[0m"
//#define LOG_CLR_RED     "\033[0;31m"
//#define LOG_CLR_YELLOW  "\033[0;33m"
//#define LOG_CLR_GREEN   "\033[0;32m"
//#define LOG_CLR_CYAN    "\033[0;36m"
//#define LOG_CLR_WHITE   "\033[0;37m"
//
//// Macros personalizadas que NUNCA puede borrar el compilador CON COLORES
////#define MY_LOGE(tag, format, ...) Serial.printf(LOG_CLR_RED    "[E][%s] " format LOG_CLR_RESET "\n", tag, ##__VA_ARGS__)
////#define MY_LOGW(tag, format, ...) Serial.printf(LOG_CLR_YELLOW "[W][%s] " format LOG_CLR_RESET "\n", tag, ##__VA_ARGS__)
////#define MY_LOGI(tag, format, ...) Serial.printf(LOG_CLR_GREEN  "[I][%s] " format LOG_CLR_RESET "\n", tag, ##__VA_ARGS__)
////#define MY_LOGD(tag, format, ...) Serial.printf(LOG_CLR_CYAN   "[D][%s] " format LOG_CLR_RESET "\n", tag, ##__VA_ARGS__)
////#define MY_LOGV(tag, format, ...) Serial.printf(LOG_CLR_WHITE  "[V][%s] " format LOG_CLR_RESET "\n", tag, ##__VA_ARGS__)
//
//// Versión limpia sin caracteres ANSI (ideal para el monitor por defecto de Arduino IDE)
//#define MY_LOGE(tag, format, ...) Serial.printf("[E][%s] " format "\n", tag, ##__VA_ARGS__)
//#define MY_LOGW(tag, format, ...) Serial.printf("[W][%s] " format "\n", tag, ##__VA_ARGS__)
//#define MY_LOGI(tag, format, ...) Serial.printf("[I][%s] " format "\n", tag, ##__VA_ARGS__)
//#define MY_LOGD(tag, format, ...) Serial.printf("[D][%s] " format "\n", tag, ##__VA_ARGS__)
//#define MY_LOGV(tag, format, ...) Serial.printf("[V][%s] " format "\n", tag, ##__VA_ARGS__)
//
//#endif