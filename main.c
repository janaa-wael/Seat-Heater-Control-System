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

#define MAX_TASKS                           12
#define MAX_MUTEXES                         12

#define CONTROL_HEATER_PERIODICITY          300U
#define READ_TEMP_PERIODICITY               800U
#define DISPLAY_DATA_PERIODICITY            1000U
#define HEATER_SETTING_PERIODICITY          100U
#define DIAGNOSTICS_PERIODICITY             400U
#define RUNTIME_MEASUREMENTS_PERIODICITY    2000U

#define DRIVER_SEAT          0
#define PASSENGER_SEAT       1

uint32_t task_start_time[MAX_TASKS] = {0};
uint32_t task_runtime[MAX_TASKS] = {0};




void(*(arr_BlueLedOn[2]))(void) = {GPIO_BlueLedOn, GPIO_Blue2LedOn};
void(*(arr_RedLedOn[2]))(void) =  {GPIO_RedLedOn, GPIO_Red2LedOn};
void(*(arr_RedLedOff[2]))(void) =  {GPIO_RedLedOff, GPIO_Red2LedOff};
void(*(arr_GreenLedOn[2]))(void) = {GPIO_GreenLedOn, GPIO_Green2LedOn};
void(*(arr_LedsOff[2]))(void) = {GPIO_LedsOff, GPIO_Leds2Off};

/* The HW setup function */
static void prvSetupHardware( void );

typedef enum {
    OFF=0, LOW=25, MEDIUM=30, HIGH=35
}Lvl;

typedef struct {
    uint32_t timestamp;  // GPTM timestamp value
    uint8_t failure_code; // You can define your own failure codes
} FailureRecord;

typedef struct {
    uint32_t timestamp;
    Lvl heating_level;   // Using your existing enum (OFF, LOW, MEDIUM, HIGH)
} HeatingLevelRecord;

typedef struct{
    FailureRecord last_failure;
    HeatingLevelRecord last_heating_lvl;
}SeatDiagnosticsData;


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
xSemaphoreHandle xBtnSemaphore[2];
//xSemaphoreHandle xDataMutex[2];
xSemaphoreHandle xTempMutex[2];
xSemaphoreHandle xDesiredHeaterLvlMutex[2];
xSemaphoreHandle xHeaterStatusMutex[2];
xSemaphoreHandle xLastFailureMutex[2];
xSemaphoreHandle xLastHeatLvlMutex[2];
xSemaphoreHandle xUART_Mutex;
xSemaphoreHandle xSystemFailureSemaphore[2];



#define MAX_MUTEXES         11  // Adjust based on your needs



MutexStats mutexStats[MAX_MUTEXES] = {0};

void RegisterMutex(void* xMutex, const char *pcName)
{
    int i ;
    for (i = 0; i < MAX_MUTEXES; i++) {
        if (mutexStats[i].mutex == NULL) {
            mutexStats[i].mutex = xMutex;
            mutexStats[i].name = pcName;
            vQueueAddToRegistry((QueueHandle_t)xMutex, pcName);  // For debugging
            break;
        }
    }
}

sint8 getMutexIndex(void *xMutex)
{
    int i;
    for(i = 0; i < MAX_MUTEXES; i++)
    {
        if(mutexStats[i].mutex == xMutex)
        {
            return i;
        }
    }
    return -1;
}


uint8 Temperature[2] = {22, 2};
uint8 Heater_Status[2][7] = {"OFF", "OFF"};
sint32 Last_Failure_Timestamp[2] = {-1, -1};
sint32 Last_HeatLvl_Timestamp[2] = {-1, -1};

Lvl Desired_Heater_Lvl[2] = {OFF, OFF};
SeatDiagnosticsData Seat_Data[2];

TaskHandle_t xTask1Handle;
TaskHandle_t xTask2Handle;
TaskHandle_t xTask3Handle;
TaskHandle_t xTask4Handle;
TaskHandle_t xTask5Handle;
TaskHandle_t xTask6Handle;
TaskHandle_t xTask7Handle;
TaskHandle_t xTask8Handle;
TaskHandle_t xTask9Handle;
TaskHandle_t xTask10Handle;
TaskHandle_t xTask11Handle;

