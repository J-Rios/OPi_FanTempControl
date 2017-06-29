/*******************************************************************/
/* Nombre: tempfanadv.c  (Temperature-Fan-Advance)                 */
/* Descripcion: Control automatico de ventilador segun temperatura */
/*              con registro periodico del estado del programa y   */
/*              cambios de modos del ventilador mediante Log       */
/* Autor: JRios                                                    */
/* Fecha: 23/06/2017                                               */
/* Version: 1.0.0                                                  */
/*******************************************************************/

// Librerias
#include <wiringPi.h>
#include <softPwm.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <time.h>

// Definiciones (modificables a antojo del usuario)
#define PIN_PWM   7                                       // Pin PWM numero 7 (PA6)
#define DIFF_TEMP 2                                       // Diferencia de temperatura minima entre lecturas (2ºC)
#define TEMP_LOW  55                                      // Valor umbral de temperatura bajo (55ºC)
#define TEMP_HIGH 68                                      // Valor umbral de temperatura alto (68ºC)
#define T_READS   5                                       // Tiempo de espera entre lecturas de temperatura (5s)
#define T_ALIVE   600                                     // Tiempo de espera entre escritura en el archivo Log, para determinar que el programa esta en ejecucion (600s -> 10m)
#define FILE_TEMP "/sys/class/thermal/thermal_zone0/temp" // Ruta y nombre del archivo de acceso a la temperatura
#define FILE_LOG  "/var/log/tempfan.log"                  // Ruta y nombre de archivo Log
#define LINE_SIZE 128                                     // Tamaño máximo de linea que puede ser escrita en el archivo Log
#define MAX_LINES 1000                                    // Numero maximo de lineas que puede contener el archivo Log (1000 lineas)

/*******************************************************************/

// Tipo de dato para determinar los estados correspondiente a la velocidad del ventilador (0%, 50% y 100%)
typedef enum {OFF, SLOW, FAST} T_state;

// Prototipo de funciones
int readTemp(const char* file_path);                      // Funcion de lectura de la temperatura
void logPrintln(const char* file_path, char* data);       // Funcion para escribir una linea en el archivo Log
void logRemoveln(const char* file_path, int line_delete); // Funcion para eliminar una linea del archivo Log
int logLinesnum(const char* file_path);                   // Funcion para consultar el numero de lineas totales en el archivo log

/*******************************************************************/

