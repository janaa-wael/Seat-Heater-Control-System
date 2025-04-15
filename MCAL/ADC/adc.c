
/*
 * adc.c
 *
 *  Created on: Apr 4, 202
 *      Author: DELL
 */

#include "adc.h"
#include "tm4c123gh6pm_registers.h"

//uint32 adcResult;
//float32 adcResultVoltage;

void ADC_init(void)
{

    /*The initialization sequence for the ADC is as follows:
    1. Enable the ADC clock using the RCGCADC register (see page 352).
    2. Enable the clock to the appropriate GPIO modules via the RCGCGPIO register (see page 340).
    To find out which GPIO ports to enable, refer to �Signal Description� on page 801.
    3. Set the GPIO AFSEL bits for the ADC input pins (see page 671). To determine which GPIOs to
    configure, see Table 23-4 on page 1344.
    4. Configure the AINx signals to be analog inputs by clearing the corresponding DEN bit in the
    GPIO Digital Enable (GPIODEN) register (see page 682).
    5. Disable the analog isolation circuit for all ADC input pins that are to be used by writing a 1 to
    the appropriate bits of the GPIOAMSEL register (see page 687) in the associated GPIO block.*/

    SYSCTL_RCGCADC_REG |= (1<<0);
    while((SYSCTL_PRADC_REG  & 0x01) == 0);

    SYSCTL_RCGCGPIO_REG |= (1<<4);
    while(!(SYSCTL_PRGPIO_REG & 0x10));                     /*Already Enabled with the GPIO PortF*/

    GPIO_PORTE_AFSEL_REG |= (1<<3) | (1<<2);                            /* Enable alternative function on PE3 */
    /*    GPIO_PORTE_PCTL_REG  |= 0x0000F000;                        Set PMC3 bits for PE3 to use it as ADC */
    GPIO_PORTE_DEN_REG   &= ~((1<<3) | (1<<2));                          /* Disable Digital I/O on PE3 */
    GPIO_PORTE_AMSEL_REG |= (1<<3) | (1<<2);                            /* Enable Analog on PE3 */
    GPIO_PORTE_DIR_REG   &= ~((1<<3) | (1<<2));                          /* Configure PE3 as output pin*/

    /* Set GPIO PORTF priority as 5 by set Bit number 21, 22 and 23 with value 2
    NVIC_PRI4_REG = (NVIC_PRI4_REG & 0x1FFF) | (3 << 13);
    NVIC_EN0_REG  |= (1<<17);                   Enable NVIC Interrupt for ADC0 Sequence 3 by set bit number 17 in EN0 Register
     */

    /*
1. Ensure that the sample sequencer is disabled by clearing the corresponding ASENn bit in the
ADCACTSS register. Programming of the sample sequencers is allowed without having them
enabled. Disabling the sequencer during programming prevents erroneous execution if a trigger
event were to occur during the configuration process.
2. Configure the trigger event for the sample sequencer in the ADCEMUX register.
3. When using a PWM generator as the trigger source, use the ADC Trigger Source Select
(ADCTSSEL) register to specify in which PWM module the generator is located. The default
register reset selects PWM module 0 for all generators.
4. For each sample in the sample sequence, configure the corresponding input source in the
ADCSSMUXn register.
5. For each sample in the sample sequence, configure the sample control bits in the corresponding
nibble in the ADCSSCTLn register. When programming the last nibble, ensure that the END bit
is set. Failure to set the END bit causes unpredictable behavior.
6. If interrupts are to be used, set the corresponding MASK bit in the ADCIM register.
7. Enable the sample sequencer logic by setting the corresponding ASENn bit in the ADCACTSS
register.*/

    ADC0_ACTSS_R &= ~(1<<3);                                /*disable all sequencers*/
    ADC0_EMUX_R &= ~(0xF000);                                  /*set trigger source to external GPIO Pin*/
    ADC0_SSMUX3_R = 0x0;                                      /* analog input 0 (Ain0 = PE3)*/
    ADC0_SSCTL3_R |= (1<<1) | (1<<2);                                   /* Enable interrupts*/
    /*ADC0_IM_R = (1<<3);                                       enable interrupt
    ADC0_ISC_R = (1<<3);                                      clear interrupt flag*/
    /*    ADC0_SSPRI_R = (ADC0_SSPRI_R & ~(0x3000)) | 0x0123;     give SS3 highest priority*/
    ADC0_ACTSS_R = (1<<3);                                   /*�enable�SS3*/

    ADC0_ACTSS_R &= ~(1<<2);                   /* Disable sequencer 2 */
    ADC0_EMUX_R &= ~(0x0F00);                  /* Software trigger */
    ADC0_SSMUX2_R = 0x1;                       /* Sample channel 1 (AIN1) */
    ADC0_SSCTL2_R = (1<<1) | (1<<2);           /* Single sample, no TS0, no IE0, END0 */
    ADC0_ACTSS_R |= (1<<2);                    /* Enable sequencer 2 */
}

uint32 ADC_readValue(int channel)
{
    uint32 adcResult;
    float32 adcResultVoltage;
    if(channel == 0)
    {
        ADC0_PSSI_R |=(1<<3);
        while(ADC0_RIS_R & 0x08 == 0x0);
        adcResult = ADC0_SSFIFO3_R;
        ADC0_ISC_R =(1<<3);
    }
    else if(channel == 1)
    {
        ADC0_PSSI_R |=(1<<2);
        while(ADC0_RIS_R & 0x04 == 0x0);
        adcResult = ADC0_SSFIFO2_R;
        ADC0_ISC_R =(1<<2);
    }

    adcResultVoltage = adcResult * ADC_REF_VOLTAGE / ADC_MAX_VALUE;

    float tempC = (adcResultVoltage - 0.5) * 100.0;  // TMP36: 0.5V offset at 0�C
    return tempC;
}

