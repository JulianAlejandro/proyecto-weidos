#include "Datalogger.h"

Datalogger::Datalogger(SDManager* sdManager) {
    _sd = sdManager;
    _logFile = "/tabla.txt"; // Valor por defecto
}

bool Datalogger::begin() {

    // No llamamos a _sd->begin() aquí, solo preguntamos si ya funciona
    if (!_sd->isReady()) { 
        Serial.println("Datalogger: SDManager no está listo aún.");
        return false;
    }
    return true;

    /*
    if (!_sd->begin()) {
        Serial.println(F("Error: SD no detectada"));
        return false;
    }
    return true;
    */
}

void Datalogger::setLogFile(const char* filename) {
    _logFile = filename;
}

bool Datalogger::writeHeader(const char** titulos, uint16_t numTitulos) {
    if (numTitulos == 0) return false;

    String lineaCompleta = "";
    lineaCompleta.reserve(128); // Reservamos espacio una sola vez para evitar fragmentar

    for (uint16_t i = 0; i < numTitulos; i++) {
        lineaCompleta += titulos[i];
        if (i < numTitulos - 1) lineaCompleta += ";";
    }

    return _sd->appendLine(_logFile.c_str(), lineaCompleta.c_str());
}

bool Datalogger::writeRow(const uint32_t timestamp, const float* values, uint16_t numValues) {
    if (numValues == 0) return false;

    // Construimos la línea de datos
    // Usamos String de forma controlada con reserve
    String lineaData = "";
    lineaData.reserve(128); 

    // Añadimos el timestamp al principio
    lineaData += String(timestamp);
    lineaData += ";";

    for (uint16_t i = 0; i < numValues; i++) {
        // Añadimos el float con 2 decimales
        lineaData += String(values[i], 2);
        
        if (i < numValues - 1) {
            lineaData += ";";
        }
    }

    return _sd->appendLine(_logFile.c_str(), lineaData.c_str());
}

void Datalogger::clearLogFile() {
    // Implementar en SDManager un método que borre o sobreescriba
    //_sd->remove(_logFile.c_str());
    _sd->clearFile(_logFile.c_str());
}


/*
#include "Datalogger.h"


// En EM750_Datalogger.cpp
Datalogger::Datalogger(SDManager* sdManager) {
    _sd = sdManager; // Guardamos la dirección de nuestro administrador
}

// Inicializa la tarjeta SD y verifica si el archivo de configuración existe
//TODO: Añadir codigo para analizar si existen los ficheros o no etc
bool Datalogger::begin() {
    // 1. Intentamos inicializar la SD a través del manager
    if (!_sd->begin()) {
        Serial.println(F("Error: No se pudo iniciar la SD desde Datalogger"));
        return false;
    }


    return true;
}
*/
/*
  // Escribe la fila de encabezado en el archivo de log limpiando caracteres conflictivos
void EM750_Datalogger::writeHeader(const std::vector<String>& titulos) {
    // 1. Construimos la línea completa en un String temporal
    // Esto es necesario porque appendLine cierra el archivo al terminar
    String lineaCompleta = "";

    for (size_t i = 0; i < titulos.size(); i++) {
        String temp = titulos[i];
        
        // Limpieza: Evitar que un ';' rompa el formato CSV
        temp.replace(';', ','); 
        
        lineaCompleta += temp;
        
        // Añadir separador si no es el último elemento
        if (i < titulos.size() - 1) {
            lineaCompleta += ";";
        }
    }

    // 2. Usamos el SDManager para guardar la línea completa
    if (_sd->appendLine(_logFile.c_str(), lineaCompleta.c_str())) {
        Serial.println(F("Títulos guardados correctamente via SDManager."));
    } else {
        Serial.println(F("Error: SDManager no pudo escribir los títulos."));
    }
}
*/

/*
// Añade una nueva fila de datos al archivo de log (formato CSV)
void EM750_Datalogger::writeRow(const std::vector<String>& datos) {
    // 1. Construimos la línea completa en una variable temporal
    String lineaAscribir = "";

    for (size_t i = 0; i < datos.size(); i++) {
        // Limpieza: Reemplazamos ';' por ',' para no romper el formato CSV
        String temp = datos[i];
        temp.replace(';', ',');
        
        lineaAscribir += temp;

        // Añadimos el separador ';' si no es el último elemento
        if (i < datos.size() - 1) {
            lineaAscribir += ";";
        }
    }

    // 2. Delegamos la escritura al Manager
    // Usamos c_str() para pasar de String a const char* que es lo que pide el Manager
    if (!_sd->appendLine(_logFile.c_str(), lineaAscribir.c_str())) {
        Serial.println(F("Error: No se pudo escribir la fila en el Log via SDManager."));
    }
}
   */ 

    // Lee todo el contenido del archivo de log y lo vuelca por el puerto serie
