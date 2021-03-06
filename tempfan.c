/*******************************************************************/
/* Nombre: tempfan.c                                               */
/* Descripcion: Control automático de ventilador segun temperatura */
/* Autor: JRios                                                    */
/* Fecha: 20/06/2017                                               */
/* Version: 1.0.2                                                  */
/*******************************************************************/

// Librerias
#include <wiringPi.h>
#include <softPwm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Definiciones (modificables a antojo del usuario)
#define PIN_PWM   7                                       // Pin PWM numero 7 (PA6)
#define PWM_MIN   0                                       // Valor PWM minimo
#define PWM_MID   512                                     // Valor PWM intermedio [poner a 750 para Raspberry Pi]
#define PWM_MAX 1023                                      // Valor PWM maximo
#define DIFF_TEMP 2                                       // Diferencia de temperatura minima entre lecturas (2ºC)
#define TEMP_LOW  55                                      // Valor umbral de temperatura bajo (55ºC) [Poner a 50 para Raspberry Pi]
#define TEMP_HIGH 68                                      // Valor umbral de temperatura alto (68ºC) [Poner a 60/65 para Raspberry Pi]
#define T_READS   5                                       // Tiempo de espera entre lecturas de temperatura (5s)
#define FILE_TEMP "/sys/class/thermal/thermal_zone0/temp" // Ruta y nombre del archivo de acceso a la temperatura

/*******************************************************************/

// Tipo de dato para determinar los estados correspondiente a la velocidad del ventilador (0%, 50% y 100%)
typedef enum {OFF, SLOW, FAST} T_state;

// Prototipo de funcion de lectura de temperatura
int readTemp(const char* file_path);

/*******************************************************************/

// Funcion principal
int main(void)
{
	int temp_last, temp_now, diff_temp;
	T_state state_last, state_now;
	
	wiringPiSetup();
	softPwmCreate(PIN_PWM, PWM_MIN, PWM_MAX);
	
	softPwmWrite(PIN_PWM, PWM_MIN); // Ventilador apagado
	state_now = OFF;
	state_last = OFF;
	
	temp_now = readTemp((char*)FILE_TEMP); // Leer temperatura
	if((temp_now < TEMP_LOW) && (state_last != OFF)) // Temperatura menor que TEMP_LOW (55ºC) y estado anterior distinto
	{
		softPwmWrite(PIN_PWM, PWM_MIN); // Ventilador apagado
		state_now = OFF; // Estado actual: Ventilador apagado
	}
	else if(((temp_now >= TEMP_LOW) && (temp_now <= TEMP_HIGH)) && (state_last != SLOW)) // Temperatura entre TEMP_LOW y TEMP_HIGH (55ºC - 70ºC) y estado anterior distinto
	{
		softPwmWrite(PIN_PWM, PWM_MID); // Ventilador al 50%
		state_now = SLOW; // Estado actual: Ventilador al 50%
	}
	else if((temp_now > TEMP_HIGH) && (state_last != FAST)) // Temperatura mayor que TEMP_HIGH (70ºC) y estado anterior distinto
	{
		softPwmWrite(PIN_PWM, PWM_MAX); // Ventilador al 100%
		state_now = FAST; // Estado actual: Ventilador al 100%
	}
	temp_last = temp_now; // Guardar la ultima lectura de temperatura (cuando se ha configurado el ventilador)
	state_last = state_now;
 
	while(1)
	{
		temp_now = readTemp((char*)FILE_TEMP); // Leer temperatura
		diff_temp = abs(temp_now - temp_last); // Calcular diferencia de temperatura
		
		if(diff_temp >= DIFF_TEMP) // Diferencia de temperatura mayor que DIFF_TEMP (2ºC)
		{
			if((temp_now < TEMP_LOW) && (state_last != OFF)) // Temperatura menor que TEMP_LOW (55ºC) y estado anterior distinto
			{
				softPwmWrite(PIN_PWM, PWM_MIN); // Ventilador apagado
				state_now = OFF; // Estado actual: Ventilador apagado
			}
			else if(((temp_now >= TEMP_LOW) && (temp_now <= TEMP_HIGH)) && (state_last != SLOW)) // Temperatura entre TEMP_LOW y TEMP_HIGH (55ºC - 70ºC) y estado anterior distinto
			{
				softPwmWrite(PIN_PWM, PWM_MID); // Ventilador al 50%
				state_now = SLOW; // Estado actual: Ventilador al 50%
			}
			else if((temp_now > TEMP_HIGH) && (state_last != FAST)) // Temperatura mayor que TEMP_HIGH (70ºC) y estado anterior distinto
			{
				softPwmWrite(PIN_PWM, PWM_MAX); // Ventilador al 100%
				state_now = FAST; // Estado actual: Ventilador al 100%
			}
			
			if(state_now != state_last) // Estado actual distinto al anterior (ha cambiado de estado)
			{
				temp_last = temp_now; // Guardar la ultima lectura de temperatura (cuando se ha configurado el ventilador)
				state_last = state_now; // Actualiza la variable de estado anterior
			}
		}
		
		sleep(T_READS); // Esperamos, liberando a la CPU del proceso actual durante T_READS (5s)
    }
	
    return 0;
}

/*******************************************************************/

// Funcion de lectura de la temperatura
int readTemp(const char* file_path)
{
	FILE* file; // Puntero a tipo archivo (para manejar el archivo)
	int temp = 0; // Variable donde almacenar la temperatura leida y para devolverla al salir de la funcion

	file = fopen(file_path, "r"); // Abrimos el archivo
	if(file) // Comprobamos que la apertura ha sido exitosa (si no, no hace nada)
	{
		fscanf(file, "%d", &temp); // Almacenamos en la variable "temp" el primer valor entero (int) que haya en el archivo
		fclose(file); // Cerramos el archivo
	}

	// Si la temperatura tiene 4 o más digitos, dividimos entre 1000 (por ejemplo, en OPi Zero, se leen esos valores)
	if(temp >= 1000)
		temp = temp/1000;
	
	return temp; // Devolvemos el valor de temperatura leida
}
