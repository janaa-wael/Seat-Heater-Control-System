/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* MCAL includes. */
#include "uart0.h"
#include "gpio.h"
#include "tm4c123gh6pm_registers.h"
#include "adc.h"

/* The HW setup function */
static void prvSetupHardware( void );

typedef enum {
    OFF=0, LOW=25, MEDIUM=30, HIGH=35
}Lvl;

/* FreeRTOS tasks */
void vControlHeater(void *pvParameters);
void vReadTemperatureHandler(void *pvParameters);
void vDisplayDataHandler(void *pvParameters);
void vHeaterSettingHandler(void *pvParameters);

/* FreeRTOS Mutexes */
xSemaphoreHandle xBtnSemaphore;
xSemaphoreHandle xDataMutex;

uint8 Temperature = 22;
uint8 Heater_Status[4];
Lvl Desired_Heater_Lvl = OFF;

int main()
{
    /* Setup the hardware for use with the Tiva C board. */
    prvSetupHardware();

    /* Create a Mutex */
    xBtnSemaphore = xSemaphoreCreateBinary();
    xDataMutex = xSemaphoreCreateMutex();

    if (xDataMutex == NULL || xBtnSemaphore == NULL)
    {
        UART0_SendString("Failed to create mutex/semaphore!\r\n");
        while(1); // Stop here
    }

    /* Create Tasks here */
    xTaskCreate(vControlHeater, "Task 1", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(vReadTemperatureHandler, "Task 2", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    xTaskCreate(vDisplayDataHandler, "Task 3", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vHeaterSettingHandler, "Task 4", configMINIMAL_STACK_SIZE, NULL, 3, NULL);

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
        vTaskDelay( pdMS_TO_TICKS(200));
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
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
        UART0_SendString("Read Temp\r\n");
    }
}

//every 1 second
void vDisplayDataHandler(void *pvParameters)
{
    TickType_t ticks = pdMS_TO_TICKS(600);
    for(;;)
    {

        UART0_SendString("Display\r\n");

        UART0_SendString("\r\nTemperature:");
        UART0_SendInteger(Temperature);
        UART0_SendString("\r\nDesired Heating Level:");
        UART0_SendInteger(Desired_Heater_Lvl);
        UART0_SendString("\r\nHeater Status:");
        UART0_SendString(Heater_Status);
        UART0_SendString("\r\n");
        vTaskDelay(ticks);

    }
}

void vHeaterSettingHandler(void *pvParameters)
{
    TickType_t ticks = pdMS_TO_TICKS(500);
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
        UART0_SendString("Heater Setting\r\n");
        vTaskDelay(ticks);
    }
}

void  GPIOPortF_Handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if(GPIO_PORTF_RIS_REG & (1<<4))
    {
        xSemaphoreGiveFromISR(xBtnSemaphore, &xHigherPriorityTaskWoken);
        GPIO_PORTF_ICR_REG   |= (1<<4);       /* Clear Trigger flag for PF0 (Interrupt Flag) */
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