void Datalogger::printLogToSerial() {
    Serial.print(F("--- Contenido del Log: "));
    Serial.print(_logFile);
    Serial.println(F(" ---"));

    _sd->printFileToSerial(_logFile.c_str());
}


/*
#include "EM_750_Datalogger.h"


// En EM750_Datalogger.cpp
EM750_Datalogger::EM750_Datalogger(SDManager* sdManager, const String& setupFile, const String& logFile) {
    _sd = sdManager; // Guardamos la dirección de nuestro administrador
    _setupFile = setupFile;
    _logFile = logFile;
}


// el antiguo sin referencia a administrador de SD 
// Constructor: Inicializa las rutas de los archivos de configuración y de registro
EM750_Datalogger::EM750_Datalogger(const String& setupFile, const String& logFile) {
    _setupFile = setupFile;
    _logFile = logFile;
    //_csPin = csPin;
}



// Inicializa la tarjeta SD y verifica si el archivo de configuración existe
//TODO: Añadir codigo para analizar si existen los ficheros o no etc
bool EM750_Datalogger::begin() {
    // 1. Intentamos inicializar la SD a través del manager
    if (!_sd->begin()) {
        Serial.println(F("Error: No se pudo iniciar la SD desde Datalogger"));
        return false;
    }

    // 2. Verificamos si el archivo de configuración existe usando el manager
    if (!_sd->exists(_setupFile.c_str())) {
        Serial.print(F("Warning: Setup file not found: "));
        Serial.println(_setupFile);
        // Aquí podrías llamar a _sd->appendLine para crear un archivo base si quisieras
    }
   
    return true;
}



// Borra el contenido del archivo de log creando uno nuevo vacío en el fichero de log. 
void EM750_Datalogger::clearLogFile() {
    // Usamos el puntero _sd para llamar a la función del administrador
    // Pasamos _logFile.c_str() porque el manager espera un const char*
    _sd->clearFile(_logFile.c_str());
    
    Serial.println("Fichero de log vaciado a través de SDManager.");
}


// Busca un registro por dirección y llena la estructura 'resultado' con sus campos
bool EM750_Datalogger::getRegDataByAddr(long addr, reg_EM_750 &resultado){
      // 1. Buscamos la línea completa en el archivo
    String linea = getRowStringByAddress(addr);

    // 2. Si la línea está vacía, es que no se encontró la dirección
    if (linea == "") {
        return false; 
    }

    // 3. Procesamos la línea encontrada para llenar el array resultado
    // Usamos ';' como separador según tu lógica previa
    return splitString(linea, ';', resultado);
}

// Divide una cadena de texto en columnas respetando las comillas dobles
bool EM750_Datalogger::splitString(const String& linea, char div_char, reg_EM_750 &resultado){
// 1. Limpieza preventiva del struct
    for (int i = 0; i < NUM_COL_REG_EM750; i++) {
        resultado.data[i] = "";
    }

    int posInicio = 0;
    int col = 0;
    bool dentroDeComillas = false;
    int n = linea.length();

    // 2. Recorremos la línea carácter por carácter
    for (int i = 0; i <= n; i++) {
        char c = (i < n) ? linea[i] : '\0'; // Carácter actual o fin de cadena

        // Detectar si entramos o salimos de una zona de comillas
        if (c == '"') {
            dentroDeComillas = !dentroDeComillas;
        }

        // Si encontramos el divisor (y no estamos en comillas) O llegamos al final de la cadena
        if ((c == div_char && !dentroDeComillas) || i == n) {
            if (col < NUM_COL_REG_EM750) {
                String segmento = linea.substring(posInicio, i);
                segmento.trim();
                
                // Opcional: Eliminar las comillas exteriores si existen en el segmento
                if (segmento.startsWith("\"") && segmento.endsWith("\"")) {
                    segmento = segmento.substring(1, segmento.length() - 1);
                }
                
                resultado.data[col] = segmento;
                col++;
            }
            posInicio = i + 1;
        }
    }

    // Retorna true solo si llenamos exactamente las columnas definidas
    return (col == NUM_COL_REG_EM750);

}

// Escanea el archivo de configuración para encontrar la línea que coincide con la dirección
String EM750_Datalogger::getRowStringByAddress(long addr) {
    // 1. Creamos un buffer para recibir la línea (ajusta el tamaño según tus necesidades)
    char bufferLinea[128]; 
    
    // 2. Convertimos el long 'addr' a una cadena de texto para buscarlo como ID
    char idABuscar[16];
    ltoa(addr, idABuscar, 10); // Función eficiente de C para convertir long a array de char

    // 3. Llamamos al SDManager usando el puntero _sd
    // getLineByID se encarga de abrir el archivo, buscar y cerrar.
    if (_sd->getLineByID(_setupFile.c_str(), idABuscar, bufferLinea, sizeof(bufferLinea))) {
        // Si lo encuentra, devolvemos el contenido del buffer como String
        return String(bufferLinea);
    }

    // 4. Si no lo encuentra o hubo error, devolvemos String vacío
    return "";
}


// Obtiene una lista de estructuras de registros dentro de un rango de direcciones
std::vector<reg_EM_750> EM750_Datalogger::getRegsByRange(long addrStart, long addrEnd){
    std::vector<reg_EM_750> listaResultados;

    if(addrStart > addrEnd){ 
        Serial.println("Error: Rango de direcciones invalido.");
        return listaResultados;
    }
    
    // 1. Obtenemos las líneas crudas (Strings con puntos y coma)
    std::vector<String> lineasCrodas = getRowsStringByAddressRange(addrStart, addrEnd);

    // 2. Iteramos por cada línea para convertirla en un array estructurado
    for (const String& linea : lineasCrodas) {
        reg_EM_750 registroTemporal;
        
        // Usamos tu función splitString pasándole el array interno de la estructura
        if (splitString(linea, ';', registroTemporal)) {
            listaResultados.push_back(registroTemporal);
        }
    }

    return listaResultados;

}


  // Escribe la fila de encabezado en el archivo de log limpiando caracteres conflictivos
void EM750_Datalogger::writeHeader(const std::vector<String>& titulos) {
    // 1. Construimos la línea completa en un String temporal
    // Esto es necesario porque appendLine cierra el archivo al terminar
    String lineaCompleta = "";

    for (size_t i = 0; i < titulos.size(); i++) {
        String temp = titulos[i];
        
        // Limpieza: Evitar que un ';' rompa el formato CSV
        temp.replace(';', ','); 
        
        lineaCompleta += temp;
        
        // Añadir separador si no es el último elemento
        if (i < titulos.size() - 1) {
            lineaCompleta += ";";
        }
    }

    // 2. Usamos el SDManager para guardar la línea completa
    if (_sd->appendLine(_logFile.c_str(), lineaCompleta.c_str())) {
        Serial.println(F("Títulos guardados correctamente via SDManager."));
    } else {
        Serial.println(F("Error: SDManager no pudo escribir los títulos."));
    }
}


// Añade una nueva fila de datos al archivo de log (formato CSV)
void EM750_Datalogger::writeRow(const std::vector<String>& datos) {
    // 1. Construimos la línea completa en una variable temporal
    String lineaAscribir = "";

    for (size_t i = 0; i < datos.size(); i++) {
        // Limpieza: Reemplazamos ';' por ',' para no romper el formato CSV
        String temp = datos[i];
        temp.replace(';', ',');
        
        lineaAscribir += temp;

        // Añadimos el separador ';' si no es el último elemento
        if (i < datos.size() - 1) {
            lineaAscribir += ";";
        }
    }

    // 2. Delegamos la escritura al Manager
    // Usamos c_str() para pasar de String a const char* que es lo que pide el Manager
    if (!_sd->appendLine(_logFile.c_str(), lineaAscribir.c_str())) {
        Serial.println(F("Error: No se pudo escribir la fila en el Log via SDManager."));
    }
}
    

    // Lee todo el contenido del archivo de log y lo vuelca por el puerto serie
void EM750_Datalogger::printLogToSerial() {
    Serial.print(F("--- Contenido del Log: "));
    Serial.print(_logFile);
    Serial.println(F(" ---"));

    _sd->printFileToSerial(_logFile.c_str());
}

std::vector<String> EM750_Datalogger::getRowsStringByAddressRange(long addrStart, long addrFin) {
    std::vector<String> resultados;
    
    // El Datalogger le pide al Manager que rellene el vector
    //_sd->getLinesByRange(_setupFile.c_str(), addrStart, addrFin, resultados);
    
    return resultados;
}

 RegRequest EM750_Datalogger::RegRequestParamsFromRangeAddr(long addrStart, long addrEnd){
       RegRequest resp; 
       int data_size = 0; 
        registros = getRegsByRange(19000, 19020);
        
        for (int i = 0; i < registros.size(); i++){
            data_size = data_size + EM750_Datalogger::getFormatSize(registros[i].data[FORMAT]);
        }

        resp.baseAddress = registros[0].data[ADDR].toInt(); 
        resp.totalSize = data_size; 

        return resp; 
 }


 std::vector<String> EM750_Datalogger::obtener_fila (std::vector<long> data, int base_addr){

    //TODO
    int idx = 0;
    std::vector<String> filaParaSD;

    for(int i = 0; i < registros.size(); i++){
      idx = registros[i].data[ADDR].toInt() - base_addr;
      uint32_t combinado = 0; 
      float resultado;

      if(registros[i].data[FORMAT] == "FLOAT"){
        combinado = ((uint32_t)data[idx] << 16) | data[idx+1];
        memcpy(&resultado, &combinado, sizeof(resultado));
        filaParaSD.push_back(String(resultado));
      }
    }
    return filaParaSD; 
 }

*/