/*
 * Tasks.h
 *
 *  Created on: Apr 6, 2025
 *      Author: hp
 */

#ifndef TASKS_H_
#define TASKS_H_


/* FreeRTOS tasks */
void vControlHeater(void *pvParameters);
void vReadTemperatureHandler(void *pvParameters);
void vDisplayDataHandler(void *pvParameters);
void vHeaterSettingHandler(void *pvParameters);
void vDiagnostics(void *pvParameters);
void vRuntimeMeasurements(void *pvParameters);


#endif /* TASKS_H_ */
