/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* MCAL includes. */
#include "uart0.h"
#include "gpio.h"
#include "tm4c123gh6pm_registers.h"
#include "adc.h"
#include "GPTM.h"

#include <string.h>

extern uint32_t get_timestamp_us(void);
#define MAX_TASKS                           6
#define CONTROL_HEATER_PERIODICITY          200
#define READ_TEMP_PERIODICITY               100
#define DISPLAY_DATA_PERIODICITY            400
#define HEATER_SETTING_PERIODICITY          250
#define DIAGNOSTICS_PERIODICITY             300
#define RUNTIME_MEASUREMENTS_PERIODICITY    500


uint32_t task_start_time[MAX_TASKS] = {0};
uint32_t task_runtime[MAX_TASKS] = {0};

extern void* pxCurrentTCB;

uint32 ullTasksOutTime[MAX_TASKS];
uint32 ullTasksInTime[MAX_TASKS];
uint32 ullTasksTotalTime[MAX_TASKS];



/* The HW setup function */
static void prvSetupHardware( void );

typedef enum {
    OFF=0, LOW=25, MEDIUM=30, HIGH=35
}Lvl;

const char* heaterLevelStr[] = {
    "OFF", "LOW", "MEDIUM", "HIGH"
};


/* FreeRTOS tasks */
void vControlHeater(void *pvParameters);
void vReadTemperatureHandler(void *pvParameters);
void vDisplayDataHandler(void *pvParameters);
void vHeaterSettingHandler(void *pvParameters);
void vDiagnostics(void *pvParameters);
void vRuntimeMeasurements(void *pvParameters);


uint32 ullTasksOutTime[MAX_TASKS];
uint32 ullTasksInTime[MAX_TASKS];
uint32 ullTasksExecutionTime[MAX_TASKS];

/* FreeRTOS Mutexes */
xSemaphoreHandle xBtnSemaphore;
xSemaphoreHandle xDataMutex;
xSemaphoreHandle xGPTM_Mutex;
xSemaphoreHandle xSystemFailureSemaphore;

uint8 Temperature = 22;
uint8 Heater_Status[4];
sint32 Last_Failure_Timestamp = -1;
sint32 Last_HeatLvl_Timestamp = -1;
Lvl Desired_Heater_Lvl = OFF;

TaskHandle_t xTask1Handle;
TaskHandle_t xTask2Handle;
TaskHandle_t xTask3Handle;
TaskHandle_t xTask4Handle;
TaskHandle_t xTask5Handle;
TaskHandle_t xTask6Handle;

int main()
{
    /* Setup the hardware for use with the Tiva C board. */
    prvSetupHardware();

    /* Create a Mutex */
    xBtnSemaphore = xSemaphoreCreateBinary();
    xDataMutex = xSemaphoreCreateMutex();
    xGPTM_Mutex = xSemaphoreCreateMutex();
    xSystemFailureSemaphore = xSemaphoreCreateBinary();


    if (xDataMutex == NULL || xBtnSemaphore == NULL)
    {
        UART0_SendString("Failed to create mutex/semaphore!\r\n");
        while(1); // Stop here
    }

    /* Create Tasks here */
    xTaskCreate(vControlHeater, "Task 1", configMINIMAL_STACK_SIZE, NULL, 2, &xTask1Handle);
    xTaskCreate(vReadTemperatureHandler, "Task 2", configMINIMAL_STACK_SIZE, NULL, 3, &xTask2Handle);
    xTaskCreate(vDisplayDataHandler, "Task 3", configMINIMAL_STACK_SIZE, NULL, 1, &xTask3Handle);
    xTaskCreate(vHeaterSettingHandler, "Task 4", configMINIMAL_STACK_SIZE, NULL, 3, &xTask4Handle);
    xTaskCreate(vDiagnostics, "Task 5", configMINIMAL_STACK_SIZE, NULL, 4, &xTask5Handle);
    xTaskCreate(vRuntimeMeasurements, "Task 6", configMINIMAL_STACK_SIZE, NULL, 2, &xTask6Handle);


    vTaskSetApplicationTaskTag( xTask1Handle, ( TaskHookFunction_t ) 1 );
    vTaskSetApplicationTaskTag( xTask2Handle, ( TaskHookFunction_t ) 2 );
    vTaskSetApplicationTaskTag( xTask3Handle, ( TaskHookFunction_t ) 3 );
    vTaskSetApplicationTaskTag( xTask4Handle, ( TaskHookFunction_t ) 4 );
    vTaskSetApplicationTaskTag( xTask5Handle, ( TaskHookFunction_t ) 5 );
    vTaskSetApplicationTaskTag( xTask6Handle, ( TaskHookFunction_t ) 6 );

    vTaskStartScheduler();

    /* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
    for (;;);

}


static void prvSetupHardware( void )
{
    /* Place here any needed HW initialization such as GPIO, UART, etc.  */
    GPIO_BuiltinButtonsLedsInit();
    UART0_Init();
    GPIO_SW1EdgeTriggeredInterruptInit();
    ADC_init();
    GPTM_WTimer0Init();
}


