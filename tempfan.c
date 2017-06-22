/*******************************************************************/
/* Nombre: tempfan.c                                               */
/* Descripcion: Control automático de ventilador segun temperatura */
/* Autor: JRios                                                    */
/* Fecha: 20/06/2017                                               */
/* Version: 1.0.0                                                  */
/*******************************************************************/

// Librerias
#include <wiringPi.h>
#include <softPwm.h>
#include <stdio.h>
#include <unistd.h>

// Las siguientes definiciones pueden modificarse a antojo del usuario (siempre y cuando se tengan en cuenta el efecto de los cambios)
#define PIN_PWM   7   // Pin PWM numero 7 (PA6)
#define DIFF_TEMP 2   // Diferencia de temperatura minima entre lecturas (2ºC)
#define TEMP_LOW  55  // Valor umbral de temperatura bajo (55ºC)
#define TEMP_HIGH 68  // Valor umbral de temperatura alto (68ºC)
#define T_READS   5   // Tiempo de espera entre lecturas de temperatura (5s)

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
	softPwmCreate(PIN_PWM, 0, 1023);
	
	softPwmWrite(PIN_PWM, 0); // Ventilador apagado
	state_now = OFF;
	state_last = OFF;
	
	temp_now = readTemp((char*)"/sys/class/thermal/thermal_zone1/temp"); // Leer temperatura
	if((temp_now < TEMP_LOW) && (state_last != OFF)) // Temperatura menor que TEMP_LOW (55ºC) y estado anterior distinto
	{
		softPwmWrite(PIN_PWM, 0); // Ventilador apagado
		state_now = OFF; // Estado actual: Ventilador apagado
	}
	else if(((temp_now >= TEMP_LOW) && (temp_now <= TEMP_HIGH)) && (state_last != SLOW)) // Temperatura entre TEMP_LOW y TEMP_HIGH (55ºC - 70ºC) y estado anterior distinto
	{
		softPwmWrite(PIN_PWM, 512); // Ventilador al 50%
		state_now = SLOW; // Estado actual: Ventilador al 50%
	}
	else if((temp_now > TEMP_HIGH) && (state_last != FAST)) // Temperatura mayor que TEMP_HIGH (70ºC) y estado anterior distinto
	{
		softPwmWrite(PIN_PWM, 1023); // Ventilador al 100%
		state_now = FAST; // Estado actual: Ventilador al 100%
	}
	temp_last = temp_now; // Guardar la ultima lectura de temperatura (cuando se ha configurado el ventilador)
	state_last = state_now;
 
	while(1)
	{
		temp_now = readTemp((char*)"/sys/class/thermal/thermal_zone1/temp"); // Leer temperatura
		diff_temp = abs(temp_now - temp_last); // Calcular diferencia de temperatura
		
		if(diff_temp >= DIFF_TEMP) // Diferencia de temperatura mayor que DIFF_TEMP (2ºC)
		{
			if((temp_now < TEMP_LOW) && (state_last != OFF)) // Temperatura menor que TEMP_LOW (55ºC) y estado anterior distinto
			{
				softPwmWrite(PIN_PWM, 0); // Ventilador apagado
				state_now = OFF; // Estado actual: Ventilador apagado
			}
			else if(((temp_now >= TEMP_LOW) && (temp_now <= TEMP_HIGH)) && (state_last != SLOW)) // Temperatura entre TEMP_LOW y TEMP_HIGH (55ºC - 70ºC) y estado anterior distinto
			{
				softPwmWrite(PIN_PWM, 512); // Ventilador al 50%
				state_now = SLOW; // Estado actual: Ventilador al 50%
			}
			else if((temp_now > TEMP_HIGH) && (state_last != FAST)) // Temperatura mayor que TEMP_HIGH (70ºC) y estado anterior distinto
			{
				softPwmWrite(PIN_PWM, 1023); // Ventilador al 100%
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

	return temp; // Devolvemos el valor de temperatura leida
}
