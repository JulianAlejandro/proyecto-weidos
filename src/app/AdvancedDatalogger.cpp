

#include "AdvancedDatalogger.h"

static const char* TAG = "ADV_DATALOG";

AdvancedDatalogger::AdvancedDatalogger(SDManager* sd, Datalogger* dl, RTC_DS3231* rtc, 
                                       EMRegInterpreter* ri, EnergyMeter750* em)
    : _sd(sd), _datalogger(dl), _rtc(rtc), _regInterpreter(ri), _energyMeter(em),
      _isInitialized(false), _anteriorMillisModbus(0), _ultimaUnidadTiempo(-1) {}

esp_err_t AdvancedDatalogger::begin(const Struct_MBRequest* mbReqs, uint16_t n_reqs) {

    for (int i = 0; i < n_reqs; i++){
        if (mbReqs[i].channel <= 0) return ESP_ERR_INVALID_ARG;
        if (mbReqs[i].length == 0 || mbReqs[i].length > MAX_MODBUS_REGS_REQUEST) return ESP_ERR_INVALID_SIZE;

        // cargamos todos los registros
        esp_err_t err = _regInterpreter->appendRequest(mbReqs[i].start_addres, mbReqs[i].length);
        if (err != ESP_OK) return err;
         ESP_LOGD(TAG, "Solicitud con Modbus con addr: %d y size %d cargada en interpretador mapas de registros",mbReqs[i].start_addres, mbReqs[i].length );
    }
    uint16_t n_reqs_aux = _regInterpreter->getCountMBRequests();
    if((n_reqs_aux <= 0) || (n_reqs_aux > MAX_MB_REQ_SECCIONS)) return ESP_FAIL;  

    ESP_LOGD(TAG, "Cargadas en el mapa de registro %d solicitudes modbus", n_reqs_aux);
/*
    uint16_t n_reqs_aux = _regInterpreter->getCountMBRequests(); 

    for ( int i = 0; i < n_reqs_aux; i++){ 
        //misTitulos.buffer = nullptr; 
        //misTitulos.size = 0; 
        nameColValues misTitulos = _regInterpreter->getLastNameValues(i);
        Serial.print("cambiamos de mb req con numero de titulos:");
        Serial.println(misTitulos.size);
        for(int j = 0; j < misTitulos.size; j++){ 
            Serial.println(misTitulos.buffer[j]); 
        }
        if(misTitulos.size == 0) return ESP_ERR_INTERPRETER_MAP_MISS;  
    }
    return ESP_FAIL; 
*/
    // Recuperamos los parámetros de la SD usando tu función auxiliar lo hacemos aqui para usar la SD una unica vez 
    _param = SDgetParameters(_sd);
    _logInterval = atoi(_param.log_interval);
    uint16_t maxFiles = atoi(_param.max_files);

    if (maxFiles <= 0 || maxFiles >= MAX_LOG_CAPACITY) return ESP_ERR_INTERPRETER_BAD_CONF;

    _datalogger->setMaxFiles(maxFiles);
    _isInitialized = true;

    return ESP_OK;
}

esp_err_t AdvancedDatalogger::execute() {
    
    if (!_isInitialized) return ESP_ERR_INTERPRETER_NOT_INIT;

    unsigned long actualMillis = millis();
    DateTime ahora = _rtc->now();
    bool debeCambiarSesion = false;
    esp_err_t err;

    if (ahora.minute() != _ultimaUnidadTiempo) {
        if (strcasecmp(_param.new_file, "minute") == 0) debeCambiarSesion = true;
        if (strcasecmp(_param.new_file, "hour") == 0 && ahora.minute() == 0) debeCambiarSesion = true;
        if (strcasecmp(_param.new_file, "day") == 0 && ahora.hour() == 0 && ahora.minute() == 0) debeCambiarSesion = true;

        if (debeCambiarSesion) {
            _ultimaUnidadTiempo = ahora.minute();
            err = crearNuevaSesionLog();
            if (err != ESP_OK) return err;
        }
    }

    if (actualMillis - _anteriorMillisModbus >= (unsigned long)_logInterval) {
        _anteriorMillisModbus += _logInterval;
        err = lecturaModbus();
        if (err != ESP_OK) return err;
    }
        
    return ESP_OK;
}