void vControlHeater(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for(;;)
    {

        if(xSemaphoreTake(xBtnSemaphore, portMAX_DELAY) == pdTRUE)
        {
            if(xSemaphoreTake(xDataMutex, portMAX_DELAY) == pdTRUE)
            {
                switch(Desired_Heater_Lvl)
                {
                case HIGH:
                    Desired_Heater_Lvl = OFF; break;
                case MEDIUM:
                    Desired_Heater_Lvl = HIGH; break;
                case LOW:
                    Desired_Heater_Lvl = MEDIUM; break;
                case OFF:
                    Desired_Heater_Lvl = LOW; break;
                }
                if(Desired_Heater_Lvl - Temperature >= 10)
                {
                    GPIO_LedsOff();
                    GPIO_BlueLedOn();
                    GPIO_GreenLedOn();
                    strcpy(Heater_Status,"ON");
                }
                else if(Desired_Heater_Lvl - Temperature >= 5)
                {
                    GPIO_LedsOff();
                    GPIO_BlueLedOn();
                    strcpy(Heater_Status,"ON");
                }
                else if(Desired_Heater_Lvl - Temperature >= 2)
                {
                    GPIO_LedsOff();
                    GPIO_GreenLedOn();
                    strcpy(Heater_Status,"ON");
                }
                else if(Temperature - Desired_Heater_Lvl >= 0)
                {
                    GPIO_LedsOff();
                    strcpy(Heater_Status,"OFF");
                }

                xSemaphoreGive(xDataMutex);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CONTROL_HEATER_PERIODICITY));

        //UART0_SendString("Control Heater\r\n");
    }
}


//every 2 seconds
void vReadTemperatureHandler(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for(;;)
    {
        if(xSemaphoreTake(xDataMutex,  portMAX_DELAY) == pdTRUE)
        {

            Temperature = ADC_readValue();
            if(Temperature < 4 || Temperature > 45)
            {
                GPIO_LedsOff();
                GPIO_RedLedOn();
                UART0_SendString("Heater Turned Off!\r\nInvalid Temp Range\r\n");
                Last_Failure_Timestamp = GPTM_WTimer0Read();
                xSemaphoreGive(xSystemFailureSemaphore);
            }
            else
            {
                GPIO_RedLedOff();
            }
            xSemaphoreGive(xDataMutex);
        }
        else
        {
            UART0_SendString("Mutex timeout!");
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(READ_TEMP_PERIODICITY));
        //UART0_SendString("Read Temp\r\n");
    }
}

//every 1 second
void vDisplayDataHandler(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for(;;)
    {
       //uint32 start_time = GPTM_WTimer0Read();
        UART0_SendString("Display\r\n");

        UART0_SendString("\r\nTemperature:");
        UART0_SendInteger(Temperature);
        UART0_SendString("\r\nDesired Heating Level:");
        UART0_SendInteger(Desired_Heater_Lvl);
        UART0_SendString("\r\nHeater Status:");
        UART0_SendString(Heater_Status);
        UART0_SendString("\r\n");
        //uint32_t end_time = GPTM_WTimer0Read();
        //UART0_SendString("Task time: ");
        //UART0_SendInteger(end_time - start_time);
        //UART0_SendString("\r\n");
        vTaskDelayUntil(&xLastWakeTime, DISPLAY_DATA_PERIODICITY);

    }

}