int main()
{
    /* Setup the hardware for use with the Tiva C board. */
    prvSetupHardware();

    /* Create a Mutex */
    xBtnSemaphore[0] = xSemaphoreCreateBinary();
    xBtnSemaphore[1] = xSemaphoreCreateBinary();
    //xDataMutex = xSemaphoreCreateMutex();
    xDesiredHeaterLvlMutex[0] = xSemaphoreCreateMutex();
    xDesiredHeaterLvlMutex[1] = xSemaphoreCreateMutex();
    xTempMutex[0] = xSemaphoreCreateMutex();
    xTempMutex[1] = xSemaphoreCreateMutex();
    xHeaterStatusMutex[0] = xSemaphoreCreateMutex();
    xHeaterStatusMutex[1] = xSemaphoreCreateMutex();
    xLastFailureMutex[0] = xSemaphoreCreateMutex();
    xLastFailureMutex[1] = xSemaphoreCreateMutex();
    xLastHeatLvlMutex[0] = xSemaphoreCreateMutex();
    xLastHeatLvlMutex[1] = xSemaphoreCreateMutex();
    xUART_Mutex = xSemaphoreCreateMutex();
    xSystemFailureSemaphore[0] = xSemaphoreCreateBinary();
    xSystemFailureSemaphore[1] = xSemaphoreCreateBinary();

    RegisterMutex(xUART_Mutex, "UART_Mutex");
    RegisterMutex(xTempMutex[0], "Driver Temp Mutex");
    RegisterMutex(xTempMutex[1], "Passenger Temp Mutex");
    RegisterMutex(xDesiredHeaterLvlMutex[0], "Driver Desired Heating Lvl Mutex");
    RegisterMutex(xDesiredHeaterLvlMutex[1], "Passenger Desired Heating Lvl Mutex");
    RegisterMutex(xLastHeatLvlMutex[0], "Driver Last Heating Lvl Mutex");
    RegisterMutex(xLastHeatLvlMutex[1], "Passenger Last Heating Lvl Mutex");
    RegisterMutex(xHeaterStatusMutex[0], "Driver Heater Status Mutex");
    RegisterMutex(xHeaterStatusMutex[1], "Passenger Heater Status Mutex");
    RegisterMutex(xLastFailureMutex[0], "Driver Last Failure Mutex");
    RegisterMutex(xLastFailureMutex[1], "Passenger Last Failure Mutex");

    if (xBtnSemaphore[0] == NULL || xBtnSemaphore[1] == NULL || xTempMutex[0] == NULL || xTempMutex[1] == NULL)
    {
        UART0_SendString("Failed to create mutex/semaphore!\r\n");
        while(1); // Stop here
    }

    /* Create Tasks here */
    xTaskCreate(vControlHeater, "Task 1", configMINIMAL_STACK_SIZE, (void*)DRIVER_SEAT, 2, &xTask1Handle);
    xTaskCreate(vControlHeater, "Task 7", configMINIMAL_STACK_SIZE, (void*)PASSENGER_SEAT, 2, &xTask7Handle);

    xTaskCreate(vReadTemperatureHandler, "Task 2", configMINIMAL_STACK_SIZE, (void*)DRIVER_SEAT, 3, &xTask2Handle);
    xTaskCreate(vReadTemperatureHandler, "Task 11", configMINIMAL_STACK_SIZE, (void*)PASSENGER_SEAT, 3, &xTask11Handle);

    xTaskCreate(vDisplayDataHandler, "Task 3", configMINIMAL_STACK_SIZE, (void*)DRIVER_SEAT, 2, &xTask3Handle);
    xTaskCreate(vDisplayDataHandler, "Task 8", configMINIMAL_STACK_SIZE, (void*)PASSENGER_SEAT, 2, &xTask8Handle);

    xTaskCreate(vHeaterSettingHandler, "Task 4", configMINIMAL_STACK_SIZE, (void*)DRIVER_SEAT, 3, &xTask4Handle);
    xTaskCreate(vHeaterSettingHandler, "Task 9", configMINIMAL_STACK_SIZE, (void*)PASSENGER_SEAT, 3, &xTask9Handle);

    xTaskCreate(vDiagnostics, "Task 5", configMINIMAL_STACK_SIZE, (void*)DRIVER_SEAT, 4, &xTask5Handle);
    xTaskCreate(vDiagnostics, "Task 10", configMINIMAL_STACK_SIZE, (void*)PASSENGER_SEAT, 4, &xTask10Handle);

    xTaskCreate(vRuntimeMeasurements, "Task 6", configMINIMAL_STACK_SIZE, NULL, 5, &xTask6Handle);


    vTaskSetApplicationTaskTag( xTask1Handle,  ( TaskHookFunction_t ) 1 );
    vTaskSetApplicationTaskTag( xTask2Handle,  ( TaskHookFunction_t ) 2 );
    vTaskSetApplicationTaskTag( xTask3Handle,  ( TaskHookFunction_t ) 3 );
    vTaskSetApplicationTaskTag( xTask4Handle,  ( TaskHookFunction_t ) 4 );
    vTaskSetApplicationTaskTag( xTask5Handle,  ( TaskHookFunction_t ) 5 );
    vTaskSetApplicationTaskTag( xTask6Handle,  ( TaskHookFunction_t ) 6 );
    vTaskSetApplicationTaskTag( xTask7Handle,  ( TaskHookFunction_t ) 7 );
    vTaskSetApplicationTaskTag( xTask8Handle,  ( TaskHookFunction_t ) 8 );
    vTaskSetApplicationTaskTag( xTask9Handle,  ( TaskHookFunction_t ) 9 );
    vTaskSetApplicationTaskTag( xTask10Handle, ( TaskHookFunction_t ) 10 );
    vTaskSetApplicationTaskTag( xTask11Handle, ( TaskHookFunction_t ) 11 );

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
    GPIO_SW2EdgeTriggeredInterruptInit();
    ADC_init();
    PassengerSeatLEDs_Init();
    GPTM_WTimer0Init();
}