esp_err_t AdvancedDatalogger::crearNuevaSesionLog() {
DateTime ahora = _rtc->now();
    char nombreFichero[16];
    snprintf(nombreFichero, sizeof(nombreFichero), "%02d%02d%02d%02d%02d%02d", 
        ahora.year(), ahora.month(), ahora.day(), ahora.hour(), ahora.minute(), ahora.second());

    uint16_t n_reqs_aux = _regInterpreter->getCountMBRequests();

    // Creamos un contenedor único para albergar todos los títulos consolidados
    const char* titulosConsolidados[MAX_MODBUS_REGS * MAX_MB_REQ_SECCIONS];
    uint16_t totalTitulos = 0;
    Serial.println("aqui van los titulos: "); 
    for (int i = 0; i < n_reqs_aux; i++) {
        nameColValues misTitulos = _regInterpreter->getLastNameValues(i); 
        
        // Copiamos los punteros de títulos de esta sección al contenedor global
        for (int j = 0; j < misTitulos.size; j++) {
            if (totalTitulos < (MAX_MODBUS_REGS * MAX_MB_REQ_SECCIONS)) {
                titulosConsolidados[totalTitulos] = misTitulos.buffer[j];
                Serial.println(titulosConsolidados[totalTitulos]);
                totalTitulos++;
            }
        }
    }
    
    // Pasamos el array completo con el conteo final de títulos concatenados
    return _datalogger->newCSVLogSesion(nombreFichero, titulosConsolidados, totalTitulos);

    /*
    DateTime ahora = _rtc->now();
    char nombreFichero[16];
    snprintf(nombreFichero, sizeof(nombreFichero), "%02d%02d%02d%02d%02d%02d", 
             ahora.year(), ahora.month(), ahora.day(), ahora.hour(), ahora.minute(), ahora.second());

    nameColValues misTitulos = _regInterpreter->getLastNameValues(0);
    return _datalogger->newCSVLogSesion(nombreFichero, misTitulos.buffer, misTitulos.size);
    */
}