// Funcion principal
int main(void)
{
    int temp_last, temp_now, diff_temp; // Variables de temperatura
    T_state state_last, state_now; // Variables de estado anterior y actual
    char log_writer[LINE_SIZE] = ""; // Cadena de caracteres para almacenar el mensaje de texto a escribir en el archivo Log
    time_t lastTime = time(NULL); // Valor inicial de tiempo
    
    logPrintln((char*)FILE_LOG, (char*)"Arrancando tempfan...\n"); // Escribimos en el archivo Log
    
    wiringPiSetup(); // Inicializamos la libreria WiringPi
    softPwmCreate(PIN_PWM, 0, 1023); // Configuramos el pin "PIN_PWM" como salida PWM en el rango 0 - 1023
    
    logPrintln((char*)FILE_LOG, (char*)"GPIO configurado\n"); // Escribimos en el archivo Log
    
    softPwmWrite(PIN_PWM, 0); // Ventilador apagado
    state_now = OFF; // Inicializamos el estado actual en OFF
    state_last = OFF; // Inicializamos el ultimo estado en OFF
    
    sprintf(log_writer, "Inicio completado, limites de temperatura:\n - Bajo (menos de %dºC)\n - Intermedio (entre %dºC y %dºC)\n - Alto (más de %dºC)\n", TEMP_LOW, TEMP_LOW, TEMP_HIGH, TEMP_HIGH); // Preparamos lo que se va a escribir en el archivo Log
    logPrintln((char*)FILE_LOG, (char*)log_writer); // Escribimos en el archivo Log
    
    temp_now = readTemp((char*)FILE_TEMP); // Leer temperatura
    if((temp_now >= TEMP_LOW) && (temp_now <= TEMP_HIGH)) // Temperatura entre TEMP_LOW y TEMP_HIGH (55ºC - 70ºC) y estado anterior distinto
    {
        softPwmWrite(PIN_PWM, 512); // Ventilador al 50%
        state_now = SLOW; // Estado actual: Ventilador al 50%
        sprintf(log_writer, "Temperatura actual intermedia (%dºC), ventilador al 50%\n", temp_now); // Preparamos lo que se va a escribir en el archivo Log
        logPrintln((char*)FILE_LOG, (char*)log_writer); // Escribimos en el archivo Log
    }
    else if(temp_now > TEMP_HIGH) // Temperatura mayor que TEMP_HIGH (70ºC) y estado anterior distinto
    {
        softPwmWrite(PIN_PWM, 1023); // Ventilador al 100%
        state_now = FAST; // Estado actual: Ventilador al 100%
        sprintf(log_writer, "Temperatura actual alta (%dºC), ventilador al 100%\n", temp_now); // Preparamos lo que se va a escribir en el archivo Log
        logPrintln((char*)FILE_LOG, (char*)log_writer); // Escribimos en el archivo Log
    }
    temp_last = temp_now; // Guardar la ultima lectura de temperatura (cuando se ha configurado el ventilador)
    state_last = state_now; // Actualizamos el ultimo estado al valor de estado actual
    
    while(1)
    {
        temp_now = readTemp((char*)FILE_TEMP); // Leer temperatura
        diff_temp = abs(temp_now - temp_last); // Calcular diferencia de temperatura
        
        if(diff_temp >= DIFF_TEMP) // Diferencia de temperatura mayor que DIFF_TEMP (2ºC)
        {
            if((temp_now < TEMP_LOW) && (state_last != OFF)) // Temperatura menor que TEMP_LOW (55ºC) y estado anterior distinto
            {
                softPwmWrite(PIN_PWM, 0); // Ventilador apagado
                state_now = OFF; // Estado actual: Ventilador apagado
                sprintf(log_writer, "Temperatura actual baja (%dºC), ventilador apagado\n", temp_now); // Preparamos lo que se va a escribir en el archivo Log
                logPrintln((char*)FILE_LOG, (char*)log_writer); // Escribimos en el archivo Log
            }
            else if(((temp_now >= TEMP_LOW) && (temp_now <= TEMP_HIGH)) && (state_last != SLOW)) // Temperatura entre TEMP_LOW y TEMP_HIGH (55ºC - 70ºC) y estado anterior distinto
            {
                softPwmWrite(PIN_PWM, 512); // Ventilador al 50%
                state_now = SLOW; // Estado actual: Ventilador al 50%
                sprintf(log_writer, "Temperatura actual intermedia (%dºC), ventilador al 50%\n", temp_now); // Preparamos lo que se va a escribir en el archivo Log
                logPrintln((char*)FILE_LOG, (char*)log_writer); // Escribimos en el archivo Log
            }
            else if((temp_now > TEMP_HIGH) && (state_last != FAST)) // Temperatura mayor que TEMP_HIGH (70ºC) y estado anterior distinto
            {
                softPwmWrite(PIN_PWM, 1023); // Ventilador al 100%
                state_now = FAST; // Estado actual: Ventilador al 100%
                sprintf(log_writer, "Temperatura actual alta (%dºC), ventilador al 100%\n", temp_now); // Preparamos lo que se va a escribir en el archivo Log
                logPrintln((char*)FILE_LOG, (char*)log_writer); // Escribimos en el archivo Log
            }
            
            if(state_now != state_last) // Estado actual distinto al anterior (ha cambiado de estado)
            {
                temp_last = temp_now; // Guardar la ultima lectura de temperatura (cuando se ha configurado el ventilador)
                state_last = state_now; // Actualiza la variable de estado anterior
            }
        }
        
        if((time(NULL) - lastTime) >= T_ALIVE) // Si ha transcurrido T_ALIVE (1min) o más, escribimos en el Log un mensaje 
        {
            logPrintln((char*)FILE_LOG, (char*)"Estado del programa: Activo y funcionando desde hace 10 minutos\n"); // Escribimos en el archivo Log
            lastTime = time(NULL);
        }
        while(logLinesnum((char*)FILE_LOG) > MAX_LINES) // Mientras haya mas lineas en el archivo de Log que el maximo permitido
            logRemoveln((char*)FILE_LOG, 1); // Se elimina la primera linea del archivo
        
        sleep(T_READS); // Esperamos, liberando a la CPU del proceso actual durante T_READS (5s)
    }
    
    return 0;
}

/*******************************************************************/

// Funcion de lectura de la temperatura
int readTemp(const char* file_path)
{
    FILE* file;   // Puntero a tipo archivo (para manejar el archivo)
    int temp = 0; // Variable donde almacenar la temperatura leida y para devolverla al salir de la funcion
    
    if((file = fopen(file_path, "r")) != NULL) // Abrimos el archivo para leer
    {
        fscanf(file, "%d", &temp); // Almacenamos en la variable "temp" el primer valor entero (int) que haya en el archivo
        fclose(file); // Cerramos el archivo
    }
    
    return temp; // Devolvemos el valor de temperatura leida
}