void vControlHeater(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    int seat_id = (int)pvParameters;
    for(;;)
    {
        //UART0_SendInteger(seat_id);
        //UART0_SendString("\r\n");

        if(xSemaphoreTake(xBtnSemaphore[seat_id], portMAX_DELAY) == pdTRUE)
        {
            if(xSemaphoreTake(xDesiredHeaterLvlMutex[seat_id], portMAX_DELAY) == pdTRUE)
            {
                switch(Desired_Heater_Lvl[seat_id])
                {
                case HIGH:
                    Desired_Heater_Lvl[seat_id] = OFF; break;
                case MEDIUM:
                    Desired_Heater_Lvl[seat_id] = HIGH; break;
                case LOW:
                    Desired_Heater_Lvl[seat_id] = MEDIUM; break;
                case OFF:
                    Desired_Heater_Lvl[seat_id] = LOW; break;
                }
                if(xSemaphoreTake(xTempMutex[seat_id], portMAX_DELAY) == pdTRUE)
                {
                    if(Desired_Heater_Lvl[seat_id] - Temperature[seat_id] >= 10)
                    {
                        arr_LedsOff[seat_id]();
                        arr_BlueLedOn[seat_id](); //GPIO_BlueLedOn();
                        arr_GreenLedOn[seat_id](); // GPIO_GreenLedOn();
                        //strcpy(Heater_Status[seat_id],"ON");
                    }
                    else if(Desired_Heater_Lvl[seat_id] - Temperature[seat_id] >= 5)
                    {
                        arr_LedsOff[seat_id]();
                        arr_BlueLedOn[seat_id](); //GPIO_BlueLedOn();
                        //strcpy(Heater_Status[seat_id],"ON");
                    }
                    else if(Desired_Heater_Lvl[seat_id] - Temperature[seat_id] >= 2)
                    {
                        arr_LedsOff[seat_id]();
                        arr_GreenLedOn[seat_id]();
                        //strcpy(Heater_Status[seat_id],"ON");
                    }
                    else if(Temperature[seat_id] - Desired_Heater_Lvl[seat_id] >= 0)
                    {
                        arr_LedsOff[seat_id]();
                        //strcpy(Heater_Status[seat_id],"OFF");
                    }
                    xSemaphoreGive(xTempMutex[seat_id]);
                }

                xSemaphoreGive(xDesiredHeaterLvlMutex[seat_id]);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CONTROL_HEATER_PERIODICITY));
    }
}


void vReadTemperatureHandler(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    int seat_id = (int)pvParameters;
    for(;;)
    {
        if(xSemaphoreTake(xTempMutex[seat_id],  portMAX_DELAY) == pdTRUE)
        {

            Temperature[seat_id] = ADC_readValue(seat_id);
            if(Temperature[seat_id] < 4 || Temperature[seat_id] > 45)
            {
                arr_LedsOff[seat_id]();
                arr_RedLedOn[seat_id]();
                //UART0_SendString("Heater Turned Off!\r\nInvalid Temp Range\r\n");
                if(xSemaphoreTake(xLastFailureMutex[seat_id], portMAX_DELAY) == pdTRUE)
                {
                    Last_Failure_Timestamp[seat_id] = GPTM_WTimer0Read();
                    xSemaphoreGive(xLastFailureMutex[seat_id]);
                }
                xSemaphoreGive(xSystemFailureSemaphore[seat_id]);
            }
            else
            {
                arr_RedLedOff[seat_id]();
            }
            xSemaphoreGive(xTempMutex[seat_id]);
        }
        else
        {
            UART0_SendString("Mutex timeout!");
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(READ_TEMP_PERIODICITY));
        //UART0_SendString("Read Temp\r\n");
    }
}