esp_err_t AdvancedDatalogger::lecturaModbus() {

    uint16_t n_reqs_aux = _regInterpreter->getCountMBRequests();

    const char* completeData[MAX_MODBUS_REGS * MAX_MB_REQ_SECCIONS];
    uint16_t n_completeData = 0;
    Serial.println("aqui van los datos: "); 
    for (int i = 0; i < n_reqs_aux; i++) {
        EM_request req = _regInterpreter->getLastEMRequest(i);
        
        if (_energyMeter->executeRequest(req) == ESP_OK) {
            rawDataBuffer raw = _energyMeter->readDataBuffer();
            _regInterpreter->getBufferDataRaw(raw.buffer, raw.size, i);
            netDataString res = _regInterpreter->getBufNetDataString(i);
            
            for (int j = 0; j < res.size; j++) {
                if (n_completeData < (MAX_MODBUS_REGS * MAX_MB_REQ_SECCIONS)) {
                    // Si el puntero es válido lo asignamos, si no, un string seguro
                    completeData[n_completeData] = (res.buffer[j] != nullptr) ? res.buffer[j] : "";
                    Serial.println(completeData[n_completeData]);
                    n_completeData++; 
                }
            }
        } else {
            ESP_LOGW(TAG, "La lectura de la solicitud numero %d ha fallado. Rellenando columnas...", i);
            
            // Obtenemos cuántos títulos/columnas correspondían a esta sección para no romper el CSV
            nameColValues titulosSeccion = _regInterpreter->getLastNameValues(i);
            
            for (int j = 0; j < titulosSeccion.size; j++) {
                if (n_completeData < (MAX_MODBUS_REGS * MAX_MB_REQ_SECCIONS)) {
                    completeData[n_completeData] = "NaN"; // Rellenamos con NaN (Not a Number) para mantener la columna alineada
                    Serial.println(completeData[n_completeData]);
                    n_completeData++;
                }
            }
        }
    }

    // Ahora la validación es segura: si hubo fallos, se habrán rellenado con "NaN"
    if (n_completeData != _datalogger->getLastNumberColumns()) {
        ESP_LOGE(TAG, "Los datos no concuerdan con el numero de columnas. Esperados: %d, Generados: %d", 
                 _datalogger->getLastNumberColumns(), n_completeData); 
        return ESP_FAIL; 
    }

    DateTime now = _rtc->now();
    char bufferTime[20];
    snprintf(bufferTime, sizeof(bufferTime), "%04d-%02d-%02d %02d:%02d:%02d", 
             now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

    esp_err_t err = _datalogger->appendNewDataCSVToLog(bufferTime, completeData, n_completeData);
    if (err != ESP_OK) {
        if (err == ESP_ERR_INVALID_STATE && _datalogger->isFileLimitReached()) {
            return ESP_OK; 
        }
        return err;
    }
    
    return ESP_OK;

    /*
    EM_request req = _regInterpreter->getLastEMRequest(0);

    //ejecutar request modbus
    esp_err_t err = _energyMeter->executeRequest(req);
    DateTime now = _rtc->now();

    if (err != ESP_OK) return err;
    
    //leer datos de energymeter
    rawDataBuffer raw = _energyMeter->readDataBuffer();
    _regInterpreter->getBufferDataRaw(raw.buffer, raw.size, 0);
    netDataString res = _regInterpreter->getBufNetDataString(0);

    // guardar informacion en el datalogger generando mensaje de timestamp

    char bufferTime[20];
    snprintf(bufferTime, sizeof(bufferTime), "%04d-%02d-%02d %02d:%02d:%02d", 
             now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

    err = _datalogger->appendNewDataCSVToLog(bufferTime, res.buffer, res.size);
    if (err != ESP_OK) {
        if (err == ESP_ERR_INVALID_STATE && _datalogger->isFileLimitReached()) {
            return ESP_OK; 
        }
        return err;
    }
    return ESP_OK;
    */
}

    /*
uint16_t n_reqs_aux = _regInterpreter->getCountMBRequests();

    // Definimos un buffer estático grande para acumular toda la fila de datos en texto
    // (Ajusta el tamaño total si estimas que los registros de texto superarán este tamaño)
    static char filaDatosCSV[1024]; 
    filaDatosCSV[0] = '\0'; // Inicializamos vacío
    bool primerDato = true;

    for (int i = 0; i < n_reqs_aux; i++) {
        EM_request req = _regInterpreter->getLastEMRequest(i);
        
        if (_energyMeter->executeRequest(req) == ESP_OK) {
            rawDataBuffer raw = _energyMeter->readDataBuffer();
            _regInterpreter->getBufferDataRaw(raw.buffer, raw.size, i);
            netDataString res = _regInterpreter->getBufNetDataString(i);
            
            // Concatenamos cada valor String del buffer de esta sección
            for (int j = 0; j < res.size; j++) {
                if (!primerDato) {
                    strlcat(filaDatosCSV, ",", sizeof(filaDatosCSV)); // Separador CSV
                }
                
                if (res.buffer[j] != nullptr) {
                    strlcat(filaDatosCSV, res.buffer[j], sizeof(filaDatosCSV));
                } else {
                    strlcat(filaDatosCSV, "0", sizeof(filaDatosCSV)); // En caso de puntero nulo
                }
                primerDato = false;
            }
            
        } else {
            ESP_LOGW(TAG, "La lectura de la solicitud numero %d ha fallado ", i);
            // Opcional: Si falla una sección, puedes rellenar con "N/A" o "0" para mantener las columnas alineadas
            // Ejemplo rápido: rellenar con comas o vacíos basándote en cuántos datos esperabas.
        }
    }

    // Generamos el timestamp
    DateTime now = _rtc->now();
    char bufferTime[20];
    snprintf(bufferTime, sizeof(bufferTime), "%04d-%02d-%02d %02d:%02d:%02d", 
             now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

    // Calculamos la longitud final de los datos construidos
    uint16_t tamanoFila = strlen(filaDatosCSV);

    // Guardamos la información en el datalogger pasando el buffer de texto plano y su tamaño
    esp_err_t err = _datalogger->appendNewDataCSVToLog(bufferTime, filaDatosCSV, tamanoFila);
    if (err != ESP_OK) {
        if (err == ESP_ERR_INVALID_STATE && _datalogger->isFileLimitReached()) {
            return ESP_OK; 
        }
        return err;
    }
    return ESP_OK;
*/