/*******************************************************************/

// Funcion para escribir una linea en el archivo Log
void logPrintln(const char* file_path, char* data)
{
    FILE* file;                     // Puntero a tipo archivo (para manejar el archivo)
    time_t t_date = time(NULL);     // Variable de tipo fecha
    struct tm* tm_date;             // Estructura de datos de fecha
    char date[32] = "";             // Cadena de caracteres para almacenar la fecha en el formato que nos interesa
    char data_line[LINE_SIZE] = ""; // Cadena de caracteres para almacenar la linea de texto completa a escribir (fecha + datos)
    
    // Obtenemos la fecha de sistema para adjuntarla a los datos a escribir en la linea
    time(&t_date);
    tm_date = localtime(&t_date);
    sprintf(date, "[%d/%d/%d-%d:%d:%d] ", tm_date->tm_mday, tm_date->tm_mon+1, tm_date->tm_year+1900, tm_date->tm_hour, tm_date->tm_min, tm_date->tm_sec);
    
    // Preparamos la linea a escribir
    strcat(data_line, date); // Añadimos a "data_line" el contnido de "date"
    strcat(data_line, data); // Añadimos a "data_line" el contenido de "data"
    
    if((file = fopen(file_path, "a")) != NULL) // Abrimos el archivo para añadir datos
    {
        fputs(data_line, file); // Escribimos los datos en el archivo
        fclose(file); // Cerramos el archivo
    }
}

// Funcion para eliminar una linea del archivo Log
void logRemoveln(const char* file_path, int line_delete)
{
    FILE *file, *file_temp;          // Punteros a tipo archivo (para manejar los archivos)
    char tempFile[32] = "";          // Ruta y nombre del archivo temporal
    char line_read[LINE_SIZE] = "";  // Array de caracteres para almacenar cada lectura de linea (tamaño maximo de linea, 128 caracteres)
    int  num_line = 1;               // Variable para llevar la cuenta de las lineas leidas
    int  open_tempFile = 0;          // Variable para determinar si se ha podido crear el archivo temporal

    strcat(tempFile, file_path); // Añade al primer array el contenido del segundo ("/tmp/" + nombre del archivo original)
    strcat(tempFile, ".tmp");    // Añade la extension .tmp al nombre del archivo temporal
    
    if((file = fopen(file_path, "r")) != NULL) // Abrir el archivo en modo lectura
    {
        if((file_temp = fopen(tempFile, "w")) != NULL) // Abrir un archivo temporal en modo escritura
        {
            open_tempFile = 1; // Archivo temporal abierto correctamente
            while(fscanf(file, "%[^\n]\n", line_read) != EOF) // Leemos cada linea hasta el final de fichero
            {
                if(num_line != line_delete) // Si la linea de lectura no es la linea a eliminar
                {
                    strcat(line_read, "\n"); // Añadimos el caracter de fin de linea (que no es leido con fs
                    fputs(line_read, file_temp); // Escribimos dicha linea en el archivo de escritura
                }
                num_line = num_line + 1; // Incrementamos el numero de linea leida
            }
            
            fclose(file_temp); // Cerramos el archivo de escritura temporal
        }
        fclose(file); // Cerramos el archivo original de lectura
    }
    
    if(open_tempFile) // Si se consiguio crear el archivo temporal
    {
        remove(file_path); // Borramos el archivo original leido
        rename(tempFile, file_path); // Renombramos el archivo temporal con el nombre original
    }
}

// Funcion para consultar el numero de lineas totales en el archivo log
int logLinesnum(const char* file_path)
{
    FILE* file;                // Puntero a tipo archivo (para manejar el archivo)
    int num_lines = 0;         // Variable para contar el numero de lineas y para devolverla al salir de la funcion
    char line[LINE_SIZE] = ""; // Array de caracteres para almacenar cada lectura de linea (tamaño maximo de linea, 128 caracteres)
    
    if((file = fopen(file_path, "r")) != NULL) // Abrir el archivo en modo lectura
    {
        while(fscanf(file, "%[^\n]\n", line) != EOF) // Leemos cada linea hasta el final de fichero
            num_lines = num_lines + 1; // Incrementamos el numero de lineas
        
        fclose(file); // Cerramos el archivo
    }
    
    return num_lines; // Devolvemos el numero de lineas contadas, que tiene el archivo
}