void vHeaterSettingHandler(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for(;;)
    {
        if(xSemaphoreTake(xDataMutex, portMAX_DELAY) == pdTRUE)
        {


            if(Desired_Heater_Lvl - Temperature >= 10)
                strcpy(Heater_Status,"ON");
            else if(Desired_Heater_Lvl - Temperature >= 5)
                strcpy(Heater_Status,"ON");
            else if(Desired_Heater_Lvl - Temperature >= 2)
                strcpy(Heater_Status,"ON");
            else if(Temperature - Desired_Heater_Lvl >= 0)
                strcpy(Heater_Status,"OFF");

            xSemaphoreGive(xDataMutex);
        }
        //UART0_SendString("Heater Setting\r\n");
        vTaskDelayUntil(&xLastWakeTime, HEATER_SETTING_PERIODICITY);

    }
}

void vDiagnostics(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for(;;)
    {
        UART0_SendString("Diagnostics\r");
        if(xSemaphoreTake(xDataMutex, portMAX_DELAY) == pdTRUE)
        {
            UART0_SendString("Last Heater Level = ");
            switch(Desired_Heater_Lvl)
            {
            case OFF: UART0_SendString("OFF"); break;
            case LOW: UART0_SendString("LOW"); break;
            case MEDIUM: UART0_SendString("MEDIUM"); break;
            case HIGH: UART0_SendString("HIGH"); break;
            }
            UART0_SendString("\r\n");
            xSemaphoreGive(xDataMutex);
        }
        if(xSemaphoreTake(xSystemFailureSemaphore, pdMS_TO_TICKS(100)) == pdTRUE)
        {

            UART0_SendString("Failure\r\n");
            UART0_SendString("Timestamp = ");
            if(xSemaphoreTake(xGPTM_Mutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                UART0_SendInteger(Last_Failure_Timestamp);
                xSemaphoreGive(xGPTM_Mutex);
            }
            UART0_SendString("\r\n");
        }
        vTaskDelayUntil(&xLastWakeTime, DIAGNOSTICS_PERIODICITY);

     }
}

void vRuntimeMeasurements(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        uint8 ucCounter, ucCPU_Load;
        uint32 ullTotalTasksTime = 0;
        vTaskDelayUntil(&xLastWakeTime, RUNTIME_MEASUREMENTS_PERIODICITY);
        for(ucCounter = 1; ucCounter < 7; ucCounter++)
        {
            ullTotalTasksTime += ullTasksTotalTime[ucCounter];
        }
        ucCPU_Load = (ullTotalTasksTime * 100) /  GPTM_WTimer0Read();

        taskENTER_CRITICAL();
        UART0_SendString("CPU Load is ");
        UART0_SendInteger(ucCPU_Load);
        //UART0_SendString("Task 2: ");
        //UART0_SendInteger(ullTasksTotalTime[5]);
        UART0_SendString("% \r\n");
        taskEXIT_CRITICAL();
    }
}

void  GPIOPortF_Handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if(GPIO_PORTF_RIS_REG & (1<<4))
    {
        xSemaphoreGiveFromISR(xBtnSemaphore, &xHigherPriorityTaskWoken);
        GPIO_PORTF_ICR_REG   |= (1<<4);       /* Clear Trigger flag for PF0 (Interrupt Flag) */
        if(xSemaphoreTakeFromISR(xGPTM_Mutex, portMAX_DELAY) == pdTRUE)
        {
            Last_HeatLvl_Timestamp = GPTM_WTimer0Read();
            xSemaphoreGiveFromISR(xGPTM_Mutex,xHigherPriorityTaskWoken);
        }
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