void vDisplayDataHandler(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    int seat_id = (int)pvParameters;

    for(;;)
    {
        if(xSemaphoreTake(xUART_Mutex, portMAX_DELAY) == pdTRUE)
        {
            //uint32 start_time = GPTM_WTimer0Read();
            //UART0_SendString("Display\r\n");
            if(seat_id == DRIVER_SEAT)
                UART0_SendString("\r\n\r\nDriver Seat Data:\r\n");
            else
                UART0_SendString("\r\n\r\nPassenger Seat Data:\r\n");
            UART0_SendString("=====================\r\n");
            UART0_SendString("Temperature: ");

            if(xSemaphoreTake(xTempMutex[seat_id], portMAX_DELAY) == pdTRUE)
            {
                UART0_SendInteger(Temperature[seat_id]);
                xSemaphoreGive(xTempMutex[seat_id]);
            }
            UART0_SendString("\xB0""C\r\n");


            UART0_SendString("Desired Heating Level: ");
            if(xSemaphoreTake(xDesiredHeaterLvlMutex[seat_id], portMAX_DELAY) == pdTRUE)
            {
                UART0_SendInteger(Desired_Heater_Lvl[seat_id]);
                xSemaphoreGive(xDesiredHeaterLvlMutex[seat_id]);
            }
            UART0_SendString("\xB0""C");

            UART0_SendString("\r\nHeater Status: ");
            if(xSemaphoreTake(xHeaterStatusMutex[seat_id], portMAX_DELAY) == pdTRUE)
            {
                UART0_SendString(Heater_Status[seat_id]);
                xSemaphoreGive(xHeaterStatusMutex[seat_id]);
            }

            UART0_SendString("\r\n");
            //uint32_t end_time = GPTM_WTimer0Read();
            //UART0_SendString("Task time: ");
            //UART0_SendInteger(end_time - start_time);
            //UART0_SendString("\r\n");
            xSemaphoreGive(xUART_Mutex);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(DISPLAY_DATA_PERIODICITY));

    }

}

void vHeaterSettingHandler(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    int seat_id = (int)pvParameters;

    for(;;)
    {
        /*UART0_SendString("Seat id : ");
        UART0_SendInteger(seat_id);
        UART0_SendString("\r\n");*/
        if(xSemaphoreTake(xDesiredHeaterLvlMutex[seat_id], portMAX_DELAY) == pdTRUE)
        {

            if(xSemaphoreTake(xTempMutex[seat_id],portMAX_DELAY) == pdTRUE)
            {
                if(xSemaphoreTake(xHeaterStatusMutex[seat_id],portMAX_DELAY) == pdTRUE)
                {
                    strcpy(Heater_Status[seat_id],"");
                    if(Desired_Heater_Lvl[seat_id] - Temperature[seat_id] >= 10)
                        strcpy(Heater_Status[seat_id],"HIGH");
                    else if(Desired_Heater_Lvl[seat_id] - Temperature[seat_id] >= 5)
                        strcpy(Heater_Status[seat_id],"MEDIUM");
                    else if(Desired_Heater_Lvl[seat_id] - Temperature[seat_id] >= 2)
                        strcpy(Heater_Status[seat_id],"LOW");
                    else if(Temperature[seat_id] - Desired_Heater_Lvl[seat_id] >= 0)
                        strcpy(Heater_Status[seat_id],"OFF");
                    xSemaphoreGive(xHeaterStatusMutex[seat_id]);
                }
                xSemaphoreGive(xTempMutex[seat_id]);
            }
            xSemaphoreGive(xDesiredHeaterLvlMutex[seat_id]);
        }
        //UART0_SendString("Heater Setting\r\n");
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(HEATER_SETTING_PERIODICITY));

    }
}

