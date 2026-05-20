#ifndef DATALOGGER_FILE_MANAGER_H
#define DATALOGGER_FILE_MANAGER_H

#include "SDManager.h"
#include <esp_err.h>
#include "LogBuffer.h"

#define MAX_LOG_CAPACITY 150
#define FILE_NAME_SIZE 32 // Supports full path (e.g., /LOGS/YYMMDDHH.txt)
#define DIR_LOG_NAME "/LOGS"

#define PATH_ERR_LOG "/LOGS/ERROR"

// el minimo tiempo que tiene que durar un log es 1 minuto sino hay problemas, NO PODRA HABER DOS LOG CON EL MISMO NOMBRE. 

class DataloggerFileManager {

private:

  // ATRIBUTOS DE HW 
  SDManager* _sd; 
  //LogBuffer _buffer;
  bool _initialized = false;
  char _baseYearPath[FILE_NAME_SIZE]; 
  char _currentLogFile[FILE_NAME_SIZE];

  uint16_t _intLastYearLog; 
  uint32_t _intLastTimestampLog; 
  //uint64_t _lastLogTimestamp = 0; // completar

// aTRIBUTOS DE LOGS
  char _filenames[MAX_LOG_CAPACITY][FILE_NAME_SIZE]; // Array to store existing log paths
  uint16_t _fileCount;  
  uint16_t _userMaxFiles; //Client limit

  esp_err_t setLastSesion(); // en funcion de la informacion que hay en la SD. // los datos tienen que ser mas antoguos que lo que hay en la SD. 
  esp_err_t setFilesLastSesion();
  esp_err_t deleteInvalidFiles(); // opcional por si se quieren borrar todas las que NO estan mas actualizadas. 
  

// otro fichero de log dedicado solo a capturar algun mensaje de error. 

  static void buscarAnioMasRecienteCallback(const char* fileName, bool isDir, void* context);
  static void getSesionFilenamesCallback(const char* fileName, bool isDir, void* context);
  static void deleteInvalidFilesCallback(const char* fileName, bool isDir, void* context);

  //void m_pushToBuffer(const char* csvLine); 

  esp_err_t setErrorLog();
  esp_err_t newYearFile(uint16_t year);

  void setLastLogTime();

public:
  DataloggerFileManager(SDManager* sdManager, uint16_t maxFiles);

  void setMaxFiles(uint16_t maxFiles);

  esp_err_t begin ();
   
  // funciones para guardar datos con formato CSV , quiza esto es un nuevo objeto. 
  esp_err_t setCSVLastEnvironment(bool delete_rest);
  //esp_err_t createNewEnvironmet();

  esp_err_t newFileLog(const char* timestamp);
  //esp_err_t newCSVLogSesion(const char * timestamp); 
  //esp_err_t newCSVLogSesion(const char * timestamp, const char** titles, uint16_t numTitles);

  char* getCurrentLogPath(); 

  //char* getCurrentBaseYearPath();

// ahora esta funcionalidad especifica de CSV va fuera 
  //esp_err_t appendNewDataCSVToLog(const char* timestamp, const char** values, uint16_t numValues);
  //esp_err_t appendNewDataCSVToLog(const char* timestamp);

  esp_err_t appendErrorLog(const char* timestamp, const char* err_message);

 // TODO esto hay que pensar en donde ponerlo. 
  //esp_err_t newLogSesion(const char * timestamp, const char* title_text);
  //esp_err_t appendNewDataToLog(const char* timestamp, const char* message_log); 

  //bool flushBuffer();
   
};

#endif

