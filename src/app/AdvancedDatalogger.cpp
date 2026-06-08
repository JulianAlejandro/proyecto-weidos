

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
    if((n_reqs_aux <= 0) || (n_reqs_aux > MAX_MB_REQ_SECTIONS)) return ESP_FAIL;  

    ESP_LOGD(TAG, "Cargadas en el mapa de registro %d solicitudes modbus", n_reqs_aux);

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

    // Calculamos el total de columnas dinámicas de los mapas Modbus
    uint16_t totalDataCount = _regInterpreter->getCountAllRegsLoaded(); 

    // Inicializamos la sesión en el datalogger añadiendo el +1 para el Timestamp
    esp_err_t err = _datalogger->newCSVLogSesion(nombreFichero, totalDataCount + 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al crear la sesion CSV [0x%X]", err);
        return err;
    }

    // 1. Insertamos de manera obligatoria el primer título de control
    if (!_datalogger->appendTitle("Timestamp")) {
        ESP_LOGE(TAG, "Error al añadir el titulo de control: Timestamp");
        return ESP_FAIL;
    }
    
    // 2. Metemos todos los títulos de las solicitudes Modbus sumadas
    for (int i = 0; i < n_reqs_aux; i++) {
        nameColValues misTitulos = _regInterpreter->getLastNameValues(i); 
        for (int j = 0; j < misTitulos.size; j++) {
            if (!_datalogger->appendTitle(misTitulos.buffer[j])) {
                ESP_LOGE(TAG, "Error al añadir el titulo: %s", misTitulos.buffer[j]);
                return ESP_FAIL;
            } 
        }
    }

    return ESP_OK;
    /*
DateTime ahora = _rtc->now();
    char nombreFichero[16];
    snprintf(nombreFichero, sizeof(nombreFichero), "%02d%02d%02d%02d%02d%02d", 
        ahora.year(), ahora.month(), ahora.day(), ahora.hour(), ahora.minute(), ahora.second());

    uint16_t n_reqs_aux = _regInterpreter->getCountMBRequests();

    // Creamos un contenedor único para albergar todos los títulos consolidados
    //const char* titulosConsolidados[MAX_MODBUS_REGS * MAX_MB_REQ_SECTIONS];

    uint16_t totalTitulos = 0;
    Serial.println("aqui van los titulos: "); 
    for (int i = 0; i < n_reqs_aux; i++) {
        nameColValues misTitulos = _regInterpreter->getLastNameValues(i);         
        totalTitulos = totalTitulos + misTitulos.size;
    }
    esp_err_t err = _datalogger->newCSVLogSesion(nombreFichero, totalTitulos);

    //_datalogger->appendDataToCSVRow("Timestamp"); 
    
    // metemos todos los titulo de todas las solicitudes sumadas
    for(int i = 0; i < n_reqs_aux; i++){
        nameColValues misTitulos = _regInterpreter->getLastNameValues(i); 
        for(int j = 0; j < misTitulos.size; j++){
            if(!_datalogger->appendTitle(misTitulos.buffer[j])){
                ESP_LOGE(TAG,"Error al añadir los titulos"); 
            } 
        }
    }

    return ESP_OK; 
*/

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

    if (_datalogger->isFileLimitReached()) {
        ESP_LOGW(TAG, "Lectura Modbus omitida: Se ha alcanzado el tamaño maximo del archivo de log.");
        return ESP_OK; // Retornamos OK para no colgar el execute() del sistema
    }

    // 1. Capturamos el tiempo real exacto del RTC para este bloque de muestreo
    DateTime now = _rtc->now();
    char bufferTime[20];
    snprintf(bufferTime, sizeof(bufferTime), "%04d-%02d-%02d %02d:%02d:%02d", 
             now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

    // 2. Insertamos el Timestamp como el primer dato de la fila CSV
    if (!_datalogger->appendDataToCSVRow(bufferTime)) {
        ESP_LOGE(TAG, "Error al insertar el Timestamp en la fila actual.");
        return ESP_FAIL;
    }

    uint16_t n_reqs_aux = _regInterpreter->getCountMBRequests();

    // 3. Procesamos de forma secuencial los requests de los medidores de energía
    for (int i = 0; i < n_reqs_aux; i++) {
        EM_request req = _regInterpreter->getLastEMRequest(i);
        
        if (_energyMeter->executeRequest(req) == ESP_OK) {
            rawDataBuffer raw = _energyMeter->readDataBuffer();
            _regInterpreter->getBufferDataRaw(raw.buffer, raw.size, i);
            netDataString res = _regInterpreter->getBufNetDataString(i);
            
            for (int j = 0; j < res.size; j++) {
                if (!_datalogger->appendDataToCSVRow(res.buffer[j])) {
                    ESP_LOGE(TAG, "Error crítico al añadir el dato en la columna. SD llena o buffer corrupto.");
                    //return ESP_FAIL; 
                } 
            }
        } else {
            ESP_LOGW(TAG, "La lectura de la solicitud numero %d ha fallado. Rellenando con NaN...", i);
            
            // Si el dispositivo falla, rescatamos cuántas columnas le correspondían para mantener la integridad del CSV
            nameColValues titulosSeccion = _regInterpreter->getLastNameValues(i);
            
            for (int j = 0; j < titulosSeccion.size; j++) {
                if (!_datalogger->appendDataToCSVRow("NaN")) {
                    ESP_LOGE(TAG, "Error crítico al añadir NaN en la columna vacía.");
                    //return ESP_FAIL;
                } 
            }
        }
    }

    // 4. Verificación de cierre de línea automático
    if (!_datalogger->newRow()) {
        ESP_LOGE(TAG, "La linea no se cerro correctamente. El conteo de columnas no coincide.");
        return ESP_FAIL;
    }

    return ESP_OK;

/*
    uint16_t n_reqs_aux = _regInterpreter->getCountMBRequests();

    //const char* completeData[MAX_MODBUS_REGS * MAX_MB_REQ_SECTIONS];
    //uint16_t n_completeData = 0;
    //Serial.println("aqui van los datos: "); 
    for (int i = 0; i < n_reqs_aux; i++) {
        EM_request req = _regInterpreter->getLastEMRequest(i);
        
        if (_energyMeter->executeRequest(req) == ESP_OK) {
            rawDataBuffer raw = _energyMeter->readDataBuffer();
            _regInterpreter->getBufferDataRaw(raw.buffer, raw.size, i);
            netDataString res = _regInterpreter->getBufNetDataString(i);
            
            for (int j = 0; j < res.size; j++) {
                if(!_datalogger->appendDataToCSVRow(res.buffer[j])){
                    ESP_LOGE(TAG,"Error al añadir los datos"); 
                } 
            }
        } else {
            ESP_LOGW(TAG, "La lectura de la solicitud numero %d ha fallado. Rellenando columnas...", i);
            
            // Obtenemos cuántos títulos/columnas correspondían a esta sección para no romper el CSV
            nameColValues titulosSeccion = _regInterpreter->getLastNameValues(i);
            
            for (int j = 0; j < titulosSeccion.size; j++) {
                if(!_datalogger->appendDataToCSVRow("NaN")){
                    ESP_LOGE(TAG,"Error al añadir los datos"); 
                } 
            }
        }
    }
    */
/*
    // Ahora la validación es segura: si hubo fallos, se habrán rellenado con "NaN"
    if (n_completeData != _datalogger->getLastNumberColumns()) {
        ESP_LOGE(TAG, "Los datos no concuerdan con el numero de columnas. Esperados: %d, Generados: %d", 
                 _datalogger->getLastNumberColumns(), n_completeData); 
        return ESP_FAIL; 
    }*/
/*
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
    */
   // return ESP_OK;

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