void vDiagnostics(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    int seat_id = (int)pvParameters;
    for(;;)
    {
        //UART0_SendString("\n\rDiagnostics: ");
        //UART0_SendInteger(seat_id);

        //UART0_SendString("Last Heater Level = ");
        /*switch(Desired_Heater_Lvl)
        {
        case OFF: UART0_SendString("OFF"); break;
        case LOW: UART0_SendString("LOW"); break;
        case MEDIUM: UART0_SendString("MEDIUM"); break;
        case HIGH: UART0_SendString("HIGH"); break;
        }*/
        //UART0_SendString("\r\n");

        if(xSemaphoreTake(xSystemFailureSemaphore[seat_id], 0) == pdTRUE)
        {

            //UART0_SendString("Failure\r\n");
            //UART0_SendString("Timestamp = ");
            if(xSemaphoreTake(xLastFailureMutex[seat_id], 0) == pdTRUE)
            {
                Seat_Data[seat_id].last_failure.timestamp = Last_Failure_Timestamp;
                Seat_Data[seat_id].last_failure.failure_code = "Temperature Out Of Range";
                //UART0_SendString("\r\n");
                xSemaphoreGive(xLastFailureMutex[seat_id]);
            }

        }

        if(xSemaphoreTake(xDesiredHeaterLvlMutex[seat_id], portMAX_DELAY) == pdTRUE)
        {
            Seat_Data[seat_id].last_heating_lvl.heating_level = Desired_Heater_Lvl[seat_id];
            xSemaphoreGive(xDesiredHeaterLvlMutex[seat_id]);
        }
        if(xSemaphoreTake(xLastHeatLvlMutex[seat_id], portMAX_DELAY) == pdTRUE)
        {
            Seat_Data[seat_id].last_heating_lvl.timestamp = Last_HeatLvl_Timestamp[seat_id];
            xSemaphoreGive(xLastHeatLvlMutex[seat_id]);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(DIAGNOSTICS_PERIODICITY));

    }
}

void vRuntimeMeasurements(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    int seat_id = (int)pvParameters;
    for (;;)
    {
        uint8 ucCounter, ucCPU_Load;
        uint32 ullTotalTasksTime = 0;
        for(ucCounter = 1; ucCounter < MAX_TASKS; ucCounter++)
        {
            ullTotalTasksTime += ullTasksTotalTime[ucCounter];
        }
        ucCPU_Load = (ullTotalTasksTime * 100) /  GPTM_WTimer0Read();
        taskENTER_CRITICAL();
        UART0_SendString("CPU Load is ");
        UART0_SendInteger(ucCPU_Load);
        UART0_SendString("% \r\n");
        UART0_SendString("\r\nTASKS EXECUTION TIMES");
        UART0_SendString("\r\n****************************\r\n");
        for(ucCounter = 1; ucCounter < MAX_TASKS; ucCounter++)
        {
            UART0_SendString("Task ");
            UART0_SendInteger(ucCounter);
            UART0_SendString(": ");
            UART0_SendInteger(ullTasksTotalTime[ucCounter]);
            UART0_SendString(" ticks / ");
            UART0_SendInteger(ullTasksTotalTime[ucCounter] / 10);
            UART0_SendString(" ms\r\n");
        }
        UART0_SendString("****************************\r\n* ");

        for(ucCounter = 0; ucCounter < MAX_MUTEXES; ucCounter++)
        {
            UART0_SendString(mutexStats[ucCounter].name);
            UART0_SendString(": ");
            UART0_SendInteger(mutexStats[ucCounter].totalLockTime);
            UART0_SendString(" ticks / ");
            UART0_SendInteger(mutexStats[ucCounter].totalLockTime/ 10);
            UART0_SendString(" ms\r\n* ");
        }
        UART0_SendString("~~~~~~~~~~~~~~~~~~~~~");
        taskEXIT_CRITICAL();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(RUNTIME_MEASUREMENTS_PERIODICITY));
    }
}

void  GPIOPortF_Handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int i;
    if(GPIO_PORTF_RIS_REG & (1<<4))
    {

        xSemaphoreGiveFromISR(xBtnSemaphore[0], &xHigherPriorityTaskWoken);
        GPIO_PORTF_ICR_REG   |= (1<<4);       /* Clear Trigger flag for PF0 (Interrupt Flag) */

        if(xSemaphoreTakeFromISR(xLastHeatLvlMutex[0], portMAX_DELAY) == pdTRUE)
        {
            Last_HeatLvl_Timestamp[0] = GPTM_WTimer0Read();
            xSemaphoreGiveFromISR(xLastHeatLvlMutex[0],xHigherPriorityTaskWoken);
        }

    }
    if(GPIO_PORTF_RIS_REG & (1<<0))
    {
        for( i = 0 ; i<8000;i++);
        //UART0_SendString("SW2 Clicked\r\n");
        xSemaphoreGiveFromISR(xBtnSemaphore[1], &xHigherPriorityTaskWoken);
        GPIO_PORTF_ICR_REG   |= (1<<0);       /* Clear Trigger flag for PF0 (Interrupt Flag) */

        if(xSemaphoreTakeFromISR(xLastHeatLvlMutex[1], portMAX_DELAY) == pdTRUE)
        {
            Last_HeatLvl_Timestamp[1] = GPTM_WTimer0Read();
            xSemaphoreGiveFromISR(xLastHeatLvlMutex[1],xHigherPriorityTaskWoken);
        }

    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
