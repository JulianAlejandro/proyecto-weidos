//aqui copiar 

#include "SDManager.h"

#include <SD.h>
#include <SPI.h>

//TODO añadir un _initialize para que sea mas robusta la aplicacion, de momento no esta para no meter mas cosas de las necesarias 

SDManager::SDManager() {
    // Cuerpo vacío
}

//SDManager::SDManager(uint8_t csPin) : _csPin(csPin), _initialized(false) {}

// TODO: pensar que mas cosas tiene que hacer el administrador de SD
bool SDManager::begin() {

    if (!SD.begin()) {
        return false;
    }
    return true; 
    /*if (_initialized) return true;
    _initialized = SD.begin(_csPin);
    return _initialized;*/
    

}


bool SDManager::exists(const char* path) {
    return SD.exists(path);
}

//borrar el contenido dentro de un fichero
void SDManager::clearFile(const char* path) {

    File f = SD.open(path, O_WRITE | O_CREAT | O_TRUNC); 
    if (f) f.close(); 
}

bool SDManager::appendLine(const char* path, const char* data) {
    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;
    f.println(data);
    f.close();
    return true;


    /*
        File myFile = SD.open(_logFile, FILE_WRITE); 

    if (myFile) {
        Serial.println("Escribiendo títulos (vector)...");
        
        for (size_t i = 0; i < titulos.size(); i++) {
            // Creamos una copia para no modificar el vector original
            String temp = titulos[i]; 
            // Reemplazamos cualquier ';' por una ','
            temp.replace(';', ','); 
            
            myFile.print(temp);
            
            if (i < titulos.size() - 1) {
                myFile.print(";");
            }
        }
        
        myFile.println(); 
        myFile.close(); 
        Serial.println("Títulos guardados.");
    } else {
        Serial.println("Error al abrir para títulos.");
    }

    */
}


//TODO funcion especifica orientada a obtener el una fila completa en funcion del primer campo separado por ; que es id  
// TODO: Esta funcion tiene que cambiar, necesito una funcion mas generica que me permita obtener una linea en funcion de cualquier campo, no solo id o pensar alternativa. 
bool SDManager::getLineByID(const char* path, const char* id, char* destBuffer, size_t bufferSize) {
    File file = SD.open(path);
    if (!file) {
        Serial.println(F("Error: No se pudo abrir el archivo"));
        return false;
    }

    // Limpiamos el buffer de destino por seguridad
    memset(destBuffer, 0, bufferSize);

    while (file.available()) {
        // Leemos la línea directamente al buffer de destino para ahorrar RAM
        // Usamos bufferSize - 1 para dejar sitio al carácter nulo final '\0'
        int bytesRead = file.readBytesUntil('\n', destBuffer, bufferSize - 1);
        destBuffer[bytesRead] = '\0'; // Terminamos la cadena manualmente

        // Si la línea está vacía (solo un \r\n), saltamos
        if (bytesRead == 0) continue;

        // Buscamos el primer ';' para aislar el ID de la línea
        char* primerPuntoComa = strchr(destBuffer, ';');
        
        if (primerPuntoComa != nullptr) {
            // Calculamos la longitud del ID encontrado en la línea
            size_t idLength = primerPuntoComa - destBuffer;

            // Comparamos el ID buscado con el ID de la línea
            // strncmp compara solo los caracteres hasta el ';'
            if (strlen(id) == idLength && strncmp(destBuffer, id, idLength) == 0) {
                file.close();
                return true; // ¡Encontrado! El resto de la línea ya está en destBuffer
            }
        }
    }

    file.close();
    return false; // No se encontró el ID
}

void SDManager::getLinesByRange(const char* path, long start, long end, std::vector<String>& result) {
    File file = SD.open(path, FILE_READ);
    if (!file) {
        Serial.println(F("Error: No se pudo abrir archivo para rango"));
        return;
    }

    // Limpiamos el vector de resultados antes de empezar
    result.clear();

    while (file.available()) {
        // Leemos la línea. Usamos readStringUntil provisionalmente 
        // para mantener compatibilidad con tu lógica de vectores.
        String line = file.readStringUntil('\n');
        line.trim();

        if (line.length() > 0) {
            // Buscamos el primer ';' para obtener el ID/Dirección
            int sepIdx = line.indexOf(';');
            if (sepIdx != -1) {
                // Convertimos el primer campo a número
                long idFound = line.substring(0, sepIdx).toInt();

                // Verificamos si está dentro del rango solicitado
                if (idFound >= start && idFound <= end) {
                    result.push_back(line);
                }
            }
        }
    }
    file.close();
}

void SDManager::printFileToSerial(const char* path) {
    File f = SD.open(path, FILE_READ);
    if (!f) {
        Serial.println(F("Error: No se pudo abrir el archivo para lectura Serial."));
        return;
    }
    
    while (f.available()) {
        Serial.write(f.read());
    }
    f.close();
}

/*
File SDManager::openFile(const char* path, const char* mode) {
    return SD.open(path, mode);
}
*/
