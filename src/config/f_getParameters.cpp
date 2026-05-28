#include "f_getParameters.h"
#include <CSV_Parser.h>
#include "global_types.h"
#include <string>

#define PARAM_FILE "/EM750map.csv"
#define LINE_MAP_START 4

Parameters SDgetParameters(SDManager* _sd) { 
    Parameters res; 
    
    // Inicializar strings vacíos de forma segura
    res.log_interval[0] = '\0'; 
    res.new_file[0] = '\0'; 
    res.max_files[0] = '\0';

    if(!_sd->isReady()){return res;}

    CSV_Parser cp("sssss", true, ';');

    _sd->withFile(PARAM_FILE, [](Stream& file, void* arg) {
        CSV_Parser* parser = (CSV_Parser*)arg;
        
        for (int i = 0; i < LINE_MAP_START; i++) {
            if (file.available()) {
                String line = file.readStringUntil('\n');
                if (line.length() > 0) {
                    line += "\n";
                    *parser << line.c_str();
                }
            }
        }
    }, &cp);

    char **values = (char**)cp[(int)1]; // Acceso a la segunda columna

    if (values != nullptr && cp.getRowsCount() >= 3) {
        // Ahora strncpy sí tiene un búfer real y seguro donde escribir
        if (values[0]) {
            strncpy(res.log_interval, values[0], MAX_TEXT_SIZE - 1);
            res.log_interval[MAX_TEXT_SIZE - 1] = '\0';
        }
        if (values[1]) {
            strncpy(res.new_file, values[1], MAX_TEXT_SIZE - 1);
            res.new_file[MAX_TEXT_SIZE - 1] = '\0';
        }
        if (values[2]) {
            strncpy(res.max_files, values[2], MAX_TEXT_SIZE - 1);
            res.max_files[MAX_TEXT_SIZE - 1] = '\0';
        }
    }
    return res; 